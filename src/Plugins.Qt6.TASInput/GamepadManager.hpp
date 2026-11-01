/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QTimer>

struct SDL_Gamepad;

class TASGamepadManager : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GamepadManager)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY connectedChanged)

    Q_PROPERTY(int stickX READ stickX NOTIFY stickChanged)
    Q_PROPERTY(int stickY READ stickY NOTIFY stickChanged)

    Q_PROPERTY(bool btnA READ btnA NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnB READ btnB NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnZ READ btnZ NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnS READ btnS NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnL READ btnL NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnR READ btnR NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnDU READ btnDU NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnDD READ btnDD NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnDL READ btnDL NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnDR READ btnDR NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnCU READ btnCU NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnCD READ btnCD NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnCL READ btnCL NOTIFY buttonsChanged)
    Q_PROPERTY(bool btnCR READ btnCR NOTIFY buttonsChanged)
  public:
    enum Button
    {
        A,
        B,
        Z,
        S,
        L,
        R,
        DU,
        DD,
        DL,
        DR,
        CU,
        CD,
        CL,
        CR,
    };

    TASGamepadManager(QObject *parent = nullptr);
    virtual ~TASGamepadManager();

    bool enabled() { return m_enabled; }
    void setEnabled(bool value);

    bool connected() { return m_gamepad != nullptr; }
    QString deviceName() { return m_device_name; }

    int stickX() { return m_stick_x; }
    int stickY() { return m_stick_y; }

    bool btnA() { return button(A); }
    bool btnB() { return button(B); }
    bool btnZ() { return button(Z); }
    bool btnS() { return button(S); }
    bool btnL() { return button(L); }
    bool btnR() { return button(R); }
    bool btnDU() { return button(DU); }
    bool btnDD() { return button(DD); }
    bool btnDL() { return button(DL); }
    bool btnDR() { return button(DR); }
    bool btnCU() { return button(CU); }
    bool btnCD() { return button(CD); }
    bool btnCL() { return button(CL); }
    bool btnCR() { return button(CR); }

    Q_INVOKABLE void poll();

  signals:
    void enabledChanged();
    void connectedChanged();
    void stickChanged();
    void buttonsChanged();

    void buttonEdge(int button, bool pressed);
    void stickMoved(int x, int y);

  private:
    bool button(Button b) { return (m_buttons >> b) & 1; }
    void openGamepad();
    void closeGamepad();

    QTimer m_timer;
    bool m_sdl_ready = false;
    bool m_enabled = true;
    int m_ticks_since_scan = 0;

    SDL_Gamepad *m_gamepad = nullptr;
    QString m_device_name;

    uint32_t m_buttons = 0;
    int m_stick_x = 0;
    int m_stick_y = 0;
};
