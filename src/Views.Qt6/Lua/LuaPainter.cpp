/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaPainter.hpp"
#include "LuaCanvas.hpp"
#include "LuaInstance.hpp"

#include <QBrush>
#include <QColor>
#include <QFile>
#include <QFontMetricsF>
#include <QLinearGradient>
#include <QAbstractTextDocumentLayout>
#include <QPen>
#include <QRadialGradient>
#include <QTextDocument>
#include <QTextLayout>
#include <QTextOption>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string_view>

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
}

namespace
{
using LuaPainter::Image;
using LuaPainter::Painter;
using LuaPainter::State;

// ---------------------------------------------------------------- helpers

[[noreturn]] void bad_arg(lua_State *L, int index, const char *what)
{
    luaL_argerror(L, index, lua_pushfstring(L, "%s expected, got %s", what, luaL_typename(L, index)));
    __builtin_unreachable();
}

// where a painter's drawing lands: its bound image, else the instance's active canvas
LuaCanvas *target_canvas(Painter *painter)
{
    return painter->targetCanvas ? painter->targetCanvas : painter->instance->activeCanvas();
}

Painter *check_painter(lua_State *L, int index)
{
    if (!luaL_testudata(L, index, LuaPainter::PAINTER_MT)) bad_arg(L, index, LuaPainter::PAINTER_MT);
    return static_cast<Painter *>(lua_touserdata(L, index));
}

Image *check_image(lua_State *L, int index)
{
    if (!luaL_testudata(L, index, LuaPainter::IMAGE_MT)) bad_arg(L, index, LuaPainter::IMAGE_MT);
    auto *image = static_cast<Image *>(lua_touserdata(L, index));
    if (!image->bitmap) return luaL_error(L, "use of a closed PainterImage"), nullptr;
    return image;
}

qreal to_finite_or(lua_State *L, int index, qreal fallback)
{
    if (lua_isnoneornil(L, index)) return fallback;
    const double value = lua_tonumber(L, index);
    if (!std::isfinite(value)) return fallback;
    return static_cast<qreal>(value);
}

// reads an optional numeric field of a table, leaving the stack balanced
qreal table_opt_number(lua_State *L, int table, const char *name, qreal fallback)
{
    if (lua_getfield(L, table, name) == LUA_TNIL)
    {
        lua_pop(L, 1);
        return fallback;
    }
    const qreal value = to_finite_or(L, -1, fallback);
    lua_pop(L, 1);
    return value;
}

QColor color_from_parts(qreal r, qreal g, qreal b, qreal a)
{
    const auto clamp255 = [](qreal v) { return static_cast<int>(std::clamp(v, qreal(0), qreal(1)) * 255.0); };
    // components in [0,1] per the contract; accept raw 0-255 integers as scripts often pass them
    const bool raw = r > 1.0 || g > 1.0 || b > 1.0;
    const auto clamp8 = [](qreal v) { return static_cast<int>(std::clamp(v, qreal(0), qreal(255))); };
    if (raw) return QColor(clamp8(r), clamp8(g), clamp8(b), std::clamp(a, qreal(0), qreal(1)) * 255.0);
    return QColor(clamp255(r), clamp255(g), clamp255(b), clamp255(a));
}

QRectF normalized_rect(qreal x, qreal y, qreal w, qreal h)
{
    if (w < 0)
    {
        x += w;
        w = -w;
    }
    if (h < 0)
    {
        y += h;
        h = -h;
    }
    return QRectF(x, y, w, h);
}

void push_rect_table(lua_State *L, const QRectF &rect)
{
    lua_newtable(L);
    lua_pushnumber(L, rect.x());
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, rect.y());
    lua_setfield(L, -2, "y");
    lua_pushnumber(L, rect.width());
    lua_setfield(L, -2, "w");
    lua_pushnumber(L, rect.height());
    lua_setfield(L, -2, "h");
}

QFont font_from_style(lua_State *L, int index)
{
    QFont font;
    if (lua_isnoneornil(L, index)) return font;
    luaL_checktype(L, index, LUA_TTABLE);

    lua_getfield(L, index, "family");
    if (const char *family = lua_tostring(L, -1)) font.setFamily(QString::fromUtf8(family));
    lua_pop(L, 1);

    const qreal size = to_finite_or(L, lua_getfield(L, index, "size") != LUA_TNIL ? -1 : 0, 0);
    if (size > 0) font.setPointSizeF(size);
    lua_pop(L, 1);

    const qreal weight = to_finite_or(L, lua_getfield(L, index, "weight") != LUA_TNIL ? -1 : 0, 400);
    font.setWeight(static_cast<QFont::Weight>(std::clamp(int(weight / 100.0 * 99.0), 0, 99)));
    lua_pop(L, 1);

    lua_getfield(L, index, "slant");
    if (const char *slant = lua_tostring(L, -1)) font.setItalic(QString::fromUtf8(slant) != "normal");
    lua_pop(L, 1);

    lua_getfield(L, index, "underline");
    if (lua_toboolean(L, -1)) font.setUnderline(true);
    lua_pop(L, 1);
    lua_getfield(L, index, "strikethrough");
    if (lua_toboolean(L, -1)) font.setStrikeOut(true);
    lua_pop(L, 1);

    const qreal letterSpacing = to_finite_or(L, lua_getfield(L, index, "letter_spacing") != LUA_TNIL ? -1 : 0, 0);
    if (letterSpacing != 0) font.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);
    lua_pop(L, 1);

    // line_height is applied by the text layout, not the font
    lua_getfield(L, index, "line_height");
    lua_pop(L, 1);

    lua_getfield(L, index, "antialiased");
    if (!lua_toboolean(L, -1)) font.setHintingPreference(QFont::PreferNoHinting);
    lua_pop(L, 1);

    return font;
}

