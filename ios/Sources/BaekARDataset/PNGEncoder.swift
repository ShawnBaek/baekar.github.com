// Minimal PNG writer for depth maps and test images, in plain Swift so the
// dataset writer builds and is tested on Linux. It stores pixels without
// compression (deflate "stored" blocks); depth maps are small (LiDAR depth is
// 256x192), and colour frames use JPEG through ImageIO on the device.

import Foundation

public enum PNGEncoder {
    /// 16-bit greyscale, for depth in millimetres.
    public static func encodeGray16(width: Int, height: Int, pixels: [UInt16]) -> Data {
        precondition(pixels.count == width * height)
        var raw = [UInt8]()
        raw.reserveCapacity(height * (1 + 2 * width))
        for y in 0..<height {
            raw.append(0)  // filter: none
            for x in 0..<width {
                let v = pixels[y * width + x]
                raw.append(UInt8(v >> 8))
                raw.append(UInt8(v & 0xff))
            }
        }
        return encode(width: width, height: height, bitDepth: 16, colourType: 0, scanlines: raw)
    }

    /// 8-bit RGB, rows top to bottom, `pixels` = r, g, b, r, g, b, ...
    public static func encodeRGB8(width: Int, height: Int, pixels: [UInt8]) -> Data {
        precondition(pixels.count == width * height * 3)
        var raw = [UInt8]()
        raw.reserveCapacity(height * (1 + 3 * width))
        for y in 0..<height {
            raw.append(0)
            raw.append(contentsOf: pixels[(y * width * 3)..<((y + 1) * width * 3)])
        }
        return encode(width: width, height: height, bitDepth: 8, colourType: 2, scanlines: raw)
    }

    static func encode(width: Int, height: Int, bitDepth: UInt8, colourType: UInt8, scanlines: [UInt8]) -> Data {
        var png = Data([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A])
        var header = [UInt8]()
        header.append(contentsOf: bigEndian(UInt32(width)))
        header.append(contentsOf: bigEndian(UInt32(height)))
        header.append(contentsOf: [bitDepth, colourType, 0, 0, 0])
        appendChunk(&png, type: "IHDR", data: header)
        appendChunk(&png, type: "IDAT", data: zlibStored(scanlines))
        appendChunk(&png, type: "IEND", data: [])
        return png
    }

    static func zlibStored(_ bytes: [UInt8]) -> [UInt8] {
        var out: [UInt8] = [0x78, 0x01]
        var offset = 0
        repeat {
            let length = min(65535, bytes.count - offset)
            let final = offset + length == bytes.count
            out.append(final ? 1 : 0)
            out.append(UInt8(length & 0xff)); out.append(UInt8(length >> 8))
            let complement = ~UInt16(length)
            out.append(UInt8(complement & 0xff)); out.append(UInt8(complement >> 8))
            out.append(contentsOf: bytes[offset..<(offset + length)])
            offset += length
        } while offset < bytes.count
        out.append(contentsOf: bigEndian(adler32(bytes)))
        return out
    }

    static func appendChunk(_ png: inout Data, type: String, data: [UInt8]) {
        let typeBytes = Array(type.utf8)
        png.append(contentsOf: bigEndian(UInt32(data.count)))
        png.append(contentsOf: typeBytes)
        png.append(contentsOf: data)
        png.append(contentsOf: bigEndian(crc32(typeBytes + data)))
    }

    static func bigEndian(_ v: UInt32) -> [UInt8] {
        [UInt8(v >> 24), UInt8((v >> 16) & 0xff), UInt8((v >> 8) & 0xff), UInt8(v & 0xff)]
    }

    static func adler32(_ bytes: [UInt8]) -> UInt32 {
        var a: UInt32 = 1, b: UInt32 = 0
        for byte in bytes {
            a = (a + UInt32(byte)) % 65521
            b = (b + a) % 65521
        }
        return (b << 16) | a
    }

    private static let crcTable: [UInt32] = (0..<256).map { n -> UInt32 in
        var c = UInt32(n)
        for _ in 0..<8 { c = (c & 1) != 0 ? 0xEDB88320 ^ (c >> 1) : c >> 1 }
        return c
    }

    static func crc32(_ bytes: [UInt8]) -> UInt32 {
        var c: UInt32 = 0xFFFFFFFF
        for byte in bytes { c = crcTable[Int((c ^ UInt32(byte)) & 0xff)] ^ (c >> 8) }
        return c ^ 0xFFFFFFFF
    }
}
