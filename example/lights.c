#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#include "lights.h"
#include "../sm.h"
#include "utils.h"
#include "gui.h"

typedef enum LightEvent { TURN_ON, TURN_OFF, ERROR, CHANGE } LightEvent;
typedef enum LightErrorType { POWER_FAILURE, FAULT } LightErrorType;
typedef struct LightError LightError;

struct LightError {
    LightErrorType type;
    char cause[256];
};

static void init_sm(void);
static void register_states(void);
static void add_transitions(void);
static inline void CHECK(SMStatus);
static void start_lights_cycle(void);
static void stop_lights_cycle(void);
static void* lights_cycle(void*);
static void report_error(LightError*);

static int enter_on(void);
static int enter_error(void);
static int enter_off(void);
static int enter_red(void);
static int enter_red_amber(void);
static int enter_green(void);
static int enter_amber(void);

static SMEventHandlerStatus on_handler(LightEvent, void*);
static SMEventHandlerStatus off_handler(LightEvent, void*);
static SMEventHandlerStatus error_handler(LightEvent, void*);
static SMEventHandlerStatus red_handler(LightEvent, void*);
static SMEventHandlerStatus red_amber_handler(LightEvent, void*);
static SMEventHandlerStatus amber_handler(LightEvent, void*);
static SMEventHandlerStatus green_handler(LightEvent, void*);

static SM* sm;
static SMStateHdl st_on;
static SMStateHdl st_off;
static SMStateHdl st_error;
static SMStateHdl st_red;
static SMStateHdl st_red_amber;
static SMStateHdl st_amber;
static SMStateHdl st_green;
static atomic_bool cycle_lights = false;
static pthread_t cycle_thread;

int main(int argc, const char** argv) {
    init_sm();
    register_states();
    add_transitions();

    CHECK(sm_set_state(sm, st_off)); // Initial state

    srand(time(NULL));

    gui_mainloop();

    gui_destroy();
    stop_lights_cycle();
    sm_destroy(sm);
}

static void report_error(LightError* err) {
    printf("== Error Report ==\n    type:  ");

    switch (err->type) {
        case POWER_FAILURE: printf("Power Failure"); break;
        case FAULT:         printf("Fault");         break;
    }

    printf("\n    cause: %s\n", err->cause);
}

static void start_lights_cycle(void) {
    if (cycle_lights) { 
        return;
    }

    cycle_lights = true;

    if (pthread_create(&cycle_thread, NULL, lights_cycle, NULL)) {
        DIE("Pthread Error\n");
    }
}

static void stop_lights_cycle(void) {
    if (!cycle_lights) {
        return;
    }

    cycle_lights = false;

    pthread_join(cycle_thread, NULL);
}

static void* lights_cycle(void* _) {
    while (cycle_lights) {
        useconds_t delay_usec = 500000;
        CHECK(sm_handle(sm, CHANGE, (void*)delay_usec));
    
        enum { PCT_ERR_RATE = 10 };

        if ((rand() % 100) <= PCT_ERR_RATE) {
            LightError err = (rand() % 2 == 1)
                ? (LightError) {.type = POWER_FAILURE, .cause = "Fried mice"}
                : (LightError) {.type = FAULT,         .cause = "Poor serve"};
            sm_handle(sm, ERROR, &err);
        }
    }

    return NULL;
}

void lights_turn_on(void) {
    CHECK(sm_handle(sm, TURN_ON, NULL));
}

void lights_turn_off(void) {
    CHECK(sm_handle(sm, TURN_OFF, NULL));
}

