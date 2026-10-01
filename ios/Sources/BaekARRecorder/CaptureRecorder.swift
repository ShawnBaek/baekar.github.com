// Records an ARKit session into a BaekAR dataset: colour (JPEG), LiDAR depth
// (16-bit PNG, mm), camera intrinsics, the ARKit camera pose (converted to
// OpenCV camera axes) and 200 Hz IMU. The engine replays the folder with
// `BaekAR --replay DIR`; `baekar_eval` scores trackers against the poses.

#if canImport(ARKit) && os(iOS)
import ARKit
import BaekARDataset
import CoreImage
import CoreMotion
import Foundation

public struct CaptureStatus: Equatable {
    public var isRecording = false
    public var framesRecorded = 0
    public var framesDropped = 0
    public var imuSamples = 0
    public var hasDepth = false
    public var tracking = "not started"
    public var directory: URL?
}

public final class CaptureRecorder: NSObject, ARSessionDelegate {
    public struct Options {
        /// Record every n-th ARKit frame (ARKit runs at 60 fps).
        public var frameStride = 2
        public var jpegQuality = 0.9
        public var recordDepth = true
        public var imuRate = 200.0

        public init() {}
    }

    public let session = ARSession()
    public var options = Options()
    /// Called on the main queue.
    public var onStatus: ((CaptureStatus) -> Void)?

    private let writerQueue = DispatchQueue(label: "com.baekar.capture.writer", qos: .userInitiated)
    private let motion = CMMotionManager()
    private let motionQueue = OperationQueue()
    private let ciContext = CIContext(options: [.useSoftwareRenderer: false])

    // Main queue.
    private var status = CaptureStatus()
    private var frameCounter = 0
    private var framesInFlight = 0

    // Writer queue.
    private var writer: DatasetWriter?
    private var pendingImu: [ImuSample] = []

    public override init() {
        super.init()
        session.delegate = self
        motionQueue.maxConcurrentOperationCount = 1
    }

    public static var supportsDepth: Bool {
        ARWorldTrackingConfiguration.supportsFrameSemantics(.sceneDepth)
    }

    public func startSession() {
        let configuration = ARWorldTrackingConfiguration()
        if CaptureRecorder.supportsDepth && options.recordDepth {
            configuration.frameSemantics.insert(.sceneDepth)
        }
        session.run(configuration)
    }

    public func pauseSession() {
        session.pause()
    }

    /// Starts writing to Documents/Recordings/<date>. Returns the folder.
    @discardableResult
    public func startRecording() throws -> URL {
        precondition(Thread.isMainThread)
        let documents = try FileManager.default.url(for: .documentDirectory, in: .userDomainMask,
                                                    appropriateFor: nil, create: true)
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        let directory = documents.appendingPathComponent("Recordings", isDirectory: true)
            .appendingPathComponent(formatter.string(from: Date()), isDirectory: true)
        let newWriter = try DatasetWriter(directory: directory, device: CaptureRecorder.deviceName() + ", wide camera",
                                          poseSource: "arkit")
        writerQueue.sync {
            writer = newWriter
            pendingImu.removeAll()
        }
        status = CaptureStatus(isRecording: true, hasDepth: status.hasDepth, tracking: status.tracking,
                               directory: directory)
        frameCounter = 0
        startMotion()
        publish()
        return directory
    }

    public func stopRecording() {
        precondition(Thread.isMainThread)
        guard status.isRecording else { return }
        motion.stopDeviceMotionUpdates()
        status.isRecording = false
        publish()
        writerQueue.async { [weak self] in
            guard let self, let writer = self.writer else { return }
            try? writer.append(imu: self.pendingImu)
            self.pendingImu.removeAll()
            try? writer.finish()
            self.writer = nil
        }
    }

    // MARK: ARSessionDelegate (main queue)

