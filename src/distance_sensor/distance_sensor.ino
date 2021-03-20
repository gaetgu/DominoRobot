#include "SerialComms.h"

#define NUM_SENSORS 1
#define TRIGGER_PIN_1 10
#define ECHO_PIN_1 9
#define LED_PIN 3

#define MICROSECONDS_TO_MILLIMETERS 0.343
#define PULSE_IN_TIMEOUT_US 100000
#define US_TIMEOUT_SEC 30

SerialComms comm(Serial);
int distances[NUM_SENSORS];
int trigger_pins[] = {TRIGGER_PIN_1};
int echo_pins[] = {ECHO_PIN_1};
bool running = false;
unsigned long start_time = millis();

int takeMeasurement(int i)
{
    // Send trigger pulse
    digitalWrite(trigger_pins[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trigger_pins[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trigger_pins[i], LOW);

    // Measure echo pulse
    unsigned long duration_us = pulseIn(echo_pins[i], HIGH, PULSE_IN_TIMEOUT_US);
    float distance_mm = (static_cast<float>(duration_us)*MICROSECONDS_TO_MILLIMETERS)/2.0;
    return round(distance_mm);
}

void checkForStartStop()
{
    String msg = comm.rcv();

    if(running && (millis() - start_time) > US_TIMEOUT_SEC*1000) 
    {
      msg = "stop";
    }
    
    if(msg.length() > 0)
    {
        if (msg == "start")
        {
            running = true;
            digitalWrite(LED_PIN, HIGH);
            start_time = millis();
        }
        else if (msg == "stop")
        {
            running = false;
            digitalWrite(LED_PIN, LOW);
        }
    }
}

void sendDistances()
{
    String msg="dist:";
    for(int i = 0; i < NUM_SENSORS; i++)
    {
        msg.concat(distances[i]);
        msg.concat(',');
    }
    comm.send(msg);

   // For debugging
//   Serial.println(' ');
}


void setup() 
{
    Serial.begin(115200);

    for (int i = 0; i < NUM_SENSORS; i++)
    {
        pinMode(trigger_pins[i], OUTPUT);
        pinMode(echo_pins[i], INPUT);
    }
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void loop() 
{
    checkForStartStop();
    if(running)
    {
        for (int i = 0; i < NUM_SENSORS; i++)
        {
            distances[i] = takeMeasurement(i);
        }
        sendDistances();
    }
}
