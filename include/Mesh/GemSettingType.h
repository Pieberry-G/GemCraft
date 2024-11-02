#pragma once

namespace GemCraft {

    enum class GemSettingType
    {
        Bezel,      // °üÏâ
        Pave,       // ¶¤Ïâ
        Prong,      // ×¦Ïâ
        Claw,       // »¢×¦Ïâ
        Invisible,  // ÎŞ±ßÏâ
    };

    inline std::string GetName(GemSettingType settingType)
    {
        static const std::unordered_map<GemSettingType, std::string> enumToString = {
            { GemSettingType::Bezel,     "Bezel Setting"     },
            { GemSettingType::Pave,      "Pave Setting"      },
            { GemSettingType::Prong,     "Prong Setting"     },
            { GemSettingType::Claw,      "Claw Setting"      },
            { GemSettingType::Invisible, "Invisible Setting" },
        };

        auto it = enumToString.find(settingType);
        if (it != enumToString.end()) {
            return it->second;
        }
        GC_CORE_ASSERT(false, "Unknown gem setting type.");
        return "Unknown";
    }

} // namespace GemCraft