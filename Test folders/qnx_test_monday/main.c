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
// Compile(Lab QNX): make
// Run(Lab QNX): ./wavegen
// +/- zoom in/out; [ - move down;] - move up; s - save data to settings.dat; l- load data from settings.dat
// sample load wave command: ./main arb 100 1.0 0.0 wave1.txt

// main.c - Waveform Generator (Integrated)
// Fully restored with Save/Load and Manual Offset

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

/* ---- File Locations ---- */
#define DATA_DIR "data/"
#define DEFAULT_WAVE DATA_DIR "wave.txt"
#define WAVE1 DATA_DIR "wave1.txt"
#define WAVE2 DATA_DIR "wave2.txt"
#define WAVE3 DATA_DIR "wave3.txt"

/* ---- Wave type enum ---- */
#define WAVE_SINE   0
#define WAVE_SQUARE 1
#define WAVE_TRI    2
#define WAVE_SAW    3
#define WAVE_ARB    4
 
typedef struct {
    int    wave_type;        
    double frequency;        
    double amplitude;        
    double offset;           
    char   arb_file[256];   
    int    params_changed;   
    int    running;          
    pthread_mutex_t lock;
    int    input_mode;
    int audio_enabled;       
} State;
 
static State  state;
static Device dev;
 
void on_sigint(int sig)
{
    (void)sig;
    state.running = 0;
}

void *wave_thread(void *arg)
{
    unsigned int buf[STEPS];
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
        timer_create(CLOCK_REALTIME, NULL, &timerid);
    #endif

    generateSine(buf, 1.0, 0.0);
    
    while (state.running) {
        if (state.input_mode) {
            usleep(100000);
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
 
            switch (local_type) {
                case WAVE_SINE:   generateSine(buf, local_amp, local_off); arb_count = STEPS; break;
                case WAVE_SQUARE: generateSquare(buf, local_amp, local_off); arb_count = STEPS; break;
                case WAVE_TRI:    generateTriangular(buf, local_amp, local_off); arb_count = STEPS; break;
                case WAVE_SAW:    generateSawtooth(buf, local_amp, local_off); arb_count = STEPS; break;
                case WAVE_ARB:
                    arb_count = generateArbitrary((int*)buf, state.arb_file, local_amp, local_off);
                    if (arb_count <= 0) { generateSine(buf, local_amp, local_off); arb_count = STEPS; }
                    break;
                default: generateSine(buf, local_amp, local_off); arb_count = STEPS; break;
            }
        } else {
            pthread_mutex_unlock(&state.lock);
        }
 
        cycle_len = (local_type == WAVE_ARB) ? arb_count : STEPS;
        delay_ns = (long)(1000000000.0 / (local_freq * cycle_len));
        if (delay_ns < 1000) delay_ns = 1000;
        
        ts.tv_sec  = delay_ns / 1000000000;
        ts.tv_nsec = delay_ns % 1000000000;

        #ifdef __QNX__           
            timer.it_value.tv_sec = ts.tv_sec;
            timer.it_value.tv_nsec = ts.tv_nsec;
            timer.it_interval.tv_sec = ts.tv_sec;
            timer.it_interval.tv_nsec = ts.tv_nsec;
            timer_settime(timerid, 0, &timer, NULL);
            
            for (i = 0; i < cycle_len; i++) {
                sigwaitinfo(&sigset, NULL);
                if (!state.running) break;
                hw_dac(&dev, 0, (unsigned short)buf[i]);
            }
        #else
            for (i = 0; i < cycle_len; i++) {
                if (!state.running) break;
                hw_dac(&dev, 0, (unsigned short)buf[i]);
                nanosleep(&ts, NULL);
            }
        #endif
    }
    #ifdef __QNX__
        timer_delete(timerid);
    #endif
    return NULL;
}
 
void *display_thread(void *arg)
{
    UIState ui;
    (void)arg;
    ui.dac_on = 1; ui.adc_enabled = 0; ui.dio_ready = 0; ui.tick = 0; ui.show_error = 0;
    strcpy(ui.last_message, "System started.");
    hide_cursor();

    while (state.running) {
        if (state.input_mode) { usleep(100000); continue; }
        pthread_mutex_lock(&state.lock);
        ui.waveform  = state.wave_type;
        ui.frequency = state.frequency;
        ui.amplitude = state.amplitude;
        ui.mean      = state.offset;
        ui.running   = state.running;
        pthread_mutex_unlock(&state.lock);
        ui.tick++;
        render_ui(&ui);
        usleep(120000);
    }
    show_cursor();
    return NULL;
}