// The text style also carries layout hints used by measure/hittest; they are read where needed.
Qt::Alignment text_alignment(lua_State *L, int styleIndex, bool &wordWrap)
{
    Qt::Alignment align;
    wordWrap = false;
    if (lua_isnoneornil(L, styleIndex)) return align | Qt::AlignLeft | Qt::AlignTop;

    lua_getfield(L, styleIndex, "align_x");
    if (const char *value = lua_tostring(L, -1))
    {
        const QString v = QString::fromUtf8(value);
        if (v == "center") align |= Qt::AlignHCenter;
        else if (v == "right" || v == "end") align |= Qt::AlignRight;
        else if (v == "justify") align |= Qt::AlignJustify;
        else align |= Qt::AlignLeft;
    }
    else
    {
        align |= Qt::AlignLeft;
    }
    lua_pop(L, 1);

    lua_getfield(L, styleIndex, "align_y");
    if (const char *value = lua_tostring(L, -1))
    {
        const QString v = QString::fromUtf8(value);
        if (v == "center") align |= Qt::AlignVCenter;
        else if (v == "bottom") align |= Qt::AlignBottom;
        else align |= Qt::AlignTop;
    }
    else
    {
        align |= Qt::AlignTop;
    }
    lua_pop(L, 1);

    lua_getfield(L, styleIndex, "wrap");
    if (const char *value = lua_tostring(L, -1)) wordWrap = QString::fromUtf8(value) == "word";
    lua_pop(L, 1);

    return align;
}

// Builds an off-screen layout shared by text drawing, metrics and hit testing.
std::unique_ptr<QTextDocument> layout_document(const QString &text, lua_State *L, int styleIndex, lua_State *LC,
    int constraintsIndex)
{
    auto document = std::make_unique<QTextDocument>();
    document->setDefaultFont(font_from_style(L, styleIndex));

    QTextOption option;
    bool wordWrap = false;
    option.setAlignment(text_alignment(LC, styleIndex, wordWrap));
    option.setWrapMode(wordWrap ? QTextOption::WordWrap : QTextOption::NoWrap);
    document->setDefaultTextOption(option);
    document->setPlainText(text);

    if (constraintsIndex && !lua_isnoneornil(LC, constraintsIndex))
    {
        const qreal w = table_opt_number(LC, constraintsIndex, "w", 0);
        if (w > 0)
        {
            document->setTextWidth(w);
            if (!wordWrap)
            {
                option.setWrapMode(QTextOption::WordWrap);
                document->setDefaultTextOption(option);
            }
        }
    }
    document->setDocumentMargin(0);
    return document;
}

void apply_text_metrics(lua_State *L, const QTextDocument &document, bool truncated)
{
    const QSizeF size = document.documentLayout()->documentSize();
    lua_newtable(L);
    lua_pushnumber(L, document.textWidth() > 0 ? document.textWidth() : size.width());
    lua_setfield(L, -2, "w");
    lua_pushnumber(L, size.height());
    lua_setfield(L, -2, "h");
    lua_pushinteger(L, document.lineCount());
    lua_setfield(L, -2, "line_count");
    lua_pushnumber(L, QFontMetricsF(document.defaultFont()).ascent());
    lua_setfield(L, -2, "baseline");
    lua_pushboolean(L, truncated);
    lua_setfield(L, -2, "truncated");
}

QBrush check_paint(lua_State *L, int index)
{
    if (lua_isnoneornil(L, index)) return QColor();
    if (!lua_istable(L, index))
    {
        QColor color;
        LuaPainter::checkColor(L, index, color);
        return color;
    }

    lua_getfield(L, index, "r");
    const bool isColor = !lua_isnil(L, -1);
    lua_pop(L, 1);
    if (isColor)
    {
        QColor color;
        LuaPainter::checkColor(L, index, color);
        return color;
    }

    lua_getfield(L, index, "type");
    const char *type = lua_tostring(L, -1);
    const QString typeName = type ? QString::fromUtf8(type) : QString();
    lua_pop(L, 1);

    const auto readStops = [L, index]() {
        QGradientStops stops;
        lua_getfield(L, index, "stops");
        if (lua_istable(L, -1))
        {
            const int count = static_cast<int>(lua_rawlen(L, -1));
            for (int i = 1; i <= count; ++i)
            {
                lua_rawgeti(L, -1, i);
                const qreal offset = table_opt_number(L, -1, "offset", 0);
                QColor color;
                if (lua_getfield(L, -1, "color") != LUA_TNIL) LuaPainter::checkColor(L, -1, color);
                lua_pop(L, 1);
                lua_pop(L, 1); // the stop entry itself
                stops.append(QGradientStop(std::clamp(offset, qreal(0), qreal(1)), color));
            }
        }
        lua_pop(L, 1);
        std::stable_sort(stops.begin(), stops.end(),
            [](const QGradientStop &a, const QGradientStop &b) { return a.first < b.first; });
        return stops;
    };

    if (typeName == "linear_gradient")
    {
        const qreal x0 = table_opt_number(L, index, "x0", 0);
        const qreal y0 = table_opt_number(L, index, "y0", 0);
        const qreal x1 = table_opt_number(L, index, "x1", 0);
        const qreal y1 = table_opt_number(L, index, "y1", 0);
        // a degenerate line would draw nothing: extend the axis by one pixel
        const bool degenerate = qFuzzyIsNull(x1 - x0) && qFuzzyIsNull(y1 - y0);
        QLinearGradient gradient(x0, y0, degenerate ? x0 + 1 : x1, degenerate ? y0 : y1);
        gradient.setStops(readStops());
        return QBrush(gradient);
    }
    if (typeName == "radial_gradient")
    {
        const qreal cx = table_opt_number(L, index, "center_x", 0);
        const qreal cy = table_opt_number(L, index, "center_y", 0);
        const qreal rx = table_opt_number(L, index, "radius_x", 0);
        const qreal ry = table_opt_number(L, index, "radius_y", 0);
        // Qt only supports circular radial gradients: draw it through an ellipse brush path instead
        const qreal radius = std::max(rx, ry);
        QRadialGradient gradient(QPointF(cx, cy), radius > 0 ? radius : 1);
        gradient.setStops(readStops());
        QBrush brush(gradient);
        if (!qFuzzyCompare(rx, ry))
        {
            // approximate the ellipse by scaling around the center at paint time via the transform
            QTransform scale;
            scale.translate(cx, cy);
            scale.scale(rx > 0 ? 1.0 : 1.0, ry > 0 && rx > 0 ? rx / ry : 1.0);
            scale.translate(-cx, -cy);
            brush.setTransform(scale);
        }
        return brush;
    }

    luaL_error(L, "unknown paint type '%s'", typeName.toUtf8().constData());
    return QColor();
}

