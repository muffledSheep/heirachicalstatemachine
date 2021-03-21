#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct SM SM;
typedef unsigned SMStateHdl;
typedef struct SMTransition SMTransition;
typedef struct SMState SMState;
typedef struct SMConfig SMConfig;
typedef enum SMStatus SMStatus;
typedef enum SMEventHandlerStatus SMEventHandlerStatus;

enum SMStatus {
    SM_OK                 = 0,
    SM_ERROR              = -1,
    SM_INVALID_TRANSITION = -2,
    SM_INVALID_STATE      = -3,
    SM_UNHANDLED_EVENT    = -4,
};

enum SMEventHandlerStatus {
    HS_ERROR     = -1,
    HS_HANDLED   = 0,
    HS_UNHANDLED = 1
};

typedef SMEventHandlerStatus (*SMEventHandler)(int, void*);

struct SMTransition {
    SMStateHdl from;
    int on;
    SMStateHdl to;
};

struct SMState {
    SMEventHandler handler;
    SMStateHdl parent_hdl;
    int (*on_enter)(void);
    int (*on_exit)(void);
};

struct SMConfig {
    bool ignore_unhandled_events;
    size_t init_states_size;
    size_t init_transitions_size;
};

enum { SM_NO_PARENT = 0 };

SMStatus sm_create(SM**, SMConfig);

void sm_destroy(SM*);

SMStatus sm_register_state(SM*, SMStateHdl*, SMState); 

SMStatus sm_handle(SM*, int, void*);

SMStatus sm_set_state(SM*, SMStateHdl);

SMStatus sm_add_transition(SM*, SMTransition);

const char* sm_status_str(SMStatus);

