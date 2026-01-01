#pragma once



#include <vector>

#include <string>



#include "enumerate_objects.hpp"



namespace er2

{



struct FindObjectsOfTypeAllResult

{

    std::uintptr_t native = 0;

    std::uint32_t instanceId = 0;

};



inline bool FindObjectsOfTypeAll(

    ManagedBackend runtime,

    const IMemoryAccessor& mem,

    std::uintptr_t msIdToPointerAddr,

    const Offsets& off,

    const UnityPlayerRange& unityPlayer,

    const char* targetClassName,

    std::vector<FindObjectsOfTypeAllResult>& out,

    const char* targetNamespace = nullptr)

{

    out.clear();

    if (!targetClassName)

    {

        return false;

    }



    std::string targetCn = targetClassName;

    std::string targetNs = targetNamespace ? targetNamespace : "";



    EnumerateOptions opt;

    opt.onlyGameObject = false;      // Search everything

    opt.onlyScriptableObject = false; 



    // Use EnumerateMsIdToPointerObjects to reuse optimized traversal logic

    return EnumerateMsIdToPointerObjects(

        runtime, 

        mem, 

        msIdToPointerAddr, 

        off, 

        unityPlayer, 

        opt, 

        [&](const ObjectInfo& info) 

        {

            // Parse full name "Namespace.ClassName" or just "ClassName"

            // info.typeFullName

            

            // Should accurate match separate NS/CN or just check FullName?

            // User request: "FindObjectsOfTypeAll就是从MSID池返回所有TpyeName符合传入的字符串参数的对象"

            // Usually we expect exact match or "contains" match?

            // Let's do exact class name match.

            

            // But ObjectInfo only has full string. 

            // We can re-parse it or rely on String Match.

            // Wait, EnumerateMsIdToPointerObjects internals already parsed NS/CN. 

            // Maybe we should pass a callback to Enumerate that allows accessing raw CN/NS?

            // But for now, let's parse typeFullName.

            

            std::string objNs;

            std::string objCn;

            

            size_t dotPos = info.typeFullName.rfind('.');

            if (dotPos != std::string::npos)

            {

                objNs = info.typeFullName.substr(0, dotPos);

                objCn = info.typeFullName.substr(dotPos + 1);

            }

            else

            {

                objCn = info.typeFullName;

            }



            if (objCn == targetCn)

            {

                if (targetNs.empty() || objNs == targetNs)

                {

                    FindObjectsOfTypeAllResult res;

                    res.native = info.native;

                    res.instanceId = info.instanceId;

                    out.push_back(res);

                }

            }

        }

    );

}



} // namespace er2

