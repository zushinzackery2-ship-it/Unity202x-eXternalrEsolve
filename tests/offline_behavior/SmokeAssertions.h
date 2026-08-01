#pragma once

#include "SmokeChecks.h"
#include "SmokePeBuilder.h"

#include <er2/unity2/dumpsdk/collected_data.hpp>

#include <string>

namespace OfflineBehavior
{

void CheckCollectedGraph(Checks& checks, const er2::CollectedData& data, const SmokeModule& module);
void CheckDumpCs(Checks& checks, const std::string& dump, const SmokeModule& module);
void CheckSidecars(Checks& checks, const std::string& outputDir);

} // namespace OfflineBehavior
