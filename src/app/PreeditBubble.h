// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The text an input method is still composing (pinyin before a Chinese word is picked), drawn on the phone's
// picture where the user last pressed (usually the phone's text box), since the phone only sees finished text.
#pragma once

#include <QFont>
#include <QQuickPaintedItem>
#include <QRectF>
#include <QString>

class QPainter;

namespace gs::app {

class PreeditBubble final : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit PreeditBubble(QQuickItem* parent);

    // Shows text with the caret before character cursor, its left edge near anchor, inside bounds; empty text
    // hides it.
    void show(const QString& text, int cursor, QPointF anchor, const QRectF& bounds);
    // Where the caret is, in the parent's coordinates: the input method puts its candidate list under it.
    [[nodiscard]] QRectF caretRect() const noexcept { return m_caret; }
    [[nodiscard]] bool showing() const noexcept { return !m_text.isEmpty(); }

    void paint(QPainter* painter) override;

private:
    QFont m_font;
    QString m_text;
    int m_cursor = 0;
    QRectF m_caret;
};

}
