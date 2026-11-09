/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInstance.hpp"
#include "LuaAction.hpp"
#include "LuaCanvas.hpp"
#include "LuaPainter.hpp"
#include "LuaWindow.hpp"
#include "EmuContext.hpp"
#include <Core/API.hpp>

#include <QGuiApplication>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStyleHints>
#include <QUrl>
#include <QWindow>

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
    // note: lua_isstring also returns true for numbers, so check integers first
    if (lua_isinteger(L, idx))
    {
        uint32_t val = static_cast<uint32_t>(lua_tointeger(L, idx));
        if ((val & 0xFF000000) != 0)
        {
            return QColor::fromRgba(val);
        }
        return QColor::fromRgb(val);
    }
    else if (lua_isstring(L, idx))
    {
        return QColor::fromString(QString::fromUtf8(lua_tostring(L, idx)));
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
int emu_atloadstate(lua_State *L)
{
    return register_callback_helper(L, LuaCallbackKey::LoadState);
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

int emu_get_speed_mode(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        lua_pushinteger(L, static_cast<int>(ctx->speedMode()));
    }
    else
    {
        lua_pushinteger(L, static_cast<int>(QmlCoreSpeedMode::Normal));
    }
    return 1;
}

int emu_set_speed_mode(lua_State *L)
{
    if (auto *ctx = EmuContext::instance())
    {
        const int mode = static_cast<int>(luaL_checkinteger(L, 1));
        if (mode >= static_cast<int>(QmlCoreSpeedMode::Normal) && mode <= static_cast<int>(QmlCoreSpeedMode::UltraFastForward))
        {
            ctx->setSpeedMode(static_cast<QmlCoreSpeedMode::Value>(mode));
        }
        else
        {
            luaL_error(L, "invalid speed mode: %d", mode);
        }
    }
    return 0;
}

int emu_inputcount(lua_State *L)
{
    auto *inst = getInstance(L);
    lua_pushinteger(L, inst ? static_cast<lua_Integer>(inst->inputCount()) : 0);
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
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    const char *text = luaL_checkstring(L, 3);
    QColor col = parseColor(L, 4);
    inst->activeCanvas()->addText(x, y, QString::fromUtf8(text), col);
    return 0;
}

int wgui_rect(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addRect(x1, y1, x2 - x1, y2 - y1, col);
    return 0;
}

int wgui_fillrect(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addFillRect(x1, y1, x2 - x1, y2 - y1, col);
    return 0;
}

int wgui_line(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x1 = luaL_checknumber(L, 1);
    qreal y1 = luaL_checknumber(L, 2);
    qreal x2 = luaL_checknumber(L, 3);
    qreal y2 = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addLine(x1, y1, x2, y2, col);
    return 0;
}

int wgui_ellipse(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    qreal rx = luaL_checknumber(L, 3);
    qreal ry = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addEllipse(x, y, rx, ry, col);
    return 0;
}

int wgui_fillellipsea(lua_State *L)
{
    auto *inst = getInstance(L);
    if (!inst || !inst->activeCanvas()) return 0;
    qreal x = luaL_checknumber(L, 1);
    qreal y = luaL_checknumber(L, 2);
    qreal rx = luaL_checknumber(L, 3);
    qreal ry = luaL_checknumber(L, 4);
    QColor col = parseColor(L, 5);
    inst->activeCanvas()->addFillEllipse(x, y, rx, ry, col);
    return 0;
}

int wgui_clear(lua_State *L)
{
    auto *inst = getInstance(L);
    if (inst && inst->activeCanvas())
    {
        inst->activeCanvas()->clear();
    }
    return 0;
}

int wgui_info(lua_State *L)
{
    auto *inst = getInstance(L);
    lua_newtable(L);
    if (inst && inst->activeCanvas())
    {
        lua_pushnumber(L, inst->activeCanvas()->width());
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, inst->activeCanvas()->height());
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
    if (m_activeCanvas && m_activeCanvas == m_canvas) m_activeCanvas = canvas;
    m_canvas = canvas;
    if (m_canvas)
    {
        m_canvas->setInstance(this);
        if (m_canvas->window()) setHostWindow(m_canvas->window());
        else
        {
            connect(m_canvas, &QQuickItem::windowChanged, this, [this](QQuickWindow *w) { setHostWindow(w); });
        }
    }
    emit canvasChanged();
}

LuaInstance *LuaInstance::fromState(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__lua_instance");
    auto *inst = static_cast<LuaInstance *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return inst;
}

QWindow *LuaInstance::hostWindow() const
{
    return m_hostWindow;
}

LuaCanvas *LuaInstance::activeCanvas() const
{
    return m_activeCanvas ? m_activeCanvas : m_canvas;
}

void LuaInstance::setActiveCanvas(LuaCanvas *canvas)
{
    m_activeCanvas = canvas;
}

LuaWindow *LuaInstance::mainWindow() const
{
    return m_mainWindow;
}

void LuaInstance::setHostWindow(QWindow *window)
{
    if (m_hostWindow.data() == window) return;
    if (m_hostWindow) m_hostWindow->removeEventFilter(this);
    m_hostWindow = window;
    if (m_hostWindow)
    {
        // keyboard and mouse go to the scripts, as on the Windows host's window procedure
        m_hostWindow->installEventFilter(this);
        connect(m_hostWindow, &QWindow::widthChanged, this, [this]() { repositionWindows(); });
        connect(m_hostWindow, &QWindow::heightChanged, this, [this]() { repositionWindows(); });
        connect(m_hostWindow, &QWindow::xChanged, this, [this]() { repositionWindows(); });
        connect(m_hostWindow, &QWindow::yChanged, this, [this]() { repositionWindows(); });
    }
}

void LuaInstance::repositionWindows()
{
    for (auto &window : m_windows)
    {
        if (window->isMain() && m_hostWindow) window->resizeClient(m_hostWindow->width(), m_hostWindow->height());
        window->reposition();
    }
}

LuaWindow *LuaInstance::findWindow(const QString &id) const
{
    for (const auto &window : m_windows)
    {
        if (!window->isMain() && window->id() == id) return window.get();
    }
    return nullptr;
}

LuaWindow *LuaInstance::addWindow(const QString &id, const QString &anchor, bool closable)
{
    auto window = std::make_unique<LuaWindow>(this);
    window->setId(id);
    window->setAnchor(anchor);
    window->setClosable(closable);
    auto *result = window.get();
    m_windows.push_back(std::move(window));
    return result;
}

void LuaInstance::dropWindow(LuaWindow *window)
{
    std::erase_if(m_windows, [window](const auto &ptr) { return ptr.get() == window; });
}

void LuaInstance::createMainWindow()
{
    auto window = std::make_unique<LuaWindow>(this);
    window->setMain(true);
    window->setId("main");
    // no window handle: size()/resize() go through the canvas, geometry through the host window
    window->m_canvas = m_canvas;
    m_mainWindow = window.get();
    m_windows.push_back(std::move(window));
}

void LuaInstance::closeWindows()
{
    for (auto &window : m_windows)
    {
        window->close();
    }
}

bool LuaInstance::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_hostWindow.data() || !m_l) return false;

    switch (event->type())
    {
        case QEvent::Close:
            // closing the app window ends the script and its windows, as WM_DESTROY does on the Windows host
            stop();
            return false;
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            const bool pressed = event->type() == QEvent::KeyPress;
            const bool ctrl = keyEvent->modifiers().testFlag(Qt::ControlModifier);
            const bool alt = keyEvent->modifiers().testFlag(Qt::AltModifier);
            const bool shift = keyEvent->modifiers().testFlag(Qt::ShiftModifier);
            const std::optional<int> sdlCode = LuaWindowApi::qtKeyToSdlKeycode(keyEvent->key());
            if (LuaAction::dispatchKey(this, qtKeyToVk(keyEvent->key()), sdlCode ? *sdlCode : 0, pressed, ctrl, alt,
                    shift, keyEvent->isAutoRepeat()))
            {
                return true; // an unlocked hotkey matched, scripts don't see the key
            }
            pushKeyTable(*keyEvent);
            if (m_mainWindow) m_mainWindow->call(LuaWindowCallback::Key, [](lua_State *L) { return lua_pcall(L, 1, 0, 0); });
            invokeCallback(LuaCallbackKey::Key, 1);
            return false;
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            // scripts see coordinates relative to the emulator content (window:size space)
            QPointF position = mouseEvent->position();
            if (m_mainWindow && m_mainWindow->canvas())
            {
                position -= m_mainWindow->canvas()->mapToScene(QPointF(0, 0));
                m_mainWindow->setLastMousePosition(position);
            }
            pushMouseTableAt(position, *mouseEvent);
            if (m_mainWindow) m_mainWindow->call(LuaWindowCallback::Mouse, [](lua_State *L) { return lua_pcall(L, 1, 0, 0); });
            invokeCallback(LuaCallbackKey::Mouse, 1);
            return false;
        }
        case QEvent::Wheel:
        {
            auto *wheelEvent = static_cast<QWheelEvent *>(event);
            QPointF position = wheelEvent->position();
            if (m_mainWindow && m_mainWindow->canvas())
            {
                position -= m_mainWindow->canvas()->mapToScene(QPointF(0, 0));
            }
            pushWheelTableAt(position, *wheelEvent);
            if (m_mainWindow) m_mainWindow->call(LuaWindowCallback::Mouse, [](lua_State *L) { return lua_pcall(L, 1, 0, 0); });
            invokeCallback(LuaCallbackKey::Mouse, 1);
            return false;
        }
        default: return false;
    }
}

