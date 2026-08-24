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
    m_frame = QImage((int32_t) width, (int32_t) height, QImage::Format_RGBA8888);
    setImplicitWidth(width);
    setImplicitHeight(height);
}

void EmuDisplay::readPixels()
{
    if (m_context == nullptr) return;
    m_context->readVideoOutput(m_frame);
    update();
}

QSGNode *EmuDisplay::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto* window = this->window();
    auto* imageNode = (oldNode)? static_cast<QSGImageNode*>(oldNode) : window->createImageNode();

    imageNode->setSourceRect(QRectF(QPoint(0, 0), m_frame.size()));
    imageNode->setRect(QRectF(boundingRect().topLeft(), m_frame.size()));
    imageNode->setTexture(window->createTextureFromImage(m_frame));
    imageNode->setOwnsTexture(true);

    return imageNode;
}