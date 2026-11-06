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

    static LuaInstance *fromState(lua_State *L);

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

    void registerCallback(LuaCallbackKey key, int luaFuncIndex);
    void unregisterCallback(LuaCallbackKey key, int luaFuncIndex);

  signals:
    void nameChanged();    void pathChanged();
    void runningChanged();
    void trustedChanged();
    void logsChanged();
    void lastErrorChanged();
    void logAdded(const QString &text);
    void errorOccurred(const QString &error);
    void started();
    void stopped();

  private:
    void registerGlobalAPIs();
    void registerEmuAPIs();
    void registerMemoryAPIs();
    void registerJoypadAPIs();

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

    std::unordered_map<LuaCallbackKey, std::vector<int>> m_callbacks;
    std::unordered_map<std::string, std::string> m_hints;
};
