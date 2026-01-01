#pragma once

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <vector>

namespace er2
{

namespace detail_window
{

struct EnumWindowClassContext
{
    const wchar_t* className = nullptr;
    std::vector<std::uint32_t>* outPids = nullptr;
};

inline BOOL CALLBACK EnumWindowClassProc(HWND hwnd, LPARAM lParam)
{
    if (!hwnd)
    {
        return TRUE;
    }

    EnumWindowClassContext* ctx = reinterpret_cast<EnumWindowClassContext*>(lParam);
    if (!ctx || !ctx->className || !ctx->outPids)
    {
        return TRUE;
    }

    wchar_t className[256] = {};
    const int n = GetClassNameW(hwnd, className, static_cast<int>(sizeof(className) / sizeof(className[0])));
    if (n <= 0)
    {
        return TRUE;
    }

    if (std::wcscmp(className, ctx->className) != 0)
    {
        return TRUE;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid)
    {
        return TRUE;
    }

    const std::uint32_t upid = static_cast<std::uint32_t>(pid);
    if (std::find(ctx->outPids->begin(), ctx->outPids->end(), upid) == ctx->outPids->end())
    {
        ctx->outPids->push_back(upid);
    }

    return TRUE;
}

} // namespace detail_window

inline std::vector<std::uint32_t> FindProcessIdsByWindowClass(const wchar_t* windowClass)
{
    std::vector<std::uint32_t> out;

    if (!windowClass || windowClass[0] == L'\0')
    {
        return out;
    }

    detail_window::EnumWindowClassContext ctx;
    ctx.className = windowClass;
    ctx.outPids = &out;

    EnumWindows(detail_window::EnumWindowClassProc, reinterpret_cast<LPARAM>(&ctx));
    return out;
}

inline std::vector<std::uint32_t> FindUnityWndClassPids()
{
    return FindProcessIdsByWindowClass(L"UnityWndClass");
}

} // namespace er2
