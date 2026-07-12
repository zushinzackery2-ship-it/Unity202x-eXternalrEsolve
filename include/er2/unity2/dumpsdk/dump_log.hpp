#pragma once

#include <string>

namespace er2
{

enum class DumpSdkLogLevel
{
    Trace,
    Info,
    Warn,
    Error,
};

using DumpSdkLogCallback = void (*)(DumpSdkLogLevel level, const std::string& message);

inline DumpSdkLogCallback& DumpSdkLogCallbackSlot()
{
    static DumpSdkLogCallback callback = nullptr;
    return callback;
}

inline void SetDumpSdkLogCallback(DumpSdkLogCallback callback)
{
    DumpSdkLogCallbackSlot() = callback;
}

inline void DumpSdkLog(DumpSdkLogLevel level, const std::string& message)
{
    DumpSdkLogCallback callback = DumpSdkLogCallbackSlot();
    if (callback != nullptr)
    {
        callback(level, message);
    }
}

} // namespace er2
