/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QObject>
#include <QAbstractTableModel>
#include <QQmlListProperty>
#include <qqmlintegration.h>

#include <memory>
#include <vector>

#include "Core/Types.hpp"
#include "EmuContext.hpp"

// Read-only object encapsulating pre-loaded data for a single ROM.
class RomData : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(int size READ size CONSTANT)
    Q_PROPERTY(int regionCode READ regionCode CONSTANT)
    Q_PROPERTY(QString romName READ romName CONSTANT)
  public:
    RomData(const std::filesystem::path &path, uintmax_t size, const std::function<void(uint8_t *rom)> &vrByteswap);

    QString path() const { return m_path; }
    int size() const { return m_size; }
    int regionCode() const { return m_rawHeader.Country_code & 0xFF; }
    QString romName() const
    {
        // This incurs extra overhead converting Shift-JIS -> UTF-8 -> UTF-16, but screw it
        auto rom_name_str = IOUtils::rom_name_to_string((const char *)m_rawHeader.nom);
        return QString::fromStdString(rom_name_str);
    }

  private:
    QString m_path;
    int m_size;
    CoreROMHeader m_rawHeader;
};

class RomManager : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  public:
    RomManager(QObject *parent = nullptr) : QAbstractTableModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QHash<int, QByteArray> roleNames() const override;


    bool isLoading() const { return m_loading; }

    Q_INVOKABLE void reloadRomList(const QString& romDir, bool recursive);

  signals:
    void loadingChanged();

  private:
    enum Column
    {
        ColRegionCode = 0,
        ColRomName,
        ColFilename,
        ColSize
    };
    enum UserRoles {
        RoleDisplayType = Qt::UserRole,
        RoleRomPath,
        RoleRegionCode,
    };

    // QML should never change this, only we do
    Q_INVOKABLE void setLoading(bool value)
    {
        if (value == m_loading) return;
        m_loading = value;
        loadingChanged();
    }

    void clearRomList();
    Q_INVOKABLE void addRom(RomData *rom);

    std::vector<std::unique_ptr<RomData>> m_romData;
    bool m_loading;
};
