#include "Resources.hpp"

#include <cassert>

#include <QFile>
#include <QResource>

namespace LuaResources {
std::string_view sandbox() {
    auto res = QResource(":/lua/sandbox.lua");
    assert(res.compressionAlgorithm() == QResource::NoCompression);
    return {(const char*) res.data(), (size_t) res.size()};
}
std::string_view api() {
    auto res = QResource(":/lua/api.lua");
    assert(res.compressionAlgorithm() == QResource::NoCompression);
    return {(const char*) res.data(), (size_t) res.size()};
}
}