// Windows Virtual-Key codes, so scripts see the same e.keycode as on the Windows view.
int LuaInstance::qtKeyToVk(int qtKey)
{
    if (qtKey >= 'A' && qtKey <= 'Z') return qtKey; // VK_A..VK_Z match ASCII
    if (qtKey >= '0' && qtKey <= '9') return qtKey;
    switch (qtKey)
    {
        case Qt::Key_Return:
        case Qt::Key_Enter: return 13; // VK_RETURN
        case Qt::Key_Space: return 32; // VK_SPACE
        case Qt::Key_Tab: return 9; // VK_TAB
        case Qt::Key_Backspace: return 8; // VK_BACK
        case Qt::Key_Escape: return 27; // VK_ESCAPE
        case Qt::Key_PageUp: return 33; // VK_PRIOR
        case Qt::Key_PageDown: return 34; // VK_NEXT
        case Qt::Key_End: return 35; // VK_END
        case Qt::Key_Home: return 36; // VK_HOME
        case Qt::Key_Left: return 37; // VK_LEFT
        case Qt::Key_Up: return 38; // VK_UP
        case Qt::Key_Right: return 39; // VK_RIGHT
        case Qt::Key_Down: return 40; // VK_DOWN
        case Qt::Key_Insert: return 45; // VK_INSERT
        case Qt::Key_Delete: return 46; // VK_DELETE
        case Qt::Key_Shift: return 16; // VK_SHIFT
        case Qt::Key_Control: return 17; // VK_CONTROL
        case Qt::Key_Alt: return 18; // VK_MENU
        case Qt::Key_Meta: return 91; // VK_LWIN
        case Qt::Key_CapsLock: return 20; // VK_CAPITAL
        case Qt::Key_Minus: return 189; // VK_OEM_MINUS
        case Qt::Key_Equal: return 187; // VK_OEM_PLUS
        case Qt::Key_BracketLeft: return 219; // VK_OEM_4
        case Qt::Key_BracketRight: return 221; // VK_OEM_6
        default: break;
    }
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24) return 111 + (qtKey - Qt::Key_F1); // VK_F1=112
    return qtKey;
}

