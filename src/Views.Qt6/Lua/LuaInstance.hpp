/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QColor>
#include <QFileInfo>
#include <QHash>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <qqmlintegration.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <Core/Types.hpp>

class LuaCanvas;
class LuaWindow;
class QKeyEvent;
class QMouseEvent;
class QQuickWindow;
class QWindow;

enum class LuaCallbackKey : uint8_t
{
    UpdateScreen,
    Paint,
    VI,
    Input,
    Stop,
    Interval,
    PlayMovie,
    StopMovie,
    LoadState,
    SaveState,
    Reset,
    SeekCompleted,
    Key,
    Mouse,
    _Count
};

class LuaInstance : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("LuaCanvas.hpp")

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool trusted READ isTrusted WRITE setTrusted NOTIFY trustedChanged)
    Q_PROPERTY(QString logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(LuaCanvas *canvas READ canvas WRITE setCanvas NOTIFY canvasChanged)

  public:
    explicit LuaInstance(QObject *parent = nullptr);
    explicit LuaInstance(const QString &path, QObject *parent = nullptr);
    ~LuaInstance() override;

    QString name() const;
    QString path() const { return m_path; }
    void setPath(const QString &path);

    bool isRunning() const { return m_running; }
    bool isTrusted() const { return m_trusted; }
    void setTrusted(bool trusted);

    QString logs() const { return m_logs; }
    QString lastError() const { return m_lastError; }
    void setLastError(const QString &err);

    LuaCanvas *canvas() const;
    void setCanvas(LuaCanvas *canvas);

    // Window the Lua windows glue themselves to (the emulator main window).
    QWindow *hostWindow() const;
    void setHostWindow(QWindow *window);

    // Canvas the wgui/painter primitives draw on: the overlay unless a window callback runs.
    LuaCanvas *activeCanvas() const;
    void setActiveCanvas(LuaCanvas *canvas);

    LuaWindow *findWindow(const QString &id) const;
    LuaWindow *addWindow(const QString &id, const QString &anchor, bool closable);
    void dropWindow(LuaWindow *window);
    LuaWindow *mainWindow() const;
    void createMainWindow();
    void closeWindows();
    void repositionWindows();

    static LuaInstance *fromState(lua_State *L);

    // Event tables passed to atkey/atmouse (also used per-window).
    void pushKeyTable(const QKeyEvent &event);
    void pushMouseTable(const QMouseEvent &event);
    // events on the app window are dispatched in the canvas coordinate space the scripts use
    void pushMouseTableAt(const QPointF &position, const QMouseEvent &event);
    void pushWheelTableAt(const QPointF &position, const QWheelEvent &event);
    // Windows Virtual-Key codes, so scripts see the same e.keycode as on the Windows view.
    static int qtKeyToVk(int qtKey);

    lua_State *state() const { return m_l; }

    // Input polls seen by this instance (emu.inputcount).
    quint64 inputCount() const { return m_inputCount; }

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void appendLog(const QString &text);

    std::string queryHint(const std::string &key) const;
    void setHint(const std::string &key, const std::string &value);

    void dispatchUpdateScreen();
    void dispatchPaint();
    void dispatchVI();
    void dispatchInput(int controllerIndex);
    void dispatchInterval();
    void dispatchReset();
    void dispatchSaveState();
    void dispatchLoadState();
    void dispatchKey(int key, bool pressed, bool ctrl, bool alt, bool shift);
    void dispatchMouse(int x, int y, int button, bool pressed);

    void registerCallback(LuaCallbackKey key, int luaFuncIndex);
    void unregisterCallback(LuaCallbackKey key, int luaFuncIndex);

  signals:
    void nameChanged();    void pathChanged();
    void runningChanged();
    void trustedChanged();
    void logsChanged();
    void lastErrorChanged();
    void canvasChanged();
    void logAdded(const QString &text);
    void errorOccurred(const QString &error);
    void started();
    void stopped();

  protected:
    // key/mouse events arriving at the emulator window: main window + emu.atkey/atmouse
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    void registerGlobalAPIs();
    void registerEmuAPIs();
    void registerMemoryAPIs();
    void registerJoypadAPIs();
    void registerGuiAPIs();

    /** Returns the current controller state of a port, as a Lua table (joypad.get). */
    void pushJoypad(lua_State *L, int port);
    /** Records a controller state from a Lua table, to be applied on the next input callback (joypad.set). */
    void setJoypad(lua_State *L, int port);
    void applySandbox();
    void invokeCallback(LuaCallbackKey key, int nargs = 0);

    lua_State *m_l{nullptr};
    QString m_path;
    QString m_name;
    bool m_running{false};
    bool m_trusted{false};
    quint64 m_inputCount{0};
    QString m_logs;
    QString m_lastError;
    QPointer<LuaCanvas> m_canvas{nullptr};
    QPointer<LuaCanvas> m_activeCanvas{nullptr};
    QPointer<QWindow> m_hostWindow{nullptr};

    std::vector<std::unique_ptr<LuaWindow>> m_windows;
    QPointer<LuaWindow> m_mainWindow{nullptr};

    // last click, to synthesize the double/triple-click flags the Win32 host gets from QPA
    int m_lastClickButton = -1;
    QPointF m_lastClickPos;
    qint64 m_lastClickMs = 0;
    int m_lastClickChain = 0;

    std::unordered_map<LuaCallbackKey, std::vector<int>> m_callbacks;
    std::unordered_map<std::string, std::string> m_hints;
};
