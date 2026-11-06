/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QColor>
#include <QFont>
#include <QImage>
#include <QJSValue>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointer>
#include <QPolygonF>
#include <QQuickPaintedItem>
#include <QRectF>
#include <QString>
#include <QTransform>
#include <QVector>
#include <memory>
#include <mutex>
#include <qqmlintegration.h>

#include "LuaInstance.hpp"

enum class LuaDrawCmdType
{
    Clear,
    Line,
    Rect,
    FillRect,
    Ellipse,
    FillEllipse,
    Polygon,
    FillPolygon,
    Text,
    Image,
    Clip,
    ResetClip,
    // Painter API (views of the state captured when the command was queued)
    FillPath,
    StrokePath,
    DrawImage,
    PushClip,
    PopClip
};

struct LuaTextRun
{
    QString text;
    QRectF rect;
    QFont font;
    Qt::Alignment alignment{};
    bool wordWrap{false};
    QString overflow{QStringLiteral("clip")};
};

struct LuaDrawCommand
{
    LuaDrawCmdType type{LuaDrawCmdType::Clear};
    QRectF rect;
    QLineF line;
    QPolygonF polygon;
    QString text;
    QColor color;
    qreal strokeWidth{1.0};
    QFont font;
    QImage image;
    QString imagePath;
    QPainterPath path;
    QVector<LuaTextRun> runs;
    QBrush brush;
    QPen pen;
    QTransform transform;
    qreal opacity{1.0};
    QRectF srcRect;
    QRectF centerRect; // nine-slice center in image pixels; invalid = plain draw
    std::shared_ptr<const QImage> sharedImage;
    Qt::TransformationMode sampling{Qt::FastTransformation};
    QColor tint; // invalid = no tint
};

class LuaCanvas : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(LuaInstance *instance READ instance WRITE setInstance NOTIFY instanceChanged)
    Q_PROPERTY(qreal targetFps READ targetFps WRITE setTargetFps NOTIFY targetFpsChanged)
    Q_PROPERTY(bool hasContent READ hasContent NOTIFY hasContentChanged)
    Q_PROPERTY(int commandCount READ commandCount NOTIFY commandCountChanged)

  public:
    explicit LuaCanvas(QQuickItem *parent = nullptr);
    ~LuaCanvas() override = default;

    LuaInstance *instance() const { return m_instance; }
    void setInstance(LuaInstance *inst);

    qreal targetFps() const { return m_targetFps; }
    void setTargetFps(qreal fps);

    bool hasContent() const { return !m_commands.isEmpty(); }
    int commandCount() const { return static_cast<int>(m_commands.size()); }

    void paint(QPainter *painter) override;

    void addClear();
    void addLine(qreal x1, qreal y1, qreal x2, qreal y2, const QColor &color, qreal width = 1.0);
    void addRect(qreal x, qreal y, qreal w, qreal h, const QColor &color, qreal width = 1.0);
    void addFillRect(qreal x, qreal y, qreal w, qreal h, const QColor &color);
    void addEllipse(qreal x, qreal y, qreal rx, qreal ry, const QColor &color, qreal width = 1.0);
    void addFillEllipse(qreal x, qreal y, qreal rx, qreal ry, const QColor &color);
    void addPolygon(const QPolygonF &points, const QColor &color, qreal width = 1.0);
    void addFillPolygon(const QPolygonF &points, const QColor &color);
    void addText(qreal x, qreal y, const QString &text, const QColor &color, const QFont &font = QFont());
    void addImage(qreal x, qreal y, const QString &path, qreal w = -1, qreal h = -1);
    void addClip(qreal x, qreal y, qreal w, qreal h);
    void addResetClip();

    // Painter API: commands carry their resolved state, queued as the script records them
    void enqueueCommand(LuaDrawCommand cmd);
    void enqueueCommand(LuaDrawCmdType type);

    Q_INVOKABLE void clear();
    void clear(const QColor &background);
    Q_INVOKABLE void requestPaint();
    Q_INVOKABLE void renderToContext(QJSValue ctx);

  signals:
    void instanceChanged();
    void targetFpsChanged();
    void hasContentChanged();
    void commandCountChanged();
    void cleared();
    void needsPaint();

  private:
    void appendCommand(LuaDrawCommand cmd);

    QPointer<LuaInstance> m_instance{nullptr};
    qreal m_targetFps{60.0};
    QVector<LuaDrawCommand> m_commands;
    mutable std::mutex m_mutex;
};
