/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "LuaInstanceManager.hpp"
#include "EmuContext.hpp"

LuaInstanceManager::LuaInstanceManager(QObject *parent) : QAbstractListModel(parent)
{
}

LuaInstanceManager::~LuaInstanceManager()
{
    clearAll();
}

int LuaInstanceManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_instances.size());
}

QVariant LuaInstanceManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_instances.size())
    {
        return QVariant();
    }

    auto *inst = m_instances[index.row()];
    switch (role)
    {
    case NameRole:
        return inst->name();
    case PathRole:
        return inst->path();
    case RunningRole:
        return inst->isRunning();
    case TrustedRole:
        return inst->isTrusted();
    case LogsRole:
        return inst->logs();
    case LastErrorRole:
        return inst->lastError();
    case InstanceRole:
        return QVariant::fromValue(inst);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LuaInstanceManager::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        {NameRole, "name"},
        {PathRole, "path"},
        {RunningRole, "running"},
        {TrustedRole, "trusted"},
        {LogsRole, "logs"},
        {LastErrorRole, "lastError"},
        {InstanceRole, "instance"},
    };
    return roles;
}

void LuaInstanceManager::setContext(EmuContext *ctx)
{
    if (m_context == ctx) return;

    if (m_context)
    {
        disconnect(m_context, nullptr, this, nullptr);
    }

    m_context = ctx;

    if (m_context)
    {
        connect(m_context, &EmuContext::updateScreen, this, [this]() {
            dispatchUpdateScreen();
            dispatchPaint();
        });

        connect(m_context, &EmuContext::launchedChanged, this, [this](bool launched) {
            if (!launched)
            {
                dispatchReset();
            }
        });
    }

    emit contextChanged();
}

void LuaInstanceManager::connectInstance(LuaInstance *inst, int row)
{
    auto notifyRowChanged = [this, inst]() {
        int r = m_instances.indexOf(inst);
        if (r >= 0)
        {
            QModelIndex idx = index(r);
            emit dataChanged(idx, idx);
        }
    };

    connect(inst, &LuaInstance::runningChanged, this, notifyRowChanged);
    connect(inst, &LuaInstance::logsChanged, this, notifyRowChanged);
    connect(inst, &LuaInstance::lastErrorChanged, this, notifyRowChanged);
    connect(inst, &LuaInstance::trustedChanged, this, notifyRowChanged);
    connect(inst, &LuaInstance::nameChanged, this, notifyRowChanged);
    connect(inst, &LuaInstance::pathChanged, this, notifyRowChanged);
}

LuaInstance *LuaInstanceManager::addInstance(const QString &path, bool autoStart)
{
    int newRow = static_cast<int>(m_instances.size());
    beginInsertRows(QModelIndex(), newRow, newRow);

    auto *inst = new LuaInstance(path, this);
    m_instances.append(inst);
    connectInstance(inst, newRow);

    endInsertRows();

    emit countChanged();
    emit instancesChanged();
    emit instanceAdded(inst);

    if (autoStart)
    {
        inst->start();
    }

    return inst;
}

void LuaInstanceManager::removeInstance(int index)
{
    if (index < 0 || index >= m_instances.size()) return;

    auto *inst = m_instances[index];
    beginRemoveRows(QModelIndex(), index, index);

    inst->stop();
    m_instances.removeAt(index);

    endRemoveRows();

    emit countChanged();
    emit instancesChanged();
    emit instanceRemoved(inst);

    inst->deleteLater();
}

void LuaInstanceManager::removeInstance(LuaInstance *instance)
{
    int idx = m_instances.indexOf(instance);
    if (idx >= 0)
    {
        removeInstance(idx);
    }
}

LuaInstance *LuaInstanceManager::get(int index) const
{
    if (index < 0 || index >= m_instances.size()) return nullptr;
    return m_instances[index];
}

LuaInstance *LuaInstanceManager::getByPath(const QString &path) const
{
    for (auto *inst : m_instances)
    {
        if (inst->path() == path) return inst;
    }
    return nullptr;
}

void LuaInstanceManager::startAll()
{
    for (auto *inst : m_instances)
    {
        if (!inst->isRunning()) inst->start();
    }
}

void LuaInstanceManager::stopAll()
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->stop();
    }
}

void LuaInstanceManager::restartAll()
{
    for (auto *inst : m_instances)
    {
        inst->restart();
    }
}

void LuaInstanceManager::clearAll()
{
    if (m_instances.isEmpty()) return;

    beginResetModel();
    for (auto *inst : m_instances)
    {
        inst->stop();
        emit instanceRemoved(inst);
        inst->deleteLater();
    }
    m_instances.clear();
    endResetModel();

    emit countChanged();
    emit instancesChanged();
}

void LuaInstanceManager::dispatchUpdateScreen()
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchUpdateScreen();
    }
}

void LuaInstanceManager::dispatchPaint()
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchPaint();
    }
}

void LuaInstanceManager::dispatchVI()
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchVI();
    }
}

void LuaInstanceManager::dispatchInput(CoreButtons *input, int controllerIndex)
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchInput(input, controllerIndex);
    }
}

void LuaInstanceManager::dispatchInterval()
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchInterval();
    }
}

void LuaInstanceManager::dispatchReset()
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchReset();
    }
}

void LuaInstanceManager::dispatchSaveState()
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchSaveState();
    }
}

void LuaInstanceManager::dispatchLoadState()
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchLoadState();
    }
}

void LuaInstanceManager::dispatchKey(int key, bool pressed, bool ctrl, bool alt, bool shift)
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchKey(key, pressed, ctrl, alt, shift);
    }
}

void LuaInstanceManager::dispatchMouse(int x, int y, int button, bool pressed)
{
    for (auto *inst : m_instances)
    {
        if (inst->isRunning()) inst->dispatchMouse(x, y, button, pressed);
    }
}
