#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
// to use mutex and malloc

#define DEFAULT_FREQ      100.0   /* Hz */
#define DEFAULT_AMPLITUDE 0x7FFF  /* half of 16-bit range */
#define DEFAULT_WAVE_TYPE 0       /* 0=sine */
#define NUM_POINTS        100     /* samples per waveform cycle */
#define DAC_MAX           0xFFFF  /* 16-bit DAC full scale */
#define DAC_MID           0x7FFF  /* 16-bit DAC midpoint */
 
/* Wave type identifiers */
#define WAVE_SINE     0
#define WAVE_TRIANGLE 1
#define WAVE_SQUARE   2
 
/* ============================================================
 * SHARED STATE - all threads read/write through this struct
 * ============================================================
 * Rule: ALWAYS lock the mutex before touching this struct.
 *       ALWAYS unlock immediately after you're done.
 */
typedef struct {
    double frequency;       /* output frequency in Hz */
    int    wave_type;       /* WAVE_SINE, WAVE_TRIANGLE, WAVE_SQUARE */
    int    amplitude;       /* 0 to DAC_MID (scales the wave) */
    int    running;         /* 1 = keep going, 0 = shutdown */
} WaveState;
 
/* Global shared state and its mutex */
WaveState    g_state;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;