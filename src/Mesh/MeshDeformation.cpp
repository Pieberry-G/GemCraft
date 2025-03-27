#include "Mesh/MeshDeformation.h"

#include "Mesh/FormatTool.h"

#include "Core/State.h"

namespace GemCraft {

    MeshDeformation::MeshDeformation(std::shared_ptr<Mesh>& mesh)
        : m_Mesh(mesh)
    {
        m_CGALMesh = FormatTool::MeshToCGALMesh(mesh, mesh->GetPsTransform());

        // Create a deformation object
        m_DeformMesh = std::make_shared<Surface_mesh_deformation>(*m_CGALMesh);

        // Definition of the region of interest (use the whole mesh)
        vertex_iterator vb, ve;
        boost::tie(vb, ve) = vertices(m_DeformMesh->triangle_mesh());
        m_DeformMesh->insert_roi_vertices(vb, ve);
    }

    void MeshDeformation::AddToPolyscope()
    {
        vertex_iterator vb, ve;
        boost::tie(vb, ve) = vertices(m_DeformMesh->triangle_mesh());

        polyscope::registerGroup("_Deformation");

        // surface control points
        m_SurfaceControlPoints.resize(20);
        for (int i = 0; i < 20; i++) {
            std::string name = "Deformation Control Point (" + std::to_string(i) + ")";
            std::vector<glm::vec3> singlePointCloud = { { 0.0f, 0.0f, 0.0f } };
            polyscope::PointCloud* pointCloud = polyscope::registerPointCloud(name, singlePointCloud);
            glm::vec3 controlPoint = m_Mesh->GetVertices()[i];
            pointCloud->setTransform(glm::translate(glm::mat4(1.0f), controlPoint));
            pointCloud->setPointColor({ 0.0f, 0.0f, 0.0f });
            pointCloud->setPointRadius(0.01f);
            polyscope::setParentGroupOfStructure(pointCloud, "_Deformation");
            m_SurfaceControlPoints[i] = pointCloud;
            State::controlPointToDeformation[pointCloud] = this;

            vertex_descriptor controlVertex = *std::next(vb, i);
            m_DeformMesh->insert_control_vertex(controlVertex);
        }
    }

    void MeshDeformation::RemoveFromPolyscope()
    {
        //// Select two control vertices ...
        //vertex_descriptor control_1 = *std::next(vb, 213);
        //vertex_descriptor control_2 = *std::next(vb, 157);

        //// ... and insert them
        //m_DeformMesh->insert_control_vertex(control_1);
        //m_DeformMesh->insert_control_vertex(control_2);

        //// The definition of the ROI and the control vertices is done, call preprocess
        //bool is_matrix_factorization_OK = m_DeformMesh->preprocess();
        //if (!is_matrix_factorization_OK) {
        //    std::cerr << "Error in preprocessing, check documentation of preprocess()" << std::endl;
        //}
        //std::cout << "dddonedd" << std::endl;
    }

    void MeshDeformation::UpdateControlPoint()
    {
        vertex_iterator vb, ve;
        boost::tie(vb, ve) = vertices(m_DeformMesh->triangle_mesh());
        std::cout << "1";

        // surface control points
        for (int i = 0; i < 20; i++) {
            glm::mat4 transform = m_SurfaceControlPoints[i]->getTransform();
            glm::vec3 controlPoint = glm::vec3(transform[3]);

            vertex_descriptor controlVertex = *std::next(vb, i);
            std::cout << controlPoint.x << " " << controlPoint.y << " " << controlPoint.z << std::endl;
            Surface_mesh_deformation::Point constrained_pos_1(controlPoint.x, controlPoint.y, controlPoint.z);
            std::cout << "locked";
            m_DeformMesh->set_target_position(controlVertex, constrained_pos_1);
            std::cout << "unlock"<< std::endl;
        }
        std::cout << "2";
        // The prepocessing step is again needed
        if (!m_DeformMesh->preprocess()) {
            std::cerr << "Error in preprocessing, check documentation of preprocess()" << std::endl;
        }
        else {
            std::cout << "preprocess!" << std::endl;
        }
        std::cout << "3";

        m_DeformMesh->deform(15, 0.0);
        std::ofstream output("deform_3.off");
        output << m_DeformMesh->triangle_mesh();
        std::cout << "4";
    }

} // namespace GemCraft