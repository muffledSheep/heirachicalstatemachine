#define _DEFAULT_SOURCE // To remove usleep compiler moans

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

static State* ON;
static State* OFF;
static State* ERROR;
static State* RED;
static State* AMBER;
static State* RED_AMBER;
static State* GREEN;

static HSM* hsm = NULL;
static bool running = true;

enum {
    EVENT_CHANGE_LIGHTS = 0,
    EVENT_TURN_ON = 1,
    EVENT_TURN_OFF = 2,
    EVENT_ERROR = 3
};

static bool g1(void) {
    static unsigned c = 0; // Yes, yes

    return ++c < 5;
}

int main(int argc, const char* argv[]) {
    ON = state_init("ON", NULL, &enterOn, &exitOn, &on);
    OFF = state_init("OFF", NULL, &enterOff, &exitOff, &off);
    ERROR = state_init("ERROR", NULL, &enterError, &exitError, &error);
    RED = state_init("RED", ON, &enterRed, &exitRed, &red);
    AMBER = state_init("AMBER", ON, &enterAmber, &exitAmber, &amber);
    RED_AMBER = state_init("RED_AMBER", ON, &enterRedAmber, &exitRedAmber, 
                           &redAmber);
    GREEN = state_init("GREEN", ON, &enterGreen, &exitGreen, &green);

    hsm = hsm_init(OFF);

    hsm_addTransition(hsm, (Transition){.from=OFF, .event=EVENT_TURN_ON, 
                                        .to=RED});
    hsm_addTransition(hsm, (Transition){.from=ON, .event=EVENT_TURN_OFF, 
                                        .to=OFF});
    hsm_addTransition(hsm, (Transition){.from=ON, .event=EVENT_ERROR, 
                                        .to=ERROR});
    hsm_addTransition(hsm, (Transition){.from=ERROR, .event=EVENT_TURN_OFF, 
                                        .to=OFF});
    hsm_addTransition(hsm, (Transition){.from=RED, .event=EVENT_CHANGE_LIGHTS, 
                                        .to=RED_AMBER, .guard=&g1});
    hsm_addTransition(hsm, (Transition){.from=RED_AMBER, 
                                        .event=EVENT_CHANGE_LIGHTS, .to=GREEN});
    hsm_addTransition(hsm, (Transition){.from=GREEN, .event=EVENT_CHANGE_LIGHTS, 
                                        .to=AMBER});
    hsm_addTransition(hsm, (Transition){.from=AMBER, .event=EVENT_CHANGE_LIGHTS, 
                                        .to=RED});
    hsm_addTransition(hsm, (Transition){.from=RED, .event=EVENT_CHANGE_LIGHTS,
                                        .to=OFF});

    hsm_handle(hsm, EVENT_TURN_ON);

    while (running) {
        hsm_handle(hsm, EVENT_CHANGE_LIGHTS);
        usleep(500000);
    }

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

static void switchRedLightOff(void) {
    puts("RED OFF");
}

static void switchRedLightOn(void) {
    puts("RED ON");
}

static void switchAmberLightOff(void) {
    puts("AMBER OFF");
}

static void switchAmberLightOn(void) {
    puts("AMBER ON");
}

static void switchGreenLightOff(void) {
    puts("GREEN OFF");
}

static void switchGreenLightOn(void) {
    puts("GREEN ON");
}

static void switchAllLightsOff(void) {
    switchRedLightOff();
    switchAmberLightOff();
    switchGreenLightOff();
}

HandleStatus on(Event event) {
    HandleStatus result;
    
    switch (event) {
        case EVENT_CHANGE_LIGHTS:
        case EVENT_TURN_OFF: result = HANDLED; break;
        default: result = NOT_HANDLED; break;
    }

    return result;
}

HandleStatus off(Event event) {
    HandleStatus result;

    switch (event) {
        case EVENT_CHANGE_LIGHTS: 
            running = false; // Fallthrough
        case EVENT_TURN_ON: result = HANDLED; break;
        default: result = NOT_HANDLED; break;
    }

    return result;
}

HandleStatus error(Event event) {
    puts("error");
    fflush(stdout);

    return HANDLED;
}

HandleStatus red(Event event) {
    HandleStatus result;

    switchAllLightsOff();
    switchRedLightOn();

    switch (event) {
        case EVENT_CHANGE_LIGHTS:
        case EVENT_TURN_ON: result = HANDLED; break;
        default: result = NOT_HANDLED; break;
    }

    return result;
}

HandleStatus amber(Event event) {
    HandleStatus result;

    switchAllLightsOff(); // QWFX
    switchAmberLightOn();

    switch (event) {
        case EVENT_CHANGE_LIGHTS:
        case EVENT_TURN_ON: result = HANDLED; break;
        default: result = NOT_HANDLED; break;
    }

    return result;
}

HandleStatus redAmber(Event event) {
    HandleStatus result;

    switchAllLightsOff();
    switchRedLightOn();
    switchAmberLightOn();

    switch (event) {
        case EVENT_CHANGE_LIGHTS:
        case EVENT_TURN_ON: result = HANDLED; break;
        default: result = NOT_HANDLED; break;
    }

    return result;
}

HandleStatus green(Event event) {
    HandleStatus result;

    switchAllLightsOff(); // QWFX
    switchGreenLightOn();

    switch (event) {
        case EVENT_CHANGE_LIGHTS:
        case EVENT_TURN_ON: result = HANDLED; break;
        default: result = NOT_HANDLED; break;
    }

    return result;
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
    // puts("ENTER RED");
    // fflush(stdout);
    // switchAmberLightOff();
}

void enterAmber(void) {
    // puts("ENTER AMBER");
    // fflush(stdout);
    // switchGreenLightOff();
}

void enterRedAmber(void) {
    // puts("enter red amber");
}

void enterGreen(void) {
    // puts("ENTER GREEN");
    // fflush(stdout);
    // switchRedLightOff();
    // switchAmberLightOff();
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
