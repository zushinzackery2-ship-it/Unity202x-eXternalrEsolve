#pragma once

#include "win_include.hpp"

#include <utility>

namespace er2
{

class WinHandle
{
public:
    WinHandle() noexcept = default;

    explicit WinHandle(HANDLE handle) noexcept
        : m_handle(handle)
    {
    }

    ~WinHandle() noexcept
    {
        Close();
    }

    WinHandle(const WinHandle&) = delete;
    WinHandle& operator=(const WinHandle&) = delete;

    WinHandle(WinHandle&& other) noexcept
        : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }

    WinHandle& operator=(WinHandle&& other) noexcept
    {
        if (this != &other)
        {
            Close();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    HANDLE Get() const noexcept
    {
        return m_handle;
    }

    HANDLE* GetAddressOf() noexcept
    {
        return &m_handle;
    }

    bool IsValid() const noexcept
    {
        return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE;
    }

    void Reset(HANDLE handle = nullptr) noexcept
    {
        if (m_handle != handle)
        {
            Close();
            m_handle = handle;
        }
    }

    HANDLE Release() noexcept
    {
        HANDLE handle = m_handle;
        m_handle = nullptr;
        return handle;
    }

    void Close() noexcept
    {
        if (IsValid())
        {
            ::CloseHandle(m_handle);
            m_handle = nullptr;
        }
    }

    explicit operator bool() const noexcept
    {
        return IsValid();
    }

private:
    HANDLE m_handle = nullptr;
};

} // namespace er2
