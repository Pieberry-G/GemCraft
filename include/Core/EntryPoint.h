#pragma once

int main()
{
	GemCraft::Log::Init();
	GC_CORE_WARN("Initialized Log!");

	auto app = GemCraft::CreateApplication();
	app->Run();
	delete app;
}