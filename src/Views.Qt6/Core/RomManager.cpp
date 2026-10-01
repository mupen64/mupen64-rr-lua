/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "RomManager.hpp"

#include <ranges>

#include <Common/Assert.hpp>

#include <QDirIterator>
#include <QThreadPool>
#include <QFileInfo>
#include <QtTranslation>

#include "EmuContext.hpp"

template <class T, class U> static inline bool is_one_of(const U &x, std::initializer_list<T> objs)
{
    return std::ranges::contains(objs, x);
}

RomData::RomData(const std::filesystem::path &path, uintmax_t size, const std::function<void(uint8_t *rom)> &vrByteswap)
    : m_path(QAnyStringView(path.native()).toString()), m_size((int)size)
{
    // read ROM header
    {
        auto file = std::ifstream{path, std::ios::binary};
        file.read((char *)&m_rawHeader, sizeof(CoreROMHeader));
    }
    // perform endianness correction
    vrByteswap((uint8_t *)&m_rawHeader);
}

RomManager::RomManager(QObject *parent) : QAbstractTableModel(parent), m_loading(false)
{
    // resolve ROM data requests if they are still pending after a load
    QObject::connect(this, &RomManager::loadingChanged, [this] {
        if (m_loading) return;
        sendRomData();
    });
}
int RomManager::rowCount(const QModelIndex &) const
{
    return (int)m_rom_data.size();
}
int RomManager::columnCount(const QModelIndex &) const
{
    return 4;
}
QVariant RomManager::data(const QModelIndex &index, int role) const
{
    using namespace Qt::Literals;
    if (!index.isValid()) return {};

    if (role == RoleRomPath)
    {
        const auto &item = m_rom_data[index.row()];
        return item->path();
    }

    switch (index.column())
    {
    case ColRegionCode: {
        if (role == RoleDisplayType) return u"flag"_s;

        const auto &item = m_rom_data[index.row()];
        if (role == Qt::DisplayRole) return u"??"_s;
        if (role == RoleRegionCode) return item->regionCode();
        return {};
    }
    break;
    case ColRomName: {
        if (role == RoleDisplayType) return u"text"_s;

        const auto &item = m_rom_data[index.row()];
        if (role != Qt::DisplayRole) return {};
        return item->romName();
    }
    break;
    case ColFilename: {
        if (role == RoleDisplayType) return u"text"_s;

        const auto &item = m_rom_data[index.row()];
        if (role != Qt::DisplayRole) return {};
        return QFileInfo(item->path()).fileName();
    }
    break;
    case ColSize: {
        if (role == RoleDisplayType) return u"text"_s;

        const auto &item = m_rom_data[index.row()];
        if (role != Qt::DisplayRole) return {};
        //% "%1 MB"
        return qtTrId("misc.units.MiB").arg(item->size() >> 20);
    }
    break;
    default:
        return {};
    }
}
QVariant RomManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    using namespace Qt::Literals;

    if (orientation == Qt::Vertical || role != Qt::DisplayRole) return {};
    switch (section)
    {
    case 0:
        return u""_s;
    case 1:
        //% "ROM Name"
        return qtTrId("romBrowser.headers.romName");
    case 2:
        //% "Filename"
        return qtTrId("romBrowser.headers.filename");
    case 3:
        //% "Size"
        return qtTrId("romBrowser.headers.size");
    default:
        return {};
    }
}

QHash<int, QByteArray> RomManager::roleNames() const
{
    static const QHash<int, QByteArray> s_instance{// text to display (QString)
        {Qt::DisplayRole, "display"}, {RoleDisplayType, "displayType"},
        // ROM path to open on double-click (QString)
        {RoleRomPath, "romPath"},
        // region code to render icon (var: int | null)
        {RoleRegionCode, "regionCode"}};
    return s_instance;
}
Qt::ItemFlags RomManager::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

Q_INVOKABLE void RomManager::reloadRomList(const QString &romDir, bool recursive)
{
    std::println("loading: {}", m_loading);
    if (isLoading()) return;

    // set loading flag now, it will be reset once the load is done
    setLoading(true);
    clearRomList();

    // perform load on a separate thread
    QThreadPool::globalInstance()->start([this, romDir, recursive, vrByteswap = EmuContext::rawContext()->vr_byteswap] {
        // ad-hoc scope guard: clear the setLoading flag on exit
        struct Guard
        {
            RomManager *self;
            ~Guard() { QMetaObject::invokeMethod(self, &RomManager::setLoading, false); }
        } scopeGuard{.self = this};

        std::filesystem::path romDirPath(std::u16string_view{romDir});
        if (!std::filesystem::is_directory(romDirPath)) return;

        auto romFilter = std::views::filter([](const std::filesystem::directory_entry &entry) {
            return entry.is_regular_file() && is_one_of(entry.path().extension(), {".n64", ".v64", ".z64", ".rom"});
        });
        auto sendRomData = [this, vrByteswap](const std::filesystem::directory_entry &entry) {
            std::println("adding {}", entry.path().string());
            auto *romData = new RomData(entry.path(), entry.file_size(), vrByteswap);

            // hand off romData back to UI thread
            romData->moveToThread(nullptr);
            QMetaObject::invokeMethod(this, &RomManager::addRom, romData);
        };

        if (recursive)
            std::ranges::for_each(std::filesystem::recursive_directory_iterator{romDirPath} | romFilter, sendRomData);
        else
            std::ranges::for_each(std::filesystem::directory_iterator{romDirPath} | romFilter, sendRomData);
    });
}

void RomManager::requestRomData(std::move_only_function<void(const RomList &)> &&callback)
{
//     if (m_rom_data_request.has_value()) throw std::logic_error("ROM data request already active");
//     m_rom_data_request.emplace(std::move(callback));
//
//     // if result can be resolved now, resolve it now
//     if (!m_loading) sendRomData();
}

void RomManager::clearRomList()
{
    beginResetModel();
    m_rom_data.clear();
    endResetModel();
}
void RomManager::addRom(RomData *rom)
{
    need(rom->thread() == nullptr, "ROM passed to addRom() must not be bound to a thread");
    // pull RomData onto the UI thread
    rom->moveToThread(thread());

    // insert a single ROM.
    // TODO: should we batch updates instead of sending them one at a time?
    int index = (int)m_rom_data.size();
    beginInsertRows({}, index, index);
    m_rom_data.emplace_back(rom);
    endInsertRows();
}
void RomManager::sendRomData()
{
    if (!m_rom_data_request.has_value()) return;
    // send state and release callback
    (*m_rom_data_request)(m_rom_data);
    m_rom_data_request.reset();
}
