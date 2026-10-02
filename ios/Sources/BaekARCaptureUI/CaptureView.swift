// The capture screen: live camera with ARKit tracking, record / stop, and
// what is being written. Recordings appear in the Files app under
// "On My iPhone / BaekAR Capture / Recordings" (the app enables file
// sharing); copy a folder to the Mac and run `BaekAR --replay <folder>`.

#if canImport(ARKit) && os(iOS)
import ARKit
import BaekARRecorder
import SceneKit
import SwiftUI

@MainActor
public final class CaptureModel: ObservableObject {
    @Published public private(set) var status = CaptureStatus()
    @Published public var errorMessage: String?
    public let recorder = CaptureRecorder()

    public init() {
        recorder.onStatus = { [weak self] status in
            Task { @MainActor in self?.status = status }
        }
    }

    public func toggleRecording() {
        if status.isRecording {
            recorder.stopRecording()
        } else {
            do {
                try recorder.startRecording()
            } catch {
                errorMessage = String(describing: error)
            }
        }
    }
}

/// ARKit's camera view with feature points, driven by the recorder's session.
struct CameraPreview: UIViewRepresentable {
    let session: ARSession

    func makeUIView(context: Context) -> ARSCNView {
        let view = ARSCNView(frame: .zero)
        view.session = session
        view.scene = SCNScene()
        view.debugOptions = [.showFeaturePoints, .showWorldOrigin]
        view.automaticallyUpdatesLighting = false
        return view
    }

    func updateUIView(_ uiView: ARSCNView, context: Context) {}
}

public struct CaptureView: View {
    @StateObject private var model = CaptureModel()
    @Environment(\.scenePhase) private var scenePhase

    public init() {}

    public var body: some View {
        ZStack(alignment: .bottom) {
            CameraPreview(session: model.recorder.session)
                .ignoresSafeArea()
            VStack(spacing: 12) {
                statusPanel
                Button(action: model.toggleRecording) {
                    Text(model.status.isRecording ? "Stop" : "Record")
                        .font(.title2.bold())
                        .frame(maxWidth: .infinity)
                        .padding()
                        .background(model.status.isRecording ? Color.red : Color.accentColor)
                        .foregroundColor(.white)
                        .clipShape(RoundedRectangle(cornerRadius: 14))
                }
                .accessibilityIdentifier("record")
            }
            .padding()
        }
        .onAppear { model.recorder.startSession() }
        .onChange(of: scenePhase) { _, phase in
            if phase == .background {
                model.recorder.stopRecording()
                model.recorder.pauseSession()
            } else if phase == .active {
                model.recorder.startSession()
            }
        }
        .alert("Cannot record", isPresented: Binding(get: { model.errorMessage != nil },
                                                     set: { if !$0 { model.errorMessage = nil } })) {
            Button("OK", role: .cancel) {}
        } message: {
            Text(model.errorMessage ?? "")
        }
    }

    private var statusPanel: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("Tracking: \(model.status.tracking)")
            Text("LiDAR depth: \(model.status.hasDepth ? "yes" : (CaptureRecorder.supportsDepth ? "waiting" : "not on this device"))")
            if model.status.isRecording || model.status.framesRecorded > 0 {
                Text("Frames: \(model.status.framesRecorded)  dropped: \(model.status.framesDropped)  IMU: \(model.status.imuSamples)")
            }
            if let directory = model.status.directory {
                Text(directory.lastPathComponent).font(.caption.monospaced())
            }
        }
        .font(.callout.monospacedDigit())
        .foregroundColor(.white)
        .padding(10)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color.black.opacity(0.55))
        .clipShape(RoundedRectangle(cornerRadius: 10))
    }
}
#endif
