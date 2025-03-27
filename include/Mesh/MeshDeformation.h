#pragma once

#include "Mesh/Mesh.h"

#include <polyscope/polyscope.h>
#include <polyscope/surface_mesh.h>
#include <polyscope/point_cloud.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Surface_mesh_deformation.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel   Kernel;
typedef Kernel::Point_3										  CGALPoint;
typedef CGAL::Surface_mesh<CGALPoint>						  CGALMesh;
typedef CGAL::Surface_mesh_deformation<CGALMesh>              Surface_mesh_deformation;

typedef boost::graph_traits<CGALMesh>::vertex_descriptor      vertex_descriptor;
typedef boost::graph_traits<CGALMesh>::vertex_iterator        vertex_iterator;

namespace GemCraft {

    class MeshDeformation
    {
    public:
        MeshDeformation(std::shared_ptr<Mesh>& mesh);

        void AddToPolyscope();
        void RemoveFromPolyscope();

        void UpdateControlPoint();

        std::shared_ptr<Mesh> GetMesh() { return m_Mesh; };

    private:
        std::shared_ptr<Mesh>& m_Mesh;
        std::shared_ptr<CGALMesh> m_CGALMesh;
        std::shared_ptr<Surface_mesh_deformation> m_DeformMesh;

        std::vector<polyscope::PointCloud*> m_SurfaceControlPoints;
    };

} // namespace GemCraft