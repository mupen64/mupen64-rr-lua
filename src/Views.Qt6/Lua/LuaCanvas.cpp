/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaCanvas.hpp"
#include "LuaInstance.hpp"

#include <QPen>

LuaCanvas::LuaCanvas(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setOpaquePainting(false);
    setFillColor(Qt::transparent);
}

void LuaCanvas::setInstance(LuaInstance *inst)
{
    if (m_instance == inst) return;
    m_instance = inst;
    emit instanceChanged();
}

void LuaCanvas::setTargetFps(qreal fps)
{
    if (qFuzzyCompare(m_targetFps, fps)) return;
    m_targetFps = fps;
    emit targetFpsChanged();
}

void LuaCanvas::appendCommand(LuaDrawCommand cmd)
{
    {
        std::lock_guard lock(m_mutex);
        m_commands.append(std::move(cmd));
    }
    emit hasContentChanged();
    emit commandCountChanged();
}

void LuaCanvas::addClear()
{
    clear();
}

void LuaCanvas::addLine(qreal x1, qreal y1, qreal x2, qreal y2, const QColor &color, qreal width)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::Line;
    cmd.line = QLineF(x1, y1, x2, y2);
    cmd.color = color;
    cmd.strokeWidth = width;
    appendCommand(std::move(cmd));
}

void LuaCanvas::addRect(qreal x, qreal y, qreal w, qreal h, const QColor &color, qreal width)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::Rect;
    cmd.rect = QRectF(x, y, w, h);
    cmd.color = color;
    cmd.strokeWidth = width;
    appendCommand(std::move(cmd));
}

void LuaCanvas::addFillRect(qreal x, qreal y, qreal w, qreal h, const QColor &color)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::FillRect;
    cmd.rect = QRectF(x, y, w, h);
    cmd.color = color;
    appendCommand(std::move(cmd));
}

void LuaCanvas::addEllipse(qreal x, qreal y, qreal rx, qreal ry, const QColor &color, qreal width)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::Ellipse;
    cmd.rect = QRectF(x - rx, y - ry, rx * 2.0, ry * 2.0);
    cmd.color = color;
    cmd.strokeWidth = width;
    appendCommand(std::move(cmd));
}

void LuaCanvas::addFillEllipse(qreal x, qreal y, qreal rx, qreal ry, const QColor &color)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::FillEllipse;
    cmd.rect = QRectF(x - rx, y - ry, rx * 2.0, ry * 2.0);
    cmd.color = color;
    appendCommand(std::move(cmd));
}

void LuaCanvas::addPolygon(const QPolygonF &points, const QColor &color, qreal width)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::Polygon;
    cmd.polygon = points;
    cmd.color = color;
    cmd.strokeWidth = width;
    appendCommand(std::move(cmd));
}

void LuaCanvas::addFillPolygon(const QPolygonF &points, const QColor &color)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::FillPolygon;
    cmd.polygon = points;
    cmd.color = color;
    appendCommand(std::move(cmd));
}

void LuaCanvas::addText(qreal x, qreal y, const QString &text, const QColor &color, const QFont &font)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::Text;
    cmd.rect = QRectF(x, y, 0, 0);
    cmd.text = text;
    cmd.color = color;
    cmd.font = font;
    appendCommand(std::move(cmd));
}

void LuaCanvas::addImage(qreal x, qreal y, const QString &path, qreal w, qreal h)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::Image;
    cmd.imagePath = path;
    cmd.image = QImage(path);
    qreal iw = (w > 0) ? w : cmd.image.width();
    qreal ih = (h > 0) ? h : cmd.image.height();
    cmd.rect = QRectF(x, y, iw, ih);
    appendCommand(std::move(cmd));
}

void LuaCanvas::addClip(qreal x, qreal y, qreal w, qreal h)
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::Clip;
    cmd.rect = QRectF(x, y, w, h);
    appendCommand(std::move(cmd));
}

void LuaCanvas::addResetClip()
{
    LuaDrawCommand cmd;
    cmd.type = LuaDrawCmdType::ResetClip;
    appendCommand(std::move(cmd));
}

void LuaCanvas::clear()
{
    {
        std::lock_guard lock(m_mutex);
        m_commands.clear();
    }
    emit hasContentChanged();
    emit commandCountChanged();
    emit cleared();
    update();
}

void LuaCanvas::requestPaint()
{
    update();
    emit needsPaint();
}

