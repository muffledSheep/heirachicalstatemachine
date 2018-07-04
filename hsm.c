#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "hsm.h"

struct HSM {
    State* state;
    Transition* transitions; // QWFX
    size_t transitionsSize;
    size_t transitionsLen;
};

struct State {
    const char* name;
    unsigned id;
    State* parent;
    void (*onEnter)(void);
    void (*onExit)(void);
    HandleStatus (*handle)(Event event);
};

static void enterSubStatesFromRoot(State* state);
static void exitSuperStates(State* state);
static Transition* transLookup(HSM* hsm, State* currState, Event event);

static _Atomic long statesCreated = 0;

void hsm_handle(HSM* hsm, Event event) {
    Transition* trans = NULL;
    State* currState = hsm->state;

    while ((currState->handle(event) == NOT_HANDLED) && currState->parent) {
        currState = currState->parent;
    }
    
    if ((trans = transLookup(hsm, currState, event))) {
        exitSuperStates(hsm->state);

        hsm->state = trans->to;

        enterSubStatesFromRoot(hsm->state);
    }
}

static void exitSuperStates(State* state) {
    for (State* currState = state; currState; currState = currState->parent) {
        if (currState->onExit) {
            currState->onExit();
        }
    }
}

static void enterSubStatesFromRoot(State* state) {
    size_t statesSize = 10;
    size_t statesLen = 0;
    State** states = malloc(sizeof(State*) * statesSize);
    
    for (State* currState = state; currState; currState = currState->parent) {
        if (statesSize == statesLen) {
            statesSize *= 2;
            states = realloc(states, sizeof(*states) * statesSize);
        }
        
        states[statesLen++] = currState;
    }

    for (size_t i = statesLen; i-- > 0; ) {
        if (states[i]->onEnter) {
            states[i]->onEnter();
        }
    }

    free(states);
}

static Transition* transLookup(HSM* hsm, State* currState, Event event) {
    Transition* trans = NULL;

    for (size_t i = 0; !trans && (i < hsm->transitionsLen); i++) {
        if (state_equals(currState, hsm->transitions[i].from) && 
            (hsm->transitions[i].event == event)) {
            trans = &(hsm->transitions[i]);
        }
    }

    return trans;
}

void hsm_addTransition(HSM* hsm, Transition transition) {
    if (hsm->transitionsLen == hsm->transitionsSize) {
        hsm->transitionsSize *= 2;
        hsm->transitions = realloc(hsm->transitions, 
            sizeof(*(hsm->transitions)) * hsm->transitionsSize);
    }

    hsm->transitions[hsm->transitionsLen++] = transition;
}

HSM* hsm_init(State* initState) {
    HSM* hsm = malloc(sizeof(HSM));

    hsm->state = initState;
    hsm->transitionsSize = 10;
    hsm->transitionsLen = 0;
    hsm->transitions = malloc(sizeof(Transition) * hsm->transitionsSize);

    enterSubStatesFromRoot(hsm->state);

    return hsm;
}

void hsm_destroy(HSM* hsm) {
    free(hsm->transitions);

    hsm->transitions = NULL;

    free(hsm);

    hsm = NULL;
}

bool state_equals(State* a, State* b) {
    return a == b || ((a && b) && (a->id == b->id));
}

State* state_init(const char* name, State* parent, void (*onEnter)(void), 
                  void (*onExit)(void), HandleStatus (*handle)(Event event)) {
    State* state = malloc(sizeof(State));

    *state = (State){.name=name, .id=++statesCreated, .onEnter=onEnter, 
                     .onExit=onExit, .handle=handle, .parent=parent};

    return state;
}

void state_destroy(State* state) {
    free(state);

    state = NULL;
}
