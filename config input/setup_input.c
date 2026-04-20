#include "setup_input.h"
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

int validate_setup(setup_t *setup);

// helper function for safe numeric input with retry
double safe_handling(const char *prompt, double min, double max, double default_val) {
    char buffer[64];
    char *endptr;
    double result;
    int valid = 0;
    
    while (!valid) {
        printf("%s", prompt);
        fflush(stdout);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Input error. Using default: %.2f\n", default_val);
            return default_val;
        }
        
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        // check if user just pressed Enter
        if (strlen(buffer) == 0) {
            printf("Using current value: %.2f\n", default_val);
            return default_val;
        }
        
        // strtod() for better error detection
        result = strtod(buffer, &endptr);
        
        // check if conversion failed (no digits read)
        if (endptr == buffer) {
            printf("ERROR: Invalid number. Please enter a valid number.\n");
            continue;
        }
        
        // check for extra characters after number
        while (*endptr == ' ') endptr++;  // skip spaces
        if (*endptr != '\0') {
            printf("ERROR: Extra characters detected: '%s'. Please enter only a number.\n", endptr);
            continue;
        }
        
        // check range
        if (result < min || result > max) {
            printf("ERROR: Value must be between %.2f and %.2f. You entered: %.2f\n", min, max, result);
            continue;
        }
        
        valid = 1;
    }
    
    return result;
}

// helper function for safe waveform type input with retry
void safe_get_waveform_type(setup_t *setup) {
    char buffer[32];
    int valid = 0;
    
    while (!valid) {
        printf("Enter waveform type (sine, square, tri, saw, arb): ");
        fflush(stdout);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Input error. Keeping current: %s\n", setup->waveform.waveform_type);
            return;
        }
        
        // remove newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
        // convert to lowercase
        for (int i = 0; buffer[i]; i++) {
            buffer[i] = tolower(buffer[i]);
        }
        
        // check against valid types
        if (strcmp(buffer, "sine") == 0) {
            strcpy(setup->waveform.waveform_type, "sine");
            valid = 1;
        } else if (strcmp(buffer, "square") == 0) {
            strcpy(setup->waveform.waveform_type, "square");
            valid = 1;
        } else if (strcmp(buffer, "tri") == 0) {
            strcpy(setup->waveform.waveform_type, "tri");
            valid = 1;
        } else if (strcmp(buffer, "saw") == 0) {
            strcpy(setup->waveform.waveform_type, "saw");
            valid = 1;
        } else if (strcmp(buffer, "arb") == 0) {
            strcpy(setup->waveform.waveform_type, "arb");
            valid = 1;
        } else {
            printf("ERROR: Invalid waveform type '%s'. Valid types: sine, square, tri, saw, arb\n", buffer);
        }
    }
}

// Default Values

static void apply_defaults(setup_t *setup) {
    strcpy(setup->waveform.waveform_type, "sine");
    setup->waveform.frequency = 440.0;
    setup->waveform.amplitude = 1.0;
    setup->waveform.offset = 0.0;
    strcpy(setup->waveform.arbitrary_file, "wave.txt");
    
    setup->output.output_mode = 1;
    setup->output.sample_rate = 48000;
    setup->output.duration_seconds = 0;
    strcpy(setup->output.output_file, "output.txt");
    
    strcpy(setup->system.config_file, "");
    strcpy(setup->system.save_file, "");
    setup->system.show_help = 0;
    
    setup->is_valid = 1;
    strcpy(setup->error_message, "");
}

