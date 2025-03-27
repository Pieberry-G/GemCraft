#pragma once

#include "Mesh/Mesh.h"
#include "Mesh/Path.h"
#include "Mesh/GemSetting.h"

namespace GemCraft {

	class GemGroup
	{
	public:
		GemGroup(GemSettingType settingType, const std::vector<std::shared_ptr<Mesh>>& gems, const std::vector<std::shared_ptr<Mesh>>& gemSettings)
			: m_SettingType(settingType), m_Gems(gems), m_GemSettings(gemSettings) {}

		GemSettingType GetGemSettingType() const { return m_SettingType; }
		const std::vector<std::shared_ptr<Mesh>>& GetGems() const { return m_Gems; }
		const std::vector<std::shared_ptr<Mesh>>& GetGemSettings() const { return m_GemSettings; }

		bool GetBooleanOpLock() const { return m_BooleanOpLock; }
		void SetBooleanOpLock(bool booleanOpLock) { m_BooleanOpLock = booleanOpLock; }

	protected:
		GemSettingType m_SettingType;
		std::vector<std::shared_ptr<Mesh>> m_Gems;
		std::vector<std::shared_ptr<Mesh>> m_GemSettings;

		bool m_BooleanOpLock = false;
	};

} // namespace GemCraft