QPen check_stroke(lua_State *L, int index, const QColor &fallback)
{
    QPen pen(fallback);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCapStyle(Qt::FlatCap);
    if (lua_isnoneornil(L, index)) return pen;
    luaL_checktype(L, index, LUA_TTABLE);

    pen.setWidthF(std::max(table_opt_number(L, index, "width", 1.0), 0.0));

    lua_getfield(L, index, "cap");
    if (const char *cap = lua_tostring(L, -1))
    {
        const QString v = QString::fromUtf8(cap);
        if (v == "round") pen.setCapStyle(Qt::RoundCap);
        else if (v == "square") pen.setCapStyle(Qt::SquareCap);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "join");
    if (const char *join = lua_tostring(L, -1))
    {
        const QString v = QString::fromUtf8(join);
        if (v == "round") pen.setJoinStyle(Qt::RoundJoin);
        else if (v == "bevel") pen.setJoinStyle(Qt::BevelJoin);
    }
    lua_pop(L, 1);

    pen.setMiterLimit(table_opt_number(L, index, "miter_limit", 4.0));

    lua_getfield(L, index, "dashes");
    if (lua_istable(L, -1))
    {
        QVector<qreal> dashes;
        const int count = static_cast<int>(lua_rawlen(L, -1));
        for (int i = 1; i <= count; ++i)
        {
            lua_rawgeti(L, -1, i);
            dashes.append(to_finite_or(L, -1, 0));
            lua_pop(L, 1);
        }
        pen.setDashPattern(dashes);
    }
    lua_pop(L, 1);
    pen.setDashOffset(table_opt_number(L, index, "dash_offset", 0.0));
    return pen;
}

// Records a path operation resolved with the painter state at call time.
void queue_path_command(lua_State *L, Painter *painter, LuaDrawCmdType type, lua_State *Lstate, int paintIndex,
    int styleIndex)
{
    LuaDrawCommand cmd;
    cmd.type = type;
    cmd.path = painter->state.path;
    cmd.runs = painter->state.texts;
    cmd.transform = painter->state.transform;
    cmd.opacity = painter->state.alpha;
    if (type == LuaDrawCmdType::FillPath)
    {
        cmd.brush = check_paint(Lstate, paintIndex);
    }
    else
    {
        QColor penColor;
        if (lua_isnoneornil(Lstate, paintIndex)) penColor = QColor(0, 0, 0);
        else
        {
            QBrush brush = check_paint(Lstate, paintIndex);
            penColor = brush.color();
        }
        cmd.pen = check_stroke(Lstate, styleIndex, penColor);
    }
    // arguments are validated even without a target: the contract must not silently swallow bad inputs
    LuaCanvas *canvas = target_canvas(painter);
    if (!canvas) return;
    canvas->enqueueCommand(std::move(cmd));
}

} // namespace

