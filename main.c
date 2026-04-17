// main.c - Waveform Generator (Integrated)
// MA4830 Major CA (NULL TERMINATORS)
// State struct holds parameters only (no buffer).
// Wave thread owns its local buffer, regenerates when params change.
// Three threads:
// 1. wave_thread   - outputs samples to DAC (high priority)
// 2. display_thread - redraws screen continuously (~10 fps)
// 3. main()        - keyboard input loop
// Modules: 
// hw.h / hw.c          - Hardware DAC interface      (Qihong)
// waveforms.h / .c     - Waveform math               (Misha/Trudy)
// setup_input.h / .c   - Config load/save/parse       (Alicia)
// display              - ASCII graphics              (Jaz)
// Compile Command(windows): gcc main.c src/hw.c sine_wave_generator_3.c ui_graphics.c setup_input.c -I./src -lpthread -lm -o main
// Compile Command(Linux):cc -Wall -o wavegen main.c src/hw.c sine_wave_generator_3.c ui_graphics.c setup_input.c -lm
// +/- zoom in/out; [ - move down;] - move up; s - save data to settings.dat; l- load data from settings.dat
// sample load wave command: ./main arb 100 1.0 0.0 wave1.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
 
#include "hw.h"
#include "sine_wave.h"
#include "ui_graphics.h"
#include "setup_input.h"
 
/* ---- Wave type enum ---- */
#define WAVE_SINE   0
#define WAVE_SQUARE 1
#define WAVE_TRI    2
#define WAVE_SAW    3
#define WAVE_ARB    4
 
// Shared state (protected by mutex)
typedef struct {
    int    wave_type;        // WAVE_SINE, etc.        
    double frequency;        // Hz                       
    double amplitude;        // 0.0 to 1.0              
    double offset;           // -1.0 to 1.0            
    char   arb_file[256];   // filename for arbitrary
    int    params_changed;   // dirty flag
    int    running;          //0 = shutdown
    pthread_mutex_t lock;
} State;
 
// Globals
static State  state;
static Device dev;
 
// SIGINT handler - sets shutdown flag
// Qihong/Jaz: extend for other graceful termination paths
void on_sigint(int sig)
{
    (void)sig;
    state.running = 0;
}
// WAVE OUTPUT THREAD 
// Copies params from state under lock (short hold) and regenerates local buffer OUTSIDE lock if params changed; Outputs samples to DAC with nanosleep timing

void *wave_thread(void *arg)
{
    unsigned int buf[STEPS];     //local buffer
    int    local_type;
    double local_freq, local_amp, local_off;
    int    arb_count;
    int    cycle_len;
    int    i;
    long   delay_ns;
    struct timespec ts;
 
    (void)arg;
    arb_count = STEPS;

    #ifdef __QNX__
        /* Real-time priority for wave output */
        struct sched_param sp;
        sp.sched_priority = 25;
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
    #endif
 
    //Force initial buffer generation
    generateSine(buf, 1.0, 0.0);
 
    while (state.running) {
        // Copy params under lock (short critical section)
        pthread_mutex_lock(&state.lock);
        local_type = state.wave_type;
        local_freq = state.frequency;
        local_amp  = state.amplitude;
        local_off  = state.offset;
 
        if (state.params_changed) {
            state.params_changed = 0;
            pthread_mutex_unlock(&state.lock);
 
            //Regenerate buffer OUTSIDE lock
            switch (local_type) {
                case WAVE_SINE:
                    generateSine(buf, local_amp, local_off);
                    arb_count = STEPS;
                    break;
                case WAVE_SQUARE:
                    generateSquare(buf, local_amp, local_off);
                    arb_count = STEPS;
                    break;
                case WAVE_TRI:
                    generateTriangular(buf, local_amp, local_off);
                    arb_count = STEPS;
                    break;
                case WAVE_SAW:
                    generateSawtooth(buf, local_amp, local_off);
                    arb_count = STEPS;
                    break;
                case WAVE_ARB:
                    arb_count = generateArbitrary(buf, state.arb_file);
                    if (arb_count <= 0) {
                        generateSine(buf, local_amp, local_off);
                        arb_count = STEPS;
                    }
                    break;
                default:
                    generateSine(buf, local_amp, local_off);
                    arb_count = STEPS;
                    break;
            }
        } else {
            pthread_mutex_unlock(&state.lock);
        }
 
        // One out put cycle
        //@Trudy this is the limiter you wanted
        cycle_len = (local_type == WAVE_ARB) ? arb_count : STEPS;
        delay_ns = (long)(1000000000.0 / (local_freq * cycle_len));
        if (delay_ns < 1000) delay_ns = 1000;            //minimum is 1 micro-sec
        ts.tv_sec  = 0;
        ts.tv_nsec = delay_ns;
 
        for (i = 0; i < cycle_len; i++) {
            if (!state.running) break;
            hw_dac(&dev, 0, (unsigned short)buf[i]);
            nanosleep(&ts, NULL);
        }
    }
    return NULL;
}
 
