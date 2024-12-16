#pragma once

int main()
{
	GemCraft::Log::Init();
	GC_CORE_WARN("Log system initialized!\n");

	auto app = GemCraft::CreateApplication();
	app->Run();
	delete app;
}