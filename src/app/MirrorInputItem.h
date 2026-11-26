#pragma once

#include "KeyForwarding.h"

#include <gs/input/Geometry.h>
#include <gs/input/MappingEngine.h>

#include <QElapsedTimer>
#include <QList>
#include <QPointF>
#include <QQuickItem>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QTimer>
#include <QVariant>

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace gs::app {

class PreeditBubble;

class MirrorInputItem final : public QQuickItem
{
    Q_OBJECT

public:
    explicit MirrorInputItem(QQuickItem* parent = nullptr);

    void setContentRect(const QRectF& rect) { m_content = rect; }
    void setCapturing(bool on);
    [[nodiscard]] bool capturing() const noexcept { return m_capturing; }
    void setTyping(bool on);
    [[nodiscard]] bool typing() const noexcept { return m_typing; }
    void setRelativeMouse(bool on, QSizeF target = {}, int turn = 0);
    // Trackpad swipes: how far the finger on the phone moves for the fingers' travel (1 = the same distance on
    // the picture), and whether, and how strongly, it keeps going with the trackpad's momentum after they leave.
    // longest: the farthest one swipe, momentum included, moves the finger, as a share of the picture in the
    // direction it goes (1 = no limit).
    void setTrackpad(double speed, bool inertia, double inertiaStrength, double longest);
    // A swipe that went at least turnFrom of the picture one way, with at least 90% of its path that way, ends in
    // a short even flick that way, so the phone turns one page; 0 turns this off.
    void setPageTurn(double turnFrom);
    // How a two-finger trackpad swipe reaches the phone (Settings.h touch.trackpadMode): one finger that turns a
    // page per deliberate swipe, one that keeps the swipe's own speed when the fingers leave so the phone flings
    // it, one that lifts where the fingers stop, or the phone's scroll wheel.
    enum class SwipeMode : std::uint8_t { PageTurn, Natural, Direct, Scroll };
    void setSwipeMode(SwipeMode mode) { m_swipeMode = mode; }
    [[nodiscard]] bool relativeMouse() const noexcept { return m_relative; }

    [[nodiscard]] static std::optional<input::Vec2> normalized(const QRectF& content, QPointF position, bool clamped);
    [[nodiscard]] static std::int16_t scrollUnits(int angleDelta) noexcept;
    [[nodiscard]] static int mouseButtonBit(Qt::MouseButton button) noexcept;
    void setButtonKeys(std::map<Qt::MouseButton, std::uint32_t> keys);
    [[nodiscard]] std::optional<std::uint32_t> keycodeOf(Qt::MouseButton button) const;
    void setClaimedInputs(std::vector<input::Chord> chords) { m_claimed = std::move(chords); }
    [[nodiscard]] bool claimed(input::InputSource source, std::uint8_t modifiers = 0) const;
    [[nodiscard]] static std::vector<input::Chord> liveClaims(const input::Profile& profile,
                                                              const std::vector<input::CursorClaim>& claims,
                                                              const QList<bool>& live);

    [[nodiscard]] QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

signals:
    void touched(gs::input::CursorPhase phase, gs::input::Vec2 position);
    void scrolled(gs::input::Vec2 position, int horizontal, int vertical);
    void androidKey(int keycode, int metaState, bool down, int repeat);
    void typed(const QString& text);
    void mouseMoved(int dx, int dy);
    void mouseRecenter(int x, int y);
    void mouseParked();
    void mouseButtonsChanged(int buttons);

protected:
    bool event(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseUngrabEvent() override;
    void hoverEnterEvent(QHoverEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void hoverLeaveEvent(QHoverEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool swipeWithTrackpad(QWheelEvent* event);
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    struct HeldKey {
        ForwardedKey key;
        int repeat = 0;
    };

    void cancel();
    void moveSwipe(QPointF delta, bool ending);
    void liftSwipe();
    [[nodiscard]] std::optional<QPointF> pageTurn() const;
    void flickStep();
    void startFlick(QPointF direction);
    void startFling();
    [[nodiscard]] QPointF releaseVelocity() const;
    void releaseSwipe();
    void releaseKeys();
    void showPreedit(const QString& text, int cursor);
    void follow(QPointF position);
    [[nodiscard]] bool pressRelative(QMouseEvent* event, bool down);

    QRectF m_content;
    // A trackpad swipe (scroll phases) under way as one finger on the phone, and where that finger is.
    bool m_swiping = false;
    // The finger reached the phone's edge and was lifted; the rest of this swipe is dropped.
    bool m_swipeSpent = false;
    QPointF m_swipePoint;
    // After the fingers left: the finger on the phone keeps moving with the trackpad's momentum (scaled by the
    // inertia strength) until the momentum ends, stops coming, or has gone on for long enough.
    bool m_coasting = false;
    QElapsedTimer m_coastClock;
    QTimer m_coastIdle;
    QElapsedTimer m_swipeClock;
    double m_swipeSpeed = 0.0;
    double m_trackpadSpeed = 1.0;
    bool m_inertia = true;
    double m_inertiaStrength = 1.0;
    double m_longestSwipe = 0.5;
    // Where this swipe's finger went down, and whether it reached the longest swipe (it then stays where it is
    // until the fingers leave, so the phone does not fling it farther).
    QPointF m_swipeStart;
    bool m_swipeCapped = false;
    // This swipe's path, summed per direction (right, left, down, up), in points of the picture.
    std::array<double, 4> m_swipePath{};
    double m_turnFrom = 0.25;
    SwipeMode m_swipeMode = SwipeMode::PageTurn;
    // The swipe's last movement and how long it took, for the natural fling's speed.
    QPointF m_lastSwipeDelta;
    qint64 m_lastSwipeMs = 1;
    // The finger's recent positions (ms since the swipe began, point), for its speed when the fingers leave.
    QElapsedTimer m_moveClock;
    // Since the finger last lifted: macOS's first momentum event can come as the start of a new scroll, with
    // movement, a few ms after the fingers left; it must not put a new finger down (that stops the phone's fling).
    QElapsedTimer m_liftClock;
    std::vector<std::pair<qint64, QPointF>> m_recentMoves;
    // The page-turn flick under way: steps left, each step, and the clock between them.
    int m_flickSteps = 0;
    QPointF m_flickStep;
    QTimer m_flickTimer;
    // How far the finger has coasted, in points of the picture.
    double m_coasted = 0.0;
    QPointF m_lastPress;
    // The pinyin (or other text) the input method is still composing, shown on the picture.
    PreeditBubble* m_preedit = nullptr;
    bool m_capturing = false;
    bool m_typing = false;
    bool m_touching = false;
    bool m_relative = false;
    int m_turn = 0;
    QSizeF m_target;
    std::optional<QPointF> m_pointerOnPhone;
    bool m_pendingRecenter = false;
    QPointF m_rest;
    int m_buttons = 0;
    Qt::MouseButtons m_keysHeld;
    std::map<Qt::MouseButton, std::uint32_t> m_buttonKeys;
    std::map<int, HeldKey> m_heldKeys;
    std::vector<input::Chord> m_claimed;
};

}
