/* Requires libraries:
*  Filters: https://github.com/JonHub/Filters
*  PID: https://playground.arduino.cc/Code/PIDLibrary/
*  Encoder: https://www.pjrc.com/teensy/td_libs_Encoder.html
*  ArduinoJson: https://arduinojson.org
*/

#include "RobotServer.h"
#include "RobotController.h"

unsigned long startTime;
unsigned long prevTime;

RobotServer server = RobotServer(Serial3, Serial);
RobotController controller = RobotController(Serial);

void setup()
{
    // Communication with the host computer
    Serial.begin(115200); 
    Serial.println("Robot starting");
}


void loop() 
{
    // Check for new command
    RobotServer::COMMAND newCmd = server.oneLoop();

    // Handle new command
    if(newCmd == RobotServer::COMMAND::MOVE)
    {
        RobotServer::MoveData data = server.getMoveData();
        controller.setCommand(data.x, data.y, data.a);
    }

    // Service controller
    controller.update();
}
