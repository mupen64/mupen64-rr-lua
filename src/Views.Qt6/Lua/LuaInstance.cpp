/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInstance.hpp"
#include "LuaCanvas.hpp"
#include "EmuContext.hpp"
#include <Core/API.hpp>

#include <QGuiApplication>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QStyleHints>
#include <QUrl>

namespace
{

LuaInstance *getInstance(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__lua_instance");
    auto *inst = static_cast<LuaInstance *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return inst;
}

QColor parseColor(lua_State *L, int idx)
{
    if (lua_isstring(L, idx))
    {
        return QColor::fromString(QString::fromUtf8(lua_tostring(L, idx)));
    }
    else if (lua_isinteger(L, idx))
    {
        uint32_t val = static_cast<uint32_t>(lua_tointeger(L, idx));
        if ((val & 0xFF000000) != 0)
        {
            return QColor::fromRgba(val);
        }
        return QColor::fromRgb(val);
    }
    else if (lua_istable(L, idx))
    {
        lua_rawgeti(L, idx, 1);
        int r = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 0;
        lua_pop(L, 1);
        lua_rawgeti(L, idx, 2);
        int g = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 0;
        lua_pop(L, 1);
        lua_rawgeti(L, idx, 3);
        int b = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 0;
        lua_pop(L, 1);
        lua_rawgeti(L, idx, 4);
        int a = lua_isnumber(L, -1) ? static_cast<int>(lua_tointeger(L, -1)) : 255;
        lua_pop(L, 1);
        return QColor(r, g, b, a);
    }
    return QColor(Qt::white);
}

int lua_global_print(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst) return 0;

    int n = lua_gettop(L);
    QString out;
    for (int i = 1; i <= n; ++i)
    {
        if (i > 1) out += '\t';
        size_t len = 0;
        const char *s = luaL_tolstring(L, i, &len);
        if (s) out += QString::fromUtf8(s, static_cast<qsizetype>(len));
        lua_pop(L, 1);
    }
    out += '\n';
    inst->appendLog(out);
    return 0;
}

int register_callback_helper(lua_State *L, LuaCallbackKey key)
{
    auto *inst = getInstance(L);
    if (!inst) return 0;

    if (!lua_isfunction(L, 1))
    {
        return luaL_error(L, "callback must be a function");
    }

    bool unregister = lua_toboolean(L, 2);
    if (unregister)
    {
        inst->unregisterCallback(key, 1);
    }
    else
    {
        inst->registerCallback(key, 1);
    }
    return 0;
}

int emu_atvi(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::VI);
}
int emu_atupdatescreen(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::UpdateScreen);
}
int emu_atpaint(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::Paint);
}
int emu_atinput(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::Input);
}
int emu_atstop(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::Stop);
}
int emu_atinterval(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::Interval);
}
int emu_atreset(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::Reset);
}
int emu_atkey(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::Key);
}
int emu_atmouse(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::Mouse);
}

int emu_framecount(lua_State *L)
{
    auto *ctx = EmuContext::rawContext();
    if (ctx && ctx->vcr_get_current_vi)
    {
        lua_pushinteger(L, ctx->vcr_get_current_vi());
    }
    else
    {
        lua_pushinteger(L, 0);
    }
    return 1;
}

int emu_samplecount(lua_State *L)
{
    auto *ctx = EmuContext::rawContext();
    if (ctx && ctx->vcr_get_seek_info)
    {
        lua_pushinteger(L, ctx->vcr_get_seek_info().current_sample);
    }
    else
    {
        lua_pushinteger(L, 0);
    }
    return 1;
}

int emu_pause(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        bool p = lua_toboolean(L, 1);
        ctx->setPaused(p);
    }
    return 0;
}

int emu_getpause(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        lua_pushboolean(L, ctx->isPaused());
    }
    else
    {
        lua_pushboolean(L, false);
    }
    return 1;
}

int emu_speed(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        int spd = static_cast<int>(luaL_checkinteger(L, 1));
        ctx->setSpeedModifier(spd);
    }
    return 0;
}

int emu_getspeed(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        lua_pushinteger(L, ctx->speedModifier());
    }
    else
    {
        lua_pushinteger(L, 100);
    }
    return 1;
}

int emu_get_hint(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst) return 0;
    const char *key = luaL_checkstring(L, 1);
    std::string val = inst->queryHint(key);
    lua_pushstring(L, val.c_str());
    return 1;
}

