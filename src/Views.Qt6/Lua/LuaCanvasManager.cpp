/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaCanvasManager.hpp"
#include "LuaCanvas.hpp"
#include "LuaInstance.hpp"
#include "LuaInstanceManager.hpp"

LuaCanvasManager::LuaCanvasManager(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, false);
}

LuaCanvasManager::~LuaCanvasManager()
{
    clearAll();
}

void LuaCanvasManager::setInstanceManager(LuaInstanceManager *manager)
{
    if (m_instanceManager == manager) return;

    if (m_instanceManager)
    {
        disconnect(m_instanceManager, nullptr, this, nullptr);
        clearAll();
    }

    m_instanceManager = manager;

    if (m_instanceManager)
    {
        connect(m_instanceManager, &LuaInstanceManager::instanceAdded, this, &LuaCanvasManager::setupCanvasForInstance);
        connect(m_instanceManager, &LuaInstanceManager::instanceRemoved, this, &LuaCanvasManager::removeCanvasForInstance);

        for (auto *inst : m_instanceManager->instances())
        {
            setupCanvasForInstance(inst);
        }
    }

    emit instanceManagerChanged();
}

void LuaCanvasManager::setDisplay(QQuickItem *display)
{
    if (m_display == display) return;

    if (m_display)
    {
        disconnect(m_display, nullptr, this, nullptr);
    }

    m_display = display;

    if (m_display)
    {
        connect(m_display, &QQuickItem::widthChanged, this, &LuaCanvasManager::updateCanvasesGeometry);
        connect(m_display, &QQuickItem::heightChanged, this, &LuaCanvasManager::updateCanvasesGeometry);
    }

    updateCanvasesGeometry();
    emit displayChanged();
}

void LuaCanvasManager::setupCanvasForInstance(LuaInstance *instance)
{
    if (!instance || m_canvases.contains(instance)) return;

    auto *canvas = new LuaCanvas(this);
    canvas->setInstance(instance);

    qreal w = (m_display && m_display->width() > 0) ? m_display->width() : width();
    qreal h = (m_display && m_display->height() > 0) ? m_display->height() : height();
    canvas->setSize(QSizeF(w, h));

    instance->setCanvas(canvas);
    m_canvases[instance] = canvas;

    emit canvasCreated(canvas);
    emit canvasCountChanged();
}

void LuaCanvasManager::removeCanvasForInstance(LuaInstance *instance)
{
    if (!instance) return;

    auto it = m_canvases.find(instance);
    if (it != m_canvases.end())
    {
        auto *canvas = it->second;
        m_canvases.erase(it);

        if (instance->canvas() == canvas)
        {
            instance->setCanvas(nullptr);
        }

        emit canvasDestroyed(canvas);
        emit canvasCountChanged();

        canvas->deleteLater();
    }
}

LuaCanvas *LuaCanvasManager::getCanvasForInstance(LuaInstance *instance) const
{
    auto it = m_canvases.find(instance);
    return (it != m_canvases.end()) ? it->second : nullptr;
}

void LuaCanvasManager::clearAll()
{
    for (auto &[inst, canvas] : m_canvases)
    {
        if (inst && inst->canvas() == canvas)
        {
            inst->setCanvas(nullptr);
        }
        emit canvasDestroyed(canvas);
        canvas->deleteLater();
    }
    m_canvases.clear();
    emit canvasCountChanged();
}

void LuaCanvasManager::requestRepaintAll()
{
    for (auto &[inst, canvas] : m_canvases)
    {
        if (canvas) canvas->requestPaint();
    }
}

void LuaCanvasManager::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    updateCanvasesGeometry();
}

void LuaCanvasManager::updateCanvasesGeometry()
{
    qreal w = (m_display && m_display->width() > 0) ? m_display->width() : width();
    qreal h = (m_display && m_display->height() > 0) ? m_display->height() : height();

    for (auto &[inst, canvas] : m_canvases)
    {
        if (canvas)
        {
            canvas->setSize(QSizeF(w, h));
        }
    }
}
