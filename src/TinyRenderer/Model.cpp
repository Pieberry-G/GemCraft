#include "TinyRenderer/Model.h"
#include "TinyRenderer/Image.h"

#include <polyscope/polyscope.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

namespace GemCraft {
namespace TinyRenderer {

    static const glm::vec3 Black(0.0f, 0.0f, 0.0f);
    static const glm::vec3 White(1.0f, 1.0f, 1.0f);

    Model::Model(const std::string& filename)
    {
        std::string extension = std::filesystem::path(filename).extension().string();
        if (extension == ".obj") {
            LoadOBJFile(filename);
        }
        else if (extension == ".gltf") {
            LoadGLTFFile(filename);
        }
        else if (extension == ".glb") {
            LoadGLBFile(filename);
        }
        else {
            GC_CORE_ASSERT(false, "Model format not supported!");
        }

        ComputeBounds();
    }

    void Model::LoadOBJFile(const std::string& filename)
    {
        std::filesystem::path basePath = std::filesystem::path(filename).parent_path();
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string error, warning;

        bool fileLoaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, filename.c_str(), basePath.string().c_str(), true);
        if (!warning.empty()) {
            GC_CORE_WARN(warning);
        }
        if (!error.empty()) {
            GC_CORE_ERROR(error);
        }
        if (fileLoaded) {
            m_Materials.resize(materials.size());
            for (uint32_t i = 0; i < materials.size(); i++) {
                if (materials[i].diffuse_texname != "") {
                    unsigned char* data;
                    int width, height, component;
                    stbi_set_flip_vertically_on_load(1);
                    std::string aa = (basePath / std::filesystem::path(materials[i].diffuse_texname)).string();
                    data = stbi_load(aa.c_str(), &width, &height, &component, 0);

                    // Get the image data from stb_image
                    unsigned char* buffer = nullptr;
                    int32_t bufferSize = 0;
                    bool deleteBuffer = false;
                    // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in OpenGL
                    if (component == 3) {
                        bufferSize = width * height * 4;
                        buffer = new unsigned char[bufferSize];
                        unsigned char* rgba = buffer;
                        unsigned char* rgb = data;
                        for (uint32_t i = 0; i < width * height; ++i) {
                            memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                            *(rgba + 3) = 255;
                            rgba += 4;
                            rgb += 3;
                        }
                        deleteBuffer = true;
                    }
                    else {
                        buffer = data;
                        bufferSize = width * height * 4;
                    }
                    // Load texture from image buffer
                    m_Images.push_back(Image::Create(width, height, buffer));
                    m_Textures.push_back(Texture{ (uint32_t)m_Images.size() - 1 });
                    m_Materials[i].BaseColorTextureIndex = (uint32_t)m_Images.size() - 1;

                    stbi_image_free(data);
                    if (deleteBuffer) {
                        delete[] buffer;
                    }
                }
                else {
                    m_Materials[i].BaseColorFactor = { materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2], 1.0f };
                }
            }