namespace LuaPainter
{

int checkColor(lua_State *L, int index, QColor &out)
{
    if (lua_isstring(L, index))
    {
        QString hex = QString::fromUtf8(lua_tostring(L, index));
        if (hex.startsWith('#')) hex.removeAt(0);
        if (hex.size() == 6 || hex.size() == 8)
        {
            bool ok = true;
            uint values[4] = {0, 0, 0, 255};
            for (int i = 0; i < hex.size(); i += 2)
            {
                values[i / 2] = hex.mid(i, 2).toUInt(&ok, 16);
                if (!ok) break;
            }
            if (ok)
            {
                out = QColor(static_cast<int>(values[0]), static_cast<int>(values[1]), static_cast<int>(values[2]),
                    static_cast<int>(values[3]));
                return 0;
            }
        }
        return luaL_error(L, "color string must be \"#RRGGBB\" or \"#RRGGBBAA\"");
    }
    if (lua_istable(L, index))
    {
        const qreal r = table_opt_number(L, index, "r", 0);
        const qreal g = table_opt_number(L, index, "g", 0);
        const qreal b = table_opt_number(L, index, "b", 0);
        const qreal a = table_opt_number(L, index, "a", 1);
        out = color_from_parts(r, g, b, a);
        return 0;
    }
    return luaL_error(L, "color must be a string or a table");
}

int checkRect(lua_State *L, int index, QRectF &out)
{
    luaL_checktype(L, index, LUA_TTABLE);
    const qreal x = table_opt_number(L, index, "x", 0);
    const qreal y = table_opt_number(L, index, "y", 0);
    const qreal w = table_opt_number(L, index, "w", 0);
    const qreal h = table_opt_number(L, index, "h", 0);
    out = normalized_rect(x, y, w, h);
    return 0;
}

QFont parseTextStyle(lua_State *L, int index)
{
    return font_from_style(L, index);
}

// ---------------------------------------------------------------- painter methods

#define SELF (check_painter(L, 1))

int painter_clear(lua_State *L)
{
    auto *painter = SELF;
    if (LuaCanvas *canvas = target_canvas(painter))
    {
        QColor color = Qt::transparent;
        if (!lua_isnoneornil(L, 2)) checkColor(L, 2, color);
        canvas->clear(color);
    }
    return 0;
}

int painter_begin_path(lua_State *L)
{
    auto *p = SELF;
    p->state.path = QPainterPath();
    p->state.texts.clear();
    return 0;
}

int painter_move_to(lua_State *L)
{
    auto *p = SELF;
    p->state.path.moveTo(to_finite_or(L, 2, 0), to_finite_or(L, 3, 0));
    return 0;
}

int painter_line_to(lua_State *L)
{
    auto *p = SELF;
    p->state.path.lineTo(to_finite_or(L, 2, 0), to_finite_or(L, 3, 0));
    return 0;
}

int painter_cubic_to(lua_State *L)
{
    auto *p = SELF;
    p->state.path.cubicTo(to_finite_or(L, 2, 0), to_finite_or(L, 3, 0), to_finite_or(L, 4, 0), to_finite_or(L, 5, 0),
        to_finite_or(L, 6, 0), to_finite_or(L, 7, 0));
    return 0;
}

int painter_quadratic_to(lua_State *L)
{
    auto *p = SELF;
    p->state.path.quadTo(to_finite_or(L, 2, 0), to_finite_or(L, 3, 0), to_finite_or(L, 4, 0), to_finite_or(L, 5, 0));
    return 0;
}

// angles in radians; ccw default false (direct2d draws clockwise from start to end)
int painter_arc(lua_State *L)
{
    auto *p = SELF;
    const qreal x = to_finite_or(L, 2, 0);
    const qreal y = to_finite_or(L, 3, 0);
    const qreal radius = to_finite_or(L, 4, 0);
    const qreal start = to_finite_or(L, 5, 0);
    const qreal end = to_finite_or(L, 6, 0);
    const bool ccw = lua_isnoneornil(L, 7) ? false : lua_toboolean(L, 7) != 0;
    QRectF rect(x - radius, y - radius, radius * 2, radius * 2);
    const qreal startDegrees = start * 180.0 / std::numbers::pi;
    qreal span = (end - start) * 180.0 / std::numbers::pi;
    if (!ccw)
    {
        while (span < 0) span += 360.0;
    }
    else
    {
        while (span > 0) span -= 360.0;
    }
    p->state.path.arcMoveTo(rect, startDegrees);
    p->state.path.arcTo(rect, startDegrees, span);
    return 0;
}

int painter_close_path(lua_State *L)
{
    SELF->state.path.closeSubpath();
    return 0;
}

int painter_rect(lua_State *L)
{
    auto *p = SELF;
    QRectF rect;
    checkRect(L, 2, rect);
    p->state.path.addRect(rect);
    return 0;
}

int painter_round_rect(lua_State *L)
{
    auto *p = SELF;
    QRectF rect;
    checkRect(L, 2, rect);
    qreal rx = to_finite_or(L, 3, 0);
    qreal ry = rx;
    if (lua_istable(L, 3))
    {
        rx = table_opt_number(L, 3, "x", 0);
        ry = table_opt_number(L, 3, "y", rx);
    }
    p->state.path.addRoundedRect(rect, rx, ry);
    return 0;
}

int painter_circle(lua_State *L)
{
    auto *p = SELF;
    // contract takes a PainterRect: the circle is inscribed in it
    QRectF rect;
    checkRect(L, 2, rect);
    p->state.path.addEllipse(rect);
    return 0;
}

int painter_line(lua_State *L)
{
    auto *p = SELF;
    p->state.path.moveTo(to_finite_or(L, 2, 0), to_finite_or(L, 3, 0));
    p->state.path.lineTo(to_finite_or(L, 4, 0), to_finite_or(L, 5, 0));
    return 0;
}

QPolygonF check_points(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);
    QPolygonF points;
    const int count = static_cast<int>(lua_rawlen(L, index));
    if (count % 2 != 0) return luaL_error(L, "points must be a flat list of x, y pairs"), points;
    for (int i = 1; i + 1 <= count; i += 2)
    {
        lua_rawgeti(L, index, i);
        const qreal x = to_finite_or(L, -1, 0);
        lua_pop(L, 1);
        lua_rawgeti(L, index, i + 1);
        const qreal y = to_finite_or(L, -1, 0);
        lua_pop(L, 1);
        points << QPointF(x, y);
    }
    return points;
}

int painter_polyline(lua_State *L)
{
    auto *p = SELF;
    const QPolygonF points = check_points(L, 2);
    if (points.isEmpty()) return 0;
    p->state.path.moveTo(points.first());
    for (int i = 1; i < points.size(); ++i) p->state.path.lineTo(points.at(i));
    return 0;
}

int painter_polygon(lua_State *L)
{
    auto *p = SELF;
    const QPolygonF points = check_points(L, 2);
    if (points.isEmpty()) return 0;
    p->state.path.moveTo(points.first());
    for (int i = 1; i < points.size(); ++i) p->state.path.lineTo(points.at(i));
    p->state.path.closeSubpath();
    return 0;
}

int painter_get_alpha(lua_State *L)
{
    lua_pushnumber(L, SELF->state.alpha);
    return 1;
}

int painter_set_alpha(lua_State *L)
{
    const double alpha = lua_tonumber(L, 2);
    SELF->state.alpha = std::isfinite(alpha) ? std::clamp(alpha, 0.0, 1.0) : 1.0;
    return 0;
}

