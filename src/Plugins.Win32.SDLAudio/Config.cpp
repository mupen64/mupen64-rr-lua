#include "Config.hpp"
#include <iomanip>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ostream>
#include <stdexcept>

namespace SDLAudio
{

void Config::read_from(std::istream &in)
{
    nlohmann::json j = nlohmann::json::parse(in);
    if (!j.is_object())
        throw std::invalid_argument("invalid config");

    swap_channels = j["swap_channels"];
    sync_audio = j["sync_audio"];
}

void Config::write_to(std::ostream &out) const
{
    nlohmann::json j {
        {"swap_channels", swap_channels},
        {"sync_audio", sync_audio},
    };

    auto old_flags = out.flags();
    out << std::setw(2) << j << '\n';
    out.flags(old_flags);
}

} // namespace SDLAudio