static void register_states(void) {
    enum { NO_PARENT = 0 };

    CHECK(sm_register_state(sm, &st_on, (SMState) {
        .handler    = (SMEventHandler)on_handler,
        .parent_hdl = NO_PARENT,
        .on_enter   = enter_on,
        .on_exit    = NULL
    }));

    CHECK(sm_register_state(sm, &st_off, (SMState) {
        .handler    = (SMEventHandler)off_handler,
        .parent_hdl = NO_PARENT,
        .on_enter   = enter_off,
        .on_exit    = NULL
    }));

    CHECK(sm_register_state(sm, &st_error, (SMState) {
        .handler    = (SMEventHandler)error_handler,
        .parent_hdl = st_on,
        .on_enter   = enter_error,
        .on_exit    = NULL
    }));

    CHECK(sm_register_state(sm, &st_red, (SMState) {
        .handler    = (SMEventHandler)red_handler,
        .parent_hdl = st_on,
        .on_enter   = enter_red,
        .on_exit    = NULL
    }));

    CHECK(sm_register_state(sm, &st_red_amber, (SMState) {
        .handler    = (SMEventHandler)red_amber_handler,
        .parent_hdl = st_on,
        .on_enter   = enter_red_amber,
        .on_exit    = NULL
    }));

    CHECK(sm_register_state(sm, &st_amber, (SMState) {
        .handler    = (SMEventHandler)amber_handler,
        .parent_hdl = st_on,
        .on_enter   = enter_amber,
        .on_exit    = NULL
    }));

    CHECK(sm_register_state(sm, &st_green, (SMState) {
        .handler    = (SMEventHandler)green_handler,
        .parent_hdl = st_on,
        .on_enter   = enter_green,
        .on_exit    = NULL
    }));
}

void add_transitions(void) {
    CHECK(sm_add_transition(sm, (SMTransition) {
        .from = st_off,
        .on   = TURN_ON,
        .to   = st_red
    }));

    CHECK(sm_add_transition(sm, (SMTransition) {
        .from = st_on,
        .on   = TURN_OFF,
        .to   = st_off
    }));

    CHECK(sm_add_transition(sm, (SMTransition) {
        .from = st_on,
        .on   = ERROR,
        .to   = st_error
    }));

    CHECK(sm_add_transition(sm, (SMTransition) {
        .from = st_red,
        .on   = CHANGE,
        .to   = st_red_amber
    }));

    CHECK(sm_add_transition(sm, (SMTransition) {
        .from = st_red_amber,
        .on   = CHANGE,
        .to   = st_green
    }));

    CHECK(sm_add_transition(sm, (SMTransition) {
        .from = st_green,
        .on   = CHANGE,
        .to   = st_amber
    }));

    CHECK(sm_add_transition(sm, (SMTransition) {
        .from = st_amber,
        .on   = CHANGE,
        .to   = st_red
    }));
}
    
static void init_sm(void) {
    sm = sm_create();

    if (sm == NULL) {
        DIE("Memory error\n");
    }

    CHECK(sm_init(sm, (SMConfig) {
        .ignore_unhandled_events = false, 
        .init_states_size        = 5, 
        .init_transitions_size   = 5
    }));
}

static int enter_on(void) {
    gui_reset_lights();
    return 0;
}

static int enter_off(void) {
    gui_reset_lights();
    return 0;
}

static int enter_error(void) {
    stop_lights_cycle();
    gui_set_light(RED, true);
    gui_set_light(AMBER, true);
    gui_set_light(GREEN, true);
    return 0;
}

static int enter_red(void) {
    gui_set_light(RED, true);
    return 0;
}

static int enter_red_amber(void) {
    gui_set_light(RED, true);
    gui_set_light(AMBER, true);
    return 0;
}

static int enter_green(void) {
    gui_set_light(GREEN, true);
    return 0;
}

static int enter_amber(void) {
    gui_set_light(AMBER, true);
    return 0;
}

static SMEventHandlerStatus on_handler(LightEvent e, void* args) { 
    switch (e) {
        case CHANGE: {
            useconds_t duration = (useconds_t)args;
            usleep(duration);
            break;
        }
        case ERROR: report_error((LightError*)args); break;
        default: ; // Ignore
    }

    return HS_HANDLED; 
}

static SMEventHandlerStatus off_handler(LightEvent e, void* args) { 
    if (e == TURN_ON) {
        start_lights_cycle();
    }

    return HS_HANDLED;
}

static SMEventHandlerStatus error_handler(LightEvent e, void* args) { 
    return HS_HANDLED; 
}

static SMEventHandlerStatus red_handler(LightEvent e, void* args) { 
    return HS_UNHANDLED; 
}

static SMEventHandlerStatus red_amber_handler(LightEvent e, void* args) { 
    return HS_UNHANDLED; 
}

static SMEventHandlerStatus amber_handler(LightEvent e, void* args) { 
    return HS_UNHANDLED; 
}

static SMEventHandlerStatus green_handler(LightEvent e, void* args) { 
    return HS_UNHANDLED; 
}

static inline void CHECK(SMStatus status) {
    if (status != SM_OK) {
        DIE("SM Error: %s\n", sm_status_str(status));
    }
}    

