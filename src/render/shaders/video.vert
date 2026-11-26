#version 440
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// One video picture as a triangle strip into its content rectangle (PLAN 4.5): corner gl_VertexIndex of the four
// in Parameters, its texture coordinate from the crop and the visible part of the texture.
layout(location = 0) out vec2 v_uv;

layout(std140, binding = 0) uniform Parameters {
    vec4 corners[4];
    vec4 toRgb[3];
    vec4 extra;
    vec4 sharpen;
    vec4 crop;
};

void main()
{
    const int id = gl_VertexIndex;
    v_uv = (crop.xy + vec2(float(id & 1), float(id >> 1)) * crop.zw) * extra.yz;
    gl_Position = corners[id];
}
