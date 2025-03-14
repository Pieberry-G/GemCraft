#include "Panels/CustomUI.h"

#include "Core/ResourceManager.h"

#include <imgui.h>
#include <filesystem>

namespace GemCraft {

    void GemSettingSelectionUI::Init()
    {
        auto& resources = ResourceManager::Get()->GetResources();
        if (resources.begin() == resources.end()) {
            GC_CORE_ASSERT(false, "No gem setting resources!");
        }
        m_CurSelectedGemSetting = resources.begin()->first;
    }

    void GemSettingSelectionUI::DrawUI()
    {
        ImGui::PushID("Gem Setting Selection UI");
        ImGui::Begin("Gem Setting Selection UI", nullptr);

        auto& resources = ResourceManager::Get()->GetResources();

        std::string name = GetName(m_CurSelectedGemSetting);
        if (ImGui::BeginCombo("##GemSetting", name.c_str())) {
            ImGui::SetItemDefaultFocus();
            for (auto& pair : resources) {
                GemSettingType gemSetting = pair.first;
                bool is_selected = (m_CurSelectedGemSetting == gemSetting);
                if (ImGui::Selectable(GetName(gemSetting).c_str(), is_selected)) {
                    m_CurSelectedGemSetting = gemSetting;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            polyscope::requestRedraw();
            ImGui::EndCombo();
        }

        static float padding = 16.0f;
        static float thumbnailSize = 64.0f;
        float cellSize = thumbnailSize + padding;
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = (int)(panelWidth / cellSize);
        if (columnCount < 1) {
            columnCount = 1;
        }
        ImGui::Columns(columnCount, 0, false);

        for (auto& pair : resources) {
            GemSettingType gemSetting = pair.first;
            void* iconTextureID = pair.second->GetIconTextureID();

            ImGui::PushID(GetName(gemSetting).c_str());
            bool is_selected = (m_CurSelectedGemSetting == gemSetting);
            if (!is_selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            }
            ImGui::ImageButton(iconTextureID, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
            if (!is_selected) {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_CurSelectedGemSetting = gemSetting;
            }
            ImGui::NextColumn();

            ImGui::PopID();
        }

        ImGui::Columns(1);
    }

    void GemPatternUI::DrawUI()
    {
        ImGui::PushID("Gem Pattern UI");
        ImGui::Begin("Gem Pattern UI", nullptr);

        if (ImGui::InputInt("Fairing Continuity", &m_FairingContinuity, 1)) {
            if (m_FairingContinuity < 0) {
                m_FairingContinuity = 0;
            }
            if (m_FairingContinuity > 1) {
                m_FairingContinuity = 1;
            }
        }
        ImGui::Separator();

        ImGui::Checkbox("Enable Hole Shrink", &m_EnableHoleShrink);
        if (m_EnableHoleShrink) {
            if (ImGui::InputFloat("Hole Shrink Length", &m_HoleShrinkLength, 0.1f, 0.0f, "%.2f")) {
                if (m_HoleShrinkLength < 0.2f) {
                    m_HoleShrinkLength = 0.2f;
                }
            }
        }
        if (ImGui::InputFloat("Hole Depth", &m_HoleDepth, 0.1f, 0.0f, "%.2f")) {
            if (m_HoleDepth < 0.1f) {
                m_HoleDepth = 0.1f;
            }
        }
        ImGui::Separator();

        if (ImGui::InputFloat("Gem Scale", &m_GemScale, 0.1f, 0.0f, "%.2f")) {
            if (m_GemScale < 0.8f) {
                m_GemScale = 0.8f;
            }
        }
        ImGui::InputFloat("Gem Exposure Length", &m_GemExposureLength, 0.1f, 0.0f, "%.2f");
        ImGui::Separator();

        std::vector<PackingMode> modes{ PackingMode::Hexagonal, PackingMode::Square };
        std::string name = GetName(m_CurSelectedPackingMode);
        if (ImGui::BeginCombo("Packing Mode", name.c_str())) {
            ImGui::SetItemDefaultFocus();
            for (auto& mode : modes) {
                bool is_selected = (m_CurSelectedPackingMode == mode);
                if (ImGui::Selectable(GetName(mode).c_str(), is_selected)) {
                    m_CurSelectedPackingMode = mode;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            polyscope::requestRedraw();
            ImGui::EndCombo();
        }
        if (m_CurSelectedPackingMode == PackingMode::Compact) {
            ImGui::InputFloat("Edge Loop Density", &m_PackingEdgeLoopDensity, 0.01f, 0.0f, "%.2f");
            ImGui::InputFloat("Center Density", &m_PackingCenterDensity, 0.01f, 0.0f, "%.2f");
        }
        ImGui::InputFloat("Grid Rotation (angle)", &m_GridRotation, 15.0f, 0.0f, "%.2f");
        ImGui::Separator();

        if (ImGui::InputFloat("Sphere Tool Radius", &m_SphereToolRadius, 0.1f, 0.0f, "%.2f")) {
            if (m_SphereToolRadius < 0.0f) {
                m_SphereToolRadius = 0.0f;
            }
        }
    }

} // namespace GemCraft