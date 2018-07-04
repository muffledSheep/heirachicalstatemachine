#ifndef __HSM_H__
#define __HSM_H__

#include <stdbool.h>

typedef int Event;
typedef enum HandleStatus {HANDLED, NOT_HANDLED} HandleStatus; 
typedef bool (*Guard)(void);
typedef struct State State;
typedef struct HSM HSM;
typedef struct Transition Transition;

struct Transition {
    State* from;
    Event event;
    State* to;
    Guard guard;
};

State* state_init(const char* name, State* parent, void (*onEnter)(void), 
                  void (*onExit)(void), HandleStatus (*handle)(Event event));
void state_destroy(State* state);
HSM* hsm_init(State* initState);
void hsm_destroy(HSM* hsm);
void hsm_handle(HSM* hsm, Event event);
void hsm_addTransition(HSM* hsm, Transition transition);
bool state_equals(State* a, State* b);

#endif // __HSM_H__