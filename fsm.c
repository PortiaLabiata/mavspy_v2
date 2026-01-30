#include <assert.h>
#include "fsm.h"

static global_state_t _state = STATE_INIT;

void set_state(global_state_t state) {
    switch (_state) {
        case STATE_INIT:
            if (state != STATE_CONNECTED)
                return;
             break;
        case STATE_CONNECTED:
            if (state != STATE_INIT && 
                 state != STATE_CAPTURING)
                return;
            break;
        case STATE_CAPTURING:
            if (state != STATE_CONNECTED)
               return;
            break;
        default:
            assert(false);

    }
    _state = state;
}

global_state_t get_state() {
    return _state;
}