// command line input
setup_t* parse_command_line(int argc, char *argv[]) {
    setup_t *setup = (setup_t*)malloc(sizeof(setup_t));
    if (!setup) return NULL;
    
    apply_defaults(setup); 
    
    // helper function for safe command line parsing
    if (argc > 1) {
        char *type = argv[1];
        
        for(int i = 0; type[i]; i++) {
            type[i] = tolower(type[i]);
        }
        
        if (strcmp(type, "sine") == 0)
            strcpy(setup->waveform.waveform_type, "sine");
        else if (strcmp(type, "square") == 0)
            strcpy(setup->waveform.waveform_type, "square");
        else if (strcmp(type, "tri") == 0)
            strcpy(setup->waveform.waveform_type, "tri");
        else if (strcmp(type, "saw") == 0)
            strcpy(setup->waveform.waveform_type, "saw");
        else if (strcmp(type, "arb") == 0)
            strcpy(setup->waveform.waveform_type, "arb");
        else {
            char *endptr;
            double freq = strtod(type, &endptr);
            if (endptr != type && *endptr == '\0') {
                setup->waveform.frequency = freq;
            } else {
                printf("Warning: Unknown argument '%s' ignored\n", type);
            }
        }
    }
    
    // validate frequency
    if (argc > 2) {
        char *endptr;
        double freq = strtod(argv[2], &endptr);
        if (endptr != argv[2] && *endptr == '\0') {
            setup->waveform.frequency = freq;
        } else {
            printf("Warning: Invalid frequency '%s', using default %.2f\n", argv[2], setup->waveform.frequency);
        }
    }
    
    // validate amplitude
    if (argc > 3) {
        char *endptr;
        double amp = strtod(argv[3], &endptr);
        if (endptr != argv[3] && *endptr == '\0') {
            if (amp >= 0 && amp <= 1) {
                setup->waveform.amplitude = amp;
            } else {
                printf("Warning: Amplitude %.2f out of range (0-1), using default %.2f\n", amp, setup->waveform.amplitude);
            }
        } else {
            printf("Warning: Invalid amplitude '%s', using default %.2f\n", argv[3], setup->waveform.amplitude);
        }
    }
    
    // validate offset
    if (argc > 4) {
        char *endptr;
        double off = strtod(argv[4], &endptr);
        if (endptr != argv[4] && *endptr == '\0') {
            if (off >= -1 && off <= 1) {
                setup->waveform.offset = off;
            } else {
                printf("Warning: Offset %.2f out of range (-1 to 1), using default %.2f\n", off, setup->waveform.offset);
            }
        } else {
            printf("Warning: Invalid offset '%s', using default %.2f\n", argv[4], setup->waveform.offset);
        }
    }
    
    if (argc > 5 && strcmp(setup->waveform.waveform_type, "arb") == 0) {
        strncpy(setup->waveform.arbitrary_file, argv[5], 255);
        setup->waveform.arbitrary_file[255] = '\0';
    }
    
    validate_setup(setup);
    
    return setup;
}

// configuration file input
setup_t* load_config_file(const char *filename) {
    setup_t *setup = (setup_t*)malloc(sizeof(setup_t));
    if (!setup) return NULL;
    
    apply_defaults(setup);
    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Cannot open config file: %s", filename);
        return setup;
    }
    
    char line[512];
    char key[256];
    char value[256];
    
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        if (sscanf(line, "%255[^=]=%255s", key, value) == 2) {
            char *k = key;
            while (*k == ' ' || *k == '\t') k++;
            char *end = k + strlen(k) - 1;
            while (end > k && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
            *(end+1) = '\0';
            
            if (strcmp(k, "waveform_type") == 0) {
                strncpy(setup->waveform.waveform_type, value, 31);
                setup->waveform.waveform_type[31] = '\0';
            } else if (strcmp(k, "frequency") == 0) {
                char *endptr;
                double freq = strtod(value, &endptr);
                if (endptr != value) {
                    setup->waveform.frequency = freq;
                }
            } else if (strcmp(k, "amplitude") == 0) {
                char *endptr;
                double amp = strtod(value, &endptr);
                if (endptr != value) {
                    setup->waveform.amplitude = amp;
                }
            } else if (strcmp(k, "offset") == 0) {
                char *endptr;
                double off = strtod(value, &endptr);
                if (endptr != value) {
                    setup->waveform.offset = off;
                }
            } else if (strcmp(k, "arbitrary_file") == 0) {
                strncpy(setup->waveform.arbitrary_file, value, 255);
                setup->waveform.arbitrary_file[255] = '\0';
            } else if (strcmp(k, "output_mode") == 0) {
                if (strcmp(value, "dac") == 0) setup->output.output_mode = 0;
                else if (strcmp(value, "terminal") == 0) setup->output.output_mode = 1;
                else if (strcmp(value, "audio") == 0) setup->output.output_mode = 2;
                else if (strcmp(value, "file") == 0) setup->output.output_mode = 3;
                else if (strcmp(value, "multi") == 0) setup->output.output_mode = 4;
            } else if (strcmp(k, "sample_rate") == 0) {
                char *endptr;
                long rate = strtol(value, &endptr, 10);
                if (endptr != value) {
                    setup->output.sample_rate = (int)rate;
                }
            } else if (strcmp(k, "duration") == 0) {
                char *endptr;
                long dur = strtol(value, &endptr, 10);
                if (endptr != value) {
                    setup->output.duration_seconds = (int)dur;
                }
            } else if (strcmp(k, "output_file") == 0) {
                strncpy(setup->output.output_file, value, 255);
                setup->output.output_file[255] = '\0';
            }
        }
    }
    
    fclose(fp);
    validate_setup(setup);
    return setup;
}

