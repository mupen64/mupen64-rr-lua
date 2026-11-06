/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "LuaWindow.hpp"

#include "LuaCanvas.hpp"
#include "LuaInstance.hpp"

#include <Core/Types.hpp>

#include <QCursor>
#include <QColor>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QQuickWindow>
#include <QTimer>

#include <algorithm>

namespace
{
constexpr const char *WINDOW_MT = "Mupen.Window";

LuaWindow *check_window(lua_State *L, bool allowClosed = false)
{
    auto *window = *static_cast<LuaWindow **>(luaL_checkudata(L, 1, WINDOW_MT));
    if (window->isClosed() && !allowClosed) luaL_error(L, "window is closed");
    return window;
}

// Returns the window userdata for this window, creating it once (keyed by pointer).
void push_window(lua_State *L, LuaWindow *window)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, window);
    if (!lua_isnil(L, -1)) return;
    lua_pop(L, 1);
    auto **value = static_cast<LuaWindow **>(lua_newuserdata(L, sizeof(LuaWindow *)));
    *value = window;
    luaL_setmetatable(L, WINDOW_MT);
    lua_newtable(L);
    lua_setuservalue(L, -2);
    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, window);
}

// window:atpaint/atkey/atmouse/closed(callback, unregister?)
int subscribe(lua_State *L, LuaWindowCallback kind)
{
    auto *window = check_window(L, kind == LuaWindowCallback::Closed && lua_toboolean(L, 3));
    luaL_checktype(L, 2, LUA_TFUNCTION);
    const bool unregister = lua_toboolean(L, 3);
    lua_getuservalue(L, 1);
    lua_rawgeti(L, -1, static_cast<int>(kind));
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_rawseti(L, -3, static_cast<int>(kind));
    }
    const auto count = static_cast<int>(lua_rawlen(L, -1));
    if (unregister)
    {
        for (int i = 1; i <= count; ++i)
        {
            lua_rawgeti(L, -1, i);
            const bool match = lua_rawequal(L, -1, 2);
            lua_pop(L, 1);
            if (!match) continue;
            for (int j = i; j < count; ++j)
            {
                lua_rawgeti(L, -1, j + 1);
                lua_rawseti(L, -2, j);
            }
            lua_pushnil(L);
            lua_rawseti(L, -2, count);
            return 0;
        }
        return luaL_error(L, "callback is not registered on this window");
    }
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, count + 1);
    if (kind == LuaWindowCallback::Paint) window->ensurePaintTimer();
    return 0;
}

int window_atpaint(lua_State *L)
{
    return subscribe(L, LuaWindowCallback::Paint);
}
int window_atkey(lua_State *L)
{
    return subscribe(L, LuaWindowCallback::Key);
}
int window_atmouse(lua_State *L)
{
    return subscribe(L, LuaWindowCallback::Mouse);
}
int window_closed(lua_State *L)
{
    return subscribe(L, LuaWindowCallback::Closed);
}

int window_size(lua_State *L)
{
    auto *window = check_window(L);
    int width = 0;
    int height = 0;
    // the main window reports its canvas (the emulator content), not the window frame
    if (window->isMain() && window->canvas())
    {
        width = static_cast<int>(window->canvas()->width());
        height = static_cast<int>(window->canvas()->height());
    }
    else if (QQuickWindow *handle = window->windowHandle())
    {
        width = handle->width();
        height = handle->height();
    }
    else
    {
        return luaL_error(L, "failed to query window size");
    }
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, width);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, height);
    lua_setfield(L, -2, "y");
    return 1;
}

int window_mouse_position(lua_State *L)
{
    auto *window = check_window(L);
    const QPoint position = window->lastMousePosition().toPoint();
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, position.x());
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, position.y());
    lua_setfield(L, -2, "y");
    return 1;
}

