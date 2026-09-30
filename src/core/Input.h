#ifndef BAEKAR_CORE_INPUT_H
#define BAEKAR_CORE_INPUT_H

namespace baekar {

// Mouse state sampled once per frame, in window pixels (origin top-left).
struct PointerState {
    double x = 0.0;
    double y = 0.0;
    bool leftDown = false;
};

}  // namespace baekar

#endif  // BAEKAR_CORE_INPUT_H
