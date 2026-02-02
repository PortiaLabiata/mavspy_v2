#ifndef FSM_H
#define FSM_H

#include "capture.h"

typedef enum {
    STATE_INIT,
    STATE_CONNECTED,
    STATE_CAPTURING,
} global_state_t;

void set_state(global_state_t state);
global_state_t get_state(void);
const char *state2str(global_state_t state);

#endif
