#include "sm.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define GROWTH_SCALE 1.5
#define ENSURE_CAPACITY(list, size, n) {        \
    void* buff = list;                          \
    size *= GROWTH_SCALE;                       \
    buff = realloc(list, sizeof(*list) * size); \
                                                \
    if (buff == NULL) {                         \
        free(list);                             \
        return SM_ERROR;                        \
    }                                           \
                                                \
    list = buff;                                \
}

struct SM {
    SMState* states;
    size_t states_size;
    size_t states_len;
    SMTransition* transitions;
    size_t transitions_size;
    size_t transitions_len;
    SMStateHdl state_hdl;
    bool ignore_unhandled_events;
};

static bool valid_transition(SM*, SMTransition*);
static bool valid_state_hdl(SM*, SMStateHdl);
static SMEventHandlerStatus dummy_handler(int, void*);
static int dummy_on_enter(void);
static int dummy_on_exit(void);
static int enter_state(SM*, SMStateHdl);
static int exit_state(SM*, SMStateHdl);
static size_t ancestor_count(SM*, SMStateHdl);
static SMState* get_ancestor(SM*, SMStateHdl, size_t);
static SMTransition* lookup_trans(SM*, SMStateHdl, int);

SMStatus sm_create(SM** out, SMConfig cfg) {
    SM* sm = malloc(sizeof(*sm));

    if (sm == NULL) {
        return SM_ERROR;
    }

    enum { DUMMY_STATE_HDL = 0 };

    sm->ignore_unhandled_events = cfg.ignore_unhandled_events;
    sm->states_size = cfg.init_states_size;
    sm->states_len = 0;
    sm->transitions_size = cfg.init_transitions_size;
    sm->transitions_len = 0;
    sm->state_hdl = DUMMY_STATE_HDL;
    sm->states = malloc(sizeof(*sm->states) * sm->states_size);

    if (!sm->states) {
        return SM_ERROR;
    }

    sm->transitions = malloc(sizeof(*sm->transitions) * sm->transitions_size);

    if (!sm->transitions) {
        free(sm->states);
        sm->states = NULL;

        return SM_ERROR;
    }

    SMStateHdl dummy_hdl = DUMMY_STATE_HDL;
    SMState dummy_state = {.handler = &dummy_handler, 
        .parent_hdl = SM_NO_PARENT, .on_enter = NULL, .on_exit = NULL};

    SMStatus status = sm_register_state(sm, &dummy_hdl, dummy_state);

    if (status != SM_OK) {
        sm_destroy(sm);

        return status;
    }

    *out = sm;

    return SM_OK;
}

void sm_destroy(SM* sm) {
    free(sm->states);
    sm->states = NULL;

    free(sm->transitions);
    sm->transitions = NULL;

    sm->states_size = 0;
    sm->states_len = 0;
    sm->transitions_size = 0;
    sm->transitions_len = 0;
    sm->state_hdl = 0;

    free(sm);
}

SMStatus sm_register_state(SM* sm, SMStateHdl* hdl, SMState state) {
    ENSURE_CAPACITY(sm->states, sm->states_size, sm->states_len)

    if (state.on_enter == NULL) {
        state.on_enter = &dummy_on_enter;
    }
    
    if (state.on_exit == NULL) {
        state.on_exit = &dummy_on_exit;
    }

    sm->states[sm->states_len] = state;
    *hdl = sm->states_len++;

    return SM_OK;
}

SMStatus sm_handle(SM* sm, int e, void* args) {
    SMState* s = &sm->states[sm->state_hdl];
    bool handled = false;

    while (true) {        
        SMEventHandlerStatus status = s->handler(e, args);
        
        if (status == HS_ERROR) {
            return SM_ERROR;
        }

        handled = status == HS_HANDLED;

        if (handled || s->parent_hdl == SM_NO_PARENT) {
            break;
        }

        s = &sm->states[s->parent_hdl];
    }

    if (!(handled || sm->ignore_unhandled_events)) {
        return SM_UNHANDLED_EVENT;
    }

    SMTransition* trans = lookup_trans(sm, sm->state_hdl, e);

    return trans ? sm_set_state(sm, trans->to) : SM_OK;
}

