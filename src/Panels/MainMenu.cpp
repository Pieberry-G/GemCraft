#include "Panels/MainMenu.h"

#include "Core/Application.h"
#include "Core/WindowsPlatformUtils.h"
#include "Mesh/Mesh.h"
#include "EventSystem/Input.h"
#include "EventSystem/ApplicationEvent.h"

#include <polyscope/polyscope.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <filesystem>

namespace GemCraft {

    MainMenu::MainMenu()
    {
        polyscope::state::mainMenuCallback = GetDrawUIFunction();
    }

    void MainMenu::DrawMainMenu()
    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Project...", "Ctrl+N")) {
                    NewProject();
                }
                if (ImGui::MenuItem("Export...", "Ctrl+S")) {
                    ExportMesh();
                }
                if (ImGui::MenuItem("Quit")) {
                    WindowCloseEvent event;
                    Application::Get()->OnEvent(event);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Themes")) {
                if (ImGui::MenuItem("Green")) {
                    polyscope::options::themeColor = "Green";
                    polyscope::options::configureImGuiStyleCallback();
                }
                else if (ImGui::MenuItem("Red")) {
                    polyscope::options::themeColor = "Red";
                    polyscope::options::configureImGuiStyleCallback();
                }
                else if (ImGui::MenuItem("Blue")) {
                    polyscope::options::themeColor = "Blue";
                    polyscope::options::configureImGuiStyleCallback();
                }
                else if (ImGui::MenuItem("Purple")) {
                    polyscope::options::themeColor = "Purple";
                    polyscope::options::configureImGuiStyleCallback();
                }
                else if (ImGui::MenuItem("Brown")) {
                    polyscope::options::themeColor = "Brown";
                    polyscope::options::configureImGuiStyleCallback();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    bool MainMenu::OnKeyReleased(KeyReleasedEvent& e)
    {
        bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
		switch (e.GetKeyCode())
		{
        case Key::S:
        {
            if (control) {
                ExportMesh();
                return true;
            }
            break;
        }
		case Key::N:
		{
            if (control) {
                NewProject();
                return true;
            }
			break;
		}
		}
        return false;
    }

    void MainMenu::NewProject()
    {
        std::string filepath = FileDialogs::OpenFile("AI Generated Mesh (*.obj)\0*.obj\0");
        if (!filepath.empty()) {
           std::shared_ptr<Scene>& scene = Application::Get()->GetScene();
           scene->Clean();

           GC_CORE_WARN("Loading ring file: {0}", filepath);
           scene->AddRing("Ring", filepath);

           // Get indices for element picking
           std::shared_ptr<Mesh>& ring = scene->GetRing();
           polyscope::state::facePickIndStart = ring->nVertices();
           polyscope::state::edgePickIndStart = polyscope::state::facePickIndStart + ring->nFaces();
           polyscope::state::halfedgePickIndStart = polyscope::state::edgePickIndStart + ring->nEdges();

           polyscope::view::resetCameraToHomeView();
       }
    }

    static void WriteMeshWithOffset(std::shared_ptr<Mesh>& mesh, const std::string& name, std::ofstream& out, int& vertexOffset)
    {
        out << "o " << name << "\n";
        glm::mat4 transform = mesh->GetPsTransform();
        for (const auto& p : mesh->GetVertices()) {
            glm::vec4 p_tf = transform * glm::vec4(p, 1.0f);
            out << "v " << p_tf.x << " " << p_tf.y << " " << p_tf.z << "\n";
        }
        for (const auto& f : mesh->GetFaces()) {
            out << "f";
            for (size_t index : f) {
                out << " " << (vertexOffset + index + 1);
            }
            out << "\n";
        }
        vertexOffset += mesh->GetVertices().size();
    }

    void MainMenu::ExportMesh()
    {
        std::string filepath = FileDialogs::SaveFile("(*.obj)\0*.obj\0");
        if (!filepath.empty()) {
            std::shared_ptr<Scene> scene = Application::Get()->GetScene();
            std::ofstream out(filepath);
            if (!out) {
                GC_CORE_ASSERT(false, "Can not open file!");
            }
            int vertexOffset = 0;
            WriteMeshWithOffset(scene->GetRing(), "Ring", out, vertexOffset);
            std::vector<GemGroup>& gemGroups = scene->GetGemGroups();
            for (GemGroup gemGroup : gemGroups) {
                const std::vector<std::shared_ptr<Mesh>>& gems = gemGroup.GetGems();
                for (std::shared_ptr<Mesh> gem : gems) {
                    WriteMeshWithOffset(gem, gem->GetName(), out, vertexOffset);
                }
                const std::vector<std::shared_ptr<Mesh>>& gemSettings = gemGroup.GetGemSettings();
                for (std::shared_ptr<Mesh> gemSetting : gemSettings) {
                    WriteMeshWithOffset(gemSetting, gemSetting->GetName(), out, vertexOffset);
                }
            }
            out.close();
        }
    }

} // namespace GemCraft