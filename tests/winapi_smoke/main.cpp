#include "pch.hpp"

namespace
{

int g_pass = 0;
int g_fail = 0;
int g_skip = 0;

void Pass(const char* tag) { ++g_pass; std::printf("[PASS] %s\n", tag); }
void Fail(const char* tag, const char* reason) { ++g_fail; std::printf("[FAIL] %s: %s\n", tag, reason); }
void Skip(const char* tag, const char* reason) { ++g_skip; std::printf("[SKIP] %s: %s\n", tag, reason); }

} // namespace

int main()
{
    const char* targetName = "Noel";

    er2::SetContextBackend(er2::CreateDefaultContextBackend());

    // ── 1. AutoInit ──
    if (!er2::AutoInit())
    {
        Fail("AutoInit", "returned false");
        return 1;
    }
    Pass("AutoInit");
    std::printf("  pid=%u runtime=%s gom=0x%llX msid=0x%llX\n",
        er2::Pid(),
        er2::ToString(er2::Runtime()),
        static_cast<unsigned long long>(er2::GomGlobalSlotVa()),
        static_cast<unsigned long long>(er2::MsIdToPointerSlotVa()));

    // ── 2. GOM structure queries ──
    const std::uintptr_t manager = er2::GomManager();
    if (manager) Pass("GomManager"); else Fail("GomManager", "returned 0");

    const std::uintptr_t bucketsPtr = er2::GomBucketsPtr();
    if (bucketsPtr) Pass("GomBucketsPtr"); else Fail("GomBucketsPtr", "returned 0");

    const std::int32_t bucketCount = er2::GomBucketCount();
    if (bucketCount > 0) Pass("GomBucketCount"); else Fail("GomBucketCount", "returned 0");
    std::printf("  manager=0x%llX bucketsPtr=0x%llX bucketCount=%d\n",
        static_cast<unsigned long long>(manager),
        static_cast<unsigned long long>(bucketsPtr),
        bucketCount);

    const std::uintptr_t listHead = er2::GomLocalGameObjectListHead();
    if (listHead) Pass("GomLocalGameObjectListHead"); else Fail("GomLocalGameObjectListHead", "returned 0");

    // ── 3. CheckGomManagerCandidate ──
    if (manager)
    {
        const er2::ManagerCandidateCheck mc = er2::CheckGomManagerCandidate(manager);
        if (mc.ok && mc.score > 0) Pass("CheckGomManagerCandidate"); else Fail("CheckGomManagerCandidate", "ok=false or score=0");
        std::printf("  ok=%d score=%d\n", mc.ok ? 1 : 0, mc.score);
    }

    // ── 4. EnumerateGameObjects ──
    const auto goEntries = er2::EnumerateGameObjects();
    if (goEntries && !goEntries->empty()) Pass("EnumerateGameObjects"); else Fail("EnumerateGameObjects", "empty or nullopt");
    const std::size_t goCount = goEntries ? goEntries->size() : 0;
    std::printf("  count=%zu\n", goCount);

    // ── 5. FindObjectsOfTypeAll(className) ──
    const auto fotaGO = er2::FindObjectsOfTypeAll("GameObject");
    if (!fotaGO.empty()) Pass("FindObjectsOfTypeAll(GameObject)"); else Fail("FindObjectsOfTypeAll(GameObject)", "empty");
    std::printf("  count=%zu\n", fotaGO.size());

    // ── 6. FindObjectsOfTypeAll(namespace, className) ──
    const auto fotaTransform = er2::FindObjectsOfTypeAll("UnityEngine", "Transform");
    if (!fotaTransform.empty()) Pass("FindObjectsOfTypeAll(UnityEngine,Transform)"); else Fail("FindObjectsOfTypeAll(UnityEngine,Transform)", "empty");
    std::printf("  count=%zu\n", fotaTransform.size());

    // ── 7. GetGameObjectByName ──
    const auto namedObjs = er2::GetGameObjectByName(targetName);
    if (!namedObjs.empty()) Pass("GetGameObjectByName"); else Skip("GetGameObjectByName", "target not in this scene");
    std::printf("  name=%s count=%zu\n", targetName, namedObjs.size());

    // ── 8. GetGameObjectName round-trip ──
    if (!namedObjs.empty())
    {
        std::string readBack;
        if (er2::GetGameObjectName(namedObjs[0], readBack) && readBack == targetName)
            Pass("GetGameObjectName");
        else
            Fail("GetGameObjectName", "name mismatch");
    }

    // ── 9. GetGameObjectName (optional overload) ──
    if (!namedObjs.empty())
    {
        const auto opt = er2::GetGameObjectName(namedObjs[0]);
        if (opt && *opt == targetName) Pass("GetGameObjectName(optional)"); else Fail("GetGameObjectName(optional)", "nullopt or mismatch");
    }

    // pick first GO from enumeration as fallback name test
    std::uintptr_t fallbackGo = 0;
    if (namedObjs.empty() && goEntries && !goEntries->empty())
    {
        for (const auto& entry : *goEntries)
        {
            std::string testName;
            if (er2::GetGameObjectName(entry.nativeObject, testName) && !testName.empty())
            {
                fallbackGo = entry.nativeObject;
                Pass("GetGameObjectName(fallback)");
                std::printf("  fallback name=%s\n", testName.c_str());
                break;
            }
        }
        if (!fallbackGo)
        {
            Fail("GetGameObjectName(fallback)", "cannot read any GO name");
        }
    }

    // ── 10. FindGameObjectThroughTag(MainCamera tag=5) ──
    const std::uintptr_t mainCamGo = er2::FindGameObjectThroughTag(5);
    if (mainCamGo) Pass("FindGameObjectThroughTag(5)"); else Skip("FindGameObjectThroughTag(5)", "no MainCamera tag in scene");

    // ── 11. GetListNodeNative / GetListNodeNext ──
    if (listHead)
    {
        std::uintptr_t nodeNative = er2::GetListNodeNative(listHead);
        if (nodeNative) Pass("GetListNodeNative"); else Fail("GetListNodeNative", "returned 0");

        std::uintptr_t nodeNext = er2::GetListNodeNext(listHead);
        if (nodeNext) Pass("GetListNodeNext"); else Fail("GetListNodeNext", "returned 0");
    }

    // pick a sample GO for component / transform tests (prefer named, then fallback, then mainCamGo)
    std::uintptr_t sampleGo = namedObjs.empty() ? 0 : namedObjs[0];
    if (!sampleGo) sampleGo = fallbackGo;
    if (!sampleGo) sampleGo = mainCamGo;

    // ── 12. GetTransformComponent ──
    std::uintptr_t sampleTransform = 0;
    if (sampleGo)
    {
        sampleTransform = er2::GetTransformComponent(sampleGo);
        if (sampleTransform) Pass("GetTransformComponent"); else Fail("GetTransformComponent", "returned 0");
    }

    // ── 13. GetAllComponents ──
    if (sampleGo)
    {
        const auto comps = er2::GetAllComponents(sampleGo);
        if (!comps.empty()) Pass("GetAllComponents"); else Fail("GetAllComponents", "empty");
        std::printf("  count=%zu\n", comps.size());
    }

    // ── 14. GetComponentThroughTypeName ──
    if (sampleGo)
    {
        const std::uintptr_t trByName = er2::GetComponentThroughTypeName(sampleGo, "Transform");
        if (trByName) Pass("GetComponentThroughTypeName(Transform)"); else Fail("GetComponentThroughTypeName(Transform)", "returned 0");
    }

    // ── 15. GetTransformWorldPosition ──
    if (sampleTransform)
    {
        glm::vec3 pos(0.0f);
        if (er2::GetTransformWorldPosition(sampleTransform, pos))
        {
            Pass("GetTransformWorldPosition");
            std::printf("  pos=(%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
        }
        else
        {
            Fail("GetTransformWorldPosition", "returned false");
        }
    }

    // ── 16. GetTransformWorldPosition (optional overload) ──
    if (sampleTransform)
    {
        const auto optPos = er2::GetTransformWorldPosition(sampleTransform);
        if (optPos) Pass("GetTransformWorldPosition(optional)"); else Fail("GetTransformWorldPosition(optional)", "nullopt");
    }

    // ── 17. FindMainCamera + GetCameraMatrix ──
    const std::uintptr_t mainCam = er2::FindMainCamera();
    if (mainCam)
    {
        Pass("FindMainCamera");
        glm::mat4 vp(1.0f);
        if (er2::GetCameraMatrix(mainCam, vp))
        {
            Pass("GetCameraMatrix");
            std::printf("  vp[0][0]=%.4f vp[1][1]=%.4f\n", vp[0][0], vp[1][1]);
        }
        else
        {
            Fail("GetCameraMatrix", "returned false");
        }

        // ── 18. GetCameraMatrix (optional overload) ──
        const auto optMat = er2::GetCameraMatrix(mainCam);
        if (optMat) Pass("GetCameraMatrix(optional)"); else Fail("GetCameraMatrix(optional)", "nullopt");
    }
    else
    {
        Skip("FindMainCamera", "no main camera in scene");
        Skip("GetCameraMatrix", "no main camera");
        Skip("GetCameraMatrix(optional)", "no main camera");
    }

    // ── 19. W2S ──
    if (mainCam && sampleTransform)
    {
        const auto optVp = er2::GetCameraMatrix(mainCam);
        const auto optPos = er2::GetTransformWorldPosition(sampleTransform);
        if (optVp && optPos)
        {
            er2::ScreenRect screen{};
            screen.width = 1920;
            screen.height = 1080;
            const er2::WorldToScreenResult w2s = er2::W2S(*optVp, screen, *optPos);
            std::printf("  W2S visible=%d x=%.1f y=%.1f\n", w2s.visible ? 1 : 0, w2s.x, w2s.y);
            Pass("W2S");
        }
        else
        {
            Fail("W2S", "missing viewProj or position");
        }
    }
    else
    {
        Skip("W2S", "no camera or transform");
    }

    // ── 20. GetBoneTransformAll ──
    if (sampleGo)
    {
        const auto bones = er2::GetBoneTransformAll(sampleGo);
        if (!bones.empty())
        {
            Pass("GetBoneTransformAll");
            std::printf("  bones=%zu first=%s\n", bones.size(), bones[0].boneName.c_str());
        }
        else
        {
            Skip("GetBoneTransformAll", "no child transforms");
        }
    }

    // ── 21. MsIdSetPtr / MsIdCount ──
    const std::uintptr_t msIdSet = er2::MsIdSetPtr();
    if (msIdSet) Pass("MsIdSetPtr"); else Fail("MsIdSetPtr", "returned 0");

    const std::uint32_t msIdCount = er2::MsIdCount();
    if (msIdCount > 0) Pass("MsIdCount"); else Fail("MsIdCount", "returned 0");
    std::printf("  msIdSet=0x%llX msIdCount=%u\n",
        static_cast<unsigned long long>(msIdSet), msIdCount);

    // ── 22. EnumerateMsIdToPointerObjects ──
    {
        std::size_t enumCount = 0;
        er2::EnumerateOptions opt;
        opt.onlyGameObject = false;
        opt.onlyScriptableObject = false;
        er2::EnumerateMsIdToPointerObjects(opt, [&](const er2::ObjectInfo& info)
        {
            (void)info;
            ++enumCount;
        });
        if (enumCount > 0) Pass("EnumerateMsIdToPointerObjects"); else Fail("EnumerateMsIdToPointerObjects", "no objects");
        std::printf("  enumerated=%zu\n", enumCount);
    }

    // ── 23. GetManagedObjectTypeInfo ──
    if (sampleGo)
    {
        std::uintptr_t managed = 0;
        if (er2::ReadPtr(sampleGo + er2::Off().unity_object_managed_ptr, managed) && managed)
        {
            er2::TypeInfo ti;
            if (er2::GetManagedObjectTypeInfo(managed, ti))
            {
                Pass("GetManagedObjectTypeInfo");
                std::printf("  ns=%s name=%s\n", ti.namespaze.c_str(), ti.name.c_str());
            }
            else
            {
                Fail("GetManagedObjectTypeInfo", "returned false");
            }

            const auto optTi = er2::GetManagedObjectTypeInfo(managed);
            if (optTi) Pass("GetManagedObjectTypeInfo(optional)"); else Fail("GetManagedObjectTypeInfo(optional)", "nullopt");
        }
        else
        {
            Skip("GetManagedObjectTypeInfo", "no managed ptr on sample GO");
        }
    }

    // ── 24. GetScriptableObjectName (try on first ScriptableObject from MSID) ──
    {
        std::uintptr_t firstSO = 0;
        er2::EnumerateOptions opt;
        opt.onlyGameObject = false;
        opt.onlyScriptableObject = true;
        er2::EnumerateMsIdToPointerObjects(opt, [&](const er2::ObjectInfo& info)
        {
            if (!firstSO && info.kind == er2::ObjectKind::ScriptableObject)
            {
                firstSO = info.native;
            }
        });
        if (firstSO)
        {
            std::string soName;
            if (er2::GetScriptableObjectName(firstSO, soName))
            {
                Pass("GetScriptableObjectName");
                std::printf("  so=0x%llX name=%s\n", static_cast<unsigned long long>(firstSO), soName.c_str());
            }
            else
            {
                Skip("GetScriptableObjectName", "name read failed (offset may not match this Unity version)");
            }

            const auto optName = er2::GetScriptableObjectName(firstSO);
            if (optName) Pass("GetScriptableObjectName(optional)"); else Skip("GetScriptableObjectName(optional)", "same as above");
        }
        else
        {
            Skip("GetScriptableObjectName", "no ScriptableObject found");
            Skip("GetScriptableObjectName(optional)", "no ScriptableObject found");
        }
    }

    // ── 25. ReadPtr / ReadValue ──
    {
        std::uintptr_t val = 0;
        if (er2::ReadPtr(er2::GomGlobalSlotVa(), val) && val != 0)
            Pass("ReadPtr");
        else
            Fail("ReadPtr", "failed reading gom slot");

        const auto optVal = er2::ReadPtr(er2::GomGlobalSlotVa());
        if (optVal && *optVal != 0) Pass("ReadPtr(optional)"); else Fail("ReadPtr(optional)", "nullopt or 0");

        std::int32_t bc = 0;
        if (manager && er2::ReadValue(manager + er2::GomOff().manager.bucket_count, bc) && bc > 0)
            Pass("ReadValue");
        else
            Fail("ReadValue", "failed or 0");

        if (manager)
        {
            const auto optBc = er2::ReadValue<std::int32_t>(manager + er2::GomOff().manager.bucket_count);
            if (optBc && *optBc > 0) Pass("ReadValue(optional)"); else Fail("ReadValue(optional)", "nullopt or 0");
        }
    }

    // ── 26. SupportsDynamicFieldOffsets ──
    {
        const bool supports = er2::SupportsDynamicFieldOffsets();
        if (er2::Runtime() == er2::ManagedBackend::Mono)
        {
            if (!supports) Pass("SupportsDynamicFieldOffsets(Mono=false)"); else Fail("SupportsDynamicFieldOffsets", "Mono should be false");
        }
        else
        {
            if (supports) Pass("SupportsDynamicFieldOffsets(IL2CPP=true)"); else Fail("SupportsDynamicFieldOffsets", "IL2CPP should be true");
        }
    }

    // ── 27. IL2CPP klassmap tests ──
    if (er2::Runtime() == er2::ManagedBackend::Il2Cpp)
    {
        if (er2::EnsureIl2CppTypeInfoInited())
        {
            Pass("EnsureIl2CppTypeInfoInited");

            const std::int32_t sysObjIdx = er2::FindClassIndex("System.Object");
            if (sysObjIdx >= 0) Pass("FindClassIndex(System.Object)"); else Fail("FindClassIndex(System.Object)", "returned -1");
            std::printf("  System.Object idx=%d\n", sysObjIdx);

            const std::int32_t sysStrIdx = er2::FindClassIndex("System.String");
            if (sysStrIdx >= 0) Pass("FindClassIndex(System.String)"); else Fail("FindClassIndex(System.String)", "returned -1");

            const std::int32_t ueObjIdx = er2::FindClassIndex("UnityEngine.Object");
            if (ueObjIdx >= 0) Pass("FindClassIndex(UnityEngine.Object)"); else Fail("FindClassIndex(UnityEngine.Object)", "returned -1");

            const std::int32_t ueTrIdx = er2::FindClassIndex("UnityEngine.Transform");
            if (ueTrIdx >= 0) Pass("FindClassIndex(UnityEngine.Transform)"); else Fail("FindClassIndex(UnityEngine.Transform)", "returned -1");

            if (sysObjIdx >= 0)
            {
                const std::uintptr_t klass = er2::FindClassByIndex(sysObjIdx);
                if (klass) Pass("FindClassByIndex(System.Object)"); else Fail("FindClassByIndex(System.Object)", "returned 0");
                std::printf("  klass=0x%llX\n", static_cast<unsigned long long>(klass));
            }

            const std::uintptr_t trKlass = er2::FindClass("UnityEngine.Transform");
            if (trKlass) Pass("FindClass(UnityEngine.Transform)"); else Fail("FindClass(UnityEngine.Transform)", "returned 0");
            std::printf("  Transform klass=0x%llX\n", static_cast<unsigned long long>(trKlass));

            const std::uintptr_t goKlass = er2::FindClass("UnityEngine.GameObject");
            if (goKlass) Pass("FindClass(UnityEngine.GameObject)"); else Fail("FindClass(UnityEngine.GameObject)", "returned 0");

            const std::int32_t bogusIdx = er2::FindClassIndex("Bogus.DoesNotExist.Type12345");
            if (bogusIdx < 0) Pass("FindClassIndex(bogus)=-1"); else Fail("FindClassIndex(bogus)", "should return -1");
        }
        else
        {
            Fail("EnsureIl2CppTypeInfoInited", "returned false");
        }

        // ── IL2CPP TryGetFieldOffset ──
        if (er2::SupportsDynamicFieldOffsets())
        {
            std::uint32_t cachedPtrOff = 0;
            if (er2::TryGetFieldOffset("UnityEngine.Object", "m_CachedPtr", cachedPtrOff))
            {
                Pass("TryGetFieldOffset(m_CachedPtr)");
                std::printf("  m_CachedPtr offset=0x%X\n", cachedPtrOff);
            }
            else
            {
                Fail("TryGetFieldOffset(m_CachedPtr)", "returned false");
            }

            std::uint32_t nameOff = 0;
            if (er2::TryGetFieldOffset("UnityEngine.Object", "m_Name", nameOff))
            {
                Pass("TryGetFieldOffset(m_Name)");
                std::printf("  m_Name offset=0x%X\n", nameOff);
            }
            else
            {
                Skip("TryGetFieldOffset(m_Name)", "field not found");
            }

            // cross-validate: read m_CachedPtr from a managed object
            if (cachedPtrOff && sampleGo)
            {
                std::uintptr_t managed2 = 0;
                if (er2::ReadPtr(sampleGo + er2::Off().unity_object_managed_ptr, managed2) && managed2)
                {
                    std::uintptr_t cachedPtr = 0;
                    if (er2::ReadPtr(managed2 + static_cast<std::uintptr_t>(cachedPtrOff), cachedPtr) && cachedPtr == sampleGo)
                    {
                        Pass("m_CachedPtr cross-validate");
                    }
                    else
                    {
                        Fail("m_CachedPtr cross-validate", "cachedPtr != sampleGo");
                        std::printf("  cachedPtr=0x%llX sampleGo=0x%llX\n",
                            static_cast<unsigned long long>(cachedPtr),
                            static_cast<unsigned long long>(sampleGo));
                    }
                }
            }
        }
    }
    else
    {
        Skip("IL2CPP klassmap", "runtime is Mono");
        Skip("IL2CPP TryGetFieldOffset", "runtime is Mono");
    }

    // ── 28. Accessor queries ──
    if (er2::IsInited()) Pass("IsInited"); else Fail("IsInited", "false");
    if (er2::UnityPlayerBase()) Pass("UnityPlayerBase"); else Fail("UnityPlayerBase", "0");

    // ── Summary ──
    std::printf("\n========================================\n");
    std::printf("[smoke] PASS=%d FAIL=%d SKIP=%d\n", g_pass, g_fail, g_skip);
    std::printf("========================================\n");

    return g_fail > 0 ? 1 : 0;
}
