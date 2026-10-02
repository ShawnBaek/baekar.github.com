// Camera pose and intrinsics in the conventions of the BaekAR dataset format
// (docs/dataset-format.md): OpenCV camera axes (x right, y down, z forward),
// metres, Hamilton quaternions. Plain Swift (no simd) so it builds on Linux.

import Foundation

/// world_from_camera rigid transform. `rotation` is row-major 3x3.
public struct Pose: Equatable {
    public var rotation: [Double]
    public var translation: [Double]

    public init(rotation: [Double] = [1, 0, 0, 0, 1, 0, 0, 0, 1], translation: [Double] = [0, 0, 0]) {
        precondition(rotation.count == 9 && translation.count == 3)
        self.rotation = rotation
        self.translation = translation
    }

    /// Converts an ARKit `ARCamera.transform` (column-major 4x4,
    /// world_from_camera with camera axes x right, y up, z backward) to the
    /// OpenCV camera convention by flipping the camera's y and z axes.
    /// Both refer to the sensor's native (landscape) image orientation.
    public static func fromARKit(columnMajor m: [Float]) -> Pose {
        precondition(m.count == 16)
        // m[column * 4 + row]
        func r(_ row: Int, _ column: Int) -> Double { Double(m[column * 4 + row]) }
        let rotation = [
            r(0, 0), -r(0, 1), -r(0, 2),
            r(1, 0), -r(1, 1), -r(1, 2),
            r(2, 0), -r(2, 1), -r(2, 2),
        ]
        return Pose(rotation: rotation, translation: [r(0, 3), r(1, 3), r(2, 3)])
    }

    /// Unit quaternion (x, y, z, w) of `rotation` (Shepperd's method).
    public var quaternion: (x: Double, y: Double, z: Double, w: Double) {
        let m = rotation
        let trace = m[0] + m[4] + m[8]
        var q: (x: Double, y: Double, z: Double, w: Double)
        if trace > 0 {
            let s = (trace + 1).squareRoot() * 2
            q = ((m[7] - m[5]) / s, (m[2] - m[6]) / s, (m[3] - m[1]) / s, 0.25 * s)
        } else if m[0] > m[4] && m[0] > m[8] {
            let s = (1 + m[0] - m[4] - m[8]).squareRoot() * 2
            q = (0.25 * s, (m[1] + m[3]) / s, (m[2] + m[6]) / s, (m[7] - m[5]) / s)
        } else if m[4] > m[8] {
            let s = (1 + m[4] - m[0] - m[8]).squareRoot() * 2
            q = ((m[1] + m[3]) / s, 0.25 * s, (m[5] + m[7]) / s, (m[2] - m[6]) / s)
        } else {
            let s = (1 + m[8] - m[0] - m[4]).squareRoot() * 2
            q = ((m[2] + m[6]) / s, (m[5] + m[7]) / s, 0.25 * s, (m[3] - m[1]) / s)
        }
        let norm = (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w).squareRoot()
        let sign: Double = q.w < 0 ? -1 : 1  // keep w >= 0
        return (sign * q.x / norm, sign * q.y / norm, sign * q.z / norm, sign * q.w / norm)
    }
}

/// Pinhole intrinsics of the stored images, OpenCV distortion (k1 k2 p1 p2 k3).
public struct Intrinsics: Equatable {
    public var fx, fy, cx, cy: Double
    public var width, height: Int
    public var distortion: [Double]

    public init(fx: Double, fy: Double, cx: Double, cy: Double, width: Int, height: Int,
                distortion: [Double] = [0, 0, 0, 0, 0]) {
        self.fx = fx; self.fy = fy; self.cx = cx; self.cy = cy
        self.width = width; self.height = height
        self.distortion = distortion
    }

    /// From ARKit's `ARCamera.intrinsics` (column-major 3x3) for an image of
    /// the given size.
    public static func fromARKit(columnMajor k: [Float], width: Int, height: Int) -> Intrinsics {
        precondition(k.count == 9)
        return Intrinsics(fx: Double(k[0]), fy: Double(k[4]), cx: Double(k[6]), cy: Double(k[7]),
                          width: width, height: height)
    }

    /// The same camera with its image resized to `newWidth` x `newHeight`.
    public func scaled(toWidth newWidth: Int, height newHeight: Int) -> Intrinsics {
        let sx = Double(newWidth) / Double(width), sy = Double(newHeight) / Double(height)
        return Intrinsics(fx: fx * sx, fy: fy * sy, cx: cx * sx, cy: cy * sy,
                          width: newWidth, height: newHeight, distortion: distortion)
    }
}
