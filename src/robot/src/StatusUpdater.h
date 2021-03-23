
#ifndef StatusUpdater_h
#define StatusUpdater_h

#include <ArduinoJson/ArduinoJson.h>
#include "Localization.h"


class StatusUpdater
{
  public:
    StatusUpdater();

    std::string getStatusJsonString();

    void updatePosition(float x, float y, float a);

    void updateVelocity(float vx, float vy, float va);

    void updateControlLoopTime(int controller_loop_ms);

    void updatePositionLoopTime(int position_loop_ms);

    void updateDistanceLoopTime(int distance_loop_ms);

    void updateInProgress(bool in_progress);

    bool getInProgress() const { return currentStatus_.in_progress; };
    
    void setErrorStatus() { currentStatus_.error_status = true;}

    void clearErrorStatus() {currentStatus_.error_status = false;};

    bool getErrorStatus() const {return currentStatus_.error_status;}

    void updateLocalizationMetrics(LocalizationMetrics localization_metrics);

    float getLocalizationConfidence() const {return currentStatus_.localization_metrics.confidence;};

    void update_motor_driver_connected(bool connected);

    void update_lifter_driver_connected(bool connected);

    struct Status
    {
      // Current position and velocity
      float pos_x;
      float pos_y;
      float pos_a;
      float vel_x;
      float vel_y;
      float vel_a;

      // Loop times
      int controller_loop_ms;
      int position_loop_ms;
      int distance_loop_ms;

      bool in_progress;
      bool error_status;
      uint8_t counter; // Just to show that the status is updating. Okay to roll over

      bool motor_driver_connected;
      bool lifter_driver_connected;

      LocalizationMetrics localization_metrics;

      //When adding extra fields, update toJsonString method to serialize and add additional capacity

      Status():
      pos_x(0.0),
      pos_y(0.0),
      pos_a(0.0),
      vel_x(0.0),
      vel_y(0.0),
      vel_a(0.0),
      controller_loop_ms(999),
      position_loop_ms(999),
      distance_loop_ms(999),
      in_progress(false),
      error_status(false),
      counter(0),
      motor_driver_connected(false),
      lifter_driver_connected(false),
      localization_metrics()
      {
      }

      std::string toJsonString()
      {
        // Size the object correctly
        const size_t capacity = JSON_OBJECT_SIZE(25); // Update when adding new fields
        DynamicJsonDocument root(capacity);

        // Format to match messages sent by server
        root["type"] = "status";
        JsonObject doc = root.createNestedObject("data");

        // Fill in data
        doc["pos_x"] = pos_x;
        doc["pos_y"] = pos_y;
        doc["pos_a"] = pos_a;
        doc["vel_x"] = vel_x;
        doc["vel_y"] = vel_y;
        doc["vel_a"] = vel_a;
        doc["controller_loop_ms"] = controller_loop_ms;
        doc["position_loop_ms"] = position_loop_ms;
        doc["distance_loop_ms"] = distance_loop_ms;
        doc["in_progress"] = in_progress;
        doc["error_status"] = error_status;
        doc["counter"] = counter++;
        doc["motor_driver_connected"] = motor_driver_connected;
        doc["lifter_driver_connected"] = lifter_driver_connected;
        doc["localization_confidence"] = localization_metrics.confidence;
        doc["localization_last_reading_reliability"] = localization_metrics.last_reading_reliability;
        doc["localization_last_reading_update_fraction"] = localization_metrics.last_reading_update_fraction;
        doc["localization_seconds_since_last_valid_reading"] = localization_metrics.seconds_since_last_valid_reading;
        doc["localization_rolling_reading_filter_fraction"] = localization_metrics.rolling_reading_filter_fraction;

        // Serialize and return string
        std::string msg;
        serializeJson(root, msg);
        return msg;
      }
    };

    Status getStatus() { return currentStatus_;};

  private:
    Status currentStatus_;

};

#endif
