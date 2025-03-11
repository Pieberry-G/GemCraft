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

		int GetFairingContinuity() const { return m_FairingContinuity; }
		bool GetEnableHoleShrink() const { return m_EnableHoleShrink; }
		float GetHoleShrinkLength() const { return m_HoleShrinkLength; }
		float GetHoleDepth() const { return m_HoleDepth; }
		float GetGemScale() const { return m_GemScale; }
		float GetGemExposureLength() const { return m_GemExposureLength; }
		PackingMode GetCurSelectedPackingMode() const { return m_CurSelectedPackingMode; }
		float GetPackingEdgeLoopDensity() const{ return m_PackingEdgeLoopDensity; }
		float GetPackingCenterDensity() const{ return m_PackingCenterDensity; }
		float GetGridRotation() const { return m_GridRotation; }
		float GetSphereToolRadius() const { return m_SphereToolRadius; }

	private:
		int m_FairingContinuity = 0;
		bool m_EnableHoleShrink = true;
		float m_HoleShrinkLength = 0.5f;
		float m_HoleDepth = 0.5f;
		float m_GemScale = 1.5f;
		float m_GemExposureLength = 0.0f;
		PackingMode m_CurSelectedPackingMode = PackingMode::Hexagonal;
		float m_PackingEdgeLoopDensity = 0.8f;
		float m_PackingCenterDensity = 1.03f;
		float m_GridRotation = 0.0f;
		float m_SphereToolRadius = 0.5f;

	};

} // namespace GemCraft