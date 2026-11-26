// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Draws the layers of GameScrcpy's Icon Composer icon (packaging/macos/icon/GameScrcpy.icon), direction B of
// docs/handoff/gemini/icon.md: a navy portrait phone behind a larger blue landscape glass screen with a white
// chevron. Each layer is alone on a 1024 canvas that is the whole icon, edge to edge; every part stays inside
// 164…860 px so the system mask never cuts it.
//
//   swift packaging/macos/icon/make_icon.swift layers <dir>
import AppKit
import CoreGraphics

func rgb(_ r: Int, _ g: Int, _ b: Int, _ a: CGFloat = 1) -> CGColor {
    CGColor(srgbRed: CGFloat(r) / 255, green: CGFloat(g) / 255, blue: CGFloat(b) / 255, alpha: a)
}

let space = CGColorSpace(name: CGColorSpace.sRGB)!

// Top-left coordinates, as in the spec, on a 1024 canvas.
func box(_ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat) -> CGRect {
    CGRect(x: x, y: 1024 - y - h, width: w, height: h)
}

let phone = box(188, 264, 260, 480)
let screen = box(316, 400, 520, 360)

func fillGradient(_ ctx: CGContext, _ rect: CGRect, radius: CGFloat, top: CGColor, bottom: CGColor,
                  horizontal: Bool = false) {
    ctx.saveGState()
    ctx.addPath(CGPath(roundedRect: rect, cornerWidth: radius, cornerHeight: radius, transform: nil))
    ctx.clip()
    let gradient = CGGradient(colorsSpace: space, colors: [top, bottom] as CFArray, locations: [0, 1])!
    let start = horizontal ? CGPoint(x: rect.minX, y: rect.maxY) : CGPoint(x: rect.midX, y: rect.maxY)
    let end = horizontal ? CGPoint(x: rect.maxX, y: rect.minY) : CGPoint(x: rect.midX, y: rect.minY)
    ctx.drawLinearGradient(gradient, start: start, end: end, options: [])
    ctx.restoreGState()
}

func drawPhone(_ ctx: CGContext) {
    fillGradient(ctx, phone, radius: 58, top: rgb(0x1D, 0x35, 0x57), bottom: rgb(0x0F, 0x20, 0x38))
}

func drawScreen(_ ctx: CGContext) {
    fillGradient(ctx, screen, radius: 52, top: rgb(0x0A, 0x84, 0xFF), bottom: rgb(0x38, 0xBD, 0xF8), horizontal: true)
}

func drawChevron(_ ctx: CGContext) {
    let cx = screen.midX + 12, cy = screen.midY
    ctx.setStrokeColor(rgb(255, 255, 255))
    ctx.setLineWidth(48)
    ctx.setLineCap(.round)
    ctx.setLineJoin(.round)
    ctx.move(to: CGPoint(x: cx - 40, y: cy + 80))
    ctx.addLine(to: CGPoint(x: cx + 40, y: cy))
    ctx.addLine(to: CGPoint(x: cx - 40, y: cy - 80))
    ctx.strokePath()
}

func write(_ ctx: CGContext, _ path: String) {
    let rep = NSBitmapImageRep(cgImage: ctx.makeImage()!)
    try! rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: path))
}

let args = CommandLine.arguments
guard args.count == 3, args[1] == "layers" else {
    FileHandle.standardError.write("usage: make_icon.swift layers <dir>\n".data(using: .utf8)!)
    exit(1)
}
for (name, draw) in [("phone", drawPhone), ("screen", drawScreen), ("chevron", drawChevron)]
    as [(String, (CGContext) -> Void)] {
    let ctx = CGContext(data: nil, width: 1024, height: 1024, bitsPerComponent: 8, bytesPerRow: 0, space: space,
                        bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
    draw(ctx)
    write(ctx, args[2] + "/" + name + ".png")
}