            for (auto& shape : shapes) {

                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;

                for (auto& index : shape.mesh.indices) {
                    Vertex vert{};
                    vert.Position.x = attrib.vertices[3 * index.vertex_index + 0];
                    vert.Position.y = attrib.vertices[3 * index.vertex_index + 1];
                    vert.Position.z = attrib.vertices[3 * index.vertex_index + 2];
                    if (!attrib.normals.empty()) {
                        vert.Normal.x = attrib.normals[3 * index.normal_index + 0];
                        vert.Normal.y = attrib.normals[3 * index.normal_index + 1];
                        vert.Normal.z = attrib.normals[3 * index.normal_index + 2];
                    }
                    if (!attrib.texcoords.empty()) {
                        vert.TexCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
                        vert.TexCoord.y = attrib.texcoords[2 * index.texcoord_index + 1];
                    }
                    vertices.push_back(vert);
                    indices.push_back(static_cast<size_t>(vertices.size()) - 1);
                }
                
                m_Meshes.push_back(Mesh(vertices, indices, shape.mesh.material_ids[0]));
            }
        }
        else {
            return;
        }
    }

    void Model::LoadGLTFFile(const std::string& filename)
    {
        tinygltf::Model glTFInput;
        tinygltf::TinyGLTF gltfContext;
        std::string error, warning;

        bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);
        if (!warning.empty()) {
            GC_CORE_WARN(warning);
        }
        if (!error.empty()) {
            GC_CORE_ERROR(error);
        }
        if (fileLoaded) {
            LoadGLTFImages(glTFInput);
            LoadGLTFMaterials(glTFInput);
            LoadGLTFTextures(glTFInput);
            const tinygltf::Scene& scene = glTFInput.scenes[0];
            for (uint32_t i = 0; i < scene.nodes.size(); i++) {
                const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
                LoadGLTFNode(node, glTFInput);
            }
        }
        else {
            return;
        }
    }

    void Model::LoadGLBFile(const std::string& filename)
    {
        tinygltf::Model glTFInput;
        tinygltf::TinyGLTF gltfContext;
        std::string error, warning;

        bool fileLoaded = gltfContext.LoadBinaryFromFile(&glTFInput, &error, &warning, filename);
        if (!warning.empty()) {
            GC_CORE_WARN(warning);
        }
        if (!error.empty()) {
            GC_CORE_ERROR(error);
        }
        if (fileLoaded) {
            LoadGLTFImages(glTFInput);
            LoadGLTFMaterials(glTFInput);
            LoadGLTFTextures(glTFInput);
            const tinygltf::Scene& scene = glTFInput.scenes[0];
            for (uint32_t i = 0; i < scene.nodes.size(); i++) {
                const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
                LoadGLTFNode(node, glTFInput);
            }
        }
        else {
            return;
        }
    }

    void Model::LoadGLTFImages(tinygltf::Model& input)
    {
        // Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
        // loading them from disk, we fetch them from the glTF loader and upload the buffers
        m_Images.resize(input.images.size());
        for (uint32_t i = 0; i < input.images.size(); i++) {
            tinygltf::Image& glTFImage = input.images[i];
            // Get the image data from the glTF loader
            unsigned char* buffer = nullptr;
            int32_t bufferSize = 0;
            bool deleteBuffer = false;
            // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in OpenGL
            if (glTFImage.component == 3) {
                bufferSize = glTFImage.width * glTFImage.height * 4;
                buffer = new unsigned char[bufferSize];
                unsigned char* rgba = buffer;
                unsigned char* rgb = &glTFImage.image[0];
                for (uint32_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
                    memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                    *(rgba + 3) = 255;
                    rgba += 4;
                    rgb += 3;
                }
                deleteBuffer = true;
            }
            else {
                buffer = &glTFImage.image[0];
                bufferSize = glTFImage.image.size();
            }
            // Load texture from image buffer
            m_Images[i] = Image::Create(glTFImage.width, glTFImage.height, buffer);
            if (deleteBuffer) {
                delete[] buffer;
            }
        }
    }

    void Model::LoadGLTFTextures(tinygltf::Model& input)
    {
        m_Textures.resize(input.textures.size());
        for (uint32_t i = 0; i < input.textures.size(); i++) {
            m_Textures[i].ImageIndex = input.textures[i].source;
        }
    }

    void Model::LoadGLTFMaterials(tinygltf::Model& input)
    {
        m_Materials.resize(input.materials.size());
        for (uint32_t i = 0; i < input.materials.size(); i++) {
            // We only read the most basic properties required for our sample
            tinygltf::Material glTFMaterial = input.materials[i];
            // Get the base color factor
            if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
                m_Materials[i].BaseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
            }
            // Get base color texture index
            if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
                m_Materials[i].BaseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
            }
        }
    }

    void Model::LoadGLTFNode(const tinygltf::Node& inputNode, const tinygltf::Model& input)
    {
        //VulkanglTFModel::Node* node = new VulkanglTFModel::Node{};
        //node->matrix = glm::mat4(1.0f);
        //node->parent = parent;

        //// Get the local node matrix
        //// It's either made up from translation, rotation, scale or a 4x4 matrix
        //if (inputNode.translation.size() == 3) {
        //    node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
        //}
        //if (inputNode.rotation.size() == 4) {
        //    glm::quat q = glm::make_quat(inputNode.rotation.data());
        //    node->matrix *= glm::mat4(q);
        //}
        //if (inputNode.scale.size() == 3) {
        //    node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
        //}
        //if (inputNode.matrix.size() == 16) {
        //    node->matrix = glm::make_mat4x4(inputNode.matrix.data());
        //};

        // Load node's children
        if (inputNode.children.size() > 0) {
            for (uint32_t i = 0; i < inputNode.children.size(); i++) {
                LoadGLTFNode(input.nodes[inputNode.children[i]], input);
            }
        }

        // If the node contains mesh data, we load vertices and indices from the buffers
        // In glTF this is done via accessors and buffer views
        if (inputNode.mesh > -1) {
            const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
            // Iterate through all primitives of this node's mesh
            for (uint32_t i = 0; i < mesh.primitives.size(); i++) {

                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;

                const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
                uint32_t indexCount = 0;
                // Vertices
                {
                    const float* positionBuffer = nullptr;
                    const float* normalsBuffer = nullptr;
                    const float* texCoordsBuffer = nullptr;
                    uint32_t vertexCount = 0;

                    // Get buffer data for vertex positions
                    if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                        const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                        positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                        vertexCount = accessor.count;
                    }
                    // Get buffer data for vertex normals
                    if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                        normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }
                    // Get buffer data for vertex texture coordinates
                    // glTF supports multiple sets, we only load the first one
                    if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                        texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }

                    // Append data to model's vertex buffer
                    for (uint32_t v = 0; v < vertexCount; v++) {
                        Vertex vert{};
                        vert.Position = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                        vert.Normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                        vert.TexCoord = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                        vertices.push_back(vert);
                    }
                }
                // Indices
                {
                    const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
                    const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

                    indexCount += static_cast<uint32_t>(accessor.count);

                    // glTF supports different component types of indices
                    switch (accessor.componentType) {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (uint32_t index = 0; index < accessor.count; index++) {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (uint32_t index = 0; index < accessor.count; index++) {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (uint32_t index = 0; index < accessor.count; index++) {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                        return;
                    }
                }
                m_Meshes.push_back(Mesh(vertices, indices, glTFPrimitive.material));
            }
        }
    }

    void Model::ComputeBounds()
    {
        constexpr float MAX_FLOAT = std::numeric_limits<float>::max();
        m_MinBounds = { MAX_FLOAT, MAX_FLOAT, MAX_FLOAT };
        m_MaxBounds = { -MAX_FLOAT, -MAX_FLOAT, -MAX_FLOAT };

        for (auto& mesh : m_Meshes) {
            for (auto& vertex : mesh.Vertices) {
                m_MinBounds[0] = std::min(vertex.Position[0], m_MinBounds[0]);
                m_MinBounds[1] = std::min(vertex.Position[1], m_MinBounds[1]);
                m_MinBounds[2] = std::min(vertex.Position[2], m_MinBounds[2]);

                m_MaxBounds[0] = std::max(vertex.Position[0], m_MaxBounds[0]);
                m_MaxBounds[1] = std::max(vertex.Position[1], m_MaxBounds[1]);
                m_MaxBounds[2] = std::max(vertex.Position[2], m_MaxBounds[2]);
            }
        }
    }

} // namespace TinyRenderer
} // namespace GemCraft