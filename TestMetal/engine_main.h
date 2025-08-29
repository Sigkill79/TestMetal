#ifndef ENGINE_MAIN_H
#define ENGINE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

// Engine states
typedef enum {
    ENGINE_STATE_INITIALIZING,
    ENGINE_STATE_RUNNING,
    ENGINE_STATE_PAUSED,
    ENGINE_STATE_SHUTDOWN
} EngineState;

// Main engine state structure
typedef struct {
    EngineState state;
} EngineStateStruct;

// Function declarations
EngineStateStruct* initialize(void);
void update(EngineStateStruct* engineState);
void engine_shutdown(EngineStateStruct* engineState);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_MAIN_H
