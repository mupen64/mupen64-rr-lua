/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QPointer>
#include <QQuickItem>
#include <qqmlintegration.h>
#include <unordered_map>

#include "LuaInstanceManager.hpp"

class LuaCanvas;

class LuaCanvasManager : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
        LuaInstanceManager *instanceManager READ instanceManager WRITE setInstanceManager NOTIFY instanceManagerChanged)
    Q_PROPERTY(QQuickItem *display READ display WRITE setDisplay NOTIFY displayChanged)
    Q_PROPERTY(int canvasCount READ canvasCount NOTIFY canvasCountChanged)

  public:
    explicit LuaCanvasManager(QQuickItem *parent = nullptr);
    ~LuaCanvasManager() override;

    LuaInstanceManager *instanceManager() const { return m_instanceManager; }
    void setInstanceManager(LuaInstanceManager *manager);

    QQuickItem *display() const { return m_display; }
    void setDisplay(QQuickItem *display);

    int canvasCount() const { return static_cast<int>(m_canvases.size()); }

    Q_INVOKABLE LuaCanvas *getCanvasForInstance(LuaInstance *instance) const;
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE void requestRepaintAll();

  signals:
    void instanceManagerChanged();
    void displayChanged();
    void canvasCountChanged();
    void canvasCreated(LuaCanvas *canvas);
    void canvasDestroyed(LuaCanvas *canvas);

  protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

  private:
    void setupCanvasForInstance(LuaInstance *instance);
    void removeCanvasForInstance(LuaInstance *instance);
    void updateCanvasesGeometry();

    QPointer<LuaInstanceManager> m_instanceManager{nullptr};
    QPointer<QQuickItem> m_display{nullptr};
    std::unordered_map<LuaInstance *, LuaCanvas *> m_canvases;
};
