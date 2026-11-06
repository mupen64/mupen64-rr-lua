/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QString>

#include <functional>
#include <optional>
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

class LuaInstance;
class LuaCanvas;
class QQuickWindow;
class QTimer;

// Per-window callback kinds, stored in the window userdata's uservalue.
enum class LuaWindowCallback : int
{
    Paint = 1,
    Key = 2,
    Mouse = 3,
    Closed = 4,
};

// A script-owned top-level window (emu.create_window), mirroring the Windows view's LuaWindow.
class LuaWindow : public QObject
{
    Q_OBJECT

  public:
    explicit LuaWindow(LuaInstance *instance);
    ~LuaWindow() override;

    LuaInstance *instance() const { return m_instance; }
    QString id() const { return m_id; }
    void setId(const QString &id) { m_id = id; }
    QString anchor() const { return m_anchor; }
    void setAnchor(const QString &anchor) { m_anchor = anchor; }
    bool closable() const { return m_closable; }
    void setClosable(bool closable) { m_closable = closable; }
    bool isMain() const { return m_main; }
    void setMain(bool main) { m_main = main; }
    bool isClosed() const { return m_closed; }

    LuaCanvas *canvas() const { return m_canvas; }
    QPointF lastMousePosition() const { return m_lastMouse; }
    void setLastMousePosition(const QPointF &position) { m_lastMouse = position; }

    // Creates the native window (never for the main window) and shows it.
    bool create(const QSize &size, bool minimizable, bool maximizable);
    // Runs the window callbacks of one kind; returns false when a callback errored.
    bool call(LuaWindowCallback kind, const std::function<int(lua_State *)> &invoke);
    // Requests a frame: clears the canvas, runs paint callbacks, repaints.
    void paintFrame();
    // Starts the ~60fps repaint loop used while paint callbacks are registered.
    void ensurePaintTimer();
    // Keeps the window glued to the emulator window according to its anchor.
    void reposition();
    // Calls the closed callbacks and destroys the window. Returns false on callback error.
    bool close();
    // Resizes the client area to the given logical size, clamped to 1..32767.
    void resizeClient(int width, int height);

    QQuickWindow *windowHandle() const;

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    void stopPaintTimer();

    LuaInstance *m_instance{nullptr};
    QString m_id;
    QString m_anchor;
    bool m_closed{false};
    bool m_closable{true};
    bool m_main{false};
    bool m_updatingAnchor{false};

    QPointer<QQuickWindow> m_window{nullptr};
    LuaCanvas *m_canvas{nullptr};
    QPointF m_lastMouse;
    QPointer<QTimer> m_paintTimer{nullptr};

    friend class LuaInstance;
};

namespace LuaWindowApi
{
void registerTypes(lua_State *L);
int create(lua_State *L);
int main(lua_State *L);
/** Maps a Qt::Key to the SDL2 keycode used as event.keycode2 (nullopt when there is no equivalent). */
std::optional<int> qtKeyToSdlKeycode(int qtKey);
/** Exposes Mupen.keycode, the SDL keycode table scripts compare event.keycode2 against. */
void registerMupenGlobals(lua_State *L);
} // namespace LuaWindowApi
