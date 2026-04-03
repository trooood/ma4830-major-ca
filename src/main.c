#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "hw.h"
#include "wv.h"

/* Shared Settings Struct */
typedef struct {
    Mode mode;
    double freq;
    int run;
    pthread_mutex_t lock;
} State;

Device dev;
State state;

/* Waveform Generation Thread (Real-Time) */
void* wave_thread(void* arg) {
    double phase = 0;
    struct timespec ts;
    struct sched_param sp;
    
    sp.sched_priority = 25;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

    while (state.run) {
        unsigned short v;
        pthread_mutex_lock(&state.lock);
        v = wv_get(state.mode, phase, 1.0);
        ts.tv_sec = 0;
        ts.tv_nsec = (long)(1000000000.0 / (state.freq * 100));
        pthread_mutex_unlock(&state.lock);

        hw_dac(&dev, 0, v);
        phase += 0.01; if (phase >= 1.0) phase = 0;
        nanosleep(&ts, NULL);
    }
    return NULL;
}

/* Shutdown Handler (Ctrl+C) */
void on_sigint(int s) { state.run = 0; }

int main(int argc, char *argv[]) {
    pthread_t tid;
    char c;

    /* Initialization */
    state.mode = SINE;
    state.freq = (argc > 1) ? atof(argv[1]) : 10.0;
    state.run = 1;
    pthread_mutex_init(&state.lock, NULL);
    signal(SIGINT, on_sigint);

    if (hw_open(&dev) != 0) return printf("HW Error\n"), 1;

    pthread_create(&tid, NULL, wave_thread, NULL);

    /* UI Loop */
    printf("1:Sine 2:Sqr 3:Tri 4:Saw | +/-:Freq | q:Quit\n");
    while (state.run) {
        c = getchar();
        pthread_mutex_lock(&state.lock);
        if (c == '1') state.mode = SINE;
        if (c == '2') state.mode = SQUARE;
        if (c == '3') state.mode = TRI;
        if (c == '4') state.mode = SAW;
        if (c == '+') state.freq += 5;
        if (c == '-') if (state.freq > 5) state.freq -= 5;
        if (c == 'q') state.run = 0;
        printf("\rMode: %d Freq: %.1f Hz  ", state.mode, state.freq);
        fflush(stdout);
        pthread_mutex_unlock(&state.lock);
    }

    pthread_join(tid, NULL);
    hw_close(&dev);
    pthread_mutex_destroy(&state.lock);
    return 0;
}