//  DISPLAY THREAD
//  Renders Jaz's ASCII dashboard, bridging our State to her UIState.
//  Refreshes at ~10 fps atm

void *display_thread(void *arg)
{
    int    local_type;
    double local_freq, local_amp, local_off;
    int    local_running;
    UIState ui;

    (void)arg;

    /* Initialize fixed fields */
    ui.dac_on = 1;
    ui.adc_enabled = 0;
    ui.dio_ready = 0;
    ui.tick = 0;
    ui.show_error = 0;
    strcpy(ui.last_message, "System started.");

    hide_cursor();

    while (state.running) {
        pthread_mutex_lock(&state.lock);
        local_type    = state.wave_type;
        local_freq    = state.frequency;
        local_amp     = state.amplitude;
        local_off     = state.offset;
        local_running = state.running;
        pthread_mutex_unlock(&state.lock);

        /* Bridge: fill Jaz's UIState from our State */
        ui.waveform  = local_type;
        ui.frequency = local_freq;
        ui.amplitude = local_amp;
        ui.mean      = local_off;
        ui.running   = local_running;
        ui.tick++;

        render_ui(&ui);

        usleep(120000);  /* ~8 fps, matches Jaz's 120ms frame time */
    }

    show_cursor();
    return NULL;
}
 

//  HELPER: map wave type string to enum
//  (bridges Alicia's config strings to our int enum)
int wave_type_from_string(const char *s)
{
    if (strcmp(s, "sine")   == 0) return WAVE_SINE;
    if (strcmp(s, "square") == 0) return WAVE_SQUARE;
    if (strcmp(s, "tri")    == 0) return WAVE_TRI;
    if (strcmp(s, "saw")    == 0) return WAVE_SAW;
    if (strcmp(s, "arb")    == 0) return WAVE_ARB;
    return WAVE_SINE;  // default fallback
}
 //  
