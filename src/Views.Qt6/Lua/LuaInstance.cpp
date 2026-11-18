/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInstance.hpp"
#include "LuaInternal.hpp"
#include "Resources.hpp"

#include <QFile>
#include <QFileInfo>

LuaInstance::LuaInstance(QObject *parent) : QObject(parent)
{
}

LuaInstance::LuaInstance(const QString &path, QObject *parent) : QObject(parent), m_path(path)
{
    m_name = QFileInfo(path).fileName();
}

LuaInstance::~LuaInstance()
{
    stop();
}

QString LuaInstance::name() const
{
    if (!m_name.isEmpty()) return m_name;
    if (!m_path.isEmpty()) return QFileInfo(m_path).fileName();
    return tr("Untitled Script");
}

void LuaInstance::setPath(const QString &path)
{
    if (m_path == path) return;
    m_path = path;
    m_name = QFileInfo(path).fileName();
    emit pathChanged();
    emit nameChanged();
}

void LuaInstance::setTrusted(bool trusted)
{
    if (m_trusted == trusted) return;
    m_trusted = trusted;
    emit trustedChanged();
}

void LuaInstance::setLastError(const QString &err)
{
    if (m_lastError == err) return;
    m_lastError = err;
    emit lastErrorChanged();
}

void LuaInstance::clearLogs()
{
    m_logs.clear();
    emit logsChanged();
}

void LuaInstance::appendLog(const QString &text)
{
    constexpr qsizetype MAX_LOG_LENGTH = 0x10000;
    m_logs.append(text);
    if (m_logs.size() > MAX_LOG_LENGTH)
    {
        m_logs.remove(0, m_logs.size() - MAX_LOG_LENGTH / 2);
    }
    emit logAdded(text);
    emit logsChanged();
}

std::string LuaInstance::queryHint(const std::string &key) const
{
    auto it = m_hints.find(key);
    return (it != m_hints.end()) ? it->second : "";
}

void LuaInstance::setHint(const std::string &key, const std::string &value)
{
    m_hints[key] = value;
}

void LuaInstance::registerCallback(LuaCallbackKey key, int luaFuncIndex)
{
    if (!m_l) return;
    lua_pushvalue(m_l, luaFuncIndex);
    int ref = luaL_ref(m_l, LUA_REGISTRYINDEX);
    m_callbacks[key].push_back(ref);
}

void LuaInstance::unregisterCallback(LuaCallbackKey key, int luaFuncIndex)
{
    if (!m_l) return;
    auto it = m_callbacks.find(key);
    if (it == m_callbacks.end()) return;

    for (auto iter = it->second.begin(); iter != it->second.end(); ++iter)
    {
        lua_rawgeti(m_l, LUA_REGISTRYINDEX, *iter);
        bool match = lua_rawequal(m_l, -1, luaFuncIndex);
        lua_pop(m_l, 1);
        if (match)
        {
            luaL_unref(m_l, LUA_REGISTRYINDEX, *iter);
            it->second.erase(iter);
            break;
        }
    }
}

void LuaInstance::invokeCallback(LuaCallbackKey key, int nargs)
{
    if (!m_l || !m_running) return;

    auto it = m_callbacks.find(key);
    if (it == m_callbacks.end() || it->second.empty())
    {
        if (nargs > 0) lua_pop(m_l, nargs);
        return;
    }

    std::vector<int> refs = it->second;
    for (int ref : refs)
    {
        lua_rawgeti(m_l, LUA_REGISTRYINDEX, ref);
        if (lua_isfunction(m_l, -1))
        {
            for (int i = 0; i < nargs; ++i)
            {
                lua_pushvalue(m_l, -(nargs + 1));
            }
            if (lua_pcall(m_l, nargs, 0, 0) != LUA_OK)
            {
                const char *err = lua_tostring(m_l, -1);
                appendLog(QString("[Error callback] %1\n").arg(err ? err : ""));
                lua_pop(m_l, 1);
            }
        }
        else
        {
            lua_pop(m_l, 1);
        }
    }
    if (nargs > 0) lua_pop(m_l, nargs);
}

bool LuaInstance::start()
{
    if (m_running) return false;

    // a new attempt starts clean: stale errors must not linger in the UI
    setLastError(QString());

    if (m_path.isEmpty())
    {
        QString err = tr("Script path is empty");
        appendLog(err + "\n");
        setLastError(err);
        return false;
    }

    const QString &localPath = m_path;

    if (!QFile::exists(localPath))
    {
        QString err = tr("Script file does not exist: %1").arg(localPath);
        appendLog(err + "\n");
        setLastError(err);
        return false;
    }

    m_l = luaL_newstate();
    if (!m_l)
    {
        QString err = tr("Failed to allocate Lua state");
        appendLog(err + "\n");
        setLastError(err);
        return false;
    }

    luaL_openlibs(m_l);

    lua_pushlightuserdata(m_l, this);
    lua_setfield(m_l, LUA_REGISTRYINDEX, "__lua_instance");

    // Mirrors LuaRealm::start() on Windows. Each step must succeed before the next one runs: in particular, the user
    // script must never run if a prelude failed, as that could compromise the sandbox.
    const bool ok = [&] {
        // API definitions and stubs; the native functions registered below replace them.
        if (LuaInternal::doStringView(m_l, LuaResources::api())) return false;

        LuaInternal::registerGlobalAPIs(m_l);
        LuaInternal::registerEmuAPIs(m_l);
        LuaInternal::registerMemoryAPIs(m_l);

        if (LuaInternal::doStringView(m_l, LuaResources::inspect())) return false;

        lua_getglobal(m_l, "__mupen_apply_shims");
        if (!lua_isfunction(m_l, -1))
        {
            lua_pushstring(m_l, "__mupen_apply_shims is not defined");
            return false;
        }
        if (lua_pcall(m_l, 0, 0, 0)) return false;
        lua_pushnil(m_l);
        lua_setglobal(m_l, "__mupen_apply_shims");

        if (!m_trusted && LuaInternal::doStringView(m_l, LuaResources::sandbox())) return false;

        return luaL_dofile(m_l, localPath.toUtf8().constData()) == LUA_OK;
    }();

    if (!ok)
    {
        const char *msg = lua_tostring(m_l, -1);
        QString err = QString::fromUtf8(msg ? msg : "Unknown Lua error");
        appendLog(QString("[Error] %1\n").arg(err));
        setLastError(err);
        emit errorOccurred(err);
        stop();
        return false;
    }

    m_running = true;
    emit runningChanged();
    emit started();
    appendLog(tr("Script started: %1\n").arg(name()));
    return true;
}

void LuaInstance::stop()
{
    if (!m_running && !m_l) return;

    if (m_l)
    {
        invokeCallback(LuaCallbackKey::Stop);

        for (auto &[key, refs] : m_callbacks)
        {
            for (int ref : refs)
            {
                luaL_unref(m_l, LUA_REGISTRYINDEX, ref);
            }
        }
        m_callbacks.clear();

        lua_close(m_l);
        m_l = nullptr;
    }

    if (m_running)
    {
        m_running = false;
        emit runningChanged();
        emit stopped();
        appendLog(tr("Script stopped: %1\n").arg(name()));
    }
}

void LuaInstance::restart()
{
    stop();
    start();
}
