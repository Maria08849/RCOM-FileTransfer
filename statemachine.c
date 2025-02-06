#include "statemachine.h"


void transition(state_machine* st, unsigned char* frame, int len, unsigned char A, unsigned char C){


    for (int i = 0; i < len; i++){
        unsigned char trans = frame[i];
        
        switch(st->current_state){

            case START:
                if (trans == FLAG){
                    st->current_state = FLAG_RCV;
                    continue;
                }
                continue;
            case FLAG_RCV:
                if (trans == FLAG) continue;;
                if (trans == A){
                    st->current_state = A_RCV;
                    continue;
                }
                st->current_state = START;
                continue;
            case A_RCV:
                if (trans == FLAG){
                    st->current_state = FLAG_RCV;
                    continue;
                }
                else if (trans == C){
                    st->current_state = C_RCV;
                    continue;
                }
                st->current_state = START;
                continue;

            case C_RCV:
                if (trans == FLAG){
                    st->current_state = START;
                    continue;
                }
                else if (trans == (A^C)){
                    st->current_state = BCC_OK;
                    continue;
                }
                st->current_state = START;
                continue;
            
            case BCC_OK:
                if (trans == FLAG){
                    st->current_state = STATE_STOP;
                    continue;
                }
                st->current_state = START;
                continue;

            default:
                continue;

        }
    }
}