//   TODO (Qihong):
//     - add potentiometer reading for freq/amp control
//     - add analog switch reading for wave type cycling
//     - hardware graceful termination path

 int main(int argc, char *argv[])
{
    pthread_t wave_tid, disp_tid;
    setup_t *cfg;
    setup_t save;
    setup_t *loaded;
    char key;
    int up, down, left, right;

    // Default state
    state.wave_type      = WAVE_SINE;
    state.frequency      = 100.0;
    state.amplitude      = 1.0;
    state.offset         = 0.0;
    strcpy(state.arb_file, "wave.txt");
    state.params_changed = 1;      // force first buffer to generate
    state.running        = 1;
    pthread_mutex_init(&state.lock, NULL);
 
    cfg = parse_command_line(argc, argv);
    if (!cfg->is_valid) {
        printf("Error: %s\n", cfg->error_message);
        free_setup(cfg);
        return 1;
    }
    state.wave_type = wave_type_from_string(cfg->waveform.waveform_type);
    state.frequency = cfg->waveform.frequency;
    state.amplitude = cfg->waveform.amplitude;
    state.offset    = cfg->waveform.offset;
    strncpy(state.arb_file, cfg->waveform.arbitrary_file, 255);
    print_setup_summary(cfg);
    free_setup(cfg);
 
    // signal handler
    signal(SIGINT, on_sigint);
 
    // hardware check
    if (hw_open(&dev) != 0) {
        printf("Hardware init failed. Check PCI device.\n");
        return 1;
    }
 
    // Thread spawning
    pthread_create(&wave_tid, NULL, wave_thread, NULL);
    pthread_create(&disp_tid, NULL, display_thread, NULL);
 
    // keyboard input loop (non-blocking via Alicia's keyboard module)
    keyboard_init();

    while (state.running) {
        key = 0;
        up = 0, down = 0, left = 0, right = 0;

        keyboard_read_arrow(&key, &up, &down, &left, &right);

        if (key || up || down || left || right) {
            pthread_mutex_lock(&state.lock);

            if (up) {
                state.frequency *= 1.1;
                if (state.frequency > 20000) state.frequency = 20000;
                state.params_changed = 1;
            }
            else if (down) {
                state.frequency /= 1.1;
                if (state.frequency < 0.01) state.frequency = 0.01;
                state.params_changed = 1;
            }
            else if (right) {
                state.wave_type = (state.wave_type + 1) % 5;
                state.params_changed = 1;
            }
            else if (left) {
                state.wave_type = (state.wave_type + 4) % 5;
                state.params_changed = 1;
            }
            else if (key == '+') {
                state.amplitude += 0.05;
                if (state.amplitude > 1.0) state.amplitude = 1.0;
                state.params_changed = 1;
            }
            else if (key == '-') {
                state.amplitude -= 0.05;
                if (state.amplitude < 0.0) state.amplitude = 0.0;
                state.params_changed = 1;
            }
            else if (key == ']') {
                state.offset += 0.05;
                if (state.offset > 1.0) state.offset = 1.0;
                state.params_changed = 1;
            }
            else if (key == '[') {
                state.offset -= 0.05;
                if (state.offset < -1.0) state.offset = -1.0;
                state.params_changed = 1;
            }
            else if (key == '1') { state.wave_type = WAVE_SINE;   state.params_changed = 1; }
            else if (key == '2') { state.wave_type = WAVE_SQUARE;  state.params_changed = 1; }
            else if (key == '3') { state.wave_type = WAVE_TRI;     state.params_changed = 1; }
            else if (key == '4') { state.wave_type = WAVE_SAW;     state.params_changed = 1; }
            else if (key == '5') { state.wave_type = WAVE_ARB;     state.params_changed = 1; }

            // THINKING IF WE WANNA DO MID SWAP FILES; RN IS HARDCODED NAMES, CAN GO TO SCAN FOR ALL TXT IF WE WANT TO
            // else if (key == 'w') {
            //     if (strcmp(state.arb_file, "wave.txt") == 0)
            //         strcpy(state.arb_file, "wave1.txt");
            //     else if (strcmp(state.arb_file, "wave1.txt") == 0)
            //         strcpy(state.arb_file, "wave2.txt");
            //     else
            //         strcpy(state.arb_file, "wave.txt");
            //     state.wave_type = WAVE_ARB;
            //     state.params_changed = 1;
            // }
            
            // Save/Load
            else if (key == 's') {
                const char *wnames[] = {"sine", "square", "tri", "saw", "arb"};
                strcpy(save.waveform.waveform_type, wnames[state.wave_type]);
                save.waveform.frequency = state.frequency;
                save.waveform.amplitude = state.amplitude;
                save.waveform.offset = state.offset;
                strcpy(save.waveform.arbitrary_file, state.arb_file);
                save.output.output_mode = 0;
                save.output.sample_rate = 48000;
                save.output.duration_seconds = 0;
                save_config_file("settings.dat", &save);
            }
            else if (key == 'l') {
                loaded = load_config_file("settings.dat");
                if (loaded && loaded->is_valid) {
                    state.wave_type = wave_type_from_string(loaded->waveform.waveform_type);
                    state.frequency = loaded->waveform.frequency;
                    state.amplitude = loaded->waveform.amplitude;
                    state.offset    = loaded->waveform.offset;
                    state.params_changed = 1;
                    free_setup(loaded);
                }
            }

            else if (key == 'q' || key == 'Q') {
                state.running = 0;
            }

            pthread_mutex_unlock(&state.lock);
        }

        usleep(20000);  // 20ms poll rate
    }

    // Graceful shutdown
    keyboard_restore();
    pthread_join(wave_tid, NULL);
    pthread_join(disp_tid, NULL);
    hw_close(&dev);
    pthread_mutex_destroy(&state.lock);
    show_cursor();
    printf("\nClean shutdown complete.\n");
 
    return 0;
}