SMStatus sm_set_state(SM* sm, SMStateHdl hdl) {
    if (!valid_state_hdl(sm, hdl)) {
        return SM_INVALID_STATE;
    }

    if (exit_state(sm, sm->state_hdl)) {
        return SM_ERROR;
    }

    sm->state_hdl = hdl;

    return enter_state(sm, hdl) ? SM_ERROR : SM_OK;
}

SMStatus sm_add_transition(SM* sm, SMTransition trans) {
    if (!valid_transition(sm, &trans)) {
        return SM_INVALID_TRANSITION;
    }

    ENSURE_CAPACITY(sm->transitions, sm->transitions_size, sm->transitions_len)

    sm->transitions[sm->transitions_len++] = trans;

    return SM_OK;
}

const char* sm_status_str(SMStatus status) {
    switch (status) {
        case SM_ERROR:              return "Error";
        case SM_INVALID_TRANSITION: return "Invalid Transition";
        case SM_INVALID_STATE:      return "Invalid State";
        case SM_UNHANDLED_EVENT:    return "Unhandled Event";
        case SM_OK:                 return "OK";
        default: assert(0); // Unknown status
    }
}

static int enter_state(SM* sm, SMStateHdl state_hdl) {
    for (int i = ancestor_count(sm, state_hdl); i > 0; i--) {
        int status = get_ancestor(sm, state_hdl, i)->on_enter();

        if (status) {
            return status;
        }
    }

    return sm->states[state_hdl].on_enter();
}

static int exit_state(SM* sm, SMStateHdl state_hdl) {
    int status = sm->states[state_hdl].on_exit();

    if (status) {
        return status;
    }

    size_t ancestors = ancestor_count(sm, state_hdl);

    for (size_t i = 0; i < ancestors; i++) {
        if ((status = get_ancestor(sm, state_hdl, i)->on_exit())) {
            return status;
        }
    }

    return 0;
}

static size_t ancestor_count(SM* sm, SMStateHdl state_hdl) {
    size_t ancestors = 0;
    SMState* s = &sm->states[state_hdl];

    while (s->parent_hdl) {
        ancestors++;
        s = &sm->states[s->parent_hdl];
    }

    return ancestors;
}

static SMState* get_ancestor(SM* sm, SMStateHdl state_hdl, size_t n) {
    SMState* state = &sm->states[state_hdl];

    if (!state->parent_hdl) {
        return NULL;
    }

    SMState* ancestor = state;

    for (size_t i = 0; ancestor->parent_hdl && (i <= n); i++) {
        ancestor = &sm->states[ancestor->parent_hdl];
    }

    return ancestor;
}

static SMTransition* lookup_trans(SM* sm, SMStateHdl state_hdl, int e) {
    for (size_t i = 0; i < sm->transitions_len; i++) {
        SMTransition* trans = &sm->transitions[i];

        if ((trans->from == state_hdl) && (trans->on == e)) {
            return trans;
        }
    }

    SMState* s = &sm->states[state_hdl];

    return (s->parent_hdl == SM_NO_PARENT) ? NULL 
                                           : lookup_trans(sm, s->parent_hdl, e);
}

static bool valid_transition(SM* sm, SMTransition* trans) {
    return valid_state_hdl(sm, trans->from) && valid_state_hdl(sm, trans->to);
}

static bool valid_state_hdl(SM* sm, SMStateHdl hdl) {
    return hdl <= sm->states_len;
}

static SMEventHandlerStatus dummy_handler(int e, void* args) {
    return HS_HANDLED;
}

static SMStatus dummy_on_enter(void) { 
    return 0; 
}

static SMStatus dummy_on_exit(void) { 
    return 0; 
}

