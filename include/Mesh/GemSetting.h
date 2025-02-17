#pragma once

namespace GemCraft {

    enum class GemSettingType
    {
        Bezel,      // ∞¸œ‚
        Pave,       // ∂§œ‚
        Prong,      // ◊¶œ‚
        Claw,       // ª¢◊¶œ‚
        Invisible,  // Œﬁ±ﬂœ‚
        //Shovel,     // ≤˘±ﬂœ‚
        //Channel,    // πÏµ¿œ‚
        Gen003,
    };

    inline std::string GetName(GemSettingType settingType)
    {
        static const std::unordered_map<GemSettingType, std::string> enumToString = {
            { GemSettingType::Bezel,     "Bezel Setting"     },
            { GemSettingType::Pave,      "Pave Setting"      },
            { GemSettingType::Prong,     "Prong Setting"     },
            { GemSettingType::Claw,      "Claw Setting"      },
            { GemSettingType::Invisible, "Invisible Setting" },
            //{ GemSettingType::Shovel,    "Shovel Setting"    },
            //{ GemSettingType::Channel,   "Channel Setting"   },
            { GemSettingType::Gen003,    "Gen003 Setting"    },
        };

        auto it = enumToString.find(settingType);
        if (it != enumToString.end()) {
            return it->second;
        }
        GC_CORE_ASSERT(false, "Unknown gem setting type.");
        return "Unknown";
    }

    struct SettingParams
    {
        float GemSpacing;
    };

    inline SettingParams GetParams(GemSettingType settingType)
    {
        static const std::unordered_map<GemSettingType, SettingParams> enumToSettingParams = {
            { GemSettingType::Bezel,     { 0.400f } },
            { GemSettingType::Pave,      { 0.133f } },
            { GemSettingType::Prong,     { 0.100f } },
            { GemSettingType::Claw,      { 0.133f } },
            { GemSettingType::Invisible, { 0.100f } },
            //{ GemSettingType::Shovel,    { 0.133f } },
            //{ GemSettingType::Channel,   { 0.100f } },
            { GemSettingType::Gen003,    { 0.100f } },
        };

        auto it = enumToSettingParams.find(settingType);
        if (it != enumToSettingParams.end()) {
            return it->second;
        }
        GC_CORE_ASSERT(false, "Unknown gem setting type.");
        return SettingParams();
    }

} // namespace GemCraft