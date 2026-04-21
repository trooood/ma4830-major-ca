#ifndef SETUP_INPUT_H
#define SETUP_INPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

/* Hardware limits - single source of truth */
#define FREQ_MIN     0.01
#define FREQ_MAX     10.0
#define FREQ_DEFAULT 5.0
#define AMP_MIN      0.0
#define AMP_MAX      1.0
#define AMP_DEFAULT  1.0
#define OFF_MIN     -1.0
#define OFF_MAX      1.0
#define OFF_DEFAULT  0.0

// waveform configuration
typedef struct {
    char waveform_type[32];     // sine, square, tri, saw, arb
    double frequency;            // Hz
    double amplitude;            // 0.0 to 1.0
    double offset;               // -1.0 to 1.0
    char arbitrary_file[256];    // Path to arbitrary waveform file
} waveform_config_t;

// output configuration
typedef struct {
    int output_mode;             // 0=DAC, 1=Terminal, 2=Audio, 3=File, 4=Multi
    int sample_rate;             // Samples per second
    int duration_seconds;        // 0 = infinite
    char output_file[256];       // For file output mode
} output_config_t;

typedef struct {
    char config_file[256];       // Input config file
    char save_file[256];         // Where to save settings
    int show_help;               // Show help flag
} system_config_t;

typedef struct {
    waveform_config_t waveform;
    output_config_t output;
    system_config_t system;
    int is_valid;                // 1 if setup is valid, 0 if error
    char error_message[256];     // Error description if invalid
} setup_t;

// keyboard input structure
typedef struct {
    int up_pressed;
    int down_pressed;
    int left_pressed;
    int right_pressed;
    int space_pressed;
    char last_char;
} keyboard_state_t;


setup_t* parse_command_line(int argc, char *argv[]);
setup_t* load_config_file(const char *filename);
void save_config_file(const char *filename, const setup_t *setup);
void print_setup_summary(const setup_t *setup);
void free_setup(setup_t *setup);
void print_usage(const char *program_name);
void setup_apply_defaults(setup_t *setup);

// Keyboard input functions
void keyboard_init(void);
void keyboard_restore(void);
char keyboard_getch(void);
int keyboard_kbhit(void);
void keyboard_read_arrow(char *key, int *up, int *down, int *left, int *right);
void interactive_input_loop(setup_t *setup);
double safe_handling(const char *prompt, double min, double max, double default_val);
#endif // SETUP_INPUT_H
