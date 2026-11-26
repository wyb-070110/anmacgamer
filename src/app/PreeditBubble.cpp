// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
#include "PreeditBubble.h"

#include <QColor>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

#include <algorithm>

namespace gs::app {

namespace {

constexpr double kPadX = 10.0;
constexpr double kPadY = 6.0;
constexpr double kRadius = 8.0;
constexpr int kTextPixels = 16;

}

PreeditBubble::PreeditBubble(QQuickItem* parent) : QQuickPaintedItem(parent), m_font(QGuiApplication::font())
{
    m_font.setPixelSize(kTextPixels);
    setAntialiasing(true);
    setVisible(false);
    setZ(1000);
}

void PreeditBubble::show(const QString& text, int cursor, QPointF anchor, const QRectF& bounds)
{
    m_text = text;
    m_cursor = std::clamp(cursor, 0, static_cast<int>(text.size()));
    if (text.isEmpty()) {
        setVisible(false);
        m_caret = {};
        return;
    }
    const QFontMetricsF metrics(m_font);
    const double width = metrics.horizontalAdvance(text) + 2 * kPadX;
    const double height = metrics.height() + 2 * kPadY;
    // The bubble sits on the place pressed, a little to its left, as if typed into the box there.
    double x = anchor.x() - kPadX;
    double y = anchor.y() - height / 2.0;
    x = std::clamp(x, bounds.left(), std::max(bounds.left(), bounds.right() - width));
    y = std::clamp(y, bounds.top(), std::max(bounds.top(), bounds.bottom() - height));
    setPosition({x, y});
    setSize({width, height});
    const double caretX = x + kPadX + metrics.horizontalAdvance(text.left(m_cursor));
    m_caret = QRectF(caretX, y, 1.0, height);
    setVisible(true);
    update();
}

void PreeditBubble::paint(QPainter* painter)
{
    const QRectF box = boundingRect().adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath shape;
    shape.addRoundedRect(box, kRadius, kRadius);
    painter->fillPath(shape, QColor(28, 28, 30, 236));
    painter->setPen(QPen(QColor(255, 255, 255, 46), 1.0));
    painter->drawPath(shape);

    const QFontMetricsF metrics(m_font);
    painter->setFont(m_font);
    painter->setPen(QColor(255, 255, 255));
    const double baseline = kPadY + metrics.ascent();
    painter->drawText(QPointF(kPadX, baseline), m_text);
    // The composing text is underlined, as macOS marks it in its own text boxes.
    const double underline = baseline + metrics.underlinePos() + 1.0;
    painter->setPen(QPen(QColor(255, 255, 255, 150), 1.0));
    painter->drawLine(QPointF(kPadX, underline), QPointF(kPadX + metrics.horizontalAdvance(m_text), underline));
    const double caret = kPadX + metrics.horizontalAdvance(m_text.left(m_cursor));
    painter->setPen(QPen(QColor(10, 132, 255), 2.0));
    painter->drawLine(QPointF(caret, kPadY - 1.0), QPointF(caret, kPadY + metrics.height() + 1.0));
}

}