// the save/restore stack lives in the painter userdata's uservalue, like Win32's states vector
int ensure_states_table(lua_State *L)
{
    lua_getuservalue(L, 1);
    if (lua_isnil(L, -1))
    {
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setuservalue(L, 1);
    }
    lua_getfield(L, -1, "states");
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "states");
    }
    return lua_gettop(L); // states table on top
}

int painter_save(lua_State *L)
{
    auto *painter = SELF;
    ensure_states_table(L);
    const int count = static_cast<int>(lua_rawlen(L, -1));

    lua_newtable(L);
    const QTransform &t = painter->state.transform;
    lua_pushnumber(L, t.m11());
    lua_setfield(L, -2, "m11");
    lua_pushnumber(L, t.m12());
    lua_setfield(L, -2, "m12");
    lua_pushnumber(L, t.m21());
    lua_setfield(L, -2, "m21");
    lua_pushnumber(L, t.m22());
    lua_setfield(L, -2, "m22");
    lua_pushnumber(L, t.m31());
    lua_setfield(L, -2, "m31");
    lua_pushnumber(L, t.m32());
    lua_setfield(L, -2, "m32");
    lua_pushnumber(L, painter->state.alpha);
    lua_setfield(L, -2, "alpha");
    lua_pushinteger(L, painter->state.clipDepth);
    lua_setfield(L, -2, "clip");
    lua_rawseti(L, -2, count + 1);
    lua_pop(L, 1);
    return 0;
}

int painter_restore(lua_State *L)
{
    auto *painter = SELF;
    ensure_states_table(L);
    const int count = static_cast<int>(lua_rawlen(L, -1));
    if (count == 0)
    {
        lua_pop(L, 1);
        return luaL_error(L, "restore without save");
    }
    lua_rawgeti(L, -1, count);
    lua_pushnil(L);
    lua_rawseti(L, -3, count); // drop the entry we are about to apply

    const auto field = [L](const char *name, qreal fallback) {
        lua_getfield(L, -1, name);
        const qreal value = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : fallback;
        lua_pop(L, 1);
        return value;
    };
    QTransform transform;
    transform.setMatrix(field("m11", 1), field("m12", 0), 0, field("m21", 0), field("m22", 1), 0, field("m31", 0),
        field("m32", 0), 1);
    painter->state.transform = transform;
    painter->state.alpha = field("alpha", 1);
    const int savedClip = static_cast<int>(field("clip", 0));
    lua_pop(L, 2); // entry + states table

    // pop the clips added since the saved depth
    LuaCanvas *canvas = target_canvas(painter);
    for (int i = painter->state.clipDepth; i > savedClip; --i)
    {
        if (canvas) canvas->enqueueCommand(LuaDrawCmdType::PopClip);
    }
    painter->state.clipDepth = savedClip;
    lua_settop(L, 1);
    return 0;
}

int painter_clip(lua_State *L)
{
    auto *painter = SELF;
    QRectF rect;
    checkRect(L, 2, rect);
    rect = painter->state.transform.mapRect(rect);
    if (LuaCanvas *canvas = target_canvas(painter))
    {
        LuaDrawCommand cmd;
        cmd.type = LuaDrawCmdType::PushClip;
        cmd.rect = rect;
        canvas->enqueueCommand(std::move(cmd));
        painter->state.clipDepth++;
    }
    return 0;
}

int painter_translate(lua_State *L)
{
    auto *p = SELF;
    QTransform translation = QTransform::fromTranslate(to_finite_or(L, 2, 0), to_finite_or(L, 3, 0));
    p->state.transform = translation * p->state.transform;
    return 0;
}

int painter_rotate(lua_State *L)
{
    auto *p = SELF;
    const qreal radians = to_finite_or(L, 2, 0);
    QTransform rotation;
    rotation.rotate(radians * 180.0 / std::numbers::pi);
    p->state.transform = rotation * p->state.transform;
    return 0;
}

int painter_scale(lua_State *L)
{
    auto *p = SELF;
    QTransform scaling = QTransform::fromScale(to_finite_or(L, 2, 1), to_finite_or(L, 3, 1));
    p->state.transform = scaling * p->state.transform;
    return 0;
}

// PainterMatrix3x2 userdata, same contract as the Win32 view: m11..m32 fields plus dx/dy/sx/sy readers.
constexpr const char *MATRIX_MT = "mupen64.PainterMatrix3x2";
struct MatrixData
{
    qreal m[6]; // m11 m12 m21 m22 m31 m32
};

MatrixData *check_matrix(lua_State *L, int index)
{
    return static_cast<MatrixData *>(luaL_checkudata(L, index, MATRIX_MT));
}

int matrix_dx(lua_State *L)
{
    lua_pushnumber(L, check_matrix(L, 1)->m[4]);
    return 1;
}
int matrix_dy(lua_State *L)
{
    lua_pushnumber(L, check_matrix(L, 1)->m[5]);
    return 1;
}
int matrix_sx(lua_State *L)
{
    const MatrixData *matrix = check_matrix(L, 1);
    lua_pushnumber(L, std::hypot(matrix->m[0], matrix->m[1]));
    return 1;
}
int matrix_sy(lua_State *L)
{
    const MatrixData *matrix = check_matrix(L, 1);
    lua_pushnumber(L, std::hypot(matrix->m[2], matrix->m[3]));
    return 1;
}

