/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QImage>
#include <QQuickItem>
#include <qqmlintegration.h>

#include "EmuContext.hpp"

class EmuDisplay : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(EmuContext *context MEMBER m_context READ context WRITE setContext NOTIFY contextChanged)
  public:
    // Inherited stuff
    // ============================================

    EmuDisplay(QQuickItem *parent = nullptr);
    virtual ~EmuDisplay() {}

    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updateData) override;

    // Properties
    // ============================================

    EmuContext *context() const { return m_context; }

    void setContext(EmuContext *context)
    {
        if (m_context == context) return;
        m_context = context;
        contextChanged();
    }

    // Graphics updates
    // ============================================

    /**
     * @brief Preemptively resizes the video buffer in preparation to receive data.
     *
     * @param width The request
     * @param height
     */
    Q_INVOKABLE void reserveSize(uint32_t width, uint32_t height);

    /**
     * @brief Reads the current context's video buffer into the internal buffer.
     */
    Q_INVOKABLE void readPixels();

  signals:

    // Properties
    // ============================================
    void contextChanged();

  private:
    void visibleChangedImpl();

    EmuContext *m_context;
    QImage m_frame;
};