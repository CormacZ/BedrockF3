// Use LeviLamina's memory operators by default. Improves the memory
// management of the mod by routing allocations through LeviLamina's
// mimalloc-backed allocator. This is the same one-line include the
// official LeviLamina mod template uses.
//
// This file is referenced by xmake's add_files("src/**.cpp"), so it is
// compiled into the mod DLL automatically.

#define LL_MEMORY_OPERATORS

#include "ll/api/memory/MemoryOperators.h" // IWYU pragma: keep