int matrix_index(lua_State *L)
{
    const MatrixData *matrix = check_matrix(L, 1);
    const size_t keyLength = lua_rawlen(L, 2);
    const char *keyData = lua_tostring(L, 2);
    const std::string_view key(keyData ? keyData : "", keyData ? keyLength : 0);
    if (key.empty())
    {
        luaL_getmetatable(L, MATRIX_MT);
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        lua_remove(L, -2);
        return 1;
    }
    for (int i = 0; i < 6; ++i)
    {
        const char *names[6] = {"m11", "m12", "m21", "m22", "m31", "m32"};
        if (key == names[i])
        {
            lua_pushnumber(L, matrix->m[i]);
            return 1;
        }
    }
    luaL_getmetatable(L, MATRIX_MT);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    lua_remove(L, -2);
    return 1;
}

int matrix_newindex(lua_State *L)
{
    MatrixData *matrix = check_matrix(L, 1);
    const size_t keyLength = lua_rawlen(L, 2);
    const char *keyData = lua_tostring(L, 2);
    const std::string_view key(keyData ? keyData : "", keyData ? keyLength : 0);
    const qreal value = static_cast<qreal>(luaL_checknumber(L, 3));
    for (int i = 0; i < 6; ++i)
    {
        const char *names[6] = {"m11", "m12", "m21", "m22", "m31", "m32"};
        if (key == names[i])
        {
            matrix->m[i] = value;
            return 0;
        }
    }
    return luaL_error(L, "unknown PainterMatrix3x2 component '%.*s'", static_cast<int>(keyLength), keyData);
}