void save_config_file(const char *filename, const setup_t *setup) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot save config file %s\n", filename);
        return;
    }
    
    time_t now = time(NULL);
    fprintf(fp, "Beat Generator Configuration File\n");
  
    fprintf(fp, "# waveform Settings\n");
    fprintf(fp, "waveform_type = %s\n", setup->waveform.waveform_type);
    fprintf(fp, "frequency = %.2f\n", setup->waveform.frequency);
    fprintf(fp, "amplitude = %.2f\n", setup->waveform.amplitude);
    fprintf(fp, "offset = %.2f\n", setup->waveform.offset);
    fprintf(fp, "arbitrary_file = %s\n", setup->waveform.arbitrary_file);
    
    fprintf(fp, "\n# output Settings\n");
    const char *modes[] = {"dac", "terminal", "audio", "file", "multi"};
    fprintf(fp, "output_mode = %s\n", modes[setup->output.output_mode]);
    fprintf(fp, "sample_rate = %d\n", setup->output.sample_rate);
    fprintf(fp, "duration = %d\n", setup->output.duration_seconds);
    fprintf(fp, "output_file = %s\n", setup->output.output_file);
    
    fclose(fp);
}

// validation
int validate_setup(setup_t *setup) {
    if (!setup->is_valid) return 0;
    
    if (strcmp(setup->waveform.waveform_type, "sine") != 0 &&
        strcmp(setup->waveform.waveform_type, "square") != 0 &&
        strcmp(setup->waveform.waveform_type, "tri") != 0 &&
        strcmp(setup->waveform.waveform_type, "saw") != 0 &&
        strcmp(setup->waveform.waveform_type, "arb") != 0) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Invalid waveform type: %s", setup->waveform.waveform_type);
        return 0;
    }
    
    if (strcmp(setup->waveform.waveform_type, "arb") == 0) {
        FILE *test = fopen(setup->waveform.arbitrary_file, "r");
        if (!test) {
            setup->is_valid = 0;
            sprintf(setup->error_message, "Arbitrary waveform file not found: %s", 
                    setup->waveform.arbitrary_file);
            return 0;
        }
        fclose(test);
    }
    
    if (setup->waveform.frequency < 0.01 || setup->waveform.frequency > 20000) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Frequency out of range: %.2f Hz (0.01-20000)", 
                setup->waveform.frequency);
        return 0;
    }
    
    if (setup->waveform.amplitude < 0 || setup->waveform.amplitude > 1) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Amplitude out of range: %.2f (0-1)", 
                setup->waveform.amplitude);
        return 0;
    }
    
    if (setup->waveform.offset < -1 || setup->waveform.offset > 1) {
        setup->is_valid = 0;
        sprintf(setup->error_message, "Offset out of range: %.2f (-1 to 1)", 
                setup->waveform.offset);
        return 0;
    }
    
    return 1;
}

// print selected setup configuration summary

