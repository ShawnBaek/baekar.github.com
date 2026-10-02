// Writes a BaekAR dataset folder (docs/dataset-format.md, version 1) that the
// engine replays with `--replay DIR` and `baekar_eval` scores.

import Foundation

/// An already-encoded colour image (JPEG or PNG bytes).
public struct EncodedImage {
    public var data: Data
    public var fileExtension: String  // "jpg" or "png"

    public init(data: Data, fileExtension: String) {
        self.data = data
        self.fileExtension = fileExtension
    }
}

/// Depth in millimetres, row-major, 0 = unknown.
public struct DepthMillimetres {
    public var width: Int
    public var height: Int
    public var values: [UInt16]

    public init(width: Int, height: Int, values: [UInt16]) {
        precondition(values.count == width * height)
        self.width = width
        self.height = height
        self.values = values
    }

    /// From depth in metres (ARKit `sceneDepth`); non-finite or non-positive
    /// values become 0.
    public init(width: Int, height: Int, metres: [Float]) {
        precondition(metres.count == width * height)
        self.init(width: width, height: height, values: metres.map { m in
            guard m.isFinite, m > 0 else { return 0 }
            return UInt16(min(Float(UInt16.max), (m * 1000).rounded()))
        })
    }
}

public struct DatasetFrame {
    public var timestamp: Double  // seconds, device clock
    public var rgb: EncodedImage
    public var intrinsics: Intrinsics  // of `rgb`
    public var depth: DepthMillimetres?
    public var pose: Pose?  // world_from_camera, OpenCV camera axes

    public init(timestamp: Double, rgb: EncodedImage, intrinsics: Intrinsics,
                depth: DepthMillimetres? = nil, pose: Pose? = nil) {
        self.timestamp = timestamp
        self.rgb = rgb
        self.intrinsics = intrinsics
        self.depth = depth
        self.pose = pose
    }
}

public struct ImuSample {
    public var timestamp: Double  // seconds, same clock as frames
    public var gyro: (x: Double, y: Double, z: Double)  // rad/s, device frame
    public var accel: (x: Double, y: Double, z: Double)  // m/s^2, device frame

    public init(timestamp: Double, gyro: (Double, Double, Double), accel: (Double, Double, Double)) {
        self.timestamp = timestamp
        self.gyro = gyro
        self.accel = accel
    }
}

public enum DatasetWriterError: Error, CustomStringConvertible {
    case cannotCreate(String)
    case finished
    case nonMonotonicTimestamp(Double)

    public var description: String {
        switch self {
        case .cannotCreate(let path): return "cannot create \(path)"
        case .finished: return "the dataset is already finished"
        case .nonMonotonicTimestamp(let t): return "timestamp \(t) is not after the previous frame"
        }
    }
}

/// Not thread-safe: call from one serial queue.
public final class DatasetWriter {
    public let directory: URL
    public let device: String
    public let poseSource: String
    public private(set) var framesWritten = 0
    public private(set) var imuSamplesWritten = 0

    private var frames: FileHandle
    private var imu: FileHandle?
    private var intrinsics: Intrinsics?
    private var lastTimestamp = -Double.infinity
    private var finished = false

    /// - Parameters:
    ///   - device: free text, e.g. "iPhone17,1, wide camera".
    ///   - poseSource: where poses come from ("arkit", "groundtruth", "none").
    public init(directory: URL, device: String, poseSource: String) throws {
        self.directory = directory
        self.device = device
        self.poseSource = poseSource
        let fm = FileManager.default
        for sub in ["rgb", "depth"] {
            let url = directory.appendingPathComponent(sub, isDirectory: true)
            do {
                try fm.createDirectory(at: url, withIntermediateDirectories: true)
            } catch {
                throw DatasetWriterError.cannotCreate(url.path)
            }
        }
        frames = try DatasetWriter.createFile(directory.appendingPathComponent("frames.csv"),
                                              header: "timestamp,rgb,depth,tx,ty,tz,qx,qy,qz,qw\n")
    }

    deinit {
        try? finish()
    }

