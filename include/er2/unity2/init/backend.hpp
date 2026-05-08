#pragma once

#include "../../os/win/win_include.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

#include "../../mem/memory_accessor.hpp"
#include "../../os/win/win_handle.hpp"
#include "../../os/win/win_memory_accessor.hpp"
#include "../../os/win/win_module.hpp"
#include "../../os/win/win_process.hpp"

namespace er2
{

class NullMemoryAccessor final : public IMemoryAccessor
{
public:
    bool Read(std::uintptr_t address, void* buffer, std::size_t size) const override
    {
        (void)address;
        (void)buffer;
        (void)size;
        return false;
    }

    bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const override
    {
        (void)address;
        (void)buffer;
        (void)size;
        return false;
    }
};

inline const IMemoryAccessor& GetNullMemoryAccessor()
{
    static NullMemoryAccessor accessor;
    return accessor;
}

class IContextBackend
{
public:
    virtual ~IContextBackend() = default;

    virtual bool Attach(std::uint32_t pid) = 0;
    virtual void Reset() = 0;
    virtual bool IsAttached() const = 0;
    virtual HANDLE GetProcessHandle() const = 0;
    virtual const IMemoryAccessor* GetMemoryAccessor() const = 0;
    virtual bool GetModuleInfo(std::uint32_t pid, const wchar_t* moduleName, ModuleInfo& out) const = 0;
};

class WinApiContextBackend final : public IContextBackend
{
public:
    ~WinApiContextBackend() override = default;

    bool Attach(std::uint32_t pid) override
    {
        Reset();

        WinHandle process(OpenProcessForRead(pid));
        if (!process.IsValid())
        {
            return false;
        }

        m_process = std::move(process);
        m_memoryAccessor = std::make_shared<WinApiMemoryAccessor>(m_process.Get());
        return true;
    }

    void Reset() override
    {
        m_memoryAccessor.reset();
        m_process.Close();
    }

    bool IsAttached() const override
    {
        return m_process.IsValid() && static_cast<bool>(m_memoryAccessor);
    }

    HANDLE GetProcessHandle() const override
    {
        return m_process.Get();
    }

    const IMemoryAccessor* GetMemoryAccessor() const override
    {
        return m_memoryAccessor.get();
    }

    bool GetModuleInfo(std::uint32_t pid, const wchar_t* moduleName, ModuleInfo& out) const override
    {
        return er2::GetRemoteModuleInfo(pid, moduleName, out);
    }

private:
    WinHandle m_process;
    std::shared_ptr<WinApiMemoryAccessor> m_memoryAccessor;
};

using ContextBackendPtr = std::shared_ptr<IContextBackend>;

inline ContextBackendPtr CreateDefaultContextBackend()
{
    return std::make_shared<WinApiContextBackend>();
}

} // namespace er2

