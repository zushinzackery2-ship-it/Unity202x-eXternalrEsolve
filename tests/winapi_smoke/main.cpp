#include "pch.hpp"

namespace
{

void PrintGameObjectSamples(const std::vector<er2::FindObjectsOfTypeAllResult>& gameObjects, std::size_t maxCount)
{
    const std::size_t sampleCount = gameObjects.size() < maxCount ? gameObjects.size() : maxCount;
    for (std::size_t index = 0; index < sampleCount; ++index)
    {
        const er2::FindObjectsOfTypeAllResult& item = gameObjects[index];
        std::string name;
        const bool gotName = er2::GetGameObjectName(item.native, name);
        std::printf(
            "[smoke] GameObject[%zu] native=0x%llX instanceId=%u name=%s\n",
            index,
            static_cast<unsigned long long>(item.native),
            item.instanceId,
            gotName ? name.c_str() : "<read-failed>");
    }
}

bool ValidateNamedGameObjects(const std::vector<std::uintptr_t>& objects, const std::string& expectedName)
{
    bool allMatched = true;
    for (std::size_t index = 0; index < objects.size(); ++index)
    {
        const std::uintptr_t nativeObject = objects[index];
        std::string name;
        const bool gotName = er2::GetGameObjectName(nativeObject, name);
        std::printf(
            "[smoke] NamedObject[%zu] native=0x%llX readName=%s\n",
            index,
            static_cast<unsigned long long>(nativeObject),
            gotName ? name.c_str() : "<read-failed>");
        if (!gotName || name != expectedName)
        {
            allMatched = false;
        }
    }

    return allMatched;
}

bool ValidateUnityObjectCachedPtrOffset(const er2::FindObjectsOfTypeAllResult& sample)
{
    if (!er2::SupportsDynamicFieldOffsets())
    {
        std::printf(
            "[smoke] DynamicOffset skipped: runtime=%s does not expose IL2CPP metadata field offsets\n",
            er2::ToString(er2::Runtime()));
        return true;
    }

    std::uint32_t cachedPtrOffset = 0;
    if (!er2::TryGetFieldOffset("UnityEngine.Object", "m_CachedPtr", cachedPtrOffset))
    {
        std::printf("[smoke] TryGetFieldOffset(UnityEngine.Object::m_CachedPtr) failed\n");
        return false;
    }

    std::uintptr_t managedObject = 0;
    if (!er2::ReadPtr(sample.native + er2::Off().unity_object_managed_ptr, managedObject) || managedObject == 0)
    {
        std::printf(
            "[smoke] Failed to read managed object for native=0x%llX\n",
            static_cast<unsigned long long>(sample.native));
        return false;
    }

    std::uintptr_t cachedPtr = 0;
    if (!er2::ReadPtr(managedObject + static_cast<std::uintptr_t>(cachedPtrOffset), cachedPtr) || cachedPtr == 0)
    {
        std::printf(
            "[smoke] Failed to read m_CachedPtr managed=0x%llX offset=0x%X\n",
            static_cast<unsigned long long>(managedObject),
            cachedPtrOffset);
        return false;
    }

    std::printf(
        "[smoke] DynamicOffset UnityEngine.Object::m_CachedPtr offset=0x%X metadataRegistration=0x%llX "
        "fieldOffsetsPtr=0x%llX managed=0x%llX cachedPtr=0x%llX native=0x%llX\n",
        cachedPtrOffset,
        static_cast<unsigned long long>(er2::FieldOffsetsMetadataRegistrationVa()),
        static_cast<unsigned long long>(er2::FieldOffsetsTableVa()),
        static_cast<unsigned long long>(managedObject),
        static_cast<unsigned long long>(cachedPtr),
        static_cast<unsigned long long>(sample.native));

    return cachedPtr == sample.native;
}

} // namespace

int main()
{
    const char* targetGameObjectName = "Main Camera";

    er2::SetContextBackend(er2::CreateDefaultContextBackend());

    if (!er2::AutoInit())
    {
        std::printf("[smoke] AutoInit failed\n");
        return 1;
    }

    std::printf(
        "[smoke] AutoInit ok pid=%u runtime=%s gom=0x%llX msid=0x%llX\n",
        er2::Pid(),
        er2::Runtime() == er2::ManagedBackend::Il2Cpp ? "IL2CPP" : "Mono",
        static_cast<unsigned long long>(er2::GomGlobalSlotVa()),
        static_cast<unsigned long long>(er2::MsIdToPointerSlotVa()));

    const std::vector<er2::FindObjectsOfTypeAllResult> gameObjects = er2::FindObjectsOfTypeAll("GameObject");
    std::printf("[smoke] FindObjectsOfTypeAll(GameObject) count=%zu\n", gameObjects.size());
    if (gameObjects.empty())
    {
        std::printf("[smoke] GameObject enumeration returned no results\n");
        return 2;
    }

    PrintGameObjectSamples(gameObjects, 8u);

    const std::vector<std::uintptr_t> namedObjects = er2::GetGameObjectByName(targetGameObjectName);
    std::printf("[smoke] GetGameObjectByName(%s) count=%zu\n", targetGameObjectName, namedObjects.size());
    if (namedObjects.empty())
    {
        std::printf("[smoke] %s was not found\n", targetGameObjectName);
        return 3;
    }

    if (!ValidateNamedGameObjects(namedObjects, targetGameObjectName))
    {
        std::printf("[smoke] Name validation failed for %s results\n", targetGameObjectName);
        return 4;
    }

    std::unordered_set<std::uintptr_t> gameObjectSet;
    gameObjectSet.reserve(gameObjects.size());
    for (const er2::FindObjectsOfTypeAllResult& item : gameObjects)
    {
        gameObjectSet.insert(item.native);
    }

    std::size_t overlapCount = 0;
    for (std::uintptr_t nativeObject : namedObjects)
    {
        if (gameObjectSet.find(nativeObject) != gameObjectSet.end())
        {
            ++overlapCount;
        }
    }

    std::printf("[smoke] Overlap(%s, GameObject)=%zu\n", targetGameObjectName, overlapCount);
    if (overlapCount == 0)
    {
        std::printf("[smoke] %s results did not overlap with GameObject enumeration\n", targetGameObjectName);
        return 5;
    }

    er2::FindObjectsOfTypeAllResult targetSample{};
    targetSample.native = namedObjects.front();
    if (!ValidateUnityObjectCachedPtrOffset(targetSample))
    {
        std::printf("[smoke] Dynamic field offset validation failed\n");
        return 6;
    }

    std::printf("[smoke] WinAPI backend smoke test passed\n");
    return 0;
}
