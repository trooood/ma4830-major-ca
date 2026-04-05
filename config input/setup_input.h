#ifndef SETUP_INPUT_H
#define SETUP_INPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
} system_config_t;

typedef struct {
    waveform_config_t waveform;
    output_config_t output;
    system_config_t system;
    int is_valid;                // 1 if setup is valid, 0 if error
    char error_message[256];     // Error description if invalid
} setup_t;

setup_t* parse_command_line(int argc, char *argv[]);
setup_t* load_config_file(const char *filename);
void save_config_file(const char *filename, const setup_t *setup);
void print_setup_summary(const setup_t *setup);
void free_setup(setup_t *setup);
void print_usage(const char *program_name);

#endif // SETUP_INPUT_H
