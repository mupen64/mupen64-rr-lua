#include "EmuDisplay.hpp"
#include <QQuickWindow>

EmuDisplay::EmuDisplay(QQuickItem* parent) : QQuickItem(parent), m_buffer() {

}

// QSGNode *EmuDisplay::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updateData)
// {
//     auto* window = this->window();
// }