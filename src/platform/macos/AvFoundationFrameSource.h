#ifndef BAEKAR_PLATFORM_MACOS_AVFOUNDATION_FRAME_SOURCE_H
#define BAEKAR_PLATFORM_MACOS_AVFOUNDATION_FRAME_SOURCE_H

#include "application/ports/IFrameSource.h"

#include <mutex>

struct AVCap;

namespace baekar {

// Camera through AVCaptureDeviceDiscoverySession (built-in, USB and iPhone
// Continuity Camera). Asks for camera permission on open().
class AvFoundationFrameSource final : public IFrameSource {
public:
    explicit AvFoundationFrameSource(int index);
    ~AvFoundationFrameSource() override;

    bool open() override;
    bool read(Frame& frame) override;
    void close() override;
    std::string describe() const override;

    // Hot-swaps the device (menu-bar Camera menu). Main thread only.
    bool switchCamera(int index);

private:
    bool openDevice(int index);

    int index_;
    AVCap* capture_ = nullptr;
    Frame last_;
    std::uint64_t sequence_ = 0;
};

// Menu-bar "Camera" menu listing every AVFoundation device; picking one
// calls switchCamera on the given source.
void installCameraMenu(AvFoundationFrameSource& source, int currentIndex);

// Stdin camera picker (--interactive). Returns -1 to keep the default.
int pickCameraIndex();

}  // namespace baekar

#endif  // BAEKAR_PLATFORM_MACOS_AVFOUNDATION_FRAME_SOURCE_H
