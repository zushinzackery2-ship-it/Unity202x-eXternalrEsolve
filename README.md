<div align="center">

# er2 — Unity Runtime Structure Resolver

**Cross-process runtime introspection framework for Unity 2020–2023 (Windows x64)**

*Header-only C++17 · No symbols/RTTI required · Mono & IL2CPP*

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey?style=flat-square)
![Header Only](https://img.shields.io/badge/Header--Only-Yes-green?style=flat-square)

</div>

---

## What This Project Does

er2 is a **pure-algorithmic library** that reconstructs Unity engine runtime data structures from a live process's memory — **without debug symbols, RTTI, or source code access**.

Given only a process handle and a raw memory read primitive, er2 can:

- **Discover and validate** the engine's internal object management structures (hash-bucketed doubly-linked lists)
- **Reconstruct the IL2CPP type system** from in-memory metadata, resolving class hierarchies and field offsets at runtime
- **Traverse managed↔native object bridges** to map between .NET managed objects and their C++ native counterparts
- **Parse hierarchical spatial data** (transform trees, projection matrices) from opaque binary layouts

The core contribution is the set of **heuristic scanning and structural validation algorithms** that make this possible without any prior knowledge of the target binary's symbol table.

> **Compatibility:** Unity 2020–2023 on Windows x64, both Mono and IL2CPP backends (auto-detected).  
> For Unity 6 support, see [Unity6-eXternalrEsolve](https://github.com/zushinzackery2-ship-it/Unity6-eXternalrEsolve).

---

## Technical Highlights

### Blind Pointer-Chain Scanning

The engine's central object manager is located through a **multi-stage pointer chain scan** with no hardcoded addresses:

1. **Seed scan** — full address space pattern match finds heap objects containing a known structural signature (bucket stride + alignment)
2. **Table head derivation** — walks backward from the seed to locate the hash table base
3. **Two-level global resolution** — cascading pattern scans narrow from heap → data section → global slot, with candidate scoring at each stage

Each candidate is validated by structural checks (circular doubly-linked list integrity via **Floyd's cycle detection**, bucket consistency, pointer range heuristics) rather than signature matching, making the approach resilient to compiler/version changes.

### Remote IL2CPP Type System Reconstruction

For IL2CPP builds, er2 reconstructs the type information table entirely from process memory:

- Exports and parses the `global-metadata.dat` header from remote memory using a **scoring-based heuristic** (no file path assumption)
- Builds a `byval_arg → class name` mapping from metadata bytes
- Locates the `Il2CppClass*[]` pointer table by sampling known types and verifying name round-trips
- Resolves **runtime field offsets** by walking `metadataRegistration → fieldOffsets` chains, enabling symbol-free struct field access

### Architecture

```
┌─────────────────────────────────────────┐
│          Application / Tests            │
├─────────────────────────────────────────┤
│         init/* (thin wrappers)          │  ← Global-context convenience layer
├──────┬──────┬───────┬───────┬───────────┤
│ gom/ │ msid/│object/│camera/│ metadata/ │  ← Domain-specific algorithms
│      │      │       │transf/│ dumpsdk/  │
├──────┴──────┴───────┴───────┴───────────┤
│     IMemoryAccessor  (pure interface)   │  ← Platform abstraction boundary
├─────────────────────────────────────────┤
│   os/win/  (WinAPI implementation)      │  ← Swappable backend
└─────────────────────────────────────────┘
```

- **Zero coupling** — algorithm layer depends only on `const IMemoryAccessor&`, a single-method read interface
- **Header-only** — all logic in `.hpp` files under `include/er2/`, no link-time dependencies
- **Backend-swappable** — `IContextBackend` abstracts process handle, module enumeration, and memory access; default is `WinApiContextBackend`

---

## Key Algorithms & Data Structures

| Algorithm | Where | Purpose |
|:----------|:------|:--------|
| **Multi-stage pointer chain scan** | `gom/scan_chain.hpp` | Locates global engine singleton from raw memory without symbols |
| **Floyd's tortoise-and-hare cycle detection** | `gom/validate_dlist.hpp` | Validates circular doubly-linked lists in untrusted memory |
| **Heuristic structure scoring** | `gom/manager_score.hpp` | Ranks candidate addresses by structural consistency (list integrity, bucket alignment, pointer range) |
| **PE section parsing** | `metadata/pe.hpp` | Identifies `.data`/`.rdata` sections to constrain scan ranges |
| **Metadata header scoring** | `metadata/export.hpp` | Locates IL2CPP metadata blob via statistical header field validation |
| **Type table brute-force with anchor verification** | `init/classmap.hpp` | Finds `Il2CppClass*[]` table by sampling known type names as anchors |
| **Quaternion → world-position SIMD pipeline** | `transform/` | Reconstructs world coordinates from hierarchical local transforms |
| **3D projection (view-projection matrix)** | `camera/` | Extracts camera matrices and performs coordinate space transformations |

---

## API Overview

After a single `er2::AutoInit()` call, all query functions are available through a global context:

| Category | Representative API | Description |
|:---------|:-------------------|:------------|
| **Context** | `AutoInit()` / `IsInited()` / `Runtime()` | Process discovery, runtime detection (Mono/IL2CPP) |
| | `Mem()` / `ReadPtr()` / `ReadValue<T>()` | Abstracted memory access |
| **Structure Discovery** | `GomManager()` / `GomBucketsPtr()` | Access reconstructed engine object manager |
| | `EnumerateGameObjects()` / `GetGameObjectByName()` | Object graph traversal and lookup |
| | `CheckGomManagerCandidate()` | Structural validation with scoring |
| **Type System (IL2CPP)** | `EnsureIl2CppTypeInfoInited()` / `FindClass()` | Remote type table initialization and class lookup |
| | `TryGetFieldOffset()` / `GetFieldOffsetOr()` | Runtime field offset resolution from metadata |
| **Object Introspection** | `GetAllComponents()` / `GetComponentThroughTypeName()` | Component pool traversal |
| | `GetManagedObjectTypeInfo()` | Managed↔native bridge queries |
| | `FindObjectsOfTypeAll()` | Type-filtered instance enumeration via global registry |
| **Spatial Data** | `GetTransformWorldPosition()` | Hierarchical transform chain resolution |
| | `GetCameraMatrix()` / `WorldToScreenPoint()` | Projection matrix extraction and coordinate transform |
| | `GetBoneTransformAll()` | Skeletal hierarchy traversal |

> **Return conventions:** functions return `std::optional<T>` on success or provide `bool` + out-parameter overloads.

---

## Build

- **C++17** / MSVC (Visual Studio 2022) / Windows x64 / Static runtime (`/MT`)
- **Dependencies:** [GLM](https://github.com/g-truc/glm) (matrix math)

---

## Testing

| Suite | Coverage | Status |
|:------|:---------|:-------|
| `tests/winapi_smoke` | Full API smoke test against live Unity process | Mono: **33 PASS** · IL2CPP: **46 PASS** · 0 FAIL |
| `tests/unit` | Pure algorithm unit tests (hash, projection, quaternion rotation) | All passing |

The smoke test covers: context initialization → structure discovery → object enumeration → type system reconstruction → component queries → spatial data extraction → managed object introspection → field offset cross-validation.

---

<details>
<summary><strong>Directory Layout</strong></summary>

```
er2/
├── Resolve202x.hpp                 # Single-include entry point
├── include/er2/
│   ├── compat/                     # Compiler/platform compatibility
│   ├── core/                       # Fundamental types
│   ├── mem/                        # IMemoryAccessor interface
│   ├── os/win/                     # Windows platform backend
│   └── unity2/
│       ├── camera/                 # Projection matrix extraction
│       ├── core/                   # Structure offset definitions
│       ├── dumpsdk/                # SDK/type descriptor export
│       ├── gom/                    # Object manager scanning & traversal
│       ├── init/                   # Global-context convenience wrappers
│       ├── metadata/               # IL2CPP metadata reconstruction
│       │   ├── header/             #   Header parsing & validation
│       │   ├── registration/       #   Registration table scanning
│       │   ├── hint/               #   Heuristic hint system
│       │   └── codegen/            #   Code generation support
│       ├── msid/                   # Instance ID registry
│       ├── object/                 # Object introspection
│       │   ├── managed/            #   Managed (.NET) object access
│       │   └── native/             #   Native (C++) object access
│       ├── transform/              # Transform hierarchy & world coords
│       └── util/                   # Shared utilities
└── tests/
    ├── unit/                       # Algorithm unit tests
    └── winapi_smoke/               # Integration smoke tests
```

</details>

---

## Quick Start

```cpp
#include "Resolve202x.hpp"
#include <cstdio>

int main()
{
    if (!er2::AutoInit())
    {
        return 1;
    }

    std::printf("Runtime: %s\n",
        er2::Runtime() == er2::ManagedBackend::Il2Cpp ? "IL2CPP" : "Mono");

    // Enumerate all objects of a given type
    auto transforms = er2::FindObjectsOfTypeAll("UnityEngine", "Transform");
    std::printf("Transform instances: %zu\n", transforms.size());

    // IL2CPP: resolve field offsets from metadata (no hardcoded values)
    if (er2::SupportsDynamicFieldOffsets())
    {
        std::uint32_t offset = 0;
        if (er2::TryGetFieldOffset("UnityEngine.Object", "m_CachedPtr", offset))
        {
            std::printf("m_CachedPtr offset = 0x%X\n", offset);
        }
    }

    // IL2CPP: look up class by fully-qualified name
    const std::uintptr_t klass = er2::FindClass("UnityEngine.Transform");
    if (klass)
    {
        std::printf("Transform class @ 0x%llX\n", (unsigned long long)klass);
    }

    return 0;
}
```

---

<div align="center">

**Platform:** Windows x64 &nbsp;|&nbsp; **License:** MIT

</div>

> **Disclaimer:** This project is intended for academic study of Unity engine internals, authorized modding/plugin development, and reverse engineering research. Users are responsible for ensuring compliance with applicable laws and terms of service.



