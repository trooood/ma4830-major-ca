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
// Compile Command(Linux):cc -Wall -o wavegen main.c hw.c sine_wave_generator_3.c ui_graphics.c setup_input.c -lm
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
    int    input_mode;       /* 1 = display thread pauses */
    int paused;
    int audio_enabled;
    char status_msg[128];
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
    #ifdef __QNX__
        struct sched_param sp;
        timer_t timerid;
        struct itimerspec timer;
        sigset_t sigset;
    #endif
    (void)arg;
    arb_count = STEPS;
    
    #ifdef __QNX__
        sp.sched_priority = 25;
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);
        sigprocmask(SIG_BLOCK, &sigset, NULL);
    #endif
    //Force initial buffer generation
    generateSine(buf, 1.0, 0.0);
 
    while (state.running) {
        // Copy params under lock (short critical section)
        if (state.input_mode) {
            usleep(100000);
            continue;
        }
        if (state.paused) {
            usleep(50000);
            continue;
        }

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
                    arb_count = generateArbitrary(buf, state.arb_file, local_amp, local_off);
                    memcpy(buf, wave_buffer, sizeof(unsigned int) * STEPS);
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
 
        #ifdef __QNX__           
            timer_create(CLOCK_REALTIME, NULL, &timerid);
            timer.it_value.tv_sec = 0;
            timer.it_value.tv_nsec = delay_ns;
            timer.it_interval.tv_sec = 0;
            timer.it_interval.tv_nsec = delay_ns;
            timer_settime(timerid, 0, &timer, NULL);
            
            for (i = 0; i < cycle_len; i++) {
                sigwaitinfo(&sigset, NULL);  /* blocks until timer fires */
                if (!state.running) break;
                hw_dac(&dev, 0, (unsigned short)buf[i]);
            }
            
            timer_delete(timerid);
        #else
            /* Windows fallback: nanosleep */
            for (i = 0; i < cycle_len; i++) {
                if (!state.running) break;
                hw_dac(&dev, 0, (unsigned short)buf[i]);
                nanosleep(&ts, NULL);
            }
        #endif

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
        if (state.input_mode) {
            usleep(100000);
            continue;
        }
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
        ui.running   = state.paused ? 0 : local_running;
        if (state.paused) {
            strcpy(ui.last_message, "PAUSED - Press 'p' to start");
            ui.running = 0;
        } else if (state.status_msg[0] != '\0') {
            strcpy(ui.last_message, state.status_msg);
            state.status_msg[0] = '\0';  /* clear after displaying once */
            ui.running = local_running;
            ui.tick++;
        } else {
            strcpy(ui.last_message, "Running");
            ui.running = local_running;
            ui.tick++;
        }
        render_ui(&ui);
        
        // Audio beep moved to UI thread to protect DAC timing
        if (state.audio_enabled && !state.paused) {
            printf("\a");
            fflush(stdout);
        }
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
//   Qihong: DIO switches and ADC potentiometer implemented in qnx_test_monday
//   SW1=Run/Stop, SW2=ADC Amp, SW3=ADC Freq, SW4=Audio

 int main(int argc, char *argv[])
{
    pthread_t wave_tid, disp_tid;
    setup_t *cfg;
    setup_t save;
    setup_t *loaded;
    char key;
    char target;
    int up, down, left, right;
    char input_buf[32];
    double input_val;
    const char *wnames[] = {"sine", "square", "tri", "saw", "arb"};


    // Default state
    state.wave_type      = WAVE_SINE;
    state.frequency = FREQ_DEFAULT;
    state.amplitude = AMP_DEFAULT;
    state.offset    = OFF_DEFAULT;
    strcpy(state.arb_file, "wave.txt");
    state.params_changed = 1;      // force first buffer to generate
    state.running        = 1;
    state.input_mode     = 0;
    state.paused         = 1;      /* start paused for trigger mode */
    state.audio_enabled  = 0;
    pthread_mutex_init(&state.lock, NULL);
    state.status_msg[0] = '\0';
 
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
    printf("===========================================================================\n");
    printf(" _   _ _   _ _     _   _____                   _             _\n");
    printf("| \\ | | | | | |   | | |_   _|__ _ __ _ __ ___ (_)_ __   __ _| |_ ___  _ __\n");
    printf("|  \\| | | | | |   | |   | |/ _ \\ '__| '_ ` _ \\| | '_ \\ / _` | __/ _ \\| '__|\n");
    printf("| |\\  | |_| | |___| |___| |  __/ |  | | | | | | | | | | (_| | || (_) | |\n");
    printf("|_| \\_|\\___/|_____|_____|_|\\___|_|  |_| |_| |_|_|_| |_|\\__,_|\\__\\___/|_|\n");
    printf("                  NULL Terminators' Waveform Generator\n");
    printf("===========================================================================\n");
    printf("   1 - Sine    2 - Square    3 - Triangle    4 - Sawtooth    5 - Arbitrary\n");
    printf("   Or press Enter for default (Sine)\n");
    printf("===========================================================================\n");
    {
        int ch = getchar();
        if (ch >= '1' && ch <= '5')
            state.wave_type = ch - '1';
            
        /*Flush leftover newline to prevent phantom keypresses */
        while ((ch = getchar()) != '\n' && ch != EOF);
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
                if (state.frequency > FREQ_MAX) state.frequency = FREQ_MAX;
                state.params_changed = 1;
            }
            else if (down) {
                state.frequency /= 1.1;
                if (state.frequency < FREQ_MIN) state.frequency = FREQ_MIN;
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
                if (state.amplitude > AMP_MAX) state.amplitude = AMP_MAX;
                state.params_changed = 1;
            }
            else if (key == '-') {
                state.amplitude -= 0.05;
                if (state.amplitude < AMP_MIN) state.amplitude = AMP_MIN;
                state.params_changed = 1;
            }
            else if (key == ']') {
                state.offset += 0.05;
                if (state.offset > OFF_MAX) state.offset = OFF_MAX;
                state.params_changed = 1;
            }
            else if (key == '[') {
                state.offset -= 0.05;
                if (state.offset < OFF_MIN) state.offset = OFF_MIN;
                state.params_changed = 1;
            }
            else if (key == '1') { state.wave_type = WAVE_SINE;   state.params_changed = 1; }
            else if (key == '2') { state.wave_type = WAVE_SQUARE;  state.params_changed = 1; }
            else if (key == '3') { state.wave_type = WAVE_TRI;     state.params_changed = 1; }
            else if (key == '4') { state.wave_type = WAVE_SAW;     state.params_changed = 1; }
            else if (key == '5') { state.wave_type = WAVE_ARB;     state.params_changed = 1; }

            /* TODO: Alicia file scanner - replace hardcoded wave file cycling
               with scan_wave_files() that reads data/ directory.
               Placeholder: void scan_wave_files(char filelist[][256], int *count); */
            else if (key == 'w') {
                if (strcmp(state.arb_file, "wave.txt") == 0)
                    strcpy(state.arb_file, "wave1.txt");
                else if (strcmp(state.arb_file, "wave1.txt") == 0)
                    strcpy(state.arb_file, "wave2.txt");
                else if (strcmp(state.arb_file, "wave2.txt") == 0)
                    strcpy(state.arb_file, "wave3.txt");
                else
                    strcpy(state.arb_file, "wave.txt");
                state.wave_type = WAVE_ARB;
                state.params_changed = 1;
            }
            else if (key == 'f' || key == 'a' || key == 'o') {
                target = key;
                state.input_mode = 1;
                pthread_mutex_unlock(&state.lock);
                keyboard_restore();
                show_cursor();

                if (target == 'f')
                    input_val = safe_handling("\nEnter frequency (0.01-10.0 Hz): ",
                                             FREQ_MIN, FREQ_MAX, state.frequency);
                else if (target == 'a')
                    input_val = safe_handling("\nEnter amplitude (0.0-1.0): ",
                                             AMP_MIN, AMP_MAX, state.amplitude);
                else
                    input_val = safe_handling("\nEnter offset (-1.0 to 1.0): ",
                                             OFF_MIN, OFF_MAX, state.offset);

                pthread_mutex_lock(&state.lock);
                if (target == 'f') state.frequency = input_val;
                else if (target == 'a') state.amplitude = input_val;
                else state.offset = input_val;
                state.params_changed = 1;
                state.input_mode = 0;     /* MOVED INSIDE THE LOCK */
                pthread_mutex_unlock(&state.lock);

                hide_cursor();
                keyboard_init();
                continue;
            }
            // Save/Load
            else if (key == 's') {
                strcpy(save.waveform.waveform_type, wnames[state.wave_type]);
                save.waveform.frequency = state.frequency;
                save.waveform.amplitude = state.amplitude;
                save.waveform.offset = state.offset;
                strcpy(save.waveform.arbitrary_file, state.arb_file);
                save.output.output_mode = 0;
                save.output.sample_rate = 48000;
                save.output.duration_seconds = 0;
                save_config_file("settings.dat", &save);
                strcpy(state.status_msg, "Settings saved to settings.dat");
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
                    strcpy(state.status_msg, "Settings loaded from settings.dat");
                }
            }

            else if (key == 'q' || key == 'Q') {
                state.running = 0;
            }

            else if (key == 'p') {
                state.paused = !state.paused;
            }
            else if (key == 'm') {
                state.audio_enabled = !state.audio_enabled;
                if (state.audio_enabled) printf("\a");
                fflush(stdout);
            }
            pthread_mutex_unlock(&state.lock);
        }
        #ifdef __QNX__
        /* Potentiometer control (Qihong) */
        {
            unsigned short adc0;
            float pot_amp;
            adc0 = read_adc(&dev, 0);
            pot_amp = (float)adc0 / 65535.0f;
            pthread_mutex_lock(&state.lock);
            state.amplitude = pot_amp;
            state.params_changed = 1;
            pthread_mutex_unlock(&state.lock);
        }
        #endif

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
