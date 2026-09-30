#include "application/InteractionController.h"

#include <cstdio>

namespace baekar {

InteractionController::InteractionController(IScene& scene) : scene_(scene) {}

void InteractionController::onPointer(const PointerState& pointer) {
    if (pointer.leftDown && mouse_.state == State::Idle) {
        // Mouse-down edge: pick the AR content under the cursor.
        const int hit = scene_.pick(pointer.x, pointer.y);
        if (hit >= 0) {
            scene_.select(hit);
            std::fprintf(stderr, "BaekAR: picked AR content #%d\n", hit);
        } else {
            scene_.deselectAll();
        }
        mouse_.state = State::Dragging;
    } else if (pointer.leftDown) {
        scene_.dragSelected(pointer.x - mouse_.lastX, pointer.y - mouse_.lastY);
    } else {
        mouse_.state = State::Idle;
    }
    mouse_.lastX = pointer.x;
    mouse_.lastY = pointer.y;
}

void InteractionController::onHand(const HandState& hand) {
    if (!hand.validPose) {
        finger_.state = State::Idle;  // gesture ended: release drag state
        return;
    }
    const double x = hand.indexFingertip.x;
    const double y = hand.indexFingertip.y;
    if (finger_.state == State::Idle) {
        // Thesis interaction model: the five-finger gesture is point/click.
        const int hit = scene_.pick(x, y);
        if (hit >= 0) {
            scene_.select(hit);
            std::fprintf(stderr, "BaekAR: fingertip picked AR content #%d at (%g,%g)\n", hit, x, y);
        }
        finger_.state = State::Dragging;
    } else {
        scene_.dragSelected(x - finger_.lastX, y - finger_.lastY);
    }
    finger_.lastX = x;
    finger_.lastY = y;
}

}  // namespace baekar