int window_resize(lua_State *L)
{
    auto *window = check_window(L);
    const auto width = static_cast<int>(std::clamp<lua_Integer>(luaL_checkinteger(L, 2), 1, 32767));
    const auto height = static_cast<int>(std::clamp<lua_Integer>(luaL_checkinteger(L, 3), 1, 32767));
    window->resizeClient(width, height);
    return 0;
}

bool window_option(lua_State *L, const char *name, bool fallback)
{
    lua_getfield(L, 1, name);
    const bool value = lua_isnil(L, -1) ? fallback : lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return value;
}

// "left" | "right" | "top" | "bottom", with the optional "_full" suffix stripped.
QString anchor_side(const QString &anchor)
{
    QString side = anchor;
    if (side.endsWith("_full")) side.chop(5);
    return side;
}
} // namespace

LuaWindow::LuaWindow(LuaInstance *instance) :
    m_instance(instance) {}

LuaWindow::~LuaWindow()
{
    stopPaintTimer();
    if (!m_main && m_window)
    {
        m_window->close();
        m_window->deleteLater();
    }
    if (!m_main && m_canvas) m_canvas->deleteLater();
}

QQuickWindow *LuaWindow::windowHandle() const
{
    return m_window;
}

bool LuaWindow::create(const QSize &size, bool minimizable, bool maximizable)
{
    if (m_main || m_window) return m_window != nullptr;
    auto *view = new QQuickWindow();
    view->setTitle(m_id);
    view->setColor(Qt::black);
    view->setFlags(Qt::Window);
    Qt::WindowFlags flags = view->flags();
    if (!minimizable) flags &= ~Qt::WindowMinimizeButtonHint;
    if (maximizable) flags |= Qt::WindowMaximizeButtonHint;
    if (!m_closable) flags &= ~Qt::WindowCloseButtonHint;
    view->setFlags(flags);

    m_canvas = new LuaCanvas();
    m_canvas->setInstance(m_instance);
    m_canvas->setParentItem(view->contentItem());
    // the drawable area follows the client size from the start, so window:size() and the paint surface never differ
    m_canvas->setSize(QSizeF(size));

    m_window = view;
    view->installEventFilter(this);
    view->resize(size);
    view->show();
    return true;
}

void LuaWindow::resizeClient(int width, int height)
{
    if (m_window)
    {
        m_updatingAnchor = true;
        m_window->resize(width, height);
        if (m_canvas) m_canvas->setSize(QSizeF(width, height));
        reposition();
        m_updatingAnchor = false;
        return;
    }
    if (m_main && m_canvas)
    {
        m_canvas->setSize(QSizeF(width, height));
        m_canvas->requestPaint();
    }
}

bool LuaWindow::call(LuaWindowCallback kind, const std::function<int(lua_State *)> &invoke)
{
    if (m_closed && kind != LuaWindowCallback::Closed) return true;
    lua_State *L = m_instance->state();
    if (!L) return true;

    const int top = lua_gettop(L);
    lua_rawgetp(L, LUA_REGISTRYINDEX, this);
    if (lua_isnil(L, -1))
    {
        lua_settop(L, top);
        return true;
    }
    lua_getuservalue(L, -1);
    lua_rawgeti(L, -1, static_cast<int>(kind));
    if (lua_isnil(L, -1))
    {
        lua_settop(L, top);
        return true;
    }
    const int count = static_cast<int>(lua_rawlen(L, -1));
    lua_newtable(L);
    for (int i = 1; i <= count; ++i)
    {
        lua_rawgeti(L, -2, i);
        lua_rawseti(L, -2, i);
    }

    // wgui/painter primitives draw onto this window while its callbacks run
    LuaCanvas *previous = m_instance->activeCanvas();
    if (kind == LuaWindowCallback::Paint && m_canvas) m_instance->setActiveCanvas(m_canvas);
    bool success = true;
    for (int i = 1; i <= count && (!m_closed || kind == LuaWindowCallback::Closed); ++i)
    {
        lua_rawgeti(L, -1, i);
        // Key/Mouse callers pushed the event table as the entry's topmost value: put it above the callback
        if (kind == LuaWindowCallback::Key || kind == LuaWindowCallback::Mouse) lua_pushvalue(L, top);
        if (invoke(L) == LUA_OK) continue;
        const char *error = lua_tostring(L, -1);
        m_instance->appendLog(QString("[Error window '%1'] %2\n").arg(m_id, error ? error : "Lua window callback failed"));
        success = false;
        break;
    }
    m_instance->setActiveCanvas(previous);
    lua_settop(L, top);
    return success;
}

