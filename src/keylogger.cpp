#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>

#include <termios.h>
#include <unistd.h>

// Function to set terminal to raw mode
void setRawMode(struct termios& orig_termios) {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    new_termios = orig_termios;
    
    // Disable canonical mode, echo, and signals
    new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_termios.c_cc[VMIN] = 1;  // Minimum characters to read
    new_termios.c_cc[VTIME] = 0; // No timeout
    
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

// Function to restore terminal settings
void restoreTerminal(const struct termios& orig_termios) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// Get current timestamp as string
std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&time);
    // Remove newline from ctime
    if (!timestamp.empty() && timestamp[timestamp.length()-1] == '\n') {
        timestamp.pop_back();
    }
    return timestamp;
}

int main() {
    struct termios orig_termios;
    std::ofstream logfile("keyboard_log.txt", std::ios::app);
    
    if (!logfile.is_open()) {
        std::cerr << "Error: Could not open log file" << std::endl;
        return 1;
    }

    std::cout << "Starting keyboard logger. Press 'q' to quit." << std::endl;
    std::cout << "Logging to keyboard_log.txt" << std::endl;

    // Set terminal to raw mode
    setRawMode(orig_termios);
    
    // Ensure terminal is restored even if program crashes
    struct RestoreGuard {
        const struct termios& term;
        ~RestoreGuard() { restoreTerminal(term); }
    } guard{orig_termios};

    char c;
    while (true) {
        // Read one character at a time
        if (read(STDIN_FILENO, &c, 1) > 0) {
            // Write to file with timestamp
            logfile << "[" << getTimestamp() << "] ";
            
            // Handle special keys
            if (c == 27) {  // ESC sequence
                logfile << "[ESC]" << std::endl;
            }
            else if (c == '\n') {
                logfile << "[ENTER]" << std::endl;
            }
            else if (c == 127) {
                logfile << "[BACKSPACE]" << std::endl;
            }
            else if (c >= 32 && c <= 126) {  // Printable characters
                logfile << c << std::endl;
            }
            else {
                logfile << "[CTRL+" << (char)(c + 64) << "]" << std::endl;
            }
            
            logfile.flush();  // write immediately
            
            // Quit on 'q'
            if (c == 'q') {
                break;
            }
        }
    }

    std::cout << "\nKeyboard logger stopped." << std::endl;
    logfile.close();
    return 0;
}