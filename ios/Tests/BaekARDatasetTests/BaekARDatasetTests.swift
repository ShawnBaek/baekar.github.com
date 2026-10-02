import Foundation
import XCTest

@testable import BaekARDataset

final class GeometryTests: XCTestCase {
    func testIdentityQuaternion() {
        let q = Pose().quaternion
        XCTAssertEqual(q.w, 1, accuracy: 1e-12)
        XCTAssertEqual(abs(q.x) + abs(q.y) + abs(q.z), 0, accuracy: 1e-12)
    }

    func testQuaternionOfQuarterTurnAboutZ() {
        // x -> y: rotation by +90 degrees about z.
        let q = Pose(rotation: [0, -1, 0, 1, 0, 0, 0, 0, 1]).quaternion
        XCTAssertEqual(q.z, 0.5.squareRoot(), accuracy: 1e-12)
        XCTAssertEqual(q.w, 0.5.squareRoot(), accuracy: 1e-12)
    }

    func testQuaternionOfHalfTurnUsesLargestDiagonal() {
        let q = Pose(rotation: [1, 0, 0, 0, -1, 0, 0, 0, -1]).quaternion  // 180 degrees about x
        XCTAssertEqual(abs(q.x), 1, accuracy: 1e-12)
        XCTAssertEqual(q.w, 0, accuracy: 1e-12)
    }

    func testARKitCameraAxesBecomeOpenCVAxes() {
        // ARKit camera at the world origin looking down -z (identity
        // transform), moved 2 m along x.
        var m: [Float] = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 0, 0, 1]
        var pose = Pose.fromARKit(columnMajor: m)
        XCTAssertEqual(pose.rotation, [1, 0, 0, 0, -1, 0, 0, 0, -1])
        XCTAssertEqual(pose.translation, [2, 0, 0])
        // The OpenCV optical axis (+z) points along ARKit's -z.
        XCTAssertEqual(pose.rotation[8], -1)

        // A camera turned 90 degrees left (about world +y): ARKit looks down
        // world -x, so the OpenCV z axis must be world -x.
        m = [0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1]
        pose = Pose.fromARKit(columnMajor: m)
        XCTAssertEqual([pose.rotation[2], pose.rotation[5], pose.rotation[8]], [-1, 0, 0])
    }

    func testIntrinsicsFromARKitAndScaling() {
        let k = Intrinsics.fromARKit(columnMajor: [1450, 0, 0, 0, 1451, 0, 959.5, 719.5, 1], width: 1920, height: 1440)
        XCTAssertEqual(k, Intrinsics(fx: 1450, fy: 1451, cx: 959.5, cy: 719.5, width: 1920, height: 1440))
        let half = k.scaled(toWidth: 960, height: 720)
        XCTAssertEqual(half.fx, 725)
        XCTAssertEqual(half.cy, 359.75)
    }
}

final class PNGEncoderTests: XCTestCase {
    func testChecksumsMatchKnownValues() {
        XCTAssertEqual(PNGEncoder.crc32(Array("IEND".utf8)), 0xAE42_6082)
        XCTAssertEqual(PNGEncoder.adler32(Array("Wikipedia".utf8)), 0x11E6_0398)
    }

    func testGray16Header() {
        let png = [UInt8](PNGEncoder.encodeGray16(width: 3, height: 2, pixels: [0, 1, 2, 1000, 65535, 7]))
        XCTAssertEqual(Array(png[0..<8]), [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A])
        XCTAssertEqual(Array(png[12..<16]), Array("IHDR".utf8))
        XCTAssertEqual(Array(png[16..<24]), [0, 0, 0, 3, 0, 0, 0, 2])
        XCTAssertEqual(png[24], 16)  // bit depth
        XCTAssertEqual(png[25], 0)  // greyscale
        XCTAssertEqual(Array(png.suffix(8)), [0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82])
    }

    func testLargeImageSplitsIntoStoredBlocks() {
        // 256x192x2 bytes is more than one 65535-byte stored block.
        let pixels = (0..<(256 * 192)).map { UInt16(truncatingIfNeeded: $0 &* 37) }
        let zlib = PNGEncoder.zlibStored([UInt8](repeating: 1, count: 70000))
        XCTAssertEqual(zlib.count, 2 + 5 + 65535 + 5 + (70000 - 65535) + 4)
        XCTAssertGreaterThan(PNGEncoder.encodeGray16(width: 256, height: 192, pixels: pixels).count, 256 * 192 * 2)
    }
}