void LuaWindow::paintFrame()
{
    if (!m_canvas || m_closed) return;
    // the paint surface follows the client area, so the size the script reads is the size it draws on
    if (!m_main && m_window && (m_canvas->width() != m_window->width() || m_canvas->height() != m_window->height()))
    {
        m_canvas->setSize(QSizeF(m_window->width(), m_window->height()));
    }
    m_canvas->clear();
    if (!call(LuaWindowCallback::Paint, [](lua_State *L) { return lua_pcall(L, 0, 0, 0); })) return;
    m_canvas->requestPaint();
}

void LuaWindow::ensurePaintTimer()
{
    if (m_paintTimer || m_closed || !m_canvas) return;
    // keep the window redrawing while paint callbacks are registered
    auto *timer = new QTimer(this);
    timer->setInterval(16);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (!m_closed && m_instance->state()) paintFrame();
    });
    m_paintTimer = timer;
    timer->start();
}

void LuaWindow::reposition()
{
    if (m_updatingAnchor || m_closed || m_main || m_anchor.isEmpty() || !m_window) return;
    QWindow *host = m_instance->hostWindow();
    if (!host) return;
    const QRect mainRect = host->frameGeometry();
    // sizes are read and written in client terms (what window:size and resizeClient speak): taking them from
    // frameGeometry would feed the title bar back into the client size and grow the window at each re-anchor
    QRect rect = m_window->geometry();

    const QString side = anchor_side(m_anchor);
    int width = rect.width();
    int height = rect.height();
    if (m_anchor.endsWith("_full"))
    {
        if (side == "left" || side == "right") height = mainRect.height();
        else width = mainRect.width();
    }
    int x = mainRect.left();
    int y = mainRect.top();
    if (side == "right") x = mainRect.right() + 1;
    if (side == "left") x -= width;
    if (side == "bottom") y = mainRect.bottom() + 1;
    if (side == "top") y -= height;

    m_updatingAnchor = true;
    m_window->setGeometry(x, y, width, height);
    m_updatingAnchor = false;
}

bool LuaWindow::close()
{
    if (m_closed) return true;
    m_closed = true;
    stopPaintTimer();
    if (!m_main && m_window) m_window->hide();
    return call(LuaWindowCallback::Closed, [](lua_State *L) { return lua_pcall(L, 0, 0, 0); });
}

void LuaWindow::stopPaintTimer()
{
    if (m_paintTimer)
    {
        m_paintTimer->stop();
        m_paintTimer->deleteLater();
        m_paintTimer = nullptr;
    }
}

bool LuaWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_window || m_closed) return false;

    switch (event->type())
    {
        case QEvent::Resize:
            if (m_canvas)
            {
                m_canvas->setSize(QSizeF(m_window->width(), m_window->height()));
                m_canvas->requestPaint();
            }
            reposition();
            return false;

        case QEvent::Close:
            if (!m_closable)
            {
                event->ignore();
                return true;
            }
            if (!close()) m_instance->stop();
            return false;

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            m_instance->pushKeyTable(*static_cast<QKeyEvent *>(event));
            const bool ok = call(LuaWindowCallback::Key, [](lua_State *L) { return lua_pcall(L, 1, 0, 0); });
            lua_pop(m_instance->state(), 1);
            if (!ok) m_instance->stop();
            return false;
        }

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        {
            m_lastMouse = static_cast<QMouseEvent *>(event)->position();
            m_instance->pushMouseTable(*static_cast<QMouseEvent *>(event));
            const bool ok = call(LuaWindowCallback::Mouse, [](lua_State *L) { return lua_pcall(L, 1, 0, 0); });
            lua_pop(m_instance->state(), 1);
            if (!ok) m_instance->stop();
            return false;
        }

        default: return false;
    }
}

