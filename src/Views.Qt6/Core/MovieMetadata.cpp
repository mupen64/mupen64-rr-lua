#include "MovieMetadata.hpp"

#include <type_traits>

#include <QPointer>
#include <QThreadPool>

#include "EmuContext.hpp"

static QString format_duration(size_t seconds)
{
    using namespace Qt::Literals;
    size_t minutes = seconds / 60;
    size_t seconds_part = seconds % 60;

    size_t hours = minutes / 60;
    size_t minutes_part = minutes % 60;

    return u"%1:%2:%3"_s.arg(hours, 2, 10, '0').arg(minutes_part, 2, 10, '0').arg(seconds_part, 2, 10, '0');
}

template <class T> static void append_block(QList<T> &list, std::initializer_list<T> items)
{
    list.reserve(list.size() + items.size());
    for (const auto &item : items) list.append(item);
}

template <class F>
static QString format_extended_data(const CoreVCRMovieHeader &header, uint8_t version, F callback)
    requires std::is_convertible_v<std::invoke_result_t<F>, QString>
{
    using namespace Qt::Literals;

    if (header.extended_version < version) return u"Unknown"_s;
    return callback();
}

static size_t count_button_presses(std::span<CoreButtons> data, int bit)
{
    size_t count = 0;
    const uint32_t mask = (1 << bit);

    // writing the loop this way makes it easier for the compiler to autovectorize.
    for (size_t i = 0; i < data.size(); i++)
    {
        bool prev_off = (i == 0) || !(data[i - 1].value & mask);
        bool curr_on = !!(data[i].value & mask);
        count += size_t(prev_off & curr_on);
    }
    return count;
}

static size_t count_empty_frames(std::span<CoreButtons> data)
{
    size_t count = 0;
    for (size_t i = 0; i < data.size(); i++)
    {
        bool empty = data[i].value == 0;
        count += size_t(empty);
    }
    return count;
}
static size_t count_joystick_frames(std::span<CoreButtons> data)
{
    size_t count = 0;
    for (size_t i = 0; i < data.size(); i++)
    {
        bool uses_joystick = (data[i].x != 0) || (data[i].y != 0);
        count += size_t(uses_joystick);
    }
    return count;
}
static size_t count_input_changes(std::span<CoreButtons> data)
{
    size_t count = 0;
    for (size_t i = 0; i < data.size(); i++)
    {
        CoreButtons last_frame = (i == 0) ? CoreButtons{.value = 0} : data[i - 1];
        count += size_t(data[i] != last_frame);
    }
    return count;
}

MovieMetadata::MovieMetadata(QObject *parent) : QAbstractTableModel(parent), m_loading(false)
{
}

int MovieMetadata::rowCount(const QModelIndex &) const
{
    // this shouldn't be an issue
    return (int)m_extraData.size();
}
int MovieMetadata::columnCount(const QModelIndex &) const
{
    return 2;
}
QVariant MovieMetadata::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) return {};

    const auto &row = m_extraData[index.row()];

    switch (index.column())
    {
    case 0:
        return row.first;
    case 1:
        return row.second;
    default:
        return {};
    }
}
QVariant MovieMetadata::headerData(int section, Qt::Orientation orientation, int role) const
{
    using namespace Qt::Literals;

    if (orientation == Qt::Vertical || role != Qt::DisplayRole) return {};
    switch (section)
    {
    case 0:
        //% "Name"
        return qtTrId("dialogs.movie.tableHeaders.name");
    case 1:
        //% "Value"
        return qtTrId("dialogs.movie.tableHeaders.value");
    default:
        return {};
    }
}

Qt::ItemFlags MovieMetadata::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled;
}
QHash<int, QByteArray> MovieMetadata::roleNames() const
{
    static const QHash<int, QByteArray> s_instance{
        // text to display (QString)
        {Qt::DisplayRole, "display"},
    };
    return s_instance;
}

