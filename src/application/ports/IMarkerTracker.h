#ifndef BAEKAR_APPLICATION_PORTS_IMARKER_TRACKER_H
#define BAEKAR_APPLICATION_PORTS_IMARKER_TRACKER_H

#include "core/Frame.h"
#include "core/Tracking.h"

#include <string>
#include <vector>

namespace baekar {

// Strategy for finding and following marker images. Implementations run
// their own worker threads; the application thread only submits frames
// and reads the latest published snapshot.
class IMarkerTracker {
public:
    virtual ~IMarkerTracker() = default;

    // Builds the marker database. `firstFrame` primes the workers.
    virtual bool start(const std::vector<std::string>& markerImages, const Frame& firstFrame) = 0;
    // Non-blocking. Frames with an already-seen sequence are ignored.
    virtual void submit(const Frame& frame) = 0;
    // One observation per marker, all from the same published snapshot.
    virtual std::vector<MarkerObservation> latest() const = 0;
    // Stops and joins the workers.
    virtual void stop() = 0;
    // True when observations should drive the 3D pose (the 2012 single-
    // marker pipeline). The multi-marker tracker draws outlines only.
    virtual bool drivesPose() const = 0;
    virtual std::string describe() const = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IMARKER_TRACKER_H