namespace LuaWindowApi
{
void registerTypes(lua_State *L)
{
    static const luaL_Reg methods[] = {{"atpaint", window_atpaint}, {"atkey", window_atkey},
        {"atmouse", window_atmouse}, {"closed", window_closed}, {"resize", window_resize}, {"size", window_size},
        {"mouse_position", window_mouse_position}, {nullptr, nullptr}};
    luaL_newmetatable(L, WINDOW_MT);
    luaL_setfuncs(L, methods, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "Window");
    lua_setfield(L, -2, "__metatable");
    lua_pop(L, 1);
}

int create(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "id");
    luaL_checktype(L, -1, LUA_TSTRING);
    size_t idLength{};
    const char *id = lua_tolstring(L, -1, &idLength);
    luaL_argcheck(L, idLength > 0 && strlen(id) == idLength, 1, "id must be nonempty and contain no NUL bytes");

    auto *instance = LuaInstance::fromState(L);
    if (!instance || !instance->state()) return luaL_error(L, "cannot create windows while the script is stopping");
    const QString windowId = QString::fromUtf8(id);
    if (instance->findWindow(windowId)) return luaL_error(L, "window id already exists: %s", id);

    lua_getfield(L, 1, "anchor");
    const QString anchor = lua_isnil(L, -1) ? QString() : QString::fromUtf8(luaL_checkstring(L, -1));
    const QString side = anchor_side(anchor);
    luaL_argcheck(L,
        anchor.isEmpty() || ((side == "left" || side == "right" || side == "top" || side == "bottom")
            && (anchor == side || anchor.endsWith("_full"))),
        1, "invalid anchor");

    lua_getfield(L, 1, "size");
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_getfield(L, -1, "x");
    const auto width = std::clamp<lua_Integer>(luaL_checkinteger(L, -1), 1, 32767);
    lua_getfield(L, -2, "y");
    const auto height = std::clamp<lua_Integer>(luaL_checkinteger(L, -1), 1, 32767);

    const bool minimizable = window_option(L, "minimizable", true);
    const bool maximizable = window_option(L, "maximizable", false);
    const bool closable = window_option(L, "closable", true);

    auto *window = instance->addWindow(windowId, anchor, closable);
    if (!window->create(QSize(static_cast<int>(width), static_cast<int>(height)), minimizable, maximizable))
    {
        instance->dropWindow(window);
        return luaL_error(L, "failed to create Lua window");
    }
    window->reposition();
    push_window(L, window);
    return 1;
}

int main(lua_State *L)
{
    auto *instance = LuaInstance::fromState(L);
    if (!instance) return luaL_error(L, "no Lua instance");
    if (!instance->mainWindow()) instance->createMainWindow();
    push_window(L, instance->mainWindow());
    return 1;
}

std::optional<int> qtKeyToSdlKeycode(int qtKey)
{
    // SDL2 scancodes >= 0x40000000 have no ASCII equivalent; printable keys map to their code point.
    if (qtKey >= 32 && qtKey <= 126) return qtKey;

    // Printable keys map to their own code point, as SDL does.
    switch (qtKey)
    {
        case Qt::Key_Return:
        case Qt::Key_Enter: return SDLK_RETURN;
        case Qt::Key_Space: return SDLK_SPACE;
        case Qt::Key_Tab: return SDLK_TAB;
        case Qt::Key_Backspace: return SDLK_BACKSPACE;
        case Qt::Key_Escape: return SDLK_ESCAPE;
        case Qt::Key_Insert: return SDLK_INSERT;
        case Qt::Key_Delete: return SDLK_DELETE;
        case Qt::Key_Home: return SDLK_HOME;
        case Qt::Key_End: return SDLK_END;
        case Qt::Key_PageUp: return SDLK_PAGEUP;
        case Qt::Key_PageDown: return SDLK_PAGEDOWN;
        case Qt::Key_Left: return SDLK_LEFT;
        case Qt::Key_Right: return SDLK_RIGHT;
        case Qt::Key_Up: return SDLK_UP;
        case Qt::Key_Down: return SDLK_DOWN;
        case Qt::Key_Shift: return SDLK_LSHIFT;
        case Qt::Key_Control: return SDLK_LCTRL;
        case Qt::Key_Alt: return SDLK_LALT;
        case Qt::Key_Meta: return SDLK_LGUI;
        case Qt::Key_CapsLock: return SDLK_CAPSLOCK;
        default: break;
    }
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12) return SDLK_F1 + (qtKey - Qt::Key_F1);
    if (qtKey >= Qt::Key_Q && qtKey <= Qt::Key_P) return qtKey; // letters map to their own code point
    return std::nullopt;
}

