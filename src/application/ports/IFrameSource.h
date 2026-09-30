#ifndef BAEKAR_APPLICATION_PORTS_IFRAME_SOURCE_H
#define BAEKAR_APPLICATION_PORTS_IFRAME_SOURCE_H

#include "core/Frame.h"

#include <string>

namespace baekar {

// Strategy for where frames come from: a camera, the synthetic marker
// simulator, a recorded folder, or a "camera unavailable" placeholder.
class IFrameSource {
public:
    virtual ~IFrameSource() = default;

    virtual bool open() = 0;
    // Fills `frame` with the latest image. Returns false when no image is
    // available; `frame` then keeps its previous contents.
    virtual bool read(Frame& frame) = 0;
    virtual void close() = 0;
    virtual std::string describe() const = 0;
    // False for sources that produce placeholder frames only.
    virtual bool isLive() const { return true; }
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IFRAME_SOURCE_H
