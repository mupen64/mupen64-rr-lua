/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "EmuDisplay.hpp"

#include <print>

#include <QQuickWindow>
#include <QSGImageNode>

EmuDisplay::EmuDisplay(QQuickItem *parent) : QQuickItem(parent), m_frame()
{
    setFlags(QQuickItem::ItemHasContents);
}
void EmuDisplay::reserveSize(uint32_t width, uint32_t height)
{
    setImplicitWidth(width);
    setImplicitHeight(height);
}

void EmuDisplay::readPixels()
{
    static int s_counter = 10;

    if (m_context == nullptr) return;
    m_context->readVideoOutput(m_frame);
    update();
}

QSGNode *EmuDisplay::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto *window = this->window();
    auto *imageNode = (oldNode) ? static_cast<QSGImageNode *>(oldNode) : window->createImageNode();

    // set texture from current frame
    imageNode->setTexture(window->createTextureFromImage(m_frame));
    imageNode->setOwnsTexture(true);

    // set display bounding boxes
    imageNode->setSourceRect(QRectF(QPoint(0, 0), m_frame.size()));
    imageNode->setRect(boundingRect());

    // OpenGL returns pixel data flipped, reflip the texture
    imageNode->setTextureCoordinatesTransform(QSGImageNode::MirrorVertically);

    return imageNode;
}