 #pragma once
 
 #include <cstddef>
 #include <cstdint>
 #include <filesystem>
 
 #include "registration_scanner_code.hpp"
 #include "registration_scanner_meta.hpp"
 #include "registration_types.hpp"
 
 namespace er2
 {
 
 inline bool FindIl2CppRegistrations(
     const IMemoryAccessor& mem,
     std::uintptr_t moduleBase,
     std::uint32_t moduleSize,
     const std::filesystem::path& modulePath,
     std::uintptr_t metaBase,
     std::size_t chunkSize,
     double maxSeconds,
     Il2CppRegs& out)
 {
     out = Il2CppRegs{};
 
     std::uintptr_t cr = 0;
     std::uintptr_t mr = 0;
 
     (void)detail_il2cpp_reg::FindCodeRegistration(mem, moduleBase, moduleSize, modulePath, metaBase, chunkSize, maxSeconds, cr);
     (void)detail_il2cpp_reg::FindMetadataRegistration(mem, moduleBase, moduleSize, modulePath, metaBase, chunkSize, maxSeconds, mr);
 
     out.codeRegistration = cr;
     out.metadataRegistration = mr;
     return (cr != 0) || (mr != 0);
 }
 
 }