void LuaCanvas::paint(QPainter *painter)
{
    std::vector<LuaDrawCommand> cmds;
    {
        std::lock_guard lock(m_mutex);
        cmds.reserve(m_commands.size());
        for (const auto &c : m_commands) cmds.push_back(c);
    }

    if (cmds.empty()) return;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    for (const auto &cmd : cmds)
    {
        switch (cmd.type)
        {
        case LuaDrawCmdType::Clear:
            painter->eraseRect(boundingRect());
            break;
        case LuaDrawCmdType::Line: {
            QPen pen(cmd.color, cmd.strokeWidth);
            painter->setPen(pen);
            painter->drawLine(cmd.line);
            break;
        }
        case LuaDrawCmdType::Rect: {
            QPen pen(cmd.color, cmd.strokeWidth);
            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(cmd.rect);
            break;
        }
        case LuaDrawCmdType::FillRect: {
            painter->fillRect(cmd.rect, cmd.color);
            break;
        }
        case LuaDrawCmdType::Ellipse: {
            QPen pen(cmd.color, cmd.strokeWidth);
            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(cmd.rect);
            break;
        }
        case LuaDrawCmdType::FillEllipse: {
            painter->setPen(Qt::NoPen);
            painter->setBrush(cmd.color);
            painter->drawEllipse(cmd.rect);
            break;
        }
        case LuaDrawCmdType::Polygon: {
            QPen pen(cmd.color, cmd.strokeWidth);
            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);
            painter->drawPolygon(cmd.polygon);
            break;
        }
        case LuaDrawCmdType::FillPolygon: {
            painter->setPen(Qt::NoPen);
            painter->setBrush(cmd.color);
            painter->drawPolygon(cmd.polygon);
            break;
        }
        case LuaDrawCmdType::Text: {
            painter->setPen(cmd.color);
            painter->setFont(cmd.font);
            painter->drawText(cmd.rect.topLeft(), cmd.text);
            break;
        }
        case LuaDrawCmdType::Image: {
            if (!cmd.image.isNull())
            {
                painter->drawImage(cmd.rect, cmd.image);
            }
            break;
        }
        case LuaDrawCmdType::Clip:
            painter->save();
            painter->setClipRect(cmd.rect);
            break;
        case LuaDrawCmdType::ResetClip:
            painter->restore();
            break;
        }
    }
}

void LuaCanvas::renderToContext(QJSValue ctx)
{
    using namespace Qt::Literals;

    if (!ctx.isObject()) return;

    std::vector<LuaDrawCommand> cmds;
    {
        std::lock_guard lock(m_mutex);
        cmds.reserve(m_commands.size());
        for (const auto &c : m_commands) cmds.push_back(c);
    }

    for (const auto &cmd : cmds)
    {
        switch (cmd.type)
        {
        case LuaDrawCmdType::Clear:
            ctx.property(u"clearRect"_s).call({0, 0, width(), height()});
            break;
        case LuaDrawCmdType::Line: {
            ctx.property(u"beginPath"_s).call();
            ctx.property(u"moveTo"_s).call({cmd.line.x1(), cmd.line.y1()});
            ctx.property(u"lineTo"_s).call({cmd.line.x2(), cmd.line.y2()});
            ctx.setProperty(u"strokeStyle"_s, cmd.color.name(QColor::HexArgb));
            ctx.setProperty(u"lineWidth"_s, cmd.strokeWidth);
            ctx.property(u"stroke"_s).call();
            break;
        }
        case LuaDrawCmdType::Rect: {
            ctx.setProperty(u"strokeStyle"_s, cmd.color.name(QColor::HexArgb));
            ctx.setProperty(u"lineWidth"_s, cmd.strokeWidth);
            ctx.property(u"strokeRect"_s).call({cmd.rect.x(), cmd.rect.y(), cmd.rect.width(), cmd.rect.height()});
            break;
        }
        case LuaDrawCmdType::FillRect: {
            ctx.setProperty(u"fillStyle"_s, cmd.color.name(QColor::HexArgb));
            ctx.property(u"fillRect"_s).call({cmd.rect.x(), cmd.rect.y(), cmd.rect.width(), cmd.rect.height()});
            break;
        }
        case LuaDrawCmdType::Ellipse: {
            ctx.property(u"beginPath"_s).call();
            ctx.property(u"ellipse"_s)
                .call({cmd.rect.center().x(), cmd.rect.center().y(), cmd.rect.width() / 2.0, cmd.rect.height() / 2.0, 0,
                    0, 6.2831853});
            ctx.setProperty(u"strokeStyle"_s, cmd.color.name(QColor::HexArgb));
            ctx.setProperty(u"lineWidth"_s, cmd.strokeWidth);
            ctx.property(u"stroke"_s).call();
            break;
        }
        case LuaDrawCmdType::FillEllipse: {
            ctx.property(u"beginPath"_s).call();
            ctx.property(u"ellipse"_s)
                .call({cmd.rect.center().x(), cmd.rect.center().y(), cmd.rect.width() / 2.0, cmd.rect.height() / 2.0, 0,
                    0, 6.2831853});
            ctx.setProperty(u"fillStyle"_s, cmd.color.name(QColor::HexArgb));
            ctx.property(u"fill"_s).call();
            break;
        }
        case LuaDrawCmdType::Text: {
            ctx.setProperty(u"fillStyle"_s, cmd.color.name(QColor::HexArgb));
            int sz = cmd.font.pixelSize() > 0 ? cmd.font.pixelSize() : 12;
            QString fontStr = QString("%1px %2").arg(sz).arg(cmd.font.family());
            ctx.setProperty(u"font"_s, fontStr);
            ctx.property(u"fillText"_s).call({cmd.text, cmd.rect.x(), cmd.rect.y()});
            break;
        }
        default:
            break;
        }
    }
}
