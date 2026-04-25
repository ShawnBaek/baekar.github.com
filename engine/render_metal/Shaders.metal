#include <metal_stdlib>
using namespace metal;

struct BgVertexOut {
    float4 position [[position]];
    float2 uv;
};

vertex BgVertexOut bg_vs(uint vid [[vertex_id]]) {
    // Fullscreen triangle strip in NDC.
    float2 ndc[4] = {
        float2(-1.0, -1.0),
        float2( 1.0, -1.0),
        float2(-1.0,  1.0),
        float2( 1.0,  1.0),
    };
    float2 uv[4] = {
        float2(0.0, 1.0),
        float2(1.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 0.0),
    };
    BgVertexOut o;
    o.position = float4(ndc[vid], 0.0, 1.0);
    o.uv = uv[vid];
    return o;
}

fragment float4 bg_fs(BgVertexOut in [[stage_in]],
                      texture2d<float> tex [[texture(0)]],
                      sampler samp [[sampler(0)]]) {
    return tex.sample(samp, in.uv);
}

struct AxisVertexIn {
    float3 position [[attribute(0)]];
    float3 color    [[attribute(1)]];
};

struct AxisVertexOut {
    float4 position [[position]];
    float3 color;
};

struct AxisUniforms {
    float4x4 viewProj;
};

vertex AxisVertexOut axis_vs(AxisVertexIn in [[stage_in]],
                             constant AxisUniforms& u [[buffer(1)]]) {
    AxisVertexOut o;
    o.position = u.viewProj * float4(in.position, 1.0);
    o.color = in.color;
    return o;
}

fragment float4 axis_fs(AxisVertexOut in [[stage_in]]) {
    return float4(in.color, 1.0);
}