int emu_set_hint(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst) return 0;
    const char *key = luaL_checkstring(L, 1);
    const char *val = luaL_optstring(L, 2, "");
    inst->setHint(key, val);
    return 0;
}

int emu_get_pref(lua_State *L)
{
    const char *pref = luaL_checkstring(L, 1);
    if (std::strcmp(pref, "theme") == 0)
    {
        bool isDark = qGuiApp ? (qGuiApp->palette().color(QPalette::Window).lightness() < 128) : true;
        lua_pushstring(L, isDark ? "dark" : "light");
        return 1;
    }
    else if (std::strcmp(pref, "accent_color") == 0)
    {
        QColor accent = qGuiApp ? qGuiApp->palette().color(QPalette::Highlight) : QColor(0, 120, 215);
        lua_createtable(L, 0, 4);
        lua_pushnumber(L, accent.redF());
        lua_setfield(L, -2, "r");
        lua_pushnumber(L, accent.greenF());
        lua_setfield(L, -2, "g");
        lua_pushnumber(L, accent.blueF());
        lua_setfield(L, -2, "b");
        lua_pushnumber(L, accent.alphaF());
        lua_setfield(L, -2, "a");
        return 1;
    }
    else if (std::strcmp(pref, "reduced_motion") == 0)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    else if (std::strcmp(pref, "caret_blink_time") == 0)
    {
        lua_pushinteger(L, qGuiApp ? qGuiApp->styleHints()->cursorFlashTime() : 500);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

template <typename T> int memory_read(lua_State *L)
{
    uint32_t addr = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    auto *ctx = EmuContext::rawContext();
    if (!ctx || !ctx->rdram)
    {
        return luaL_error(L, "Core or RDRAM not available");
    }
    auto val = core_rdram_load<T>((uint8_t *)ctx->rdram, addr);
    if (!val.has_value())
    {
        return luaL_error(L, "address out of bounds");
    }
    if constexpr (std::is_floating_point_v<T>)
    {
        lua_pushnumber(L, static_cast<lua_Number>(val.value()));
    }
    else
    {
        lua_pushinteger(L, static_cast<lua_Integer>(val.value()));
    }
    return 1;
}

template <typename T> int memory_write(lua_State *L)
{
    uint32_t addr = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    T val{};
    if constexpr (std::is_floating_point_v<T>)
    {
        val = static_cast<T>(luaL_checknumber(L, 2));
    }
    else
    {
        val = static_cast<T>(luaL_checkinteger(L, 2));
    }

    auto *ctx = EmuContext::rawContext();
    if (!ctx || !ctx->rdram)
    {
        return luaL_error(L, "Core or RDRAM not available");
    }
    bool ok = core_rdram_store<T>((uint8_t *)ctx->rdram, addr, val);
    if (!ok)
    {
        return luaL_error(L, "address out of bounds");
    }
    return 0;
}

int wgui_text(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->canvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    const char *text = luaL_checkstring(L, 3);
    QColor col = parseColor(L, 4);
    inst->canvas()->addText(x, y, QString::fromUtf8(text), col);
    return 0;
}

int wgui_rect(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->canvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->canvas()->addRect(x1, y1, x2 - x1, y2 - y1, col);
    return 0;
}

int wgui_fillrect(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->canvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->canvas()->addFillRect(x1, y1, x2 - x1, y2 - y1, col);
    return 0;
}

int wgui_line(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->canvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->canvas()->addLine(x1, y1, x2, y2, col);
    return 0;
}

int wgui_ellipse(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->canvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    qreal rx = luaL_checknumber(L, 3);
    qreal ry = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->canvas()->addEllipse(x, y, rx, ry, col);
    return 0;
}

int wgui_fillellipsea(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->canvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    qreal rx = luaL_checknumber(L, 3);
    qreal ry = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->canvas()->addFillEllipse(x, y, rx, ry, col);
    return 0;
}

int wgui_clear(lua_State *L)
{
    auto *inst = getInstance(L);
    if (inst && inst->canvas())
    {
        inst->canvas()->clear();
    }
    return 0;
}

int wgui_info(lua_State *L)
{
    auto *inst = getInstance(L);
    lua_newtable(L);
    if (inst && inst->canvas())
    {
        lua_pushnumber(L, inst->canvas()->width());
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, inst->canvas()->height());
        lua_setfield(L, -2, "height");
    }
    else
    {
        lua_pushnumber(L, 640);
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, 480);
        lua_setfield(L, -2, "height");
    }
    return 1;
}

} // namespace

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