int wave_type_from_string(const char *s)
{
    if (strcmp(s, "sine") == 0) return WAVE_SINE;
    if (strcmp(s, "square") == 0) return WAVE_SQUARE;
    if (strcmp(s, "tri") == 0) return WAVE_TRI;
    if (strcmp(s, "saw") == 0) return WAVE_SAW;
    if (strcmp(s, "arb") == 0) return WAVE_ARB;
    return WAVE_SINE;
}

int main(int argc, char *argv[])
{
    pthread_t wave_tid, disp_tid;
    setup_t *cfg, save, *loaded;
    char input_buf[32];
    double input_val;
    const char *wnames[] = {"sine", "square", "tri", "saw", "arb"};

    state.wave_type = WAVE_SINE; state.frequency = 5.0; state.amplitude = 1.0;
    state.offset = 0.0; strcpy(state.arb_file, DEFAULT_WAVE);
    state.params_changed = 1; state.running = 1; state.input_mode = 0;
    pthread_mutex_init(&state.lock, NULL);

    #ifdef __QNX__
    {
        sigset_t block_set;
        sigemptyset(&block_set);
        sigaddset(&block_set, SIGALRM);
        pthread_sigmask(SIG_BLOCK, &block_set, NULL);
    }
    #endif
 
    cfg = parse_command_line(argc, argv);
    if (cfg->is_valid) {
        state.wave_type = wave_type_from_string(cfg->waveform.waveform_type);
        state.frequency = cfg->waveform.frequency;
        state.amplitude = cfg->waveform.amplitude;
        state.offset = cfg->waveform.offset;
        strncpy(state.arb_file, cfg->waveform.arbitrary_file, 255);
        free_setup(cfg);
    }
 
    signal(SIGINT, on_sigint);
    if (hw_open(&dev) != 0) { printf("Hardware init failed.\n"); return 1; }
 
    pthread_create(&wave_tid, NULL, wave_thread, NULL);
    pthread_create(&disp_tid, NULL, display_thread, NULL);
    keyboard_init();

    while (state.running) {
        char key;
        int up, down, left, right;
        #ifdef __QNX__
        int switches, sw1, sw2, sw3;
        unsigned short adc0, adc1_val;
        #else
        int sw2 = 0, sw3 = 0;
        #endif

        key = 0; up = 0; down = 0; left = 0; right = 0;

        #ifdef __QNX__
        /* 1. Read all 4 bits from the hardware switch register */
        switches = hw_read_switch(&dev);
        sw1 = (switches & 0x01) ? 1 : 0;
        sw2 = (switches & 0x02) ? 1 : 0;
        sw3 = (switches & 0x04) ? 1 : 0;
        
        /* 2. Update shared state for the wave thread */
        pthread_mutex_lock(&state.lock);
        state.audio_enabled = (switches & 0x08) ? 1 : 0;
        pthread_mutex_unlock(&state.lock);

        /* 3. Print updated status bar including Audio (SW4) */
        printf("\r[HW] SW1(Run): %d | SW2(Amp): %d | SW3(Freq): %d | SW4(Audio): %d    ", 
               sw1, sw2, sw3, state.audio_enabled);
        fflush(stdout);

        /* 4. Safety check for system termination */
        if (sw1 == 0) { 
            state.running = 0; 
            break; 
        }
        #endif

        keyboard_read_arrow(&key, &up, &down, &left, &right);
        if (key || up || down || left || right) {
            pthread_mutex_lock(&state.lock);
            if (up && sw3 == 0) { state.frequency *= 1.1; if (state.frequency > 10.0) state.frequency = 10.0; state.params_changed = 1; }
            else if (down && sw3 == 0) { state.frequency /= 1.1; if (state.frequency < 0.01) state.frequency = 0.01; state.params_changed = 1; }
            else if (right) { state.wave_type = (state.wave_type + 1) % 5; state.params_changed = 1; }
            else if (left) { state.wave_type = (state.wave_type + 4) % 5; state.params_changed = 1; }
            else if (key == '+' && sw2 == 0) { state.amplitude += 0.05; if (state.amplitude > 1.0) state.amplitude = 1.0; state.params_changed = 1; }
            else if (key == '-' && sw2 == 0) { state.amplitude -= 0.05; if (state.amplitude < 0.0) state.amplitude = 0.0; state.params_changed = 1; }
            else if (key == ']') { state.offset += 0.05; if (state.offset > 1.0) state.offset = 1.0; state.params_changed = 1; }
            else if (key == '[') { state.offset -= 0.05; if (state.offset < -1.0) state.offset = -1.0; state.params_changed = 1; }
            else if (key >= '1' && key <= '5') { state.wave_type = key - '1'; state.params_changed = 1; }
            
            else if (key == 'w') {
                if (strcmp(state.arb_file, DEFAULT_WAVE) == 0) strcpy(state.arb_file, WAVE1);
                else if (strcmp(state.arb_file, WAVE1) == 0) strcpy(state.arb_file, WAVE2);
                else if (strcmp(state.arb_file, WAVE2) == 0) strcpy(state.arb_file, WAVE3);
                else strcpy(state.arb_file, DEFAULT_WAVE);
                state.wave_type = WAVE_ARB; state.params_changed = 1;
            }
            else if (key == 's') {
                /* 1. Ensure the structure is completely cleared before use */
                memset(&save, 0, sizeof(setup_t));

                /* 2. Safely copy the waveform name string */
                if (state.wave_type >= 0 && state.wave_type < 5) {
                    strncpy(save.waveform.waveform_type, wnames[state.wave_type], sizeof(save.waveform.waveform_type) - 1);
                } else {
                    strncpy(save.waveform.waveform_type, "sine", sizeof(save.waveform.waveform_type) - 1);
                }

                /* 3. Assign numeric values */
                save.waveform.frequency = state.frequency;
                save.waveform.amplitude = state.amplitude;
                save.waveform.offset    = state.offset;

                /* 4. Safely copy the arbitrary file path string */
                strncpy(save.waveform.arbitrary_file, state.arb_file, sizeof(save.waveform.arbitrary_file) - 1);

                /* 5. Mark as valid and save to disk */
                save.is_valid = 1;
                save_config_file("settings.dat", &save);
                
                printf("\rSettings saved to settings.dat          ");
                fflush(stdout);
            }
            else if (key == 'l') {
                loaded = load_config_file("settings.dat");
                if (loaded && loaded->is_valid) {
                    pthread_mutex_lock(&state.lock);
                    
                    state.wave_type = wave_type_from_string(loaded->waveform.waveform_type);
                    state.frequency = loaded->waveform.frequency;
                    state.amplitude = loaded->waveform.amplitude;
                    state.offset    = loaded->waveform.offset;
                    strncpy(state.arb_file, loaded->waveform.arbitrary_file, 255);

                    state.params_changed = 1;
                    
                    pthread_mutex_unlock(&state.lock);
                    free_setup(loaded);
                    
                    printf("\r[System] Loaded: %s at %.2f Hz          ", 
                            wnames[state.wave_type], state.frequency);
                    fflush(stdout);
                }
            }
            else if ((key == 'f' && sw3 == 0) || (key == 'a' && sw2 == 0) || key == 'o') {
                char target = key;
                state.input_mode = 1; 
                pthread_mutex_unlock(&state.lock);
                
                keyboard_restore(); 
                show_cursor();
                
                printf("\nEnter new %s: ", (target == 'f') ? "Freq" : (target == 'a') ? "Amp" : "Offset");
                fflush(stdout);
                
                if (fgets(input_buf, sizeof(input_buf), stdin)) {
                    input_val = atof(input_buf);
                    
                    pthread_mutex_lock(&state.lock);
                    if (target == 'f') {
                        if (input_val >= 0.01 && input_val <= 10.0) state.frequency = input_val;
                    } 
                    else if (target == 'a') {
                        if (input_val >= 0.0 && input_val <= 1.0) state.amplitude = input_val;
                    } 
                    else {
                        if (input_val >= -1.0 && input_val <= 1.0) state.offset = input_val;
                    }
                    state.params_changed = 1;
                    pthread_mutex_unlock(&state.lock);
                }

                hide_cursor(); 
                keyboard_init(); 
                state.input_mode = 0;
            }
            else if (key == 'q' || key == 'Q') state.running = 0;
            pthread_mutex_unlock(&state.lock);
        }

        #ifdef __QNX__
        if (sw2 == 1) {
            adc0 = read_adc(&dev, 0); pthread_mutex_lock(&state.lock);
            state.amplitude = (float)adc0 / 65535.0f; state.params_changed = 1; pthread_mutex_unlock(&state.lock);
        }
        if (sw3 == 1) {
            adc1_val = read_adc(&dev, 1); pthread_mutex_lock(&state.lock);
            state.frequency = 0.01 + ((float)adc1_val / 65535.0f) * 9.99; state.params_changed = 1; pthread_mutex_unlock(&state.lock);
        }
        #endif
        usleep(20000);
    }

    keyboard_restore(); pthread_join(wave_tid, NULL); pthread_join(disp_tid, NULL);
    hw_close(&dev); pthread_mutex_destroy(&state.lock);
    printf("\nClean shutdown complete.\n");
    return 0;
}