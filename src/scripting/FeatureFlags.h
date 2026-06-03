#pragma once

// Fine-grained compile-time feature flags for advanced scripting constructs.
// All flags default to enabled so existing scripts continue to work unless a
// build explicitly disables a specific construct.

#ifndef ET_ENABLE_SCRIPT_IF
#define ET_ENABLE_SCRIPT_IF 1
#endif

#ifndef ET_ENABLE_SCRIPT_FOR
#define ET_ENABLE_SCRIPT_FOR 1
#endif

#ifndef ET_ENABLE_SCRIPT_WHILE
#define ET_ENABLE_SCRIPT_WHILE 1
#endif

#ifndef ET_ENABLE_SCRIPT_FUNCTIONS
#define ET_ENABLE_SCRIPT_FUNCTIONS 1
#endif