final class DatasetWriterTests: XCTestCase {
    /// Writes a small dataset. CI sets BAEKAR_SWIFT_DATASET_OUT and then
    /// replays the folder with the C++ reader (baekar_eval dataset).
    func testWritesTheBaekARFormat() throws {
        let fm = FileManager.default
        let directory: URL
        if let out = ProcessInfo.processInfo.environment["BAEKAR_SWIFT_DATASET_OUT"] {
            directory = URL(fileURLWithPath: out, isDirectory: true)
        } else {
            directory = fm.temporaryDirectory.appendingPathComponent("baekar_swift_dataset_\(UUID().uuidString)")
        }
        try? fm.removeItem(at: directory)

        let width = 64, height = 48
        let k = Intrinsics(fx: 60, fy: 60, cx: 31.5, cy: 23.5, width: width, height: height)
        let writer = try DatasetWriter(directory: directory, device: "unit test \"fixture\"", poseSource: "arkit")
        for i in 0..<5 {
            var rgb = [UInt8](repeating: 0, count: width * height * 3)
            for p in 0..<(width * height) { rgb[p * 3] = UInt8((p + i * 10) % 256); rgb[p * 3 + 1] = 128; rgb[p * 3 + 2] = UInt8(i * 40) }
            let image = EncodedImage(data: PNGEncoder.encodeRGB8(width: width, height: height, pixels: rgb), fileExtension: "png")
            let depth = DepthMillimetres(width: 16, height: 12,
                                         metres: [Float](repeating: 0.5 + Float(i) * 0.25, count: 16 * 12))
            let arkit: [Float] = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, Float(i) * 0.1, 0, 0, 1]
            let pose = i == 2 ? nil : Pose.fromARKit(columnMajor: arkit)
            try writer.append(imu: (0..<10).map { s in
                ImuSample(timestamp: 100 + Double(i) / 30 - 0.03 + Double(s) * 0.003, gyro: (0.01, 0.02, 0.03), accel: (0, 9.81, 0))
            })
            try writer.append(DatasetFrame(timestamp: 100 + Double(i) / 30, rgb: image, intrinsics: k, depth: depth, pose: pose))
        }
        XCTAssertThrowsError(try writer.append(DatasetFrame(timestamp: 50, rgb: EncodedImage(data: Data(), fileExtension: "png"), intrinsics: k)))
        try writer.finish()
        XCTAssertEqual(writer.framesWritten, 5)
        XCTAssertEqual(writer.imuSamplesWritten, 50)

        let json = try String(contentsOf: directory.appendingPathComponent("baekar_dataset.json"), encoding: .utf8)
        let parsed = try JSONSerialization.jsonObject(with: Data(json.utf8)) as? [String: Any]
        XCTAssertEqual(parsed?["format"] as? String, "baekar-dataset")
        XCTAssertEqual(parsed?["device"] as? String, "unit test \"fixture\"")
        let intrinsics = parsed?["intrinsics"] as? [String: Any]
        XCTAssertEqual(intrinsics?["width"] as? Int, 64)
        XCTAssertEqual(intrinsics?["fx"] as? Double, 60)

        let rows = try String(contentsOf: directory.appendingPathComponent("frames.csv"), encoding: .utf8)
            .split(separator: "\n").map(String.init)
        XCTAssertEqual(rows.count, 6)
        XCTAssertEqual(rows[0], "timestamp,rgb,depth,tx,ty,tz,qx,qy,qz,qw")
        XCTAssertTrue(rows[1].hasPrefix("100.000000000,rgb/000000.png,depth/000000.png,0.000000000,"))
        XCTAssertTrue(rows[3].hasSuffix("depth/000002.png,,,,,,,"), "frame without a pose leaves the pose empty")
        // ARKit identity -> OpenCV rotation of 180 degrees about x: q = (1, 0, 0, 0).
        XCTAssertTrue(rows[2].hasSuffix(",1.000000000,0.000000000,0.000000000,0.000000000"), rows[2])

        let imuRows = try String(contentsOf: directory.appendingPathComponent("imu.csv"), encoding: .utf8)
            .split(separator: "\n")
        XCTAssertEqual(imuRows.count, 51)
        XCTAssertTrue(fm.fileExists(atPath: directory.appendingPathComponent("depth/000004.png").path))
        if ProcessInfo.processInfo.environment["BAEKAR_SWIFT_DATASET_OUT"] == nil {
            try? fm.removeItem(at: directory)
        }
    }

    func testDepthFromMetres() {
        let depth = DepthMillimetres(width: 4, height: 1, metres: [1.2345, 0, -1, .nan])
        XCTAssertEqual(depth.values, [1235, 0, 0, 0])
    }
}
