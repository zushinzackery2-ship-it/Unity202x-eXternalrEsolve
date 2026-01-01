#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include "context.hpp"
#include "component.hpp"
#include "object.hpp"
#include "../object/native/native_component.hpp"
#include "../transform/transform.hpp"

namespace er2
{
struct BoneTransformAllItem { std::int32_t index = 0; std::string boneName; std::uintptr_t transform = 0; };

inline std::vector<BoneTransformAllItem> GetBoneTransformAll(std::uintptr_t rootGameObjectNative, int maxDepth = 10, int maxNodes = 4096)
{
    std::vector<BoneTransformAllItem> out;
    if (!IsInited() || !rootGameObjectNative) return out;
    const std::uintptr_t rootTr = GetTransformComponent(rootGameObjectNative);
    if (!rootTr) return out;
    static constexpr std::uint32_t kTransformChildrenPtr = 0x70;
    static constexpr std::uint32_t kTransformChildrenCount = 0x80;
    struct Pending { std::uintptr_t tr; int depth; };
    std::vector<Pending> stack; stack.push_back({rootTr, 0});
    std::unordered_set<std::uintptr_t> visited;
    while (!stack.empty()) {
        const Pending cur = stack.back(); stack.pop_back();
        if (cur.depth > maxDepth || !IsCanonicalUserPtr(cur.tr) || visited.count(cur.tr)) continue;
        visited.insert(cur.tr);
        std::uintptr_t curGo = 0;
        if (er2::GetNativeComponentGameObject(Mem(), cur.tr, g_ctx.gomOff, curGo) && curGo) {
            BoneTransformAllItem item; item.transform = cur.tr;
            TransformHierarchyState st; std::int32_t idx = 0;
            if (er2::ReadTransformHierarchyState(Mem(), cur.tr, g_ctx.transformOff, st, idx)) item.index = idx;
            std::string name;
            if (er2::ReadNativeGameObjectName(Mem(), curGo, g_ctx.gomOff, name)) item.boneName = std::move(name);
            out.push_back(std::move(item));
            if ((int)out.size() >= maxNodes) break;
        }
        std::uintptr_t childrenPtr = 0; std::uint32_t childrenCount = 0;
        if (!er2::ReadPtr(Mem(), cur.tr + kTransformChildrenPtr, childrenPtr) || !childrenPtr) continue;
        if (!er2::ReadValue(Mem(), cur.tr + kTransformChildrenCount, childrenCount) || childrenCount == 0 || childrenCount >= 100) continue;
        for (std::uint32_t i = 0; i < childrenCount; ++i) {
            std::uintptr_t child = 0;
            if (er2::ReadPtr(Mem(), childrenPtr + i * sizeof(std::uintptr_t), child) && child) stack.push_back({child, cur.depth + 1});
        }
    }
    return out;
}
} // namespace er2
