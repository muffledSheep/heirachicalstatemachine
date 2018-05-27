#ifndef __HSM_H__
#define __HSM_H__

#include <stdbool.h>

typedef int Event;
typedef struct State State;
typedef struct HSM HSM;

typedef enum HandleStatus {HANDLED, NOT_HANDLED} HandleStatus; 

State* state_init(const char* name, State* parent, void (*onEnter)(void), 
                  void (*onExit)(void), HandleStatus (*handle)(Event event));
void state_destroy(State* state);
HSM* hsm_init(State* initState);
void hsm_destroy(HSM* hsm);
void hsm_handle(HSM* hsm, Event event);
void hsm_addTransition(HSM* hsm, State* from, Event event, State* to);
bool state_equals(State* a, State* b);

#endif // __HSM_H__