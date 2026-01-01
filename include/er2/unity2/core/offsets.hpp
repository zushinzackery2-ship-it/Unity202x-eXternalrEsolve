#pragma once

#include <cstdint>

namespace er2
{

/// <summary>



/// </summary>
struct Offsets
{
    std::uint32_t ms_id_to_pointer_rva = 0;

    
    std::uint32_t game_object_name_ptr = 0x60;
    
    std::uint32_t scriptable_object_name_ptr = 0x38;
    
    std::uint32_t unity_object_instance_id = 0x08;
    
    std::uint32_t unity_object_managed_ptr = 0x28;

    
    // Il2CppManagedObjectHeader.klass = +0x00
    std::uint32_t managed_object_klass = 0x00;

    
    std::uint32_t il2cppclass_name_ptr = 0x10;
    
    std::uint32_t il2cppclass_namespace_ptr = 0x18;
    
    std::uint32_t il2cppclass_parent = 0x58;

    // Mono Offsets
    std::uint32_t mono_class_name = 0x48;
    std::uint32_t mono_class_namespace = 0x50;
    std::uint32_t mono_class_parent = 0x30;
};

} // namespace er2
