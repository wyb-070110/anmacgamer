#version 440
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The video picture from three planes: luma, Cb and Cr (software pictures).
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform Parameters {
    vec4 corners[4];
    vec4 toRgb[3];
    vec4 extra;
    vec4 sharpen;
    vec4 crop;
};

layout(binding = 1) uniform sampler2D luma;
layout(binding = 2) uniform sampler2D cb;
layout(binding = 3) uniform sampler2D cr;

float neighbour(vec2 uv, vec2 offset)
{
    const vec2 last = extra.yz - 0.5 * sharpen.yz;
    return textureLod(luma, min(uv + offset, last), 0.0).r;
}

// With sharpen.x above 0 the luma gets a clamped unsharp mask over its four neighbouring texels.
float lumaAt(vec2 uv)
{
    float y = texture(luma, uv).r;
    if (sharpen.x > 0.0) {
        const float left = neighbour(uv, vec2(-sharpen.y, 0.0));
        const float right = neighbour(uv, vec2(sharpen.y, 0.0));
        const float up = neighbour(uv, vec2(0.0, -sharpen.z));
        const float down = neighbour(uv, vec2(0.0, sharpen.z));
        const float lowest = min(y, min(min(left, right), min(up, down)));
        const float highest = max(y, max(max(left, right), max(up, down)));
        const float blur = (left + right + up + down) * 0.25;
        y = clamp(y + sharpen.x * 2.0 * (y - blur), lowest, highest);
    }
    return y;
}

void main()
{
    const vec3 yuv = vec3(lumaAt(v_uv), texture(cb, v_uv).r, texture(cr, v_uv).r);
    const vec3 rgb = clamp(vec3(dot(toRgb[0].xyz, yuv) + toRgb[0].w,
                                dot(toRgb[1].xyz, yuv) + toRgb[1].w,
                                dot(toRgb[2].xyz, yuv) + toRgb[2].w), 0.0, 1.0);
    fragColor = vec4(rgb * extra.x, extra.x);
}
