#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "engine/render_metal/Renderer.h"
#include <opencv2/imgproc.hpp>
#include <simd/simd.h>
#include <vector>
#include <stdexcept>

namespace baekar {
namespace {

struct AxisVertex {
    simd_float3 position;
    simd_float3 color;
};

class MetalRenderer : public Renderer {
public:
    MetalRenderer(NSWindow* window, int width, int height)
        : _width(width), _height(height) {
        _device = MTLCreateSystemDefaultDevice();
        if (!_device) throw std::runtime_error("Metal not supported on this device");

        _layer = [CAMetalLayer layer];
        _layer.device = _device;
        _layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        _layer.framebufferOnly = YES;
        _layer.drawableSize = CGSizeMake(width, height);

        NSView* view = [window contentView];
        view.wantsLayer = YES;
        view.layer = _layer;

        _queue = [_device newCommandQueue];

        loadLibrary();
        buildBackgroundPipeline();
        buildAxisPipeline();
        buildAxisBuffers();
        buildSampler();
        buildDepth();

        identity(_proj);
        identity(_view);
    }

    void beginFrame() override {
        @autoreleasepool {
            _drawable = [_layer nextDrawable];
            if (!_drawable) { _cmd = nil; return; }
            MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
            rp.colorAttachments[0].texture = _drawable.texture;
            rp.colorAttachments[0].loadAction = MTLLoadActionClear;
            rp.colorAttachments[0].storeAction = MTLStoreActionStore;
            rp.colorAttachments[0].clearColor = MTLClearColorMake(0,0,0,1);
            rp.depthAttachment.texture = _depthTex;
            rp.depthAttachment.loadAction = MTLLoadActionClear;
            rp.depthAttachment.storeAction = MTLStoreActionDontCare;
            rp.depthAttachment.clearDepth = 1.0;

            _cmd = [_queue commandBuffer];
            _enc = [_cmd renderCommandEncoderWithDescriptor:rp];
        }
    }

