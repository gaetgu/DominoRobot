#ifndef CameraTrackerBase_h
#define CameraTrackerBase_h

#include "utils.h"

struct CameraTrackerOutput
{
  Point pose;
  bool ok;
};

class CameraTrackerBase
{
  public:

    virtual void start() = 0;

    virtual void stop() = 0;

    virtual CameraTrackerOutput getPoseFromCamera() = 0; 

    virtual int getLoopTimeMs() = 0;
};


#endif //CameraTrackerBase_h