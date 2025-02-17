#pragma once

#include "Mesh/GemSetting.h"

#include <filesystem>

namespace GemCraft {

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
		float GetGridRotation() const { return m_GridRotation; }

	private:
		float m_ExposureDepth = 0.3f;
		float m_GemScale = 2.0f;
		float m_GridRotation = 0.0f;
	};

} // namespace GemCraft