/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <memory>
#include <vector>

#include <QObject>
#include <QAbstractTableModel>
#include <QQmlListProperty>
#include <qqmlintegration.h>

#include "Core/Types.hpp"

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

    const CoreROMHeader &rawHeader() const { return m_rawHeader; }

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
    using RomList = std::vector<std::shared_ptr<RomData>>;

    RomManager(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isLoading() const { return m_loading; }

    /**
     * @brief Regenerates the ROM list using the given parameters.
     *
     * @param romDir The directory to search for ROMs.
     * @param recursive Whether to recursively search for ROMs.
     */
    Q_INVOKABLE void reloadRomList(const QString &romDir, bool recursive);

    /**
     * @brief Requests a copy of the current ROM list.
     *
     * @param callback A callback which will send the ROM list.
     * @throws std::logic_error if a future has already been requested.
     */
    void requestRomData(std::move_only_function<void(const RomList &)> &&callback);

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
    enum UserRoles
    {
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
    void sendRomData();

    std::vector<std::shared_ptr<RomData>> m_rom_data;
    std::optional<std::move_only_function<void(const RomList &)>> m_rom_data_request;
    bool m_loading;
};