void print_setup_summary(const setup_t *setup) {
    if (!setup) return;
    
    printf("\n");
    printf("======== INITIAL SETUP SUMMARY ========\n");
    
    printf("\nWAVEFORM CONFIGURATION:\n");
    printf("Type:        %s\n", setup->waveform.waveform_type);
    printf("Frequency:   %.2f Hz\n", setup->waveform.frequency);
    printf("Amplitude:   %.2f\n", setup->waveform.amplitude);
    printf("Offset:      %.2f\n", setup->waveform.offset);
    
    if (strcmp(setup->waveform.waveform_type, "arb") == 0) {
        printf("File:        %s\n", setup->waveform.arbitrary_file);
    }
    
    const char *modes[] = {"DAC", "TERMINAL", "AUDIO", "FILE", "MULTI"};
    printf("\nOUTPUT CONFIGURATION:\n");
    printf("Mode:        %s\n", modes[setup->output.output_mode]);
    printf("Sample Rate: %d Hz\n", setup->output.sample_rate);
    if (setup->output.duration_seconds > 0) {
        printf("Duration:    %d seconds\n", setup->output.duration_seconds);
    } else {
        printf("Duration:    INFINITE\n");
    }
    
    printf("\nSetup Status: %s\n", setup->is_valid ? "VALID" : "INVALID");
    if (!setup->is_valid) {
        printf("Error: %s\n", setup->error_message);
    }
    printf("\n");
}
    
void free_setup(setup_t *setup) {
    if (setup) {
        free(setup);
    }
}

// KEYBOARD INPUT FUNCTIONS
#include <termios.h>
#include <fcntl.h>

static struct termios orig_termios;

