#pragma once

#include "context.hpp"

#include "../dumpsdk.hpp"

namespace er2
{

inline bool DumpSdk6Dump(DumpSdk6Paths& outPaths)
{
    outPaths = DumpSdk6Paths{};
    if (!IsInited())
    {
        return false;
    }

    return DumpSdk6DumpByPid(g_ctx.pid, outPaths);
}

} // namespace er2
