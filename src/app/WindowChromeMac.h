// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
#pragma once

class QWindow;

namespace gs::app {

// macOS: the system title bar, transparent over the content; false if the window has no native window yet.
bool styleMacWindow(QWindow* window);
void setMacWindowDark(QWindow* window, bool dark);
// Liquid Glass: the desktop shows through the window, blurred by the system, wherever its content is transparent.
// Off puts back an opaque window.
void setMacWindowGlass(QWindow* window, bool on);
// While the user drags the window's edge, keep the picture at this width-to-height ratio; chromeWidth and
// chromeHeight (points) are the window's parts around the picture. 0 resizes freely.
void setMacWindowAspect(QWindow* window, double aspect, int chromeWidth, int chromeHeight);
bool startMacWindowDrag(QWindow* window);
// Above the windows of every app and on every desktop (also beside an app in full screen); off puts it back.
void setMacWindowOnTop(QWindow* window, bool on);
void performMacTitleBarDoubleClick(QWindow* window);

}
