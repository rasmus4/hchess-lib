#include <hchess.h>

const char* GET_WS_REQUEST = "GET /chess HTTP/1.1\r\nSec-WebSocket-Key: /AUS0YqQn2Vu3HNRvoPH0Q==\r\n\r\n";

uint16_t hchess_connect(char* address, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        printf("Failed to create socket, errno %d\n", errno);
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(address);

    if (-1 == connect(sockfd, (struct sockaddr*) &server, sizeof(server))) {
        printf("Failed to connect to %s:%d, errno %d\n", address, port, errno);
        return -1;
    }


    send(sockfd, GET_WS_REQUEST, strlen(GET_WS_REQUEST), 0);
    char* recvbuf = (char*) malloc(sizeof(char) * 512);
    recv(sockfd, recvbuf, 512, 0);
    printf("Connected\n");
    if (chessSessions == NULL) {
        chessSessions = (struct chessSession*) malloc(sizeof(struct chessSession));
        numAllocatedChessSessions = 1;
    }
    else if (numChessSessions >= numAllocatedChessSessions) {
        chessSessions = (struct chessSession*) realloc(chessSessions, sizeof(struct chessSession) * numAllocatedChessSessions * 2);
        assert(chessSessions != NULL);
        numAllocatedChessSessions = 2 * numAllocatedChessSessions;
    }
    memset(&chessSessions[numChessSessions], 0, sizeof(struct chessSession));
    chessSessions[numChessSessions].sockfd = sockfd;
    return numChessSessions++;
}

int hchess_close(uint16_t sessionIndex) {
    return close(chessSessions[sessionIndex].sockfd);
}

int hchess_join(uint16_t sessionIndex, int32_t roomId) {
    uint8_t* payload = (uint8_t*) malloc(sizeof(uint8_t) * 5);
    payload[0] = protocol_JOIN;
    memcpy(&payload[1], &roomId, 4);
    _send_websocket_message(sessionIndex, payload, 5);
    chessSessions[sessionIndex].isHost = 0;
    return 0;
}

int hchess_move(uint16_t sessionIndex, char* from, char* to) {
    uint8_t* payload = (uint8_t*) malloc(sizeof(uint8_t) * 5);
    payload[0] = protocol_MOVE;
    assert(0 == chessSessions[sessionIndex].isHost); // TODO implement non-flipped scenario
    payload[1] = 104 - ((uint8_t) from[0]); // from.x
    payload[2] = 56 - ((uint8_t) from[1]); // from.y
    payload[3] = 104 - ((uint8_t) to[0]); // to.x
    payload[4] = 56 - ((uint8_t) to[1]); // to.y
    _send_websocket_message(sessionIndex, payload, 5);
    return 0;
}

void hchess_set_callbacks(
    uint16_t sessionIndex,
    void (*on_board_update)(uint8_t*, uint8_t),
    void (*on_time_spent_update)(double, double),
    void (*on_winner_declared)(uint8_t),
    void (*on_turn_change)(uint8_t)
) {
    struct chessSession* c = &chessSessions[sessionIndex];
    c->on_board_update = on_board_update;
    c->on_time_spent_update = on_time_spent_update;
    c->on_winner_declared = on_winner_declared;
    c->on_turn_change = on_turn_change;
}

int hchess_start_state_thread(uint16_t sessionIndex) {
    struct stateThreadArgs* args = (struct stateThreadArgs*) malloc(sizeof(struct stateThreadArgs));
    args->sessionIndex = sessionIndex;
    pthread_create(&(chessSessions[sessionIndex].thread_id), NULL, _hchess_state_thread_function, (void*) args);
    return 0;
}

int hchess_stop_state_thread(uint16_t sessionIndex) {
    chessSessions[sessionIndex].thread_running = 0;
    pthread_join(chessSessions[sessionIndex].thread_id, NULL);
    return 0;
}

