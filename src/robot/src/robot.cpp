#include "robot.h"

#include <plog/Log.h> 

#include <iostream>


Robot::Robot()
: statusUpdater_(),
  server_(statusUpdater_),
  controller_(statusUpdater_),
  tray_controller_(),
  mm_wrapper_(),
  position_time_averager_(20),
  curCmd_(COMMAND::NONE)
{
    PLOGI.printf("Robot starting");
}

void Robot::run()
{
    while(true)
    {
        runOnce();
    }
}


void Robot::runOnce() 
{
    // Check for new command and try to start it
    COMMAND newCmd = server_.oneLoop();
    bool status = tryStartNewCmd(newCmd);

    // Update our current command if we successfully started a new command
    if(status)
    {
        curCmd_ = newCmd;
        statusUpdater_.updateInProgress(true);
    }

    // Service marvelmind
    std::vector<float> positions = mm_wrapper_.getPositions();
    if (positions.size() == 3)
    {
        position_time_averager_.mark_point();
        controller_.inputPosition(positions[0], positions[1], positions[2]);
    }

    // Service controllers
    controller_.update();
    tray_controller_.update();

    // Check if the current command has finished
    bool done = checkForCmdComplete(curCmd_);
    if(done)
    {
        curCmd_ = COMMAND::NONE;
        statusUpdater_.updateInProgress(false);
    }

    // Update loop time and status updater
    statusUpdater_.updatePositionLoopTime(position_time_averager_.get_ms());
}


bool Robot::tryStartNewCmd(COMMAND cmd)
{
    // Position info doesn't count as a real 'command' since it doesn't interrupt anything
    // Always service it, but don't consider it starting a new command
    if (cmd == COMMAND::POSITION)
    {
        RobotServer::PositionData data = server_.getPositionData();
        controller_.inputPosition(data.x, data.y, data.a);

        // Update the position rate
        position_time_averager_.mark_point();

        return false;
    }
    // Same with ESTOP
    if (cmd == COMMAND::ESTOP)
    {
        controller_.estop();
        tray_controller_.estop();
        return false;
    }
    // Same with LOAD_COMPLETE
    if (cmd == COMMAND::LOAD_COMPLETE)
    {
        tray_controller_.setLoadComplete();
        return false;
    }

    // Just do nothing for NONE
    if (cmd == COMMAND::NONE) { return false;}
    
    // For all other commands, we need to make sure we aren't doing anything else at the moment
    if(statusUpdater_.getInProgress())
    {
        PLOGW << "Command " << static_cast<int>(curCmd_) << " already running, rejecting new command: " << static_cast<int>(cmd);
        return false;
    }
    else if (statusUpdater_.getErrorStatus())
    {
        return false;
    }
    
    // Start new command
    if(cmd == COMMAND::MOVE)
    {
        RobotServer::PositionData data = server_.getMoveData();
        controller_.moveToPosition(data.x, data.y, data.a);
    }
    else if(cmd == COMMAND::MOVE_REL)
    {
        RobotServer::PositionData data = server_.getMoveData();
        controller_.moveToPositionRelative(data.x, data.y, data.a);
    }
    else if(cmd == COMMAND::MOVE_FINE)
    {
        RobotServer::PositionData data = server_.getMoveData();
        controller_.moveToPositionFine(data.x, data.y, data.a);
    }
    else if(cmd == COMMAND::MOVE_CONST_VEL)
    {
        RobotServer::VelocityData data = server_.getVelocityData();
        controller_.moveConstVel(data.vx, data.vy, data.va, data.t);
    }
    else if(cmd == COMMAND::PLACE_TRAY)
    {
        tray_controller_.place();
    }
    else if(cmd == COMMAND::LOAD_TRAY)
    {
        tray_controller_.load();
    }
    else if(cmd == COMMAND::INITIALIZE_TRAY)
    {
        tray_controller_.initialize();
    }
    else
    {
        PLOGW.printf("Unknown command!");
        return false;
    }

    return true;
}

bool Robot::checkForCmdComplete(COMMAND cmd)
{
    if (cmd == COMMAND::NONE)
    {
        return true;
    }
    else if(cmd == COMMAND::MOVE || 
            cmd == COMMAND::MOVE_REL ||
            cmd == COMMAND::MOVE_FINE ||
            cmd == COMMAND::MOVE_CONST_VEL)
    {
        return !controller_.isTrajectoryRunning();;
    }
    else if(cmd == COMMAND::PLACE_TRAY ||
            cmd == COMMAND::LOAD_TRAY ||
            cmd == COMMAND::INITIALIZE_TRAY)
    {
        return !tray_controller_.isActionRunning();
    }
    else
    {
        PLOGI.printf("Completion check not implimented for command: %i",cmd);
        return true;
    }
        
}
