#ifndef DistanceTracker_h
#define DistanceTracker_h

#include "utils.h"
#include "serial/SerialComms.h"
#include "DistanceTrackerBase.h"

class DistanceTracker : public DistanceTrackerBase
{

  public:
    DistanceTracker();

    // Start measurement loop
    void start() override;

    // Stop measurement loop
    void stop() override;

    // Main 'update' function that must be called regularly
    void checkForMeasurement() override;

    // Get latest distance values in meters
    Point getDistancePose() override {return current_distance_pose_;};

    // Returns bool indicating if distance measurements are running
    bool isRunning() override { return running_;};

    // How long the time delay between measurements is
    int getAverageMeasurementTimeMs() override {return measurement_time_averager_.get_ms();};

  private:

    // Handles getting measurements from serial port and parsing into number
    std::vector<float> getMeasurement();

    // Computes the current pose from the buffered distances
    void computePoseFromDistances();

    Point current_distance_pose_;
    std::vector<CircularBuffer<float>> distance_buffers_;
    bool running_;
    TimeRunningAverage measurement_time_averager_;
    SerialCommsBase* serial_to_arduino_; 

    // Various constant parameters
    int fwd_left_id_;
    int fwd_right_id_;
    int angled_left_id_;
    int angled_right_id_;
    float angle_from_fwd_radians_;
    float left_fwd_offset_;                   
    float right_fwd_offset_;
    float left_angle_offset_;
    float right_angle_offset_;
    uint num_sensors_;

};


#endif //DistanceTracker_h