    public func append(_ frame: DatasetFrame) throws {
        guard !finished else { throw DatasetWriterError.finished }
        guard frame.timestamp > lastTimestamp else { throw DatasetWriterError.nonMonotonicTimestamp(frame.timestamp) }
        if intrinsics == nil {
            intrinsics = frame.intrinsics
            try writeMetadata()  // early, so an interrupted recording still replays
        }
        let index = String(framesWritten)
        let name = String(repeating: "0", count: max(0, 6 - index.count)) + index
        let rgbPath = "rgb/\(name).\(frame.rgb.fileExtension)"
        try frame.rgb.data.write(to: directory.appendingPathComponent(rgbPath))
        var depthPath = ""
        if let depth = frame.depth {
            depthPath = "depth/\(name).png"
            try PNGEncoder.encodeGray16(width: depth.width, height: depth.height, pixels: depth.values)
                .write(to: directory.appendingPathComponent(depthPath))
        }
        var row = DatasetWriter.number(frame.timestamp) + ",\(rgbPath),\(depthPath)"
        if let pose = frame.pose {
            let q = pose.quaternion
            for v in pose.translation + [q.x, q.y, q.z, q.w] { row += "," + DatasetWriter.number(v) }
        } else {
            row += ",,,,,,,"
        }
        frames.write(Data((row + "\n").utf8))
        framesWritten += 1
        lastTimestamp = frame.timestamp
    }

    public func append(imu samples: [ImuSample]) throws {
        guard !finished else { throw DatasetWriterError.finished }
        if imu == nil {
            imu = try DatasetWriter.createFile(directory.appendingPathComponent("imu.csv"),
                                               header: "timestamp,gx,gy,gz,ax,ay,az\n")
        }
        var text = ""
        for s in samples {
            text += [s.timestamp, s.gyro.x, s.gyro.y, s.gyro.z, s.accel.x, s.accel.y, s.accel.z]
                .map(DatasetWriter.number).joined(separator: ",") + "\n"
        }
        imu?.write(Data(text.utf8))
        imuSamplesWritten += samples.count
    }

    /// Writes the metadata and closes the files. Further appends throw.
    public func finish() throws {
        guard !finished else { return }
        finished = true
        try frames.close()
        try imu?.close()
        try writeMetadata()
    }

    private func writeMetadata() throws {
        var json = "{\n  \"format\": \"baekar-dataset\",\n  \"version\": 1,\n"
        json += "  \"device\": \(DatasetWriter.quoted(device)),\n"
        if let k = intrinsics {
            json += "  \"intrinsics\": { \"fx\": \(DatasetWriter.number(k.fx)), \"fy\": \(DatasetWriter.number(k.fy)), "
            json += "\"cx\": \(DatasetWriter.number(k.cx)), \"cy\": \(DatasetWriter.number(k.cy)),\n"
            json += "                  \"width\": \(k.width), \"height\": \(k.height),\n"
            json += "                  \"distortion\": [" + k.distortion.map(DatasetWriter.number).joined(separator: ", ") + "] },\n"
        }
        json += "  \"depth_scale\": 1000.0,\n"
        json += "  \"pose_source\": \(DatasetWriter.quoted(poseSource))\n}\n"
        try Data(json.utf8).write(to: directory.appendingPathComponent("baekar_dataset.json"), options: .atomic)
    }

    private static func createFile(_ url: URL, header: String) throws -> FileHandle {
        guard FileManager.default.createFile(atPath: url.path, contents: Data(header.utf8)),
              let handle = try? FileHandle(forWritingTo: url) else {
            throw DatasetWriterError.cannotCreate(url.path)
        }
        handle.seekToEndOfFile()
        return handle
    }

    /// Fixed-point with 9 decimals: exact enough for nanosecond timestamps
    /// and never in exponent form, which the C++ CSV reader would accept but
    /// a spreadsheet might not.
    static func number(_ value: Double) -> String {
        String(format: "%.9f", value)
    }

    static func quoted(_ text: String) -> String {
        var escaped = ""
        for c in text.unicodeScalars {
            switch c {
            case "\"": escaped += "\\\""
            case "\\": escaped += "\\\\"
            case "\n": escaped += "\\n"
            default:
                if c.value < 0x20 { escaped += String(format: "\\u%04x", c.value) } else { escaped.unicodeScalars.append(c) }
            }
        }
        return "\"\(escaped)\""
    }
}
