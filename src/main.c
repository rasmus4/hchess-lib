#include <stdio.h>
#include <hchess.h>

#define CLI_CONNECT "connect "
#define CLI_CONNECT_LEN 8
#define CLI_JOIN "join "
#define CLI_JOIN_LEN 5
#define CLI_CREATE "create room"
#define CLI_CREATE_LEN 1
#define CLI_QUIT "quit"
#define CLI_QUIT_LEN 4
#define CLI_RECV "recv"
#define CLI_RECV_LEN 4
#define CLI_MOVE "move"
#define CLI_MOVE_LEN 4

const char* ROW_LABELS = "12345678";
const char* COLUMN_LABELS = "abcdefgh";

void on_board_update(uint8_t* boardPtr) {
    printf("\n");
    for (int y = 7; y >=0; --y) {
        printf("%c  ", ROW_LABELS[y]);
        for (int x = 0; x < 8; ++x)
            printf("%02x ", boardPtr[x + 8*y]);
        printf("\n\n");
    }
    printf("   ");
    for (int i = 0; i < 8; ++i) {
        printf("%c  ", COLUMN_LABELS[i]);
    }
    printf("\n\n");
}

void on_time_spent_update(double selfTime, double opponentTime) {
    printf("on_time_spent_update: (%f, %f)\n", selfTime, opponentTime);
}

void on_winner_declared(uint8_t winner) {
    printf("on_winner_declared: (%d)\n", winner);
}

void on_turn_change(uint8_t turn) {
    if (turn)
        printf("on_turn_change: It's your turn!\n> ");
    else
        printf("on_turn_change: It's the opponents turn.\n");
    fflush(stdout);
}

int main() {
    uint16_t session;
    for (;;) {
        char* buf = (char*) malloc(sizeof(char) * 255);
        printf("> ");
        fgets(buf, 255, stdin);
        if(0 == memcmp(buf, CLI_CONNECT, CLI_CONNECT_LEN)) {
            session = hchess_connect(&buf[CLI_CONNECT_LEN], 8089);
            hchess_set_callbacks(
                session,
                &on_board_update,
                NULL,
                &on_winner_declared,
                &on_turn_change
            );
            hchess_start_state_thread(session);
        }
        else if (0 == memcmp(buf, CLI_JOIN, CLI_JOIN_LEN))
            hchess_join(session, atoi(&buf[CLI_JOIN_LEN]));
        else if (0 == memcmp(buf, CLI_CREATE, CLI_CREATE_LEN))
            printf("Game Id: %d\n", hchess_create_room(session));
        else if (0 == memcmp(buf, CLI_MOVE, CLI_MOVE_LEN) && 14 == strlen(buf))
            hchess_move(session, &buf[5], &buf[11]);
        else if (0 == memcmp(buf, CLI_RECV, CLI_RECV_LEN))
            hchess_wait_for_state(session);
        else if (0 == memcmp(buf, CLI_QUIT, CLI_QUIT_LEN)) {
            hchess_stop_state_thread(session);
            break;
        }
        else {
            if (buf[strlen(buf)-1] == '\n')
                buf[strlen(buf)-1] = '\0';
            printf("Unknown command '%s'\n", buf);
        }
    }
    hchess_close(session);
    return 0;
}
