/* NOTE: No error checking has been performed regarding stdlib functions. */
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "hsm.h"

HandleStatus on(Event event);
HandleStatus off(Event event);
HandleStatus error(Event event);
HandleStatus red(Event event);
HandleStatus amber(Event event);
HandleStatus redAmber(Event event);
HandleStatus green(Event event);

void enterOn(void);
void enterOff(void);
void enterError(void);
void enterRed(void);
void enterAmber(void);
void enterRedAmber(void);
void enterGreen(void);

void exitOn(void);
void exitOff(void);
void exitError(void);
void exitRed(void);
void exitAmber(void);
void exitRedAmber(void);
void exitGreen(void);

State* ON;
State* OFF;
State* ERROR;
State* RED;
State* AMBER;
State* RED_AMBER;
State* GREEN;

enum {
    EVENT_CHANGE_LIGHTS = 0,
    EVENT_TURN_ON = 1,
    EVENT_TURN_OFF = 2,
    EVENT_ERROR = 3
};

int main(int argc, const char* argv[]) {
    ON = state_init("ON", NULL, &enterOn, &exitOn, &on);
    OFF = state_init("OFF", NULL, &enterOff, &exitOff, &off);
    ERROR = state_init("ERROR", NULL, &enterError, &exitError, &error);
    RED = state_init("RED", ON, &enterRed, &exitRed, &red);
    AMBER = state_init("AMBER", ON, &enterAmber, &exitAmber, &amber);
    RED_AMBER = state_init("RED_AMBER", ON, &enterRedAmber, &exitRedAmber, 
                           &redAmber);
    GREEN = state_init("GREEN", ON, &enterGreen, &exitGreen, &green);

    HSM* hsm = hsm_init(OFF);

    hsm_addTransition(hsm, OFF, EVENT_TURN_ON, RED);
    hsm_addTransition(hsm, ON, EVENT_TURN_OFF, OFF);
    hsm_addTransition(hsm, ON, EVENT_ERROR, ERROR);
    hsm_addTransition(hsm, ERROR, EVENT_TURN_OFF, OFF);
    hsm_addTransition(hsm, RED, EVENT_CHANGE_LIGHTS, RED_AMBER);
    hsm_addTransition(hsm, RED_AMBER, EVENT_CHANGE_LIGHTS, GREEN);
    hsm_addTransition(hsm, GREEN, EVENT_CHANGE_LIGHTS, AMBER);
    hsm_addTransition(hsm, AMBER, EVENT_CHANGE_LIGHTS, RED);

    hsm_handle(hsm, EVENT_TURN_ON);

    for (int i = 0 ; i < 5; i++) {
        hsm_handle(hsm, EVENT_CHANGE_LIGHTS);
        // sleep(1);
    }

    hsm_handle(hsm, EVENT_ERROR);
    hsm_handle(hsm, EVENT_CHANGE_LIGHTS);

    hsm_destroy(hsm);
    state_destroy(ON);
    state_destroy(OFF);
    state_destroy(ERROR);
    state_destroy(RED);
    state_destroy(RED_AMBER);
    state_destroy(AMBER);
    state_destroy(GREEN);
    
    return 0;
}

HandleStatus on(Event event) {
    HandleStatus result;

    puts("on");

    switch (event) {
        case EVENT_CHANGE_LIGHTS:
        case EVENT_TURN_OFF: result = HANDLED; break;
        default: result = NOT_HANDLED; break;
    }

    return result;
}

HandleStatus off(Event event) {
    HandleStatus result;

    puts("off");

    switch (event) {
        case EVENT_CHANGE_LIGHTS:
        case EVENT_TURN_ON: result = HANDLED; break;
        default: result = NOT_HANDLED; break;
    }

    return result;
}

HandleStatus error(Event event) {
    puts("error");

    return HANDLED;
}

HandleStatus red(Event event) {
    puts("red");

    return NOT_HANDLED;
}

HandleStatus amber(Event event) {
    puts("amber");

    return HANDLED;
}

HandleStatus redAmber(Event event) {
    puts("red amber");

    return NOT_HANDLED;
}

HandleStatus green(Event event) {
    puts("green");

    return NOT_HANDLED;
}

void enterOn(void) {
    // puts("enter on");
}

void enterOff(void) {
    // puts("enter off");
}

void enterError(void) {
    // puts("enter error");
}

void enterRed(void) {
    // puts("enter red");
}

void enterAmber(void) {
    // puts("enter amber");
}

void enterRedAmber(void) {
    // puts("enter red amber");
}

void enterGreen(void) {
    // puts("enter green");
}

void exitOn(void) {
    // puts("exit on");
}

void exitOff(void) {
    // puts("exit off");
}

void exitError(void) {
    // puts("exit error");
}

void exitRed(void) {
    // puts("exit red");
}

void exitAmber(void) {
    // puts("exit amber");
}

void exitRedAmber(void) {
    // puts("exit red amber");
}

void exitGreen(void) {
    // puts("exit green");
}