#pragma once

#include <windows.h>

#include <vector>
#include <algorithm>
#include <cwchar>

namespace UnityExternal
{

namespace detail_window
{

struct EnumWindowClassContext
{
    const wchar_t* className = nullptr;
    std::vector<DWORD>* outPids = nullptr;
};

inline BOOL CALLBACK EnumWindowClassProc(HWND hwnd, LPARAM lParam)
{
    if (!hwnd) return TRUE;

    EnumWindowClassContext* ctx = reinterpret_cast<EnumWindowClassContext*>(lParam);
    if (!ctx || !ctx->className || !ctx->outPids) return TRUE;

    wchar_t className[256] = {};
    int n = GetClassNameW(hwnd, className, (int)(sizeof(className) / sizeof(className[0])));
    if (n <= 0) return TRUE;

    if (std::wcscmp(className, ctx->className) != 0)
    {
        return TRUE;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return TRUE;

    if (std::find(ctx->outPids->begin(), ctx->outPids->end(), pid) == ctx->outPids->end())
    {
        ctx->outPids->push_back(pid);
    }

    return TRUE;
}

}

inline std::vector<DWORD> FindProcessIdsByWindowClass(const wchar_t* windowClass)
{
    std::vector<DWORD> out;

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

inline std::vector<DWORD> FindUnityWndClassPids()
{
    return FindProcessIdsByWindowClass(L"UnityWndClass");
}

} // namespace UnityExternal
