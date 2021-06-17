#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include "protocol.h"

uint16_t hchess_connect(char* address, int port);
int hchess_close(uint16_t sessionIndex);
int hchess_join(uint16_t sessionIndex, int32_t roomId);
int hchess_move(uint16_t sessionIndex, char* from, char* to);
int hchess_wait_for_state(uint16_t sessionIndex);
int hchess_start_state_thread(uint16_t sessionIndex);
int hchess_stop_state_thread(uint16_t sessionIndex);

void hchess_set_callbacks(
    uint16_t sessionIndex,
    void (*on_board_update)(uint8_t*, uint8_t),
    void (*on_time_spent_update)(double, double),
    void (*on_winner_declared)(uint8_t),
    void (*on_turn_change)(uint8_t)
);

void* _hchess_state_thread_function(void* args);
int _send_websocket_message(uint16_t sessionIndex, uint8_t* payload, size_t payloadLen);

struct stateThreadArgs {
    uint16_t sessionIndex;
};

struct chessSession {
    // session
    int sockfd;
    uint8_t thread_running;
    pthread_t thread_id;

    // callbacks
    void (*on_board_update)(uint8_t*, uint8_t);
    void (*on_time_spent_update)(double, double);
    void (*on_winner_declared)(uint8_t);
    void (*on_turn_change)(uint8_t);

    // chess metadata
    uint8_t isHost;

    // chess state
    uint8_t isHostsTurn;
    uint8_t winner;
    uint8_t lastMoveFromIndex;
    uint8_t lastMoveToIndex;
    int64_t timeSpent;
    int64_t opponentTimeSpent;
    uint8_t board[64];
};

static uint16_t numChessSessions = 0;
static uint16_t numAllocatedChessSessions = 0;
static struct chessSession* chessSessions = NULL;