    public func session(_ session: ARSession, didUpdate frame: ARFrame) {
        let tracking = CaptureRecorder.describe(frame.camera.trackingState)
        let hasDepth = frame.sceneDepth != nil
        if tracking != status.tracking || hasDepth != status.hasDepth {
            status.tracking = tracking
            status.hasDepth = hasDepth
            publish()
        }
        guard status.isRecording else { return }
        frameCounter += 1
        guard frameCounter % max(1, options.frameStride) == 0 else { return }
        // Do not hold ARKit's buffers while the writer is behind: ARKit stops
        // delivering frames when too many are retained.
        guard framesInFlight < 2 else {
            status.framesDropped += 1
            return
        }

        let timestamp = frame.timestamp
        let image = frame.capturedImage
        let resolution = frame.camera.imageResolution
        let k = frame.camera.intrinsics
        let intrinsics = Intrinsics.fromARKit(
            columnMajor: [k.columns.0.x, k.columns.0.y, k.columns.0.z,
                          k.columns.1.x, k.columns.1.y, k.columns.1.z,
                          k.columns.2.x, k.columns.2.y, k.columns.2.z],
            width: Int(resolution.width), height: Int(resolution.height))
        let t = frame.camera.transform
        let pose: Pose? = frame.camera.trackingState == .normal ? Pose.fromARKit(columnMajor: [
            t.columns.0.x, t.columns.0.y, t.columns.0.z, t.columns.0.w,
            t.columns.1.x, t.columns.1.y, t.columns.1.z, t.columns.1.w,
            t.columns.2.x, t.columns.2.y, t.columns.2.z, t.columns.2.w,
            t.columns.3.x, t.columns.3.y, t.columns.3.z, t.columns.3.w,
        ]) : nil
        let depthMap = options.recordDepth ? frame.sceneDepth?.depthMap : nil
        let quality = options.jpegQuality

        framesInFlight += 1
        writerQueue.async { [weak self] in
            guard let self else { return }
            var written = false
            if let writer = self.writer, let jpeg = self.jpeg(image, quality: quality) {
                let depth = depthMap.flatMap(CaptureRecorder.depthMillimetres)
                do {
                    try writer.append(imu: self.pendingImu)
                    self.pendingImu.removeAll()
                    try writer.append(DatasetFrame(timestamp: timestamp,
                                                   rgb: EncodedImage(data: jpeg, fileExtension: "jpg"),
                                                   intrinsics: intrinsics, depth: depth, pose: pose))
                    written = true
                } catch {
                    NSLog("BaekAR capture: %@", String(describing: error))
                }
            }
            DispatchQueue.main.async {
                self.framesInFlight -= 1
                if written { self.status.framesRecorded += 1 } else if self.status.isRecording { self.status.framesDropped += 1 }
                self.publish()
            }
        }
    }

    public func session(_ session: ARSession, didFailWithError error: Error) {
        status.tracking = "failed: \(error.localizedDescription)"
        publish()
    }

    // MARK: Private

    private func publish() {
        let snapshot = status
        onStatus?(snapshot)
    }

    private func startMotion() {
        guard motion.isDeviceMotionAvailable else { return }
        motion.deviceMotionUpdateInterval = 1.0 / options.imuRate
        // Same clock as ARFrame.timestamp (seconds since boot).
        motion.startDeviceMotionUpdates(to: motionQueue) { [weak self] data, _ in
            guard let self, let data else { return }
            // Total acceleration (gravity + user) in g, sign as the raw
            // accelerometer; an IMU's specific force is its negative.
            let g0 = 9.80665
            let total = (data.gravity.x + data.userAcceleration.x,
                         data.gravity.y + data.userAcceleration.y,
                         data.gravity.z + data.userAcceleration.z)
            let sample = ImuSample(timestamp: data.timestamp,
                                   gyro: (data.rotationRate.x, data.rotationRate.y, data.rotationRate.z),
                                   accel: (-total.0 * g0, -total.1 * g0, -total.2 * g0))
            self.writerQueue.async {
                guard self.writer != nil else { return }
                self.pendingImu.append(sample)
                DispatchQueue.main.async { self.status.imuSamples += 1 }
            }
        }
    }

    private func jpeg(_ pixelBuffer: CVPixelBuffer, quality: Double) -> Data? {
        let image = CIImage(cvPixelBuffer: pixelBuffer)
        guard let colourSpace = CGColorSpace(name: CGColorSpace.sRGB) else { return nil }
        let key = CIImageRepresentationOption(rawValue: kCGImageDestinationLossyCompressionQuality as String)
        return ciContext.jpegRepresentation(of: image, colorSpace: colourSpace, options: [key: quality])
    }

    private static func depthMillimetres(_ depthMap: CVPixelBuffer) -> DepthMillimetres? {
        guard CVPixelBufferGetPixelFormatType(depthMap) == kCVPixelFormatType_DepthFloat32 else { return nil }
        CVPixelBufferLockBaseAddress(depthMap, .readOnly)
        defer { CVPixelBufferUnlockBaseAddress(depthMap, .readOnly) }
        guard let base = CVPixelBufferGetBaseAddress(depthMap) else { return nil }
        let width = CVPixelBufferGetWidth(depthMap), height = CVPixelBufferGetHeight(depthMap)
        let rowBytes = CVPixelBufferGetBytesPerRow(depthMap)
        var metres = [Float](repeating: 0, count: width * height)
        for y in 0..<height {
            let row = base.advanced(by: y * rowBytes).assumingMemoryBound(to: Float.self)
            for x in 0..<width { metres[y * width + x] = row[x] }
        }
        return DepthMillimetres(width: width, height: height, metres: metres)
    }

    private static func describe(_ state: ARCamera.TrackingState) -> String {
        switch state {
        case .normal: return "normal"
        case .notAvailable: return "not available"
        case .limited(let reason):
            switch reason {
            case .initializing: return "limited: initializing"
            case .excessiveMotion: return "limited: excessive motion"
            case .insufficientFeatures: return "limited: insufficient features"
            case .relocalizing: return "limited: relocalizing"
            @unknown default: return "limited"
            }
        }
    }

    static func deviceName() -> String {
        var info = utsname()
        uname(&info)
        return withUnsafeBytes(of: &info.machine) { buffer in
            String(decoding: buffer.prefix(while: { $0 != 0 }), as: UTF8.self)
        }
    }
}
#endif
