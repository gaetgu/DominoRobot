// Uses stepper driver library from here: https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html
#include <AccelStepper.h>
#include <Servo.h>
#include "SerialComms.h"

// LEFT/RIGHT is WRT standing at back of robot looking forward
#define STEP_PIN_LEFT 9
#define STEP_PIN_RIGHT 7
#define DIR_PIN_LEFT 8
#define DIR_PIN_RIGHT 6
#define INCREMENTAL_UP_PIN 11
#define INCREMENTAL_DOWN_PIN 12
#define ENABLE_PIN 13
#define LATCH_SERVO_PIN 2
#define HOMING_SWITCH_PIN 10

#define STEPS_PER_REV 800

#define MAX_VEL 2  // revs/sec
#define MAX_ACC 10  // rev/sec^2

#define SAFETY_MAX_POS 120  // Revs, Sanity check on desired position to make sure it isn't larger than this
#define SAFETY_MIN_POS 0 // Revs, Sanity check on desired position to make sure it isn't less than this

#define LATCH_ACTIVE_MS 2000
#define LATCH_OPEN_POS 180
#define LATCH_CLOSE_POS 0

enum MODE
{
    NONE,
    MANUAL_VEL,
    AUTO_POS,
    HOMING,
    LATCH_OPEN,
    LATCH_CLOSE,
};

struct Command
{
    int abs_pos;
    bool home;
    bool valid;
    bool stop;
    bool latch_open;
    bool latch_close;
};

AccelStepper motors[2];
Servo latchServo;
SerialComms comms(Serial);

MODE activeMode = MODE::NONE;
unsigned long prevLatchMillis = millis();

void setup() 
{
    Serial.begin(115200);
    
    pinMode(INCREMENTAL_UP_PIN, INPUT_PULLUP);
    pinMode(INCREMENTAL_DOWN_PIN, INPUT_PULLUP);
    pinMode(HOMING_SWITCH_PIN, INPUT_PULLUP);
    pinMode(ENABLE_PIN, INPUT);
    digitalWrite(ENABLE_PIN, HIGH);

    latchServo.attach(LATCH_SERVO_PIN);
    
    motors[0] = AccelStepper(AccelStepper::DRIVER, STEP_PIN_LEFT, DIR_PIN_LEFT);
    motors[1] = AccelStepper(AccelStepper::DRIVER, STEP_PIN_RIGHT, DIR_PIN_RIGHT);

    for(int i = 0; i < 2; i++)
    {
      motors[i].setMaxSpeed(MAX_VEL*STEPS_PER_REV);
      motors[i].setAcceleration(MAX_ACC*STEPS_PER_REV);
      motors[i].setMinPulseWidth(10); // Min from docs is 2.5 microseconds
    }
}


// Possible inputs are
// "home", "stop", or an integer representing position to move to
// "open" or "close" controls the latch servo
Command getAndDecodeMsg()
{
    String msg = comms.rcv();
    Command c = {0, false, false, false, false, false};
    if(msg.length() > 0)
    {
        if(msg == "home")
        {
            c.home = true;
            c.valid = true;
        }
        else if(msg == "stop")
        {
            c.stop = true;
            c.valid = true;
        }
        else if (msg == "open")
        {
            c.latch_open = true;
            c.valid = true;
        }
        else if (msg == "close")
        { 
            c.latch_close = true;
            c.valid = true;
        }
        else
        {
            c.abs_pos = msg.toInt();
            c.valid = true;
        }
    }

    return c;
}

void loop()
{
    // Check for an incomming command
    Command inputCommand = getAndDecodeMsg();
    
    // Read inputs for manual mode
    bool vel_up = digitalRead(INCREMENTAL_UP_PIN);
    bool vel_down = digitalRead(INCREMENTAL_DOWN_PIN);

    // For debugging
//    Serial.print("Vel up: ");
//    Serial.print(vel_up);
//    Serial.print(" Vel dn: ");
//    Serial.println(vel_down);

    // If we got a stop command, make sure to handle it immediately
    if (inputCommand.valid && inputCommand.stop)
    {
        activeMode = MODE::NONE;
    }
    // For any other command, we will only handle it when there are no other active commands
    else if (activeMode == MODE::NONE)
    {
        // Figure out what mode we are in
        if(vel_up || vel_down)
        {
            activeMode = MODE::MANUAL_VEL;
            if(vel_up)
            {
                motors[0].setSpeed(-1*MAX_VEL*STEPS_PER_REV);
                motors[1].setSpeed(-1*MAX_VEL*STEPS_PER_REV);
            }
            else if(vel_down)
            {
                motors[0].setSpeed(MAX_VEL*STEPS_PER_REV);
                motors[1].setSpeed(MAX_VEL*STEPS_PER_REV);
            }   
        }
        else if(inputCommand.valid && inputCommand.home)
        {
            activeMode = MODE::HOMING;
            motors[0].setSpeed(-1*MAX_VEL*STEPS_PER_REV);
            motors[1].setSpeed(-1*MAX_VEL*STEPS_PER_REV);
        }
        else if(inputCommand.valid && inputCommand.latch_open)
        {
            activeMode = MODE::LATCH_OPEN;
            latchServo.write(LATCH_OPEN_POS);
            prevLatchMillis = millis();
        }
        else if(inputCommand.valid && inputCommand.latch_close)
        {
            activeMode = MODE::LATCH_CLOSE;
            latchServo.write(LATCH_CLOSE_POS);
            prevLatchMillis = millis();
        }
        else if(inputCommand.valid)
        {
            if(inputCommand.abs_pos <= SAFETY_MAX_POS && inputCommand.abs_pos >= SAFETY_MIN_POS)
            {
                activeMode = MODE::AUTO_POS;
                long target = inputCommand.abs_pos;
                motors[0].moveTo(target*STEPS_PER_REV);
                motors[1].moveTo(target*STEPS_PER_REV);
            }
        }
    }    


    // Handle continous updates for each mode
    String status_str = "none";
    if(activeMode == MODE::MANUAL_VEL)
    {
        motors[0].runSpeed();
        motors[1].runSpeed();
        status_str = "manual";
        if(!(vel_up || vel_down))
        {
            activeMode = MODE::NONE;
        }
    }
    else if(activeMode == MODE::AUTO_POS)
    {
        // run() returns a bool indicating if the motors are still moving
        bool tmp1 = motors[0].run();
        bool tmp2 = motors[1].run();
        status_str = "pos";
        if(!(tmp1 || tmp2))
        {
            activeMode = MODE::NONE;
        }
    }
    else if(activeMode == MODE::HOMING)
    {
        motors[0].runSpeed();
        motors[1].runSpeed();
        if(digitalRead(HOMING_SWITCH_PIN))
        {
            motors[0].stop();
            motors[1].stop();  
            motors[0].setCurrentPosition(0);
            motors[1].setCurrentPosition(0);
            activeMode = MODE::NONE;
        }
        status_str = "homing";
    }
    else if (activeMode == MODE::LATCH_CLOSE)
    {
        status_str = "close";
        if(millis() - prevLatchMillis > LATCH_ACTIVE_MS)
        {
            activeMode = MODE::NONE;
        }
    }
    else if (activeMode == MODE::LATCH_OPEN)
    {
        status_str = "open";
        if(millis() - prevLatchMillis > LATCH_ACTIVE_MS)
        {
            activeMode = MODE::NONE;
        }
    }
    else if (activeMode == MODE::NONE)
    {
        motors[0].stop();
        motors[1].stop();
    }

    // Will send back one of [none, manual, pos, homing, open, close]
    comms.send(status_str);
    // For debugging
    Serial.println("");
}
