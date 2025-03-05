#pragma once

int main()
{
	GemCraft::Log::Init();
	GC_CORE_WARN("Log system initialized!");

	GemCraft::Application* app = new GemCraft::Application();
	app->Run();
	delete app;
}