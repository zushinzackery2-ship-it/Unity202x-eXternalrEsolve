#pragma once

#include <er2/unity2/dumpsdk/offline/PeImage.h>
#include <er2/unity2/dumpsdk/xrefs/GlobalStringXrefTypes.h>

#include <vector>

namespace er2
{

class X64ReferenceScanner
{
public:
    static std::vector<GlobalStringReferenceCandidate> Scan(const PeImage& image);
};

} // namespace er2
