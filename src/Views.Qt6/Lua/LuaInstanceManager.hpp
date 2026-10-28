/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QPointer>
#include <qqmlintegration.h>

#include <Core/Types.hpp>
#include "LuaInstance.hpp"
#include "EmuContext.hpp"

class LuaInstanceManager : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(EmuContext *context READ context WRITE setContext NOTIFY contextChanged)
    Q_PROPERTY(QList<LuaInstance *> instances READ instances NOTIFY instancesChanged)

  public:
    enum Roles
    {
        NameRole = Qt::DisplayRole,
        PathRole = Qt::UserRole + 1,
        RunningRole,
        TrustedRole,
        LogsRole,
        LastErrorRole,
        InstanceRole
    };
    Q_ENUM(Roles)

    explicit LuaInstanceManager(QObject *parent = nullptr);
    ~LuaInstanceManager() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_instances.size()); }
    QList<LuaInstance *> instances() const { return m_instances; }

    EmuContext *context() const { return m_context; }
    void setContext(EmuContext *ctx);

    Q_INVOKABLE LuaInstance *addInstance(const QString &path, bool autoStart = false);
    Q_INVOKABLE void removeInstance(int index);
    Q_INVOKABLE void removeInstance(LuaInstance *instance);
    Q_INVOKABLE LuaInstance *get(int index) const;
    Q_INVOKABLE LuaInstance *getByPath(const QString &path) const;

    Q_INVOKABLE void startAll();
    Q_INVOKABLE void stopAll();
    Q_INVOKABLE void restartAll();
    Q_INVOKABLE void clearAll();

    void dispatchUpdateScreen();
    void dispatchPaint();
    void dispatchVI();
    void dispatchInput(CoreButtons *input, int controllerIndex);
    void dispatchInterval();
    void dispatchReset();
    void dispatchSaveState();
    void dispatchLoadState();
    void dispatchKey(int key, bool pressed, bool ctrl, bool alt, bool shift);
    void dispatchMouse(int x, int y, int button, bool pressed);

  signals:
    void countChanged();
    void contextChanged();
    void instancesChanged();
    void instanceAdded(LuaInstance *instance);
    void instanceRemoved(LuaInstance *instance);

  private:
    void connectInstance(LuaInstance *inst, int row);

    QList<LuaInstance *> m_instances;
    QPointer<EmuContext> m_context{nullptr};
};
