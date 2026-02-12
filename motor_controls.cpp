#include <iostream>
#include <pigpio.h>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <algorithm>

// Motor pins (BCM numbering)
#define LEFT_MOTOR 17
#define RIGHT_MOTOR 18

// Pulse width boundaries (microseconds)
#define PULSE_NEUTRAL 1500
#define PULSE_FORWARD 1750
#define PULSE_REVERSE 1250

// --- Ramping state ---
int leftPulse  = PULSE_NEUTRAL;
int rightPulse = PULSE_NEUTRAL;

int leftTarget  = PULSE_NEUTRAL;
int rightTarget = PULSE_NEUTRAL;

// --- Terminal control for non-blocking input ---
char getKey() {
    struct termios oldt, newt;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int bytesWaiting;
    ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
    if (bytesWaiting > 0) {
        read(STDIN_FILENO, &ch, 1);
    } else {
        ch = 0;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// --- Smooth ramping function ---
void updateMotors() {
    const int rampRate = 10;  // microseconds per update (smaller = smoother)

    if (leftPulse < leftTarget)
        leftPulse = std::min(leftPulse + rampRate, leftTarget);
    else if (leftPulse > leftTarget)
        leftPulse = std::max(leftPulse - rampRate, leftTarget);

    if (rightPulse < rightTarget)
        rightPulse = std::min(rightPulse + rampRate, rightTarget);
    else if (rightPulse > rightTarget)
        rightPulse = std::max(rightPulse - rampRate, rightTarget);

    gpioServo(LEFT_MOTOR, leftPulse);
    gpioServo(RIGHT_MOTOR, rightPulse);
}

// --- Motion commands (set targets only) ---
void stop() {
    leftTarget  = PULSE_NEUTRAL;
    rightTarget = PULSE_NEUTRAL;
}

void forward() {
    leftTarget  = PULSE_FORWARD;
    rightTarget = PULSE_FORWARD;
}

void backward() {
    leftTarget  = PULSE_REVERSE;
    rightTarget = PULSE_REVERSE;
}

void turnLeft() {
    leftTarget  = PULSE_REVERSE;
    rightTarget = PULSE_FORWARD;
}

void turnRight() {
    leftTarget  = PULSE_FORWARD;
    rightTarget = PULSE_REVERSE;
}

int main() {
    if (gpioInitialise() < 0) {
        std::cerr << "Failed to initialize pigpio.\n";
        return 1;
    }

    std::cout << "=== Sabertooth WASD Control (Smooth Ramp) ===\n";
    std::cout << "W - Forward\n";
    std::cout << "S - Backward\n";
    std::cout << "A - Turn Left\n";
    std::cout << "D - Turn Right\n";
    std::cout << "Space - Stop\n";
    std::cout << "Q - Quit\n\n";

    stop();

    bool running = true;
    while (running) {
        char key = getKey();

        switch (key) {
            case 'w': case 'W':
                std::cout << "Forward\n";
                forward();
                break;
            case 's': case 'S':
                std::cout << "Backward\n";
                backward();
                break;
            case 'a': case 'A':
                std::cout << "Turn Left\n";
                turnLeft();
                break;
            case 'd': case 'D':
                std::cout << "Turn Right\n";
                turnRight();
                break;
            case ' ':
                std::cout << "Stop\n";
                stop();
                break;
            case 'q': case 'Q':
                std::cout << "Quit\n";
                stop();
                running = false;
                break;
        }

        updateMotors();   // <<< Smooth transition happens here
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    stop();
    gpioTerminate();
    return 0;
}
