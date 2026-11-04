/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <QObject>
#include <QQmlEngine>

class TASJoystickData : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(JoystickData)

    Q_PROPERTY(int posX READ posX WRITE setPosX NOTIFY posXChanged)
    Q_PROPERTY(int posY READ posY WRITE setPosY NOTIFY posYChanged)
  public:
    TASJoystickData(QObject *parent = nullptr) : QObject(parent) {}

    int posX() { return m_posX; }
    void setPosX(int value)
    {
        if (value == m_posX) return;
        m_posX = value;
        posXChanged();
    }

    int posY() { return m_posY; }
    void setPosY(int value)
    {
        if (value == m_posY) return;
        m_posY = value;
        posYChanged();
    }

    Q_INVOKABLE void setPos(int x, int y) {
        setPosX(x);
        setPosY(y);
    }
  signals:
    void posXChanged();
    void posYChanged();

  private:
    int m_posX = 0;
    int m_posY = 0;
};