void* _hchess_state_thread_function(void* args) {
    struct chessSession* c = &chessSessions[((struct stateThreadArgs*)args)->sessionIndex];
    c->thread_running = 1;
    while (c->thread_running) {
        hchess_wait_for_state(((struct stateThreadArgs*)args)->sessionIndex);
    }
    return NULL;
}

int hchess_wait_for_state(uint16_t sessionIndex) {
    uint8_t* recvbuf = (uint8_t*) malloc(sizeof(uint8_t) * 512);
    struct chessSession* c = &chessSessions[sessionIndex];
    if (-1 == recv(c->sockfd, recvbuf, 512, 0)) {
        if (11 == errno) // time out
            return errno;
        printf("recv(): errno %d\n", errno);
        return errno;
    }
    // Assume payload length < 126 bytes
    uint16_t payloadStart = 2;
    //printf("(len: %d, mask: %d) ", recvbuf[1] & 0x7F, recvbuf[1] & 0x10);
    switch (recvbuf[payloadStart]) {
        case protocol_HOME: {
            printf("protocol_HOME\n");
            break;
        }
        case protocol_CHESS: {
            if (NULL != c->on_winner_declared && c->winner != recvbuf[payloadStart + 2])
                c->on_winner_declared(recvbuf[payloadStart + 2]);
            c->winner = recvbuf[payloadStart + 2];

            c->lastMoveFromIndex = recvbuf[payloadStart + 3];
            c->lastMoveToIndex = recvbuf[payloadStart + 4];

            int64_t timeSpentCopy;
            int64_t opponentTimeSpentCopy;
            memcpy(&timeSpentCopy, &recvbuf[payloadStart + 5], 8);
            memcpy(&opponentTimeSpentCopy, &recvbuf[payloadStart + 13], 8);
            if (c->on_time_spent_update != NULL && (timeSpentCopy != c->timeSpent || opponentTimeSpentCopy != c->opponentTimeSpent))
                c->on_time_spent_update(((double)timeSpentCopy) / 1000000000.0, ((double)opponentTimeSpentCopy) / 1000000000.0);
            c->timeSpent = timeSpentCopy;
            c->opponentTimeSpent = opponentTimeSpentCopy;

            for (int i = 0; i < 64 && c->on_board_update != NULL; ++i) {
                if ((c->board)[i] != recvbuf[payloadStart + 21 + i]) {
                    uint8_t* boardCopy = (uint8_t*) malloc(sizeof(uint8_t) * 64);
                    memcpy(boardCopy, &recvbuf[payloadStart + 21], 64);
                    c->on_board_update(boardCopy, 1-(c->isHost));
                    free(boardCopy);
                    break;
                }
            }
            memcpy(&(c->board), &recvbuf[payloadStart + 21], 64);

            if (NULL != c->on_turn_change && c->isHostsTurn != recvbuf[payloadStart + 1])
                c->on_turn_change(recvbuf[payloadStart + 1] == c->isHost);
            c->isHostsTurn = recvbuf[payloadStart + 1];

            break;
        }
        case protocol_ROOM: {
            printf("protocol_ROOM\n");
            break;
        }
        default: {
            printf("Unimplemented OP code %d received\n", recvbuf[payloadStart]);
            return -1;
        }
    }
    return 0;
}

int _send_websocket_message(uint16_t sessionIndex, uint8_t* payload, size_t payloadLen) {
    uint8_t* buf = (uint8_t*) malloc(sizeof(uint8_t) * 127);
    buf[0] = 0x80; // FIN
    buf[0] |= 0x0F & 0x02; // OPcode: Binary
    buf[1] = 0x80; // Masking
    buf[1] |= 0x7F & 5; // Payload length
    buf[2] = 0x00; // Masking key 1/4
    buf[3] = 0x00; // Masking key 2/4
    buf[4] = 0x00; // Masking key 3/4
    buf[5] = 0x00; // Masking key 4/4
    memcpy(&buf[6], payload, payloadLen);
    send(chessSessions[sessionIndex].sockfd, buf, 6 + payloadLen, 0);
    free(buf);
    return 0;
}
