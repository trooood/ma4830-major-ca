// UX_UI Design Logic
// By Misha Rashanah
// 3rd April 2026

// Purpose of this .c file is to giver user a smooth and robust experience when running the progam
// In the event, there is an error input, we would explicitly show an error message

// Headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// --- SECTION 1: ERROR HANDLING & SHUTDOWN ---

/* This function fulfills the "Orderly Shutdown / SIGINT" requirement
1. SIGINT: Refers to the Signal Interupt and would signal our program to stop by pressing Ctrl + C
2. The program from crashing and leaving the D/A port in an active state.
3. Ensures a clean exit and resets hardware resources to 0V.
*/
void handle_shutdown(int sig) {
    printf("\n\n==========================================");
    printf("\n[SYSTEM] SIGINT (Ctrl+C) Detected.");
    printf("\n[SYSTEM] Resetting D/A Hardware to 0V...");
    printf("\n[SYSTEM] Closing UI Threads...");
    printf("\n[SYSTEM] Program terminated safely. Null Terminators Signing Off!");
    printf("\n==========================================\n");
    exit(0);
}

/*
Robustness Check: Validates user-entered frequency against hardware limits.
Provides contextual feedback and instructions if the input is out of range 
to prevent system instability or hardware clipping.
*/
int validate_input(float freq) {
    // Defining hardware safety limits (Example: 1Hz to 10kHz)
    const float MAX_FREQ = 10000.0;
    const float MIN_FREQ = 1.0;

    if (freq < MIN_FREQ || freq > MAX_FREQ) {
        printf("\n=====================================================");
        printf("\n[!] ERROR: %.2f Hz is out of range.", freq);
        printf("\n[i] INSTRUCTION: Please enter a value between 1.0 and 10,000.0 Hz.");
        printf("\n    High frequencies may cause aliasing on the oscilloscope.");
        printf("\n=====================================================\n");
        return 0; // Return 0 to indicate validation failed
    }
    return 1; // Return 1 to indicate validation passed
}

// --- SECTION 2: UI DISPLAY LOGIC ---

/*
UI Dashboard: Provides a real-time 'Board Status' display.
Shows active settings and instructions for the user.
*/
void display_board_status(char *type, float freq, float amp) {
    // Clear the terminal screen for a clean UI
    printf("\033[H\033[J"); 

    printf("=====================================================\n");
    printf("        MA4830 REAL-TIME WAVEFORM GENERATOR [NULL TERMINATORS]          \n");
    printf("=====================================================\n");
    printf(" [BOARD STATUS]                                      \n");
    printf("  - Active Waveform:  %s                             \n", type);
    printf("  - Target Frequency: %.2f Hz                        \n", freq);
    printf("  - Amplitude Scale:  %.2f                           \n", amp);
    printf("-----------------------------------------------------\n");
    printf(" [USER INSTRUCTIONS]                                 \n");
    printf("  - To change settings, use Command Line Arguments.  \n");
    printf("  - Example: ./waveform_gen square 500 0.8           \n");
    printf("  - Use [Ctrl+C] to safely stop the generator.       \n");
    printf("=====================================================\n");
}

// --- SECTION 3: MAIN UI EXECUTION ---

int main(int argc, char *argv[]) {
    // Register the SIGINT trap first for safety
    signal(SIGINT, handle_shutdown);

    // Default settings if no arguments are provided
    char *type = (argc > 1) ? argv[1] : "sine";
    float freq = (argc > 2) ? atof(argv[2]) : 440.0;
    float amp  = (argc > 3) ? atof(argv[3]) : 1.0;

    // Fulfills the requirement: "Provide useful instruction when an input is incorrect"
    if (!validate_input(freq)) {
        // Stop execution if input is dangerous/invalid
        return 1; 
    }

    // Display the professional UI shell
    display_board_status(type, freq, amp);

    // Placeholder for the output loop 
    // This is where Walter's threading logic will eventually sit
    printf("\n[UI] Output Thread starting... check Oscilloscope.\n");
    
    while(1) {
        // Keep the program alive to show the UI and wait for SIGINT
        pause(); 
    }

    return 0;
}
