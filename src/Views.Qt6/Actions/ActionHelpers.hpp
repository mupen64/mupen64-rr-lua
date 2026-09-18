/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QString>
#include <QKeyCombination>
#include <QKeySequence>
#include <QVariant>
#include <qqmlintegration.h>

class ActionHelpers : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
  public:
    ActionHelpers(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief Convert a Shortcut::sequence property value to a single Qt::Key, if possible.
     * @param qmlSequence the sequence being matched
     * @return The matching key if the conversion succeeded. Otherwise, null.
     *
     * A sequence can be converted to a key if all of the following are true:
     * - The sequence only contains one combination
     * - The sequence does not use any modifiers whatsoever
     */
    Q_INVOKABLE QVariant sequenceToKey(const QVariant& qmlSequence);

    /**
     * @brief Convert a Shortcut::sequences property value to a list of keys, if possible.
     * @param qmlSequences the sequences being matched
     * @return The matching keys if the conversion succeeded. Otherwise, null.
     *
     * @note See sequenceToKey() for more details.
     */
    Q_INVOKABLE QVariant sequenceListToKeys(const QVariantList& qmlSequences);

    /**
     * Converts a sequence of combined Qt key codes (e.g. Qt.Key_5 | Qt.KeypadModifier) to
     * a string key sequence.
     */
    Q_INVOKABLE QString fromIntKeys(const QVariantList& keyList);
};

constexpr size_t bleh = sizeof(QVariant);
