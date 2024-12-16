#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace GemCraft {

	class Log
	{
	public:
		static void Init();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger();
		static std::shared_ptr<spdlog::logger>& GetClientLogger();
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

} // namespace GemCraft

//Core log macros
#define GC_CORE_TRACE(...)	::GemCraft::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define GC_CORE_INFO(...)	::GemCraft::Log::GetCoreLogger()->info(__VA_ARGS__)
#define GC_CORE_WARN(...)	::GemCraft::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define GC_CORE_ERROR(...)	::GemCraft::Log::GetCoreLogger()->error(__VA_ARGS__)
#define GC_CORE_FATAL(...)	::GemCraft::Log::GetCoreLogger()->critical(__VA_ARGS__)

//Client log macros
#define GC_TRACE(...)		::GemCraft::Log::GetClientLogger()->trace(__VA_ARGS__)
#define GC_INFO(...)		::GemCraft::Log::GetClientLogger()->info(__VA_ARGS__)
#define GC_WARN(...)		::GemCraft::Log::GetClientLogger()->warn(__VA_ARGS__)
#define GC_ERROR(...)		::GemCraft::Log::GetClientLogger()->error(__VA_ARGS__)
#define GC_FATAL(...)		::GemCraft::Log::GetClientLogger()->critical(__VA_ARGS__)