void LuaInstance::pushKeyTable(const QKeyEvent &event)
{
    lua_newtable(m_l);
    lua_pushinteger(m_l, qtKeyToVk(event.key()));
    lua_setfield(m_l, -2, "keycode");
    if (auto code2 = LuaWindowApi::qtKeyToSdlKeycode(event.key()))
    {
        lua_pushinteger(m_l, *code2);
        lua_setfield(m_l, -2, "keycode2");
    }
    lua_pushinteger(m_l, event.nativeScanCode());
    lua_setfield(m_l, -2, "rawkeycode");
    lua_pushinteger(m_l, static_cast<qint64>(event.nativeVirtualKey()));
    lua_setfield(m_l, -2, "rawvirtualkeycode");
    lua_pushboolean(m_l, event.type() == QEvent::KeyPress);
    lua_setfield(m_l, -2, "pressed");
    lua_pushboolean(m_l, event.isAutoRepeat());
    lua_setfield(m_l, -2, "repeat");
    const Qt::KeyboardModifiers mods = event.modifiers();
    lua_pushboolean(m_l, mods.testFlag(Qt::ControlModifier));
    lua_setfield(m_l, -2, "ctrl");
    lua_pushboolean(m_l, mods.testFlag(Qt::AltModifier));
    lua_setfield(m_l, -2, "alt");
    lua_pushboolean(m_l, mods.testFlag(Qt::ShiftModifier));
    lua_setfield(m_l, -2, "shift");
    lua_pushboolean(m_l, mods.testFlag(Qt::MetaModifier));
    lua_setfield(m_l, -2, "meta");
    const QString text = event.text();
    if (!text.isEmpty() && text.front().isPrint())
    {
        lua_pushstring(m_l, text.toUtf8().constData());
        lua_setfield(m_l, -2, "text");
    }
}

