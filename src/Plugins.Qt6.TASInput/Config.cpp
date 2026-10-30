#include "Config.hpp"

using nlohmann::json;

void to_json(json &j, const InputPortConfig &self) {
    #define TASINPUT_FIELD(name) j[#name] = self.name;
    TASINPUT_FIELD(present)
    #undef TASINPUT_FIELD
}
void from_json(const json &j, InputPortConfig &self) {
    #define TASINPUT_FIELD(name) self.name = j[#name];
    TASINPUT_FIELD(present)
    #undef TASINPUT_FIELD
}

void to_json(json &j, const InputConfig &self) {
    j["version"] = self.version;

    auto ports = nlohmann::json::array_t(4, nullptr);
    for (size_t i = 0; i < 4; i++) {
        to_json(ports[i], self.ports[i]);
    }
    j["ports"] = std::move(ports);
}
void from_json(const json &j, InputConfig &self) {
    self.version = j["version"];

    const auto& ports = j["ports"].get_ref<const json::array_t&>();
    for (size_t i = 0; i < 4; i++) {
        from_json(ports[i], self.ports[i]);
    }
}
