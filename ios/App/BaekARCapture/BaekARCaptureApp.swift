// iPhone capture app (roadmap F3). The screen and the recorder live in the
// Swift package (ios/Sources); this target only hosts them.

import BaekARCaptureUI
import SwiftUI

@main
struct BaekARCaptureApp: App {
    var body: some Scene {
        WindowGroup {
            CaptureView()
        }
    }
}
