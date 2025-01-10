#pragma once

#include "Mesh/GemSetting.h"

#include <filesystem>

namespace GemCraft {

	class GemSelectionUI
	{
	public:
		void Init();

		void DrawUI();
		std::function<void()> GetDrawUIFunction() { return std::bind(&GemSelectionUI::DrawUI, this); }

		const std::string& GetCurSelectedGem() const { return m_CurSelectedGem; }

	private:
		std::string m_CurSelectedGem;
	};

	class GemSettingSelectionUI
	{
	public:
		void Init();

		void DrawUI();
		std::function<void()> GetDrawUIFunction() { return std::bind(&GemSettingSelectionUI::DrawUI, this); }

		GemSettingType GetCurSelectedGemSetting() const { return m_CurSelectedGemSetting; }

	private:
		GemSettingType m_CurSelectedGemSetting;
	};

	class GemPatternUI
	{
	public:
		void DrawUI();
		std::function<void()> GetDrawUIFunction() { return std::bind(&GemPatternUI::DrawUI, this); }

		float GetExposureDepth() const { return m_ExposureDepth; }
		float GetGemScale() const { return m_GemScale; }

	private:
		float m_ExposureDepth = 0.3f;
		float m_GemScale = 1.3f;
	};

} // namespace GemCraft