// ugui and TAS scripts expect 0/1/2 for left/right/middle (like the Windows port), not Qt values.
static int mouseButtonId(Qt::MouseButton button)
{
    switch (button)
    {
        case Qt::LeftButton: return 0;
        case Qt::RightButton: return 1;
        case Qt::MiddleButton: return 2;
        default: return static_cast<int>(button);
    }
}

void LuaInstance::pushMouseTable(const QMouseEvent &event)
{
    pushMouseTableAt(event.position(), event);
}

void LuaInstance::pushMouseTableAt(const QPointF &position, const QMouseEvent &event)
{
    lua_newtable(m_l);
    lua_pushinteger(m_l, static_cast<int>(position.x()));
    lua_setfield(m_l, -2, "x");
    lua_pushinteger(m_l, static_cast<int>(position.y()));
    lua_setfield(m_l, -2, "y");
    const Qt::KeyboardModifiers mods = event.modifiers();
    lua_pushboolean(m_l, mods.testFlag(Qt::ControlModifier));
    lua_setfield(m_l, -2, "ctrl");
    lua_pushboolean(m_l, mods.testFlag(Qt::AltModifier));
    lua_setfield(m_l, -2, "alt");
    lua_pushboolean(m_l, mods.testFlag(Qt::ShiftModifier));
    lua_setfield(m_l, -2, "shift");
    lua_pushboolean(m_l, mods.testFlag(Qt::MetaModifier));
    lua_setfield(m_l, -2, "meta");
    if (event.button() != Qt::NoButton)
    {
        const int button = mouseButtonId(event.button());
        const bool pressed = event.type() != QEvent::MouseButtonRelease;
        // double/triple-click detection, as QPA would synthesize it for the Windows host
        if (pressed)
        {
            const bool same = button == m_lastClickButton
                && (position - m_lastClickPos).manhattanLength() < 6.0;
            const qint64 sinceLast = QDateTime::currentMSecsSinceEpoch() - m_lastClickMs;
            int clickChain = 1;
            // same window as QPA/Windows uses for synthesizing double-clicks
            if (same && sinceLast < 500) clickChain = m_lastClickChain + 1;
            m_lastClickButton = button;
            m_lastClickPos = position;
            m_lastClickMs = QDateTime::currentMSecsSinceEpoch();
            m_lastClickChain = clickChain;
            if (clickChain >= 2)
            {
                lua_pushboolean(m_l, clickChain == 2);
                lua_setfield(m_l, -2, "double_click");
                lua_pushboolean(m_l, clickChain >= 3);
                lua_setfield(m_l, -2, "triple_click");
                if (clickChain >= 3) m_lastClickChain = 0; // the next press starts a fresh triple
            }
        }
        lua_pushinteger(m_l, button);
        lua_setfield(m_l, -2, "button");
        lua_pushboolean(m_l, pressed);
        lua_setfield(m_l, -2, "pressed");
    }
}

void LuaInstance::pushWheelTableAt(const QPointF &position, const QWheelEvent &event)
{
    lua_newtable(m_l);
    lua_pushinteger(m_l, static_cast<int>(position.x()));
    lua_setfield(m_l, -2, "x");
    lua_pushinteger(m_l, static_cast<int>(position.y()));
    lua_setfield(m_l, -2, "y");
    const Qt::KeyboardModifiers mods = event.modifiers();
    lua_pushboolean(m_l, mods.testFlag(Qt::ControlModifier));
    lua_setfield(m_l, -2, "ctrl");
    lua_pushboolean(m_l, mods.testFlag(Qt::AltModifier));
    lua_setfield(m_l, -2, "alt");
    lua_pushboolean(m_l, mods.testFlag(Qt::ShiftModifier));
    lua_setfield(m_l, -2, "shift");
    lua_pushboolean(m_l, mods.testFlag(Qt::MetaModifier));
    lua_setfield(m_l, -2, "meta");
    int dx = event.pixelDelta().x() ? event.pixelDelta().x() : event.angleDelta().x();
    int dy = event.pixelDelta().y() ? event.pixelDelta().y() : event.angleDelta().y();
    if (dx != 0)
    {
        lua_pushinteger(m_l, dx);
        lua_setfield(m_l, -2, "x_wheel");
    }
    if (dy != 0)
    {
        // the contract wants a positive value when scrolling down; Qt's delta is positive upward
        lua_pushinteger(m_l, -dy);
        lua_setfield(m_l, -2, "y_wheel");
    }
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

void LuaInstance::dispatchInput(int controllerIndex)
{
    if (!m_l || !m_running) return;
    ++m_inputCount;
    // the official API passes a 0-based controller index (see emu.atinput)
    lua_pushinteger(m_l, controllerIndex);
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
    Qt::KeyboardModifiers mods = Qt::NoModifier;
    if (ctrl) mods |= Qt::ControlModifier;
    if (alt) mods |= Qt::AltModifier;
    if (shift) mods |= Qt::ShiftModifier;
    const QKeyEvent event(pressed ? QEvent::KeyPress : QEvent::KeyRelease, key, mods);
    pushKeyTable(event);
    invokeCallback(LuaCallbackKey::Key, 1);
}

void LuaInstance::dispatchMouse(int x, int y, int button, bool pressed)
{
    if (!m_l || !m_running) return;
    const QEvent::Type type = pressed ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease;
    const QMouseEvent event(type, QPointF(x, y), QPointF(x, y), static_cast<Qt::MouseButton>(button),
        static_cast<Qt::MouseButton>(button), Qt::NoModifier);
    pushMouseTable(event);
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
        {"atloadstate", emu_atloadstate},
        {"framecount", emu_framecount},
        {"samplecount", emu_samplecount},
        {"inputcount", emu_inputcount},
        {"pause", emu_pause},
        {"getpause", emu_getpause},
        {"speed", emu_speed},
        {"getspeed", emu_getspeed},
        {"get_speed_mode", emu_get_speed_mode},
        {"set_speed_mode", emu_set_speed_mode},
        {"get_hint", emu_get_hint},
        {"set_hint", emu_set_hint},
        {"get_pref", emu_get_pref},
        {"create_window", LuaWindowApi::create},
        {"main_window", LuaWindowApi::main},
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

void LuaInstance::pushJoypad(lua_State *L, int port)
{
    CoreButtons buttons{};
    if (auto *ctx = EmuContext::instance())
    {
        buttons = ctx->luaControllerInput(port);
    }

    lua_newtable(L);
#define P(a, s)                                                                                                        \
    lua_pushboolean(L, buttons.a);                                                                                     \
    lua_setfield(L, -2, s)
    P(dr, "right");
    P(dl, "left");
    P(dd, "down");
    P(du, "up");
    P(start, "start");
    P(z, "Z");
    P(b, "B");
    P(a, "A");
    P(cr, "Cright");
    P(cl, "Cleft");
    P(cd, "Cdown");
    P(cu, "Cup");
    P(r, "R");
    P(l, "L");
#undef P
    lua_pushinteger(L, buttons.y);
    lua_setfield(L, -2, "Y");
    lua_pushinteger(L, buttons.x);
    lua_setfield(L, -2, "X");
}

void LuaInstance::setJoypad(lua_State *L, int port)
{
    CoreButtons keys{};
#define P(a, s)                                                                                                        \
    lua_getfield(L, -1, s);                                                                                            \
    keys.a = static_cast<unsigned>(lua_toboolean(L, -1));                                                              \
    lua_pop(L, 1)
    P(dr, "right");
    P(dl, "left");
    P(dd, "down");
    P(du, "up");
    P(start, "start");
    P(z, "Z");
    P(b, "B");
    P(a, "A");
    P(cr, "Cright");
    P(cl, "Cleft");
    P(cd, "Cdown");
    P(cu, "Cup");
    P(r, "R");
    P(l, "L");
#undef P
    lua_getfield(L, -1, "Y");
    keys.y = static_cast<signed>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "X");
    keys.x = static_cast<signed>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    if (auto *ctx = EmuContext::instance())
    {
        ctx->luaSetControllerInput(port, keys);
    }
}

void LuaInstance::registerJoypadAPIs()
{
    lua_newtable(m_l);

    lua_pushcfunction(m_l, [](lua_State *L) -> int {
        auto *inst = getInstance(L);
        if (!inst) return 0;
        int port = static_cast<int>(luaL_optinteger(L, 1, 1)) - 1;
        if (port < 0 || port >= 4) return luaL_error(L, "port: 1-4");
        inst->pushJoypad(L, port);
        return 1;
    });
    lua_setfield(m_l, -2, "get");

    lua_pushcfunction(m_l, [](lua_State *L) -> int {
        auto *inst = getInstance(L);
        if (!inst) return 0;
        int port;
        if (lua_type(L, 1) == LUA_TTABLE)
        {
            port = 0;
        }
        else
        {
            port = static_cast<int>(luaL_optinteger(L, 1, 1)) - 1;
        }
        if (port < 0 || port >= 4) return luaL_error(L, "port: 1-4");
        if (!lua_istable(L, -1)) return luaL_error(L, "table expected");
        inst->setJoypad(L, port);
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
    static const luaL_Reg painterFuncs[] = {
        {"current", LuaPainter::mod_current},
        {"new_image", LuaPainter::mod_new_image},
        {"load_image", LuaPainter::mod_load_image},
        {"decode_image", LuaPainter::mod_decode_image},
        {"measure_text", LuaPainter::mod_measure_text},
        {"hittest_text_position", LuaPainter::mod_hittest_text_position},
        {"hittest_text_index", LuaPainter::mod_hittest_text_index},
        {"hitest_text_index", LuaPainter::mod_hittest_text_index}, // historical alias
        {"get_target_fps", LuaPainter::mod_get_target_fps},
        {"set_target_fps", LuaPainter::mod_set_target_fps},
        {nullptr, nullptr},
    };
    luaL_setfuncs(m_l, painterFuncs, 0);
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

    // a new attempt starts clean: stale errors must not linger in the UI
    setLastError(QString());
    m_inputCount = 0;

    if (m_path.isEmpty())
    {
        QString err = tr("Script path is empty");
        appendLog(err + "\n");
        setLastError(err);
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
    Q_INIT_RESOURCE(lua_api_prelude);
    // same prelude the Windows host embeds: API stubs (overwritten by the natives below)
    // plus the deprecated-compatibility shims scripts like SM64LuaRedux rely on
    if (QFile preludeFile(":/mupen/api.lua"); preludeFile.open(QIODevice::ReadOnly))
    {
        const QByteArray prelude = preludeFile.readAll();
        if (luaL_dostring(m_l, prelude.constData()) != LUA_OK)
        {
            const char *msg = lua_tostring(m_l, -1);
            QString err = QString("API prelude failed: %1").arg(msg ? msg : "unknown error");
            appendLog(err + "\n");
            setLastError(err);
            lua_pop(m_l, 1);
            lua_close(m_l);
            m_l = nullptr;
            return false;
        }
    }
    else
    {
        appendLog("API prelude not found: :/mupen/api.lua\n");
    }

    lua_pushlightuserdata(m_l, this);
    lua_setfield(m_l, LUA_REGISTRYINDEX, "__lua_instance");

    registerGlobalAPIs();
    registerEmuAPIs();
    registerMemoryAPIs();
    registerJoypadAPIs();
    registerGuiAPIs();
    LuaPainter::registerTypes(m_l);
    LuaWindowApi::registerTypes(m_l);
    LuaWindowApi::registerMupenGlobals(m_l);
    LuaAction::registerInstance(m_l, this);

    // the shims run against the registered natives, exactly like on the Windows host
    // the shims run against the registered natives, exactly like on the Windows host
    if (lua_getglobal(m_l, "__mupen_apply_shims") != LUA_TFUNCTION)
    {
        lua_pop(m_l, 1);
    }
    else if (lua_pcall(m_l, 0, 0, 0) != LUA_OK)
    {
        const char *msg = lua_tostring(m_l, -1);
        appendLog(QString("API shims failed: %1\n").arg(msg ? msg : "unknown error"));
        lua_pop(m_l, 1);
    }
    lua_pushnil(m_l);
    lua_setglobal(m_l, "__mupen_apply_shims");

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

        // script-owned windows go away with the script (their closed callbacks run first)
        closeWindows();

        for (auto &[key, refs] : m_callbacks)
        {
            for (int ref : refs)
            {
                luaL_unref(m_l, LUA_REGISTRYINDEX, ref);
            }
        }
        m_callbacks.clear();

        LuaAction::unregisterInstance(this); // unrefs action callbacks while the state is alive
        lua_close(m_l);
        m_l = nullptr;
    }

    m_windows.clear();
    m_mainWindow = nullptr;
    m_activeCanvas = nullptr;

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
