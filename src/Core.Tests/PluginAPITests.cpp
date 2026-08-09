#include "Common.hpp"

#define PLUGIN_WITH_CALLBACKS
#include <m64rr/Plugin.hpp>

#define MATCH_PLUGIN_FN(name)                                                                                          \
    static_assert(std::is_same_v<decltype(&M64RR##name), M64RRSpec::Ptr##name>,                                        \
                  "signature of M64RR" #name " does not match Ptr" #name);

MATCH_PLUGIN_FN(GetMetadata)
MATCH_PLUGIN_FN(ProcessEvent)
MATCH_PLUGIN_FN(ShowConfig)
MATCH_PLUGIN_FN(ProcessDList)
MATCH_PLUGIN_FN(ProcessRDPList)
MATCH_PLUGIN_FN(ReadVideo)
MATCH_PLUGIN_FN(AIDacrateChanged)
MATCH_PLUGIN_FN(AILenChanged)
MATCH_PLUGIN_FN(GetKeys)
MATCH_PLUGIN_FN(SetKeys)
MATCH_PLUGIN_FN(ReadController)