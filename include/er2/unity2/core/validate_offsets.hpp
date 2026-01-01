#pragma once

#include <cstdint>
#include "offsets.hpp"
#include "../msid/ms_id_to_pointer.hpp"

namespace er2
{

/// <summary>

/// </summary>
inline bool ValidateOffsets(const Offsets& off)
{
    
    if (off.game_object_name_ptr > 0x200)
    {
        return false;
    }
    if (off.scriptable_object_name_ptr > 0x200)
    {
        return false;
    }
    if (off.unity_object_managed_ptr > 0x100)
    {
        return false;
    }
    if (kMsIdEntryStride == 0 || kMsIdEntryStride > 0x100)
    {
        return false;
    }

    return true;
}

} // namespace er2