void keyboard_init(void) {
    struct termios raw;
    
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    
    // Disable canonical mode, echo, signals
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_cc[VMIN] = 0;  // Non-blocking
    raw.c_cc[VTIME] = 0; // No timeout
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void keyboard_restore(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

char keyboard_getch(void) {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        return ch;
    }
    return 0;
}

int keyboard_kbhit(void) {
    int count;
    ioctl(STDIN_FILENO, FIONREAD, &count);
    return count > 0;
}

void keyboard_read_arrow(char *key, int *up, int *down, int *left, int *right) {
    *up = *down = *left = *right = 0;
    
    if (keyboard_kbhit()) {
        char ch = keyboard_getch();
        
        if (ch == 27) {  // ESC sequence for arrows
            if (keyboard_kbhit() && keyboard_getch() == '[') {
                if (keyboard_kbhit()) {
                    ch = keyboard_getch();
                    switch(ch) {
                        case 'A': *up = 1; *key = 'U'; break;
                        case 'B': *down = 1; *key = 'D'; break;
                        case 'C': *right = 1; *key = 'R'; break;
                        case 'D': *left = 1; *key = 'L'; break;
                    }
                }
            }
        } else {
            *key = ch;
        }
    }
}

// keyboard input loop with error handling and retry
void keyboard_input_loop(setup_t *setup) {
    char key = 0;
    int up = 0, down = 0, left = 0, right = 0;
        
    keyboard_init();
    
    while (1) {
        keyboard_read_arrow(&key, &up, &down, &left, &right);
        
        if (up) {
            setup->waveform.frequency *= 1.1;
            if (setup->waveform.frequency > 20000) setup->waveform.frequency = 20000;
            printf("\rFrequency: %.2f Hz     ", setup->waveform.frequency);
            fflush(stdout);
        }
        else if (down) {
            setup->waveform.frequency /= 1.1;
            if (setup->waveform.frequency < 0.01) setup->waveform.frequency = 0.01;
            printf("\rFrequency: %.2f Hz     ", setup->waveform.frequency);
            fflush(stdout);
        }
        else if (left) {
            if (strcmp(setup->waveform.waveform_type, "sine") == 0)
                strcpy(setup->waveform.waveform_type, "saw");
            else if (strcmp(setup->waveform.waveform_type, "saw") == 0)
                strcpy(setup->waveform.waveform_type, "tri");
            else if (strcmp(setup->waveform.waveform_type, "tri") == 0)
                strcpy(setup->waveform.waveform_type, "square");
            else if (strcmp(setup->waveform.waveform_type, "square") == 0)
                strcpy(setup->waveform.waveform_type, "arb");
            else if (strcmp(setup->waveform.waveform_type, "arb") == 0)
                strcpy(setup->waveform.waveform_type, "sine");
            
            printf("\rWaveform: %s     ", setup->waveform.waveform_type);
            fflush(stdout);
        }
        else if (right) {
            if (strcmp(setup->waveform.waveform_type, "sine") == 0)
                strcpy(setup->waveform.waveform_type, "square");
            else if (strcmp(setup->waveform.waveform_type, "square") == 0)
                strcpy(setup->waveform.waveform_type, "tri");
            else if (strcmp(setup->waveform.waveform_type, "tri") == 0)
                strcpy(setup->waveform.waveform_type, "saw");
            else if (strcmp(setup->waveform.waveform_type, "saw") == 0)
                strcpy(setup->waveform.waveform_type, "arb");
            else if (strcmp(setup->waveform.waveform_type, "arb") == 0)
                strcpy(setup->waveform.waveform_type, "sine");
            
            printf("\rWaveform: %s     ", setup->waveform.waveform_type);
            fflush(stdout);
        }
        else if (key == '+') {
            setup->waveform.amplitude += 0.05;
            if (setup->waveform.amplitude > 1.0) setup->waveform.amplitude = 1.0;
            printf("\rAmplitude: %.2f     ", setup->waveform.amplitude);
            fflush(stdout);
        }
        else if (key == '-') {
            setup->waveform.amplitude -= 0.05;
            if (setup->waveform.amplitude < 0.0) setup->waveform.amplitude = 0.0;
            printf("\rAmplitude: %.2f     ", setup->waveform.amplitude);
            fflush(stdout);
        }
        else if (key == '[') {
            setup->waveform.offset -= 0.05;
            if (setup->waveform.offset < -1.0) setup->waveform.offset = -1.0;
            printf("\rOffset: %.2f     ", setup->waveform.offset);
            fflush(stdout);
        }
        else if (key == ']') {
            setup->waveform.offset += 0.05;
            if (setup->waveform.offset > 1.0) setup->waveform.offset = 1.0;
            printf("\rOffset: %.2f     ", setup->waveform.offset);
            fflush(stdout);
        }
        else if (key == 'f' || key == 'F') {
            // temporarily exit raw mode for input
            keyboard_restore();
            printf("\n");
            double new_freq = safe_handling("Enter frequency (Hz, 0.01-20000): ", 
                                               0.01, 20000.0, setup->waveform.frequency);
            setup->waveform.frequency = new_freq;
            printf("Frequency set to %.2f Hz\n", setup->waveform.frequency);
            // re-enter raw mode
            keyboard_init();
            printf("\rFrequency: %.2f Hz     ", setup->waveform.frequency);
            fflush(stdout);
        }
        else if (key == 'a' || key == 'A') {
            keyboard_restore();
            printf("\n");
            double new_amp = safe_handling("Enter amplitude (0.0-1.0): ", 
                                             0.0, 1.0, setup->waveform.amplitude);
            setup->waveform.amplitude = new_amp;
            printf("Amplitude set to %.2f\n", setup->waveform.amplitude);
            keyboard_init();
            printf("\rAmplitude: %.2f     ", setup->waveform.amplitude);
            fflush(stdout);
        }
        else if (key == 'o' || key == 'O') {
            keyboard_restore();
            printf("\n");
            double new_off = safe_handling("Enter offset (-1.0 to 1.0): ", 
                                             -1.0, 1.0, setup->waveform.offset);
            setup->waveform.offset = new_off;
            printf("Offset set to %.2f\n", setup->waveform.offset);
            keyboard_init();
            printf("\rOffset: %.2f     ", setup->waveform.offset);
            fflush(stdout);
        }
        else if (key == 's' || key == 'S') {
            save_config_file("keyboard_settings.dat", setup);
            printf("\n✓ Configuration saved to keyboard_settings.dat\n");
        }
        else if (key == 'l' || key == 'L') {
            setup_t *loaded = load_config_file("keyboard_settings.dat");
            if (loaded && loaded->is_valid) {
                *setup = *loaded;
                free_setup(loaded);
                printf("\n✓ Configuration loaded from keyboard_settings.dat\n");
                print_setup_summary(setup);
            } else {
                printf("\n✗ No saved configuration found\n");
            }
        }
        else if (key == 'q' || key == 'Q') {
            break;
        }
        
        usleep(50000);  // 50ms delay
    }
    
    keyboard_restore();
}
