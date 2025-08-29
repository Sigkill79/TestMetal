#include "engine_main.h"
#include <stdlib.h>

// Initialize engine and return new instance
EngineStateStruct* initialize(void) {
    EngineStateStruct* engineState = (EngineStateStruct*)malloc(sizeof(EngineStateStruct));
    if (engineState) {
        engineState->state = ENGINE_STATE_INITIALIZING;
    }
    return engineState;
}

// Update engine state (currently does nothing, future game loop step)
void update(EngineStateStruct* engineState) {
    if (engineState && engineState->state == ENGINE_STATE_RUNNING) {
        // Future: Execute one step in the game loop
        // For now, this function does nothing
    }
}

// Shutdown engine and free memory
void engine_shutdown(EngineStateStruct* engineState) {
    if (engineState) {
        engineState->state = ENGINE_STATE_SHUTDOWN;
        free(engineState);
    }
}
