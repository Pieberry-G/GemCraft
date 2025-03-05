#pragma once

#include "Mesh/GemSetting.h"

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

		bool GetEnableHoleShrink() const { return m_EnableHoleShrink; }
		float GetHoleShrinkLength() const { return m_HoleShrinkLength; }
		float GetHoleDepth() const { return m_HoleDepth; }
		PackingMode GetCurSelectedPackingMode() const { return m_CurSelectedPackingMode; }
		float GetGemScale() const { return m_GemScale; }
		float GetGemExposureDepth() const { return m_GemExposureDepth; }
		float GetGridRotation() const { return m_GridRotation; }

	private:
		bool m_EnableHoleShrink = true;
		float m_HoleShrinkLength = 0.5f;
		float m_HoleDepth = 0.5f;
		PackingMode m_CurSelectedPackingMode = PackingMode::Hexagonal;
		float m_GemScale = 1.5f;
		float m_GemExposureDepth = 0.0f;
		float m_GridRotation = 0.0f;
	};

} // namespace GemCraft