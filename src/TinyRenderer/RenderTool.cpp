#include "TinyRenderer/RenderTool.h"

#include <filesystem>

namespace GemCraft {
namespace TinyRenderer {

	static const glm::vec3 Black{ 0.0f, 0.0f, 0.0f };
	static const glm::vec3 White{ 1.0f, 1.0f, 1.0f };

    std::unique_ptr<RenderToolData> RenderTool::s_Data = nullptr;

    void RenderTool::Init()
    {
        s_Data = std::make_unique<RenderToolData>();

        s_Data->ShaderProgram = polyscope::render::engine->requestShader(
            "TINY_RENDERER", std::vector<std::string>(), polyscope::render::ShaderReplacementDefaults::Process);

        s_Data->TinyRendererFb = polyscope::render::engine->tinyRendererFb.get();
        s_Data->TinyRendererFb->resize(1024, 1024);
        s_Data->TinyRendererFb->setViewport(0, 0, 1024, 1024);
        s_Data->TinyRendererFb->clearColor = glm::vec3(0.0f);

        uint32_t whiteTextureData = 0xffffffff;
        s_Data->WhiteTexture = Image::Create(1, 1, &whiteTextureData);
    }

    void RenderTool::Render(Model& model, Camera& camera)
	{
        // Bind frameBuffer
		if (!s_Data->TinyRendererFb->bindForRendering()) return;

        polyscope::render::engine->setDepthMode();
        polyscope::render::engine->setBlendMode();
		polyscope::render::engine->setBackfaceCull(true);

		s_Data->TinyRendererFb->clear();
		s_Data->TinyRendererFb->clear(0, White);
		s_Data->TinyRendererFb->clear(1, Black);
        s_Data->TinyRendererFb->clear(2, { -1.0f, -1.0f, -1.0f });

		DrawModel(model, camera, s_Data->ShaderProgram);

		polyscope::render::engine->setBackfaceCull(); // return to default setting
	}

    void RenderTool::BackProjection(MeshSubset& subset, const std::string& maskFile)
    {
        std::shared_ptr<Image> image = Image::Create(maskFile);
        std::vector<std::array<float, 4>> outData = image->ReadBuffer();

        std::vector<glm::vec4> faceIDs;
        faceIDs = polyscope::render::engine->tinyRendererBuffer[2]->getDataVector4();

        for (size_t i = 0; i < 1024; i++) {
            for (size_t j = 0; j < 1024; j++) {
                if (std::abs(faceIDs[i * 1024 + j].x + 1.0f) >= 1e-6f && std::abs(outData[i * 1024 + j][0] - 1.0f) < 1e-6f) {
                    size_t faceID = static_cast<size_t>(std::round(faceIDs[i * 1024 + j].x));
                    subset.AddFace(faceID);
                }
            }
        }
    }

    void RenderTool::SaveRenderResult(const std::string& outFile, uint32_t location)
    {
        std::vector<glm::vec4> data;
        data = polyscope::render::engine->tinyRendererBuffer[location]->getDataVector4();
        switch (location)
        {
        case 0: // Render
        {
            unsigned char* buffer = new unsigned char[1024 * 1024 * 3];
            for (size_t i = 0; i < data.size(); i++) {
                buffer[i * 3 + 0] = static_cast<unsigned char>(data[i].r * 255.0f); // R
                buffer[i * 3 + 1] = static_cast<unsigned char>(data[i].g * 255.0f); // G
                buffer[i * 3 + 2] = static_cast<unsigned char>(data[i].b * 255.0f); // B
            }
            polyscope::saveImage(outFile, buffer, 1024, 1024, 3);
            break;
        }
        case 1: // Mask
        {
            unsigned char* buffer = new unsigned char[1024 * 1024];
            for (size_t i = 0; i < data.size(); i++) {
                buffer[i] = static_cast<unsigned char>(data[i].r * 255.0f); // R
            }
            polyscope::saveImage(outFile, buffer, 1024, 1024, 1);
            break;
        }
        }
    }

    void RenderTool::DrawModel(Model& model, Camera& camera, std::shared_ptr<polyscope::render::ShaderProgram> gltfShaderProgram)
    {
        auto& images = model.m_Images;
        auto& textures = model.m_Textures;
        auto& materials = model.m_Materials;
        auto& meshes = model.m_Meshes;

        size_t faceIDStart = polyscope::state::facePickIndStart;
        for (auto& mesh : meshes) {
            // Store data in buffers
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> texcoords;
            std::vector<double> faceID;

            GC_CORE_ASSERT(mesh.Vertices.size() == mesh.Indices.size());
            for (size_t i = 0; i < mesh.Vertices.size(); i++) {
                auto& vert = mesh.Vertices[i];
                positions.push_back(vert.Position);
                normals.push_back(vert.Normal);
                texcoords.push_back(vert.TexCoord);
                faceID.push_back(double(i / 3));
            }
            faceIDStart += mesh.Vertices.size() / 3;

            gltfShaderProgram->setAttribute("a_position", positions);
            gltfShaderProgram->setAttribute("a_normal", normals);
            gltfShaderProgram->setAttribute("a_texcoord", texcoords);
            gltfShaderProgram->setAttribute("a_faceID", faceID);

            // Set indices
            gltfShaderProgram->setIndex(mesh.Indices);

            // Set uniforms
            glm::vec3 lightDir = camera.GetForwardDirection();
            gltfShaderProgram->setUniform("u_lightDir", lightDir);

            glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
            modelMatrix = glm::translate(modelMatrix, -model.GetCenter());
            glm::mat4 viewMatrix = camera.GetViewMatrix();
            glm::mat4 projectionMatrix = camera.GetProjection();
            gltfShaderProgram->setUniform("u_model", glm::value_ptr(modelMatrix));
            gltfShaderProgram->setUniform("u_view", glm::value_ptr(viewMatrix));
            gltfShaderProgram->setUniform("u_projection", glm::value_ptr(projectionMatrix));
            gltfShaderProgram->setUniform("u_baseColorFactor", materials[mesh.MaterialIndex].BaseColorFactor);
            gltfShaderProgram->setUniform("u_baseColorTexture", 0);

            if (mesh.MaterialIndex == 0) {
                gltfShaderProgram->setUniform("u_maskColor", White);
            }
            else if (mesh.MaterialIndex == 1) {
                gltfShaderProgram->setUniform("u_maskColor", Black);
            }

            // Bind textures
            int baseColorTextureIndex = materials[mesh.MaterialIndex].BaseColorTextureIndex;
            if (baseColorTextureIndex != -1)
                images[textures[baseColorTextureIndex].ImageIndex]->Bind(0);
            else
                s_Data->WhiteTexture->Bind(0);

            gltfShaderProgram->draw();
        }
    }

} // namespace TinyRenderer
} // namespace GemCraft