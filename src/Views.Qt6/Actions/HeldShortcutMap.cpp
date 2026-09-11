/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "HeldShortcutMap.hpp"
#include <QEvent>
#include <QKeyEvent>
#include <stdexcept>

using namespace Qt::Literals;

static constexpr const char* HELD_SHORTCUT_PROP = "_priv_g_heldShortcutMap";

HeldShortcutMap::HeldShortcutMap(QObject* parent) : QObject(parent) {
    if (qGuiApp->property(HELD_SHORTCUT_PROP).isValid())
        throw std::logic_error("HeldShortcutMap can only be instantiated once");
    qGuiApp->setProperty(HELD_SHORTCUT_PROP, QVariant::fromValue(this));

    qGuiApp->installEventFilter(this);
}

HeldShortcutMap::~HeldShortcutMap() {
    qGuiApp->setProperty(HELD_SHORTCUT_PROP, QVariant {});
}

bool HeldShortcutMap::addShortcut(Qt::Key key, QObject* object) {
    if (object == nullptr)
        return false;
    auto [node, inserted] = m_shortcuts.try_emplace(key, object);
    return inserted;
}
void HeldShortcutMap::clearShortcuts(QObject* object) {
    if (object == nullptr)
    {
        m_shortcuts.clear();
        return;
    }

    auto it = m_shortcuts.begin();
    while (it != m_shortcuts.end()) {
        if (it->second == object)
            it = m_shortcuts.erase(it);
        else
            ++it;
    }
}

bool HeldShortcutMap::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease)
    {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        auto matchedNode = m_shortcuts.find((Qt::Key) keyEvent->key());
        if (matchedNode != m_shortcuts.end())
        {
            // trigger signal "released"
            QMetaObject::invokeMethod(matchedNode->second, "released");
        }

    }
    return false;
}
