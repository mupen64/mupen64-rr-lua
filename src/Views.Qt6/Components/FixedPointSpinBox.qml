/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls

SpinBox {
    id: root
    property int decimals: 2

    property double dFrom: 0.0
    property double dTo: 99.99
    property double dStepSize: 1.0
    property double dValue: 0.0

    from: Math.round(dFrom * priv.factor)
    to: Math.round(dTo * priv.factor)
    stepSize: Math.round(dStepSize * priv.factor)
    value: dValue * priv.factor

    Binding {
        root.dValue: root.value / priv.factor
    }

    QtObject {
        id: priv
        readonly property double factor: Math.pow(10, root.decimals)
    }

    validator: DoubleValidator {
        bottom: Math.min(root.dFrom, root.dTo)
        top: Math.max(root.dFrom, root.dTo)
    }

    textFromValue: (value, locale) => {
        return Number(value / priv.factor).toLocaleString(locale, 'f', root.decimals)
    }
    valueFromText: (text, locale) => {
        return Number.fromLocaleString(locale, text) * priv.factor
    }
}
