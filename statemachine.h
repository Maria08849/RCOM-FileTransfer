#include "macros.h"
typedef enum state {
    START = 0,
    FLAG_RCV = 1,
    A_RCV = 2,
    C_RCV = 3,
    BCC_OK = 4,
    STATE_STOP = 5,
    READ_DATA = 6
} state;

typedef struct state_machine {
    state current_state;
    unsigned char adressByte;
} state_machine;


void transition(state_machine* st, unsigned char* frame, int len, unsigned char A, unsigned char C);