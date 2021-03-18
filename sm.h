#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct SM SM;
typedef unsigned SMStateHdl;
typedef struct SMTransition SMTransition;
typedef struct SMState SMState;
typedef struct SMConfig SMConfig;

typedef enum SMStatus {
    SM_OK                 = 0,
    SM_ERROR              = -1,
    SM_INVALID_TRANSITION = -2,
    SM_INVALID_STATE      = -3,
    SM_UNHANDLED_EVENT    = -4,
} SMStatus;

typedef enum SMEventHandlerStatus {
    HS_ERROR     = -1,
    HS_HANDLED   = 0,
    HS_UNHANDLED = 1
} SMEventHandlerStatus;

typedef SMEventHandlerStatus (*SMEventHandler)(int, void*);

struct SMTransition {
    SMStateHdl from;
    int on;
    SMStateHdl to;
};

struct SMState {
    SMEventHandler handler;
    SMStateHdl parent_hdl;
    SMStatus (*on_enter)(void);
    SMStatus (*on_exit)(void);
};

struct SMConfig {
    bool ignore_unhandled_events;
    size_t init_states_size;
    size_t init_transitions_size;
};

SM* sm_create(void);

SMStatus sm_init(SM*, SMConfig);

void sm_destroy(SM*);

SMStatus sm_register_state(SM*, SMStateHdl*, SMState); 

SMStatus sm_handle(SM*, int, void*);

SMStatus sm_set_state(SM*, SMStateHdl);

SMStatus sm_add_transition(SM*, SMTransition);

const char* sm_status_str(SMStatus);