LuaCanvas *LuaInstance::canvas() const
{
    return m_canvas;
}

void LuaInstance::setCanvas(LuaCanvas *canvas)
{
    if (m_canvas == canvas) return;
    m_canvas = canvas;
    if (m_canvas)
    {
        m_canvas->setInstance(this);
    }
    emit canvasChanged();
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

void LuaInstance::dispatchVI()
{
    invokeCallback(LuaCallbackKey::VI);
}

void LuaInstance::dispatchUpdateScreen()
{
    invokeCallback(LuaCallbackKey::UpdateScreen);
}

void LuaInstance::dispatchPaint()
{
    if (m_canvas) m_canvas->clear();
    invokeCallback(LuaCallbackKey::Paint);
    if (m_canvas) m_canvas->requestPaint();
}

void LuaInstance::dispatchInput(CoreButtons *input, int controllerIndex)
{
    if (!m_l || !m_running) return;
    lua_pushinteger(m_l, controllerIndex + 1);
    invokeCallback(LuaCallbackKey::Input, 1);
}

void LuaInstance::dispatchInterval()
{
    invokeCallback(LuaCallbackKey::Interval);
}

void LuaInstance::dispatchReset()
{
    invokeCallback(LuaCallbackKey::Reset);
}

void LuaInstance::dispatchSaveState()
{
    invokeCallback(LuaCallbackKey::SaveState);
}

void LuaInstance::dispatchLoadState()
{
    invokeCallback(LuaCallbackKey::LoadState);
}

void LuaInstance::dispatchKey(int key, bool pressed, bool ctrl, bool alt, bool shift)
{
    if (!m_l || !m_running) return;
    lua_newtable(m_l);
    lua_pushinteger(m_l, key);
    lua_setfield(m_l, -2, "keycode");
    lua_pushboolean(m_l, pressed);
    lua_setfield(m_l, -2, "pressed");
    lua_pushboolean(m_l, ctrl);
    lua_setfield(m_l, -2, "ctrl");
    lua_pushboolean(m_l, alt);
    lua_setfield(m_l, -2, "alt");
    lua_pushboolean(m_l, shift);
    lua_setfield(m_l, -2, "shift");
    invokeCallback(LuaCallbackKey::Key, 1);
}

void LuaInstance::dispatchMouse(int x, int y, int button, bool pressed)
{
    if (!m_l || !m_running) return;
    lua_newtable(m_l);
    lua_pushinteger(m_l, x);
    lua_setfield(m_l, -2, "x");
    lua_pushinteger(m_l, y);
    lua_setfield(m_l, -2, "y");
    lua_pushinteger(m_l, button);
    lua_setfield(m_l, -2, "button");
    lua_pushboolean(m_l, pressed);
    lua_setfield(m_l, -2, "pressed");
    invokeCallback(LuaCallbackKey::Mouse, 1);
}

void LuaInstance::registerGlobalAPIs()
{
    lua_register(m_l, "print", lua_global_print);
    lua_register(m_l, "console", lua_global_print);
}

void LuaInstance::registerEmuAPIs()
{
    lua_newtable(m_l);

    const luaL_Reg emuFuncs[] = {
        {"atvi", emu_atvi},
        {"atupdatescreen", emu_atupdatescreen},
        {"atpaint", emu_atpaint},
        {"atinput", emu_atinput},
        {"atstop", emu_atstop},
        {"atinterval", emu_atinterval},
        {"atreset", emu_atreset},
        {"atkey", emu_atkey},
        {"atmouse", emu_atmouse},
        {"framecount", emu_framecount},
        {"samplecount", emu_samplecount},
        {"pause", emu_pause},
        {"getpause", emu_getpause},
        {"speed", emu_speed},
        {"getspeed", emu_getspeed},
        {"get_hint", emu_get_hint},
        {"set_hint", emu_set_hint},
        {"get_pref", emu_get_pref},
        {nullptr, nullptr},
    };

    luaL_setfuncs(m_l, emuFuncs, 0);
    lua_setglobal(m_l, "emu");
}

void LuaInstance::registerMemoryAPIs()
{
    lua_newtable(m_l);

    const luaL_Reg memFuncs[] = {
        {"readbyte", memory_read<uint8_t>},
        {"readbytesigned", memory_read<int8_t>},
        {"readword", memory_read<uint16_t>},
        {"readwordsigned", memory_read<int16_t>},
        {"readdword", memory_read<uint32_t>},
        {"readdwordsigned", memory_read<int32_t>},
        {"readfloat", memory_read<float>},
        {"readdouble", memory_read<double>},
        {"writebyte", memory_write<uint8_t>},
        {"writeword", memory_write<uint16_t>},
        {"writedword", memory_write<uint32_t>},
        {"writefloat", memory_write<float>},
        {"writedouble", memory_write<double>},
        {nullptr, nullptr},
    };

    luaL_setfuncs(m_l, memFuncs, 0);
    lua_setglobal(m_l, "memory");
}

void LuaInstance::registerJoypadAPIs()
{
    lua_newtable(m_l);

    lua_pushcfunction(m_l, [](lua_State *L) -> int {
        int port = static_cast<int>(luaL_optinteger(L, 1, 1)) - 1;
        if (port < 0 || port >= 4) return luaL_error(L, "port must be 1-4");
        lua_newtable(L);
        return 1;
    });
    lua_setfield(m_l, -2, "get");

    lua_pushcfunction(m_l, [](lua_State *L) -> int {
        int port = static_cast<int>(luaL_checkinteger(L, 1)) - 1;
        if (port < 0 || port >= 4) return luaL_error(L, "port must be 1-4");
        if (!lua_istable(L, 2)) return luaL_error(L, "table expected");
        return 0;
    });
    lua_setfield(m_l, -2, "set");

    lua_setglobal(m_l, "joypad");
}

void LuaInstance::registerGuiAPIs()
{
    lua_newtable(m_l);
    const luaL_Reg wguiFuncs[] = {
        {"text", wgui_text},
        {"drawtext", wgui_text},
        {"rect", wgui_rect},
        {"fillrect", wgui_fillrect},
        {"line", wgui_line},
        {"ellipse", wgui_ellipse},
        {"fillellipsea", wgui_fillellipsea},
        {"fillrecta", wgui_fillrect},
        {"clear", wgui_clear},
        {"info", wgui_info},
        {nullptr, nullptr},
    };
    luaL_setfuncs(m_l, wguiFuncs, 0);
    lua_setglobal(m_l, "wgui");

    lua_newtable(m_l);
    lua_pushcfunction(m_l, [](lua_State *L) -> int {
        lua_newtable(L);
        lua_pushcfunction(L, wgui_clear);
        lua_setfield(L, -2, "clear");
        lua_pushcfunction(L, wgui_fillrect);
        lua_setfield(L, -2, "fill_rect");
        lua_pushcfunction(L, wgui_rect);
        lua_setfield(L, -2, "stroke_rect");
        lua_pushcfunction(L, wgui_line);
        lua_setfield(L, -2, "draw_line");
        lua_pushcfunction(L, wgui_text);
        lua_setfield(L, -2, "draw_text");
        return 1;
    });
    lua_setfield(m_l, -2, "current");
    lua_setglobal(m_l, "painter");
}

void LuaInstance::applySandbox()
{
    const char *sandboxScript = R"(
        os.execute = nil
        os.remove = nil
        os.rename = nil
        os.tmpname = nil
        os.exit = nil
        io = nil
        package.loadlib = nil
        package.cpath = nil
        debug = nil
    )";
    luaL_dostring(m_l, sandboxScript);
}

bool LuaInstance::start()
{
    if (m_running) return false;

    if (m_path.isEmpty())
    {
        setLastError(tr("Script path is empty"));
        return false;
    }

    QString localPath = m_path;
    if (localPath.startsWith("file:///"))
    {
        localPath = QUrl(localPath).toLocalFile();
    }

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

    registerGlobalAPIs();
    registerEmuAPIs();
    registerMemoryAPIs();
    registerJoypadAPIs();
    registerGuiAPIs();

    if (!m_trusted)
    {
        applySandbox();
    }

    int status = luaL_dofile(m_l, localPath.toUtf8().constData());
    if (status != LUA_OK)
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

    if (m_canvas)
    {
        m_canvas->clear();
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
