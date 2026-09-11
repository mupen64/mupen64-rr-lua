/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ActionHelpers.hpp"
#include <print>

static QVariant sequenceToKeyImpl(const QKeySequence &seq)
{
    if (seq.count() != 1) return {};

    auto combo = seq[0];
    if (combo.keyboardModifiers() != 0) return {};

    return combo.key();
}

QVariant ActionHelpers::sequenceToKey(QVariant variant)
{
    // bool converted = false;
    auto type = variant.metaType();
    if (type.id() == QMetaType::Int)
    {
        return sequenceToKeyImpl(QKeySequence::keyBindings((QKeySequence::StandardKey) variant.toInt())[0]);
    }

    if (type.id() == QMetaType::QString || type.id() == QMetaType::QChar)
    {
        return sequenceToKeyImpl(QKeySequence::fromString(variant.toString()));
    }
    return {};
}

QVariant ActionHelpers::sequenceListToKeys(QVariantList qmlSequence)
{
    QVariantList output{};
    output.reserve(qmlSequence.size());

    for (qsizetype i = 0; i < qmlSequence.size(); ++i)
    {
        auto key = sequenceToKey(qmlSequence[i]);
        if (key.isNull())
            return {};
        output.push_back(key);
    }

    return output;
}
