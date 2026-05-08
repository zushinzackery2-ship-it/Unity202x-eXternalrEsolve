#pragma once

// Unified Windows.h include wrapper
// Prevents min/max macro conflicts and reduces compile time

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef NOCRYPT
#define NOCRYPT
#endif

#ifndef NOGDI
#define NOGDI
#endif

#include <Windows.h>
