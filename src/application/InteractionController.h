#ifndef BAEKAR_APPLICATION_INTERACTION_CONTROLLER_H
#define BAEKAR_APPLICATION_INTERACTION_CONTROLLER_H

#include "application/ports/IScene.h"
#include "core/Hand.h"
#include "core/Input.h"

namespace baekar {

// Pick-then-drag for AR content, driven by the mouse or by the fingertip
// gesture. Each input runs a two-state machine:
//   Idle --press / gesture starts--> Dragging  (pick; select the hit)
//   Dragging --held--> Dragging                 (drag by the pointer delta)
//   Dragging --release / gesture ends--> Idle
// The 2012 main loop had one copy of this logic per input.
class InteractionController {
public:
    explicit InteractionController(IScene& scene);

    void onPointer(const PointerState& pointer);
    void onHand(const HandState& hand);

    enum class State { Idle, Dragging };
    State mouseState() const { return mouse_.state; }
    State fingerState() const { return finger_.state; }

private:
    struct Channel {
        State state = State::Idle;
        double lastX = 0.0;
        double lastY = 0.0;
    };

    IScene& scene_;
    Channel mouse_;
    Channel finger_;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_INTERACTION_CONTROLLER_H
