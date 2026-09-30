// InteractionController: pick-then-drag for the mouse and the fingertip
// gesture, against a fake scene.

#include "application/InteractionController.h"

#include <cstdio>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

class FakeScene final : public baekar::IScene {
public:
    int hitIndex = -1;  // what pick() returns
    int picks = 0;
    int selected = -2;
    int deselects = 0;
    double draggedX = 0, draggedY = 0;

    void setWindowTexture(const cv::Mat&) override {}
    void render(const baekar::Mat4&, const baekar::Mat4&) override {}
    int pick(double, double) const override {
        ++const_cast<FakeScene*>(this)->picks;
        return hitIndex;
    }
    void select(int index) override { selected = index; }
    void deselectAll() override {
        selected = -1;
        ++deselects;
    }
    void dragSelected(double dx, double dy) override {
        draggedX += dx;
        draggedY += dy;
    }
};

baekar::PointerState mouse(double x, double y, bool down) {
    baekar::PointerState p;
    p.x = x;
    p.y = y;
    p.leftDown = down;
    return p;
}

baekar::HandState hand(float x, float y, bool valid) {
    baekar::HandState h;
    h.validPose = valid;
    h.indexFingertip = cv::Point2f(x, y);
    return h;
}

}  // namespace

int main() {
    using State = baekar::InteractionController::State;
    {
        FakeScene scene;
        scene.hitIndex = 0;
        baekar::InteractionController controller(scene);
        controller.onPointer(mouse(100, 100, false));
        expect(scene.picks == 0 && controller.mouseState() == State::Idle, "hover does not pick");
        controller.onPointer(mouse(100, 100, true));
        expect(scene.picks == 1 && scene.selected == 0, "press picks and selects the hit");
        expect(controller.mouseState() == State::Dragging, "press starts dragging");
        controller.onPointer(mouse(110, 95, true));
        controller.onPointer(mouse(120, 90, true));
        expect(scene.picks == 1, "holding does not pick again");
        expect(scene.draggedX == 20 && scene.draggedY == -10, "drag follows the pointer delta");
        controller.onPointer(mouse(120, 90, false));
        expect(controller.mouseState() == State::Idle, "release ends dragging");
    }
    {
        FakeScene scene;
        scene.hitIndex = -1;
        baekar::InteractionController controller(scene);
        controller.onPointer(mouse(5, 5, true));
        expect(scene.deselects == 1, "a miss deselects everything");
    }
    {
        FakeScene scene;
        scene.hitIndex = 2;
        baekar::InteractionController controller(scene);
        controller.onHand(hand(50, 60, false));
        expect(scene.picks == 0, "no gesture, no pick");
        controller.onHand(hand(50, 60, true));
        expect(scene.picks == 1 && scene.selected == 2, "gesture start picks");
        controller.onHand(hand(58, 64, true));
        expect(scene.draggedX == 8 && scene.draggedY == 4, "gesture drags");
        controller.onHand(hand(0, 0, false));
        expect(controller.fingerState() == State::Idle, "gesture end releases");
        controller.onHand(hand(10, 10, true));
        expect(scene.picks == 2, "a new gesture picks again");
        expect(scene.deselects == 0, "a fingertip miss does not deselect (2012 behavior)");
    }

    if (failures == 0) std::fprintf(stderr, "interaction_controller_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