void registerMupenGlobals(lua_State *L)
{
    static const struct
    {
        const char *name;
        int value;
    } keycodes[] = {
        {"SDLK_BACKSPACE", SDLK_BACKSPACE},     {"SDLK_TAB", SDLK_TAB},
        {"SDLK_RETURN", SDLK_RETURN},           {"SDLK_ESCAPE", SDLK_ESCAPE},
        {"SDLK_SPACE", SDLK_SPACE},             {"SDLK_INSERT", SDLK_INSERT},
        {"SDLK_DELETE", SDLK_DELETE},           {"SDLK_HOME", SDLK_HOME},
        {"SDLK_END", SDLK_END},                 {"SDLK_PAGEUP", SDLK_PAGEUP},
        {"SDLK_PAGEDOWN", SDLK_PAGEDOWN},       {"SDLK_LEFT", SDLK_LEFT},
        {"SDLK_RIGHT", SDLK_RIGHT},             {"SDLK_UP", SDLK_UP},
        {"SDLK_DOWN", SDLK_DOWN},               {"SDLK_LCTRL", SDLK_LCTRL},
        {"SDLK_RCTRL", SDLK_RCTRL},             {"SDLK_LSHIFT", SDLK_LSHIFT},
        {"SDLK_RSHIFT", SDLK_RSHIFT},           {"SDLK_LALT", SDLK_LALT},
        {"SDLK_RALT", SDLK_RALT},               {"SDLK_LGUI", SDLK_LGUI},
        {"SDLK_RGUI", SDLK_RGUI},               {"SDLK_CAPSLOCK", SDLK_CAPSLOCK},
        {"SDLK_KP_0", SDLK_KP_0},               {"SDLK_KP_1", SDLK_KP_1},
        {"SDLK_KP_2", SDLK_KP_2},               {"SDLK_KP_3", SDLK_KP_3},
        {"SDLK_KP_4", SDLK_KP_4},               {"SDLK_KP_5", SDLK_KP_5},
        {"SDLK_KP_6", SDLK_KP_6},               {"SDLK_KP_7", SDLK_KP_7},
        {"SDLK_KP_8", SDLK_KP_8},               {"SDLK_KP_9", SDLK_KP_9},
        {"SDLK_KP_ENTER", SDLK_KP_ENTER},       {"SDLK_KP_PLUS", SDLK_KP_PLUS},
        {"SDLK_KP_MINUS", SDLK_KP_MINUS},       {"SDLK_KP_MULTIPLY", SDLK_KP_MULTIPLY},
        {"SDLK_KP_DIVIDE", SDLK_KP_DIVIDE},     {nullptr, 0},
    };

    lua_getglobal(L, "Mupen");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "Mupen");
    }
    lua_newtable(L);
    for (const auto &entry : keycodes)
    {
        if (!entry.name) break;
        lua_pushinteger(L, entry.value);
        lua_setfield(L, -2, entry.name);
    }
    for (char c = 'A'; c <= 'Z'; ++c)
    {
        lua_pushinteger(L, c);
        const char name[7] = {'S', 'D', 'L', 'K', '_', c, 0};
        lua_setfield(L, -2, name);
    }
    for (char c = '0'; c <= '9'; ++c)
    {
        lua_pushinteger(L, c);
        const char name[7] = {'S', 'D', 'L', 'K', '_', c, 0};
        lua_setfield(L, -2, name);
    }
    for (int f = 1; f <= 12; ++f)
    {
        lua_pushinteger(L, SDLK_F1 + (f - 1));
        lua_setfield(L, -2, (QString("SDLK_F%1").arg(f)).toUtf8().constData());
    }
    lua_setfield(L, -2, "keycode");

    static const struct
    {
        const char *name;
        int value;
    } vkeys[] = {
        {"VK_LBUTTON", 1},
        {"VK_RBUTTON", 2},
        {"VK_CANCEL", 3},
        {"VK_MBUTTON", 4},
        {"VK_XBUTTON1", 5},
        {"VK_XBUTTON2", 6},
        {"VK_BACK", 8},
        {"VK_TAB", 9},
        {"VK_CLEAR", 12},
        {"VK_RETURN", 13},
        {"VK_SHIFT", 16},
        {"VK_CONTROL", 17},
        {"VK_MENU", 18},
        {"VK_PAUSE", 19},
        {"VK_CAPITAL", 20},
        {"VK_KANA", 21},
        {"VK_HANGUL", 21},
        {"VK_IME_ON", 22},
        {"VK_JUNJA", 23},
        {"VK_FINAL", 24},
        {"VK_HANJA", 25},
        {"VK_KANJI", 25},
        {"VK_IME_OFF", 26},
        {"VK_ESCAPE", 27},
        {"VK_CONVERT", 28},
        {"VK_NONCONVERT", 29},
        {"VK_ACCEPT", 30},
        {"VK_MODECHANGE", 31},
        {"VK_SPACE", 32},
        {"VK_PRIOR", 33},
        {"VK_NEXT", 34},
        {"VK_END", 35},
        {"VK_HOME", 36},
        {"VK_LEFT", 37},
        {"VK_UP", 38},
        {"VK_RIGHT", 39},
        {"VK_DOWN", 40},
        {"VK_SELECT", 41},
        {"VK_PRINT", 42},
        {"VK_EXECUTE", 43},
        {"VK_SNAPSHOT", 44},
        {"VK_INSERT", 45},
        {"VK_DELETE", 46},
        {"VK_HELP", 47},
        {"VK_0", 48},
        {"VK_1", 49},
        {"VK_2", 50},
        {"VK_3", 51},
        {"VK_4", 52},
        {"VK_5", 53},
        {"VK_6", 54},
        {"VK_7", 55},
        {"VK_8", 56},
        {"VK_9", 57},
        {"VK_A", 65},
        {"VK_B", 66},
        {"VK_C", 67},
        {"VK_D", 68},
        {"VK_E", 69},
        {"VK_F", 70},
        {"VK_G", 71},
        {"VK_H", 72},
        {"VK_I", 73},
        {"VK_J", 74},
        {"VK_K", 75},
        {"VK_L", 76},
        {"VK_M", 77},
        {"VK_N", 78},
        {"VK_O", 79},
        {"VK_P", 80},
        {"VK_Q", 81},
        {"VK_R", 82},
        {"VK_S", 83},
        {"VK_T", 84},
        {"VK_U", 85},
        {"VK_V", 86},
        {"VK_W", 87},
        {"VK_X", 88},
        {"VK_Y", 89},
        {"VK_Z", 90},
        {"VK_LWIN", 91},
        {"VK_RWIN", 92},
        {"VK_APPS", 93},
        {"VK_SLEEP", 95},
        {"VK_NUMPAD0", 96},
        {"VK_NUMPAD1", 97},
        {"VK_NUMPAD2", 98},
        {"VK_NUMPAD3", 99},
        {"VK_NUMPAD4", 100},
        {"VK_NUMPAD5", 101},
        {"VK_NUMPAD6", 102},
        {"VK_NUMPAD7", 103},
        {"VK_NUMPAD8", 104},
        {"VK_NUMPAD9", 105},
        {"VK_MULTIPLY", 106},
        {"VK_ADD", 107},
        {"VK_SEPARATOR", 108},
        {"VK_SUBTRACT", 109},
        {"VK_DECIMAL", 110},
        {"VK_DIVIDE", 111},
        {"VK_F1", 112},
        {"VK_F2", 113},
        {"VK_F3", 114},
        {"VK_F4", 115},
        {"VK_F5", 116},
        {"VK_F6", 117},
        {"VK_F7", 118},
        {"VK_F8", 119},
        {"VK_F9", 120},
        {"VK_F10", 121},
        {"VK_F11", 122},
        {"VK_F12", 123},
        {"VK_F13", 124},
        {"VK_F14", 125},
        {"VK_F15", 126},
        {"VK_F16", 127},
        {"VK_F17", 128},
        {"VK_F18", 129},
        {"VK_F19", 130},
        {"VK_F20", 131},
        {"VK_F21", 132},
        {"VK_F22", 133},
        {"VK_F23", 134},
        {"VK_F24", 135},
        {"VK_NUMLOCK", 144},
        {"VK_SCROLL", 145},
        {"VK_LSHIFT", 160},
        {"VK_RSHIFT", 161},
        {"VK_LCONTROL", 162},
        {"VK_RCONTROL", 163},
        {"VK_LMENU", 164},
        {"VK_RMENU", 165},
        {"VK_BROWSER_BACK", 166},
        {"VK_BROWSER_FORWARD", 167},
        {"VK_BROWSER_REFRESH", 168},
        {"VK_BROWSER_STOP", 169},
        {"VK_BROWSER_SEARCH", 170},
        {"VK_BROWSER_FAVORITES", 171},
        {"VK_BROWSER_HOME", 172},
        {"VK_VOLUME_MUTE", 173},
        {"VK_VOLUME_DOWN", 174},
        {"VK_VOLUME_UP", 175},
        {"VK_MEDIA_NEXT_TRACK", 176},
        {"VK_MEDIA_PREV_TRACK", 177},
        {"VK_MEDIA_STOP", 178},
        {"VK_MEDIA_PLAY_PAUSE", 179},
        {"VK_LAUNCH_MAIL", 180},
        {"VK_LAUNCH_MEDIA_SELECT", 181},
        {"VK_LAUNCH_APP1", 182},
        {"VK_LAUNCH_APP2", 183},
        {"VK_OEM_1", 186},
        {"VK_OEM_PLUS", 187},
        {"VK_OEM_COMMA", 188},
        {"VK_OEM_MINUS", 189},
        {"VK_OEM_PERIOD", 190},
        {"VK_OEM_2", 191},
        {"VK_OEM_3", 192},
        {"VK_OEM_4", 219},
        {"VK_OEM_5", 220},
        {"VK_OEM_6", 221},
    };
    lua_newtable(L);
    for (const auto &vk : vkeys)
    {
        lua_pushinteger(L, vk.value);
        lua_setfield(L, -2, vk.name);
    }
    lua_setfield(L, -2, "VKeycodes");

    static const struct
    {
        const char *name;
        int value;
    } speedModes[] = {
        {"Normal", static_cast<int>(::CoreSpeedMode::Normal)},
        {"FastForward", static_cast<int>(::CoreSpeedMode::FastForward)},
        {"UltraFastForward", static_cast<int>(::CoreSpeedMode::UltraFastForward)},
        {nullptr, 0},
    };
    lua_newtable(L);
    for (const auto &mode : speedModes)
    {
        if (!mode.name) continue;
        lua_pushinteger(L, mode.value);
        lua_setfield(L, -2, mode.name);
    }
    lua_setfield(L, -2, "CoreSpeedMode");
    lua_pop(L, 1);
}
} // namespace LuaWindowApi