MovieMetadata::ExtraDataList MovieMetadata::extractExtraData(
    CoreCtx *core_ctx, const CoreVCRMovieHeader &header, std::span<CoreButtons> inputs)
{
    using namespace Qt::Literals;
    ExtraDataList metadata;
    append_block(metadata,
        {
            // basic info
            {
                "ROM",
                u"%1 (%2, %3)"_s.arg((char *)header.rom_name)
                    .arg(core_ctx->vr_country_code_to_country_name(header.rom_country))
                    .arg(header.rom_crc1, 8, 16, '0'),
            },
            {
                "Length",
                u"%1 (%2 input)"_s.arg(header.length_vis).arg(header.length_samples),
            },
            {
                "Duration",
                format_duration(static_cast<size_t>((double)header.length_samples / (double)header.vis_per_second)),
            },
            {
                "Rerecords",
                QString::number(
                    static_cast<uint64_t>(header.extended_data.rerecord_count) << 32 | header.rerecord_count),
            },
            {
                "Video Plugin",
                QString::fromUtf8(header.video_plugin_name),
            },
            {
                "Input Plugin",
                QString::fromUtf8(header.input_plugin_name),
            },
            {
                "Sound Plugin",
                QString::fromUtf8(header.audio_plugin_name),
            },
            {
                "RSP Plugin",
                QString::fromUtf8(header.rsp_plugin_name),
            },
        });

    // Controller info
    for (int i = 0; i < 4; i++)
    {
        QString desc = header.controller_flags & CONTROLLER_X_PRESENT(i) ? "Present%1%2"_L1 : "Disconnected%1%2"_L1;
        desc = desc.arg(header.controller_flags & CONTROLLER_X_MEMPAK(i) ? "with mempak" : "",
            header.controller_flags & CONTROLLER_X_MEMPAK(i) ? "with rumble" : "");
        metadata.emplace_back(u"Controller %1"_s.arg(i + 1), std::move(desc));
    }

    // Lag frames, assuming 30 Hz input polling.
    // This is somewhat SM64-specific and generally isn't applicable to other games.
    const auto lag_frames = std::max((int64_t)0, (int64_t)header.length_vis - (2 * (int64_t)header.length_samples));

    append_block(metadata,
        {
            // extended data
            {
                "WiiVC",
                format_extended_data(header, 1, [&] { return header.extended_flags.wii_vc ? "Enabled" : "Disabled"; }),
            },
            {
                "Accurate RDP completion",
                format_extended_data(
                    header, 3, [&] { return header.extended_flags.accurate_rdp_completion ? "Enabled" : "Disabled"; }),
            },
            {
                "CPU counter factor",
                format_extended_data(header, 4, [&] { return QString::number(header.extended_data.cpu_cf); }),
            },
            {
                "RCP lag factor",
                format_extended_data(header, 3, [&] { return QString::number(header.extended_data.rcp_lag_factor); }),
            },
            {
                "Authorship",
                format_extended_data(
                    header, 1, [&] { return QString::fromLatin1((char *)header.extended_data.authorship_tag, 4); }),
            },
            // button press counts
            {"A Presses", QString::number(count_button_presses(inputs, 7))},
            {"B Presses", QString::number(count_button_presses(inputs, 6))},
            {"Z Presses", QString::number(count_button_presses(inputs, 5))},
            {"S Presses", QString::number(count_button_presses(inputs, 4))},
            {"R Presses", QString::number(count_button_presses(inputs, 12))},
            {"C^ Presses", QString::number(count_button_presses(inputs, 11))},
            {"Cv Presses", QString::number(count_button_presses(inputs, 10))},
            {"C< Presses", QString::number(count_button_presses(inputs, 9))},
            {"C> Presses", QString::number(count_button_presses(inputs, 8))},
            // misc input stats
            {"Lag Frames (approximation)", QString::number(lag_frames)},
            {"Empty Frames", QString::number(count_empty_frames(inputs))},
            {"Joystick Frames", QString::number(count_joystick_frames(inputs))},
            {"Input Changes", QString::number(count_input_changes(inputs))},
        });

    return metadata;
}

void MovieMetadata::loadMovie(const QString &path)
{
    if (m_loading) return;
    setLoading(true);
    // perform loading on a separate thread
    QThreadPool::globalInstance()->start(
        [self = QPointer<MovieMetadata>(this), path = std::filesystem::path((std::u16string_view)path)] {
            auto scope = qScopeGuard([self] { QMetaObject::invokeMethod(self, &MovieMetadata::setLoading, false); });

            auto *core_ctx = EmuContext::rawContext();
            assert(core_ctx != nullptr);

            CoreVCRMovieHeader header = {};
            if (core_ctx->vcr_parse_header(path, &header) != CoreResult::Res_Ok) return;
            std::vector<CoreButtons> inputs = {};
            if (core_ctx->vcr_read_movie_inputs(path, inputs) != CoreResult::Res_Ok) return;

            auto author = QString::fromUtf8(header.author);
            auto description = QString::fromUtf8(header.description);
            auto startFlags = (int) header.start_flags;
            auto extra_data = MovieMetadata::extractExtraData(core_ctx, header, inputs);

            QMetaObject::invokeMethod(self, &MovieMetadata::updateValues, author, description, startFlags, extra_data);
        });
}

void MovieMetadata::updateValues(QAnyStringView author, QAnyStringView description, int startFlags, const ExtraDataList &extraData)
{
    beginResetModel();
    m_extraData = extraData;
    endResetModel();

    auto author_str = author.toString();
    auto description_str = description.toString();

    if (author_str != m_author)
    {
        m_author = std::move(author_str);
        authorChanged();
    }
    if (description_str != m_description)
    {
        m_description = std::move(description_str);
        descriptionChanged();
    }
    if (startFlags != m_startFlags)
    {
        m_startFlags = startFlags;
        startFlagsChanged();
    }

    valuesUpdated();
}
