#pragma once

#include "Mesh/GemSettingType.h"

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

		int GetNumberOfPaths() const { return m_NumberOfPaths; }
		float GetPathSpacing() const { return m_PathSpacing; }
		int GetGemCountPerPath() const { return m_GemCountPerPath; }
		float GetGemRotation() const { return m_GemRotation; }
		float GetGemScale() const { return m_GemScale; }
		float GetGemDepth() const { return m_GemDepth; }
	private:
		int m_NumberOfPaths = 1;
		float m_PathSpacing = 1.0f;
		int m_GemCountPerPath = 2;
		float m_GemRotation = 0.0f;
		float m_GemScale = 1.0f;
		float m_GemDepth = 0.3f;
	};

} // namespace GemCraft