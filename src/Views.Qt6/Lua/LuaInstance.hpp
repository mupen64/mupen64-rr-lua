/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QColor>
#include <QFileInfo>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <qqmlintegration.h>

#include <functional>
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

    LuaCanvas *canvas() const { return m_canvas; }
    void setCanvas(LuaCanvas *canvas);

    lua_State *state() const { return m_l; }

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
    void dispatchInput(CoreButtons *input, int controllerIndex);
    void dispatchInterval();
    void dispatchReset();
    void dispatchSaveState();
    void dispatchLoadState();
    void dispatchKey(int key, bool pressed, bool ctrl, bool alt, bool shift);
    void dispatchMouse(int x, int y, int button, bool pressed);

    void registerCallback(LuaCallbackKey key, int luaFuncIndex);
    void unregisterCallback(LuaCallbackKey key, int luaFuncIndex);

  signals:
    void nameChanged();
    void pathChanged();
    void runningChanged();
    void trustedChanged();
    void logsChanged();
    void lastErrorChanged();
    void canvasChanged();
    void logAdded(const QString &text);
    void errorOccurred(const QString &error);
    void started();
    void stopped();

  private:
    void registerGlobalAPIs();
    void registerEmuAPIs();
    void registerMemoryAPIs();
    void registerJoypadAPIs();
    void registerGuiAPIs();
    void applySandbox();
    void invokeCallback(LuaCallbackKey key, int nargs = 0);

    lua_State *m_l{nullptr};
    QString m_path;
    bool m_running{false};
    bool m_trusted{false};
    QString m_logs;
    QString m_lastError;
    QPointer<LuaCanvas> m_canvas{nullptr};

    std::unordered_map<LuaCallbackKey, std::vector<int>> m_callbacks;
    std::unordered_map<std::string, std::string> m_hints;
};
