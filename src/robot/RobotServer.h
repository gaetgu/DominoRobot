/*
* Robot server that handles gettings messages from master
* and responding correctly
*/

#ifndef RobotServer_h
#define RobotServer_h

#include <HardwareSerial.h>

#define START_CHAR '<'
#define END_CHAR '>'

class RobotServer
{
  public:

    enum COMMAND
    {
       NONE,
       ERROR_NO_TYPE,
       ERROR_UNKOWN_TYPE,
       ERROR_BAD_JSON,
       MOVE,
       PLACE,
       DOCK,
       UNDOCK,
       DROPOFF,
       PICKUP,
       POSITION,
       STATUS
    };

    struct MoveData
    {
      float x;
      float y;
      float a;
    };
    
    // Constructor
    RobotServer(HardwareSerial& serial, HardwareSerial& debug);

    RobotServer::COMMAND oneLoop();

    RobotServer::MoveData getMoveData();

  private:

    HardwareSerial& serial_;
    HardwareSerial& debug_;
    bool clientConnected_;
    bool wifiConnected_;
    bool recvInProgress_;
    int recvIdx_;
    String buffer_;
    MoveData moveData_;

    String getAnyIncomingMessage();
    String cleanString(String message);
    COMMAND getCommand(String message);
    
    void sendMsg(String msg);
    void sendAck(String data);
    void sendErr(String data);
    void sendStatus();

};



#endif