    void drawCameraBackground(const cv::Mat& bgr) override {
        if (!_enc) return;
        if (bgr.empty()) return;

        cv::Mat src = bgr;
        if (src.cols != _bgTexW || src.rows != _bgTexH) {
            createBackgroundTexture(src.cols, src.rows);
        }

        cv::Mat bgra;
        if (src.channels() == 3) {
            cv::cvtColor(src, bgra, cv::COLOR_BGR2BGRA);
        } else if (src.channels() == 4) {
            bgra = src;
        } else {
            cv::Mat tmp;
            cv::cvtColor(src, tmp, cv::COLOR_GRAY2BGRA);
            bgra = tmp;
        }
        if (!bgra.isContinuous()) bgra = bgra.clone();

        MTLRegion region = MTLRegionMake2D(0, 0, _bgTexW, _bgTexH);
        [_bgTex replaceRegion:region
                  mipmapLevel:0
                    withBytes:bgra.data
                  bytesPerRow:bgra.step];

        [_enc setRenderPipelineState:_bgPipeline];
        [_enc setFragmentTexture:_bgTex atIndex:0];
        [_enc setFragmentSamplerState:_sampler atIndex:0];
        [_enc setDepthStencilState:_depthAlways];
        [_enc drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }

    void setProjection(const float m[16]) override { copy16(_proj, m); }
    void setView(const float m[16]) override { copy16(_view, m); }

    void drawAxis(float length) override {
        if (!_enc) return;

        // Scale axis vertex buffer with a world matrix baked into VP.
        simd_float4x4 P = toSimd(_proj);
        simd_float4x4 V = toSimd(_view);
        simd_float4x4 S = simdScale(length);
        simd_float4x4 VP = simd_mul(P, simd_mul(V, S));

        [_enc setRenderPipelineState:_axisPipeline];
        [_enc setVertexBuffer:_axisVB offset:0 atIndex:0];
        [_enc setVertexBytes:&VP length:sizeof(VP) atIndex:1];
        [_enc setDepthStencilState:_depthLessEq];
        [_enc drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:6];
    }

    void endFrame() override {
        if (!_enc) return;
        [_enc endEncoding];
        if (_drawable) [_cmd presentDrawable:_drawable];
        [_cmd commit];
        _enc = nil;
        _cmd = nil;
        _drawable = nil;
    }

private:
    static void identity(float m[16]) {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    static void copy16(float dst[16], const float src[16]) {
        for (int i = 0; i < 16; ++i) dst[i] = src[i];
    }
    static simd_float4x4 toSimd(const float m[16]) {
        // m is column-major already.
        return simd_matrix(simd_make_float4(m[0],  m[1],  m[2],  m[3]),
                           simd_make_float4(m[4],  m[5],  m[6],  m[7]),
                           simd_make_float4(m[8],  m[9],  m[10], m[11]),
                           simd_make_float4(m[12], m[13], m[14], m[15]));
    }
    static simd_float4x4 simdScale(float s) {
        return simd_matrix(simd_make_float4(s, 0, 0, 0),
                           simd_make_float4(0, s, 0, 0),
                           simd_make_float4(0, 0, s, 0),
                           simd_make_float4(0, 0, 0, 1));
    }

    void loadLibrary() {
        NSError* err = nil;
        NSString* libPath = [[[NSBundle mainBundle] bundlePath]
            stringByAppendingPathComponent:@"default.metallib"];
        if (![[NSFileManager defaultManager] fileExistsAtPath:libPath]) {
            // Fallback: try alongside the executable (CMake places it in the build dir).
            NSString* exe = [[NSBundle mainBundle] executablePath];
            libPath = [[exe stringByDeletingLastPathComponent]
                stringByAppendingPathComponent:@"default.metallib"];
        }
        _library = [_device newLibraryWithFile:libPath error:&err];
        if (!_library) {
            NSString* msg = err ? err.localizedDescription : @"unknown error";
            throw std::runtime_error(std::string("Failed to load default.metallib: ")
                                     + msg.UTF8String);
        }
    }

    void buildBackgroundPipeline() {
        id<MTLFunction> vs = [_library newFunctionWithName:@"bg_vs"];
        id<MTLFunction> fs = [_library newFunctionWithName:@"bg_fs"];
        MTLRenderPipelineDescriptor* d = [[MTLRenderPipelineDescriptor alloc] init];
        d.vertexFunction = vs;
        d.fragmentFunction = fs;
        d.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        d.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        NSError* err = nil;
        _bgPipeline = [_device newRenderPipelineStateWithDescriptor:d error:&err];
        if (!_bgPipeline) throw std::runtime_error("bg pipeline failed");
    }

    void buildAxisPipeline() {
        id<MTLFunction> vs = [_library newFunctionWithName:@"axis_vs"];
        id<MTLFunction> fs = [_library newFunctionWithName:@"axis_fs"];

        MTLVertexDescriptor* vd = [[MTLVertexDescriptor alloc] init];
        vd.attributes[0].format = MTLVertexFormatFloat3;
        vd.attributes[0].offset = offsetof(AxisVertex, position);
        vd.attributes[0].bufferIndex = 0;
        vd.attributes[1].format = MTLVertexFormatFloat3;
        vd.attributes[1].offset = offsetof(AxisVertex, color);
        vd.attributes[1].bufferIndex = 0;
        vd.layouts[0].stride = sizeof(AxisVertex);

        MTLRenderPipelineDescriptor* d = [[MTLRenderPipelineDescriptor alloc] init];
        d.vertexFunction = vs;
        d.fragmentFunction = fs;
        d.vertexDescriptor = vd;
        d.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        d.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        NSError* err = nil;
        _axisPipeline = [_device newRenderPipelineStateWithDescriptor:d error:&err];
        if (!_axisPipeline) throw std::runtime_error("axis pipeline failed");

        MTLDepthStencilDescriptor* dsAlways = [[MTLDepthStencilDescriptor alloc] init];
        dsAlways.depthCompareFunction = MTLCompareFunctionAlways;
        dsAlways.depthWriteEnabled = NO;
        _depthAlways = [_device newDepthStencilStateWithDescriptor:dsAlways];

        MTLDepthStencilDescriptor* dsLE = [[MTLDepthStencilDescriptor alloc] init];
        dsLE.depthCompareFunction = MTLCompareFunctionLessEqual;
        dsLE.depthWriteEnabled = YES;
        _depthLessEq = [_device newDepthStencilStateWithDescriptor:dsLE];
    }

    void buildAxisBuffers() {
        AxisVertex v[6] = {
            {{0,0,0}, {1,0,0}}, {{1,0,0}, {1,0,0}},  // +X red
            {{0,0,0}, {0,1,0}}, {{0,1,0}, {0,1,0}},  // +Y green
            {{0,0,0}, {0,0,1}}, {{0,0,1}, {0,0,1}},  // +Z blue
        };
        _axisVB = [_device newBufferWithBytes:v
                                       length:sizeof(v)
                                      options:MTLResourceStorageModeShared];
    }

    void buildSampler() {
        MTLSamplerDescriptor* sd = [[MTLSamplerDescriptor alloc] init];
        sd.minFilter = MTLSamplerMinMagFilterLinear;
        sd.magFilter = MTLSamplerMinMagFilterLinear;
        sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
        _sampler = [_device newSamplerStateWithDescriptor:sd];
    }

    void buildDepth() {
        MTLTextureDescriptor* td = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                         width:_width
                                        height:_height
                                     mipmapped:NO];
        td.usage = MTLTextureUsageRenderTarget;
        td.storageMode = MTLStorageModePrivate;
        _depthTex = [_device newTextureWithDescriptor:td];
    }

    void createBackgroundTexture(int w, int h) {
        MTLTextureDescriptor* td = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                         width:w
                                        height:h
                                     mipmapped:NO];
        td.usage = MTLTextureUsageShaderRead;
        _bgTex = [_device newTextureWithDescriptor:td];
        _bgTexW = w;
        _bgTexH = h;
    }

    int _width, _height;
    int _bgTexW = 0, _bgTexH = 0;

    id<MTLDevice> _device = nil;
    CAMetalLayer* _layer = nil;
    id<MTLCommandQueue> _queue = nil;
    id<MTLLibrary> _library = nil;
    id<MTLRenderPipelineState> _bgPipeline = nil;
    id<MTLRenderPipelineState> _axisPipeline = nil;
    id<MTLDepthStencilState> _depthAlways = nil;
    id<MTLDepthStencilState> _depthLessEq = nil;
    id<MTLSamplerState> _sampler = nil;
    id<MTLTexture> _bgTex = nil;
    id<MTLTexture> _depthTex = nil;
    id<MTLBuffer> _axisVB = nil;

    id<CAMetalDrawable> _drawable = nil;
    id<MTLCommandBuffer> _cmd = nil;
    id<MTLRenderCommandEncoder> _enc = nil;

    float _proj[16];
    float _view[16];
};

} // namespace

std::unique_ptr<Renderer> Renderer::Create(void* nsWindowHandle, int w, int h) {
    NSWindow* win = (__bridge NSWindow*)nsWindowHandle;
    return std::make_unique<MetalRenderer>(win, w, h);
}

} // namespace baekar
