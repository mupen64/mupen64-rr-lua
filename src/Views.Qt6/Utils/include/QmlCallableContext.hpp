/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QJSEngine>
#include <QJSValue>
#include <qqmlintegration.h>

class QmlCallableContext : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
  public:
    using JSCallable = QJSValue(QJSEngine *, const QJSValue &);

    QmlCallableContext(QObject *parent = nullptr) : QObject(parent) {}
    QmlCallableContext(std::function<JSCallable> &&fn, QObject *parent = nullptr)
        : QObject(parent), m_callback(std::move(fn))
    {
    }

    QmlCallableContext(const QmlCallableContext &rhs) : m_callback(rhs.m_callback) {}
    QmlCallableContext &operator=(const QmlCallableContext &rhs)
    {
        if (this == &rhs) return *this;
        m_callback = rhs.m_callback;
        return *this;
    }

    Q_INVOKABLE QJSValue call(const QJSValue &args) { return m_callback(qjsEngine(this), args); }

  private:
    std::function<JSCallable> m_callback;
};