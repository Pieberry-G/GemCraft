#pragma once

#include "Core/Log.h"

#ifdef GC_DEBUG
	#define GC_DEBUGBREAK() __debugbreak()
	#define GC_ENABLE_ASSERTS
#else
	#define GC_DEBUGBREAK()
#endif

#define GC_EXPAND_MACRO(x) x
#define GC_STRINGIFY_MACRO(x) #x

#ifdef GC_ENABLE_ASSERTS
	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define GC_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { GC##type##ERROR(msg, __VA_ARGS__); GC_DEBUGBREAK(); } }
	#define GC_INTERNAL_ASSERT_WITH_MSG(type, check, ...) GC_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define GC_INTERNAL_ASSERT_NO_MSG(type, check) GC_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", GC_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define GC_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define GC_INTERNAL_ASSERT_GET_MACRO(...) GC_EXPAND_MACRO( GC_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, GC_INTERNAL_ASSERT_WITH_MSG, GC_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define GC_ASSERT(...) GC_EXPAND_MACRO( GC_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define GC_CORE_ASSERT(...) GC_EXPAND_MACRO( GC_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define GC_ASSERT(...)
	#define GC_CORE_ASSERT(...)
#endif

#define GC_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