int painter_get_transform(lua_State *L)
{
    const State &state = SELF->state;
    auto *matrix = static_cast<MatrixData *>(lua_newuserdata(L, sizeof(MatrixData)));
    matrix->m[0] = state.transform.m11();
    matrix->m[1] = state.transform.m12();
    matrix->m[2] = state.transform.m21();
    matrix->m[3] = state.transform.m22();
    matrix->m[4] = state.transform.m31();
    matrix->m[5] = state.transform.m32();
    luaL_getmetatable(L, MATRIX_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int painter_set_transform(lua_State *L)
{
    auto *p = SELF;
    qreal m[6]{};
    if (luaL_testudata(L, 2, MATRIX_MT))
    {
        const auto *matrix = check_matrix(L, 2);
        std::copy(std::begin(matrix->m), std::end(matrix->m), std::begin(m));
    }
    else
    {
        luaL_checktype(L, 2, LUA_TTABLE);
        const auto field = [L](int table, const char *name) {
            lua_getfield(L, table, name);
            const qreal value = lua_tonumber(L, -1);
            lua_pop(L, 1);
            return value;
        };
        m[0] = field(2, "m11");
        m[1] = field(2, "m12");
        m[2] = field(2, "m21");
        m[3] = field(2, "m22");
        m[4] = field(2, "m31");
        m[5] = field(2, "m32");
    }
    QTransform transform;
    transform.setMatrix(m[0], m[1], 0, m[2], m[3], 0, m[4], m[5], 1);
    p->state.transform = transform;
    return 0;
}

int painter_stroke(lua_State *L)
{
    queue_path_command(L, SELF, LuaDrawCmdType::StrokePath, L, 2, 3);
    return 0;
}

int painter_fill(lua_State *L)
{
    queue_path_command(L, SELF, LuaDrawCmdType::FillPath, L, 2, 0);
    return 0;
}

int painter_text(lua_State *L)
{
    auto *painter = SELF;
    const QString text = luaL_checkstring(L, 2);
    QRectF rect;
    checkRect(L, 3, rect);

    // like the Windows view, the text rides on the current path and fill/stroke draws it
    LuaTextRun run;
    run.text = text;
    run.rect = rect;
    run.font = font_from_style(L, 4);
    run.alignment = text_alignment(L, 4, run.wordWrap);
    if (lua_istable(L, 4))
    {
        lua_getfield(L, 4, "overflow");
        if (const char *overflow = lua_tostring(L, -1)) run.overflow = QString::fromUtf8(overflow);
        lua_pop(L, 1);
    }
    painter->state.texts.push_back(std::move(run));
    return 0;
}

void push_image_table(lua_State *L, const std::shared_ptr<QImage> &bitmap)
{
    // lua_newuserdata memory is uninitialized: construct the shared_ptr, do not assign into garbage
    auto *image = new (lua_newuserdata(L, sizeof(Image))) Image{bitmap};
    luaL_getmetatable(L, IMAGE_MT);
    lua_setmetatable(L, -2);
    return;
}

int painter_image(lua_State *L)
{
    auto *painter = SELF;
    Image *image = check_image(L, 2);
    QRectF destination;
    checkRect(L, 3, destination);

    QRectF source;
    QRectF center;
    QColor tint;
    qreal opacity = 1.0;
    Qt::TransformationMode sampling = Qt::SmoothTransformation;
    if (lua_istable(L, 4))
    {
        if (lua_getfield(L, 4, "source") != LUA_TNIL) checkRect(L, -1, source);
        lua_pop(L, 1);
        if (lua_getfield(L, 4, "tint") != LUA_TNIL) checkColor(L, -1, tint);
        lua_pop(L, 1);
        opacity = std::clamp(table_opt_number(L, 4, "opacity", 1.0), 0.0, 1.0);
        lua_getfield(L, 4, "sampling");
        if (const char *mode = lua_tostring(L, -1); mode && QString::fromUtf8(mode) == "nearest")
        {
            sampling = Qt::FastTransformation;
        }
        lua_pop(L, 1);
        if (lua_getfield(L, 4, "center") != LUA_TNIL) checkRect(L, -1, center);
        lua_pop(L, 1);
    }

    LuaCanvas *canvas = target_canvas(painter);
    if (!canvas) return 0;
    if (source.isNull() && image->bitmap) source = QRectF(0, 0, image->bitmap->width(), image->bitmap->height());
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::DrawImage;
    cmd.rect = destination;
    cmd.srcRect = source;
    if (center.isValid() && source.contains(center)) cmd.centerRect = center;
    cmd.sharedImage = image->bitmap;
    cmd.tint = tint;
    cmd.opacity = painter->state.alpha * opacity;
    cmd.sampling = sampling;
    cmd.transform = painter->state.transform;
    canvas->enqueueCommand(std::move(cmd));
    return 0;
}

int painter_gc(lua_State *)
{
    return 0;
}

// ---------------------------------------------------------------- PainterImage

int image_gc(lua_State *L)
{
    auto *image = static_cast<Image *>(lua_touserdata(L, 1));
    image->bitmap.reset();
    return 0;
}

int image_close(lua_State *L)
{
    return image_gc(L);
}

int image_index(lua_State *L)
{
    auto *image = static_cast<Image *>(lua_touserdata(L, 1));
    const QString key = luaL_checkstring(L, 2);
    if (key == "w")
    {
        lua_pushinteger(L, image->bitmap ? image->bitmap->width() : 0);
        return 1;
    }
    if (key == "h")
    {
        lua_pushinteger(L, image->bitmap ? image->bitmap->height() : 0);
        return 1;
    }
    if (key == "paint" || key == "close")
    {
        luaL_getmetatable(L, IMAGE_MT);
        lua_getfield(L, -1, key.toUtf8().constData());
        return 1;
    }
    return 0;
}

int image_paint(lua_State *L)
{
    auto *image = check_image(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    if (!image->bitmap) return luaL_error(L, "use of a closed PainterImage");

    lua_getfield(L, LUA_REGISTRYINDEX, "__lua_instance");
    auto *instance = static_cast<LuaInstance *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (!instance) return luaL_error(L, "no Lua instance");

    // a scratch canvas records the callback's drawing; it is applied onto the image on return
    LuaCanvas *scratch = new LuaCanvas();
    auto *ud = static_cast<Painter *>(lua_newuserdata(L, sizeof(Painter)));
    new (ud) Painter{instance, {}, scratch};
    luaL_getmetatable(L, PAINTER_MT);
    lua_setmetatable(L, -2);

    lua_pushvalue(L, 2); // callback
    lua_pushvalue(L, -2); // painter userdata
    const int result = lua_pcall(L, 1, 0, 0);

    const int top = lua_gettop(L);
    if (result != LUA_OK)
    {
        const char *error = lua_tostring(L, -1);
        const QString message = error ? error : "?";
        delete scratch;
        return luaL_error(L, "image:paint callback failed: %s", message.toUtf8().constData());
    }
    QPainter painter(image->bitmap.get());
    if (painter.isActive()) scratch->paint(&painter);
    delete scratch;
    lua_settop(L, top);
    return 0;
}

// ---------------------------------------------------------------- module

int mod_current(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__lua_instance");
    auto *instance = static_cast<LuaInstance *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (!instance) return luaL_error(L, "no Lua instance");
    LuaPainter::pushCurrent(L, instance);
    return 1;
}

int mod_new_image(lua_State *L)
{
    const int width = static_cast<int>(luaL_checkinteger(L, 1));
    const int height = static_cast<int>(luaL_checkinteger(L, 2));
    if (width <= 0 || height <= 0) return luaL_error(L, "image dimensions must be greater than zero");
    auto bitmap = std::make_shared<QImage>(width, height, QImage::Format_ARGB32_Premultiplied);
    bitmap->fill(Qt::transparent);
    push_image_table(L, bitmap);
    return 1;
}

std::shared_ptr<QImage> load_image_file(lua_State *L, const QString &path, QString &error)
{
    QImage image(path);
    if (image.isNull())
    {
        error = "failed to load image: " + path;
        return nullptr;
    }
    return std::make_shared<QImage>(image.convertToFormat(QImage::Format_ARGB32_Premultiplied));
}

int mod_load_image(lua_State *L)
{
    const QString path = QString::fromUtf8(luaL_checkstring(L, 1));
    QString error;
    auto bitmap = load_image_file(L, path, error);
    if (!bitmap)
    {
        lua_pushnil(L);
        lua_pushstring(L, error.toUtf8().constData());
        return 2;
    }
    push_image_table(L, bitmap);
    return 1;
}

int mod_decode_image(lua_State *L)
{
    QByteArray bytes;
    if (lua_isstring(L, 1)) bytes = QByteArray(lua_tolstring(L, 1, nullptr), lua_rawlen(L, 1));
    else if (lua_istable(L, 1))
    {
        const int count = static_cast<int>(lua_rawlen(L, 1));
        for (int i = 1; i <= count; ++i)
        {
            lua_rawgeti(L, 1, i);
            bytes.append(static_cast<char>(lua_tointeger(L, -1) & 0xff));
            lua_pop(L, 1);
        }
    }
    else return luaL_error(L, "decode_image expects image bytes as a string or byte table");

    QImage decoded = QImage::fromData(bytes);
    if (decoded.isNull())
    {
        lua_pushnil(L);
        lua_pushstring(L, "failed to decode image data");
        return 2;
    }
    push_image_table(L, std::make_shared<QImage>(decoded.convertToFormat(QImage::Format_ARGB32_Premultiplied)));
    return 1;
}

int mod_measure_text(lua_State *L)
{
    const QString text = luaL_checkstring(L, 1);
    const auto document = layout_document(text, L, 2, L, 3);
    const bool truncated = false; // QTextDocument-based metrics do not truncate
    apply_text_metrics(L, *document, truncated);
    return 1;
}

int mod_hittest_text_position(lua_State *L)
{
    const QString text = luaL_checkstring(L, 1);
    const qreal x = to_finite_or(L, 2, 0);
    const qreal y = to_finite_or(L, 3, 0);
    const auto document = layout_document(text, L, 4, L, 5);
    QTextLayout layout(text, document->defaultFont());
    layout.setTextOption(document->defaultTextOption());
    layout.beginLayout();
    int lineIndex = 0;
    int charIndex = -1;
    while (true)
    {
        QTextLine line = layout.createLine();
        if (!line.isValid()) break;
        line.setLineWidth(document->textWidth() > 0 ? document->textWidth() : 1e9);
        if (y >= line.y() && y <= line.y() + line.height())
        {
            charIndex = line.textStart() + line.xToCursor(x - line.x());
            break;
        }
        ++lineIndex;
    }
    layout.endLayout();
    if (charIndex < 0)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    // 1-based byte index into the UTF-8 text
    const QByteArray utf8 = text.left(charIndex).toUtf8();
    lua_pushinteger(L, utf8.size() + 1);
    lua_setfield(L, -2, "index");
    lua_pushinteger(L, lineIndex + 1);
    lua_setfield(L, -2, "line");
    lua_pushboolean(L, x >= 0 && y >= 0);
    lua_setfield(L, -2, "inside");
    return 1;
}

int mod_hittest_text_index(lua_State *L)
{
    const QString text = luaL_checkstring(L, 1);
    const int byteIndex = static_cast<int>(luaL_checkinteger(L, 2));
    const auto document = layout_document(text, L, 3, L, 4);
    QTextLayout layout(text, document->defaultFont());
    layout.setTextOption(document->defaultTextOption());
    layout.beginLayout();
    const QByteArray utf8Before = text.toUtf8().left(std::max(byteIndex - 1, 0));
    const int characterIndex = QString::fromUtf8(utf8Before).size();
    qreal x = 0;
    qreal y = 0;
    int lineIndex = 0;
    while (true)
    {
        QTextLine line = layout.createLine();
        if (!line.isValid()) break;
        line.setLineWidth(document->textWidth() > 0 ? document->textWidth() : 1e9);
        if (characterIndex >= line.textStart() && characterIndex <= line.textStart() + line.textLength())
        {
            int cursor = characterIndex;
            x = line.x() + line.cursorToX(&cursor);
            y = line.y();
            break;
        }
        ++lineIndex;
    }
    layout.endLayout();
    lua_newtable(L);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, lineIndex + 1);
    lua_setfield(L, -2, "line");
    return 1;
}

int mod_get_target_fps(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__lua_instance");
    auto *instance = static_cast<LuaInstance *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (!instance) return 0;
    if (LuaCanvas *canvas = instance->canvas())
    {
        lua_pushnumber(L, canvas->targetFps());
        return 1;
    }
    return 0;
}

int mod_set_target_fps(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "__lua_instance");
    auto *instance = static_cast<LuaInstance *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (!instance) return 0;
    if (LuaCanvas *canvas = instance->canvas())
    {
        canvas->setTargetFps(lua_isnoneornil(L, 1) ? 60.0 : to_finite_or(L, 1, 60.0));
    }
    return 0;
}

void registerTypes(lua_State *L)
{
    static const luaL_Reg matrixMethods[] = {
        {"dx", matrix_dx},
        {"dy", matrix_dy},
        {"sx", matrix_sx},
        {"sy", matrix_sy},
        {nullptr, nullptr},
    };
    luaL_newmetatable(L, MATRIX_MT);
    lua_pushcfunction(L, matrix_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, matrix_newindex);
    lua_setfield(L, -2, "__newindex");
    luaL_setfuncs(L, matrixMethods, 0);
    lua_pop(L, 1);

    static const luaL_Reg imageMethods[] = {
        {"paint", image_paint},
        {"close", image_close},
        {nullptr, nullptr},
    };
    luaL_newmetatable(L, IMAGE_MT);
    lua_pushcfunction(L, image_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, image_gc);
    lua_setfield(L, -2, "__gc");
    luaL_setfuncs(L, imageMethods, 0);
    lua_pop(L, 1);

    static const luaL_Reg painterMethods[] = {
        {"clear", painter_clear},
        {"begin_path", painter_begin_path},
        {"move_to", painter_move_to},
        {"line_to", painter_line_to},
        {"cubic_to", painter_cubic_to},
        {"quadratic_to", painter_quadratic_to},
        {"arc", painter_arc},
        {"close_path", painter_close_path},
        {"get_alpha", painter_get_alpha},
        {"set_alpha", painter_set_alpha},
        {"save", painter_save},
        {"restore", painter_restore},
        {"clip", painter_clip},
        {"translate", painter_translate},
        {"rotate", painter_rotate},
        {"scale", painter_scale},
        {"get_transform", painter_get_transform},
        {"set_transform", painter_set_transform},
        {"stroke", painter_stroke},
        {"fill", painter_fill},
        {"text", painter_text},
        {"rect", painter_rect},
        {"round_rect", painter_round_rect},
        {"circle", painter_circle},
        {"line", painter_line},
        {"polyline", painter_polyline},
        {"polygon", painter_polygon},
        {"image", painter_image},
        {nullptr, nullptr},
    };
    luaL_newmetatable(L, PAINTER_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, painter_gc);
    lua_setfield(L, -2, "__gc");
    luaL_setfuncs(L, painterMethods, 0);
    lua_pop(L, 1);
}

void pushCurrent(lua_State *L, LuaInstance *instance)
{
    auto *ud = static_cast<Painter *>(lua_newuserdata(L, sizeof(Painter)));
    new (ud) Painter{instance, {}};
    luaL_getmetatable(L, PAINTER_MT);
    lua_setmetatable(L, -2);
}

} // namespace LuaPainter
