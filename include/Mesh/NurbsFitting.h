#pragma once

#include "Mesh/Mesh.h"

#include <polyscope/polyscope.h>
#include <polyscope/surface_mesh.h>
#include <polyscope/point_cloud.h>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>

#include <pcl/surface/on_nurbs/fitting_surface_tdm.h>
#include <pcl/surface/on_nurbs/fitting_curve_2d_asdm.h>
#include <pcl/surface/on_nurbs/triangulation.h>

typedef pcl::PointXYZ PCLPoint;

namespace GemCraft {

    class NurbsFitting
    {
    public:
        NurbsFitting(const std::string& name);
        NurbsFitting(const std::string& name, const std::vector<glm::vec3>& points);
        NurbsFitting(const std::string& name, std::shared_ptr<Mesh>& mesh);

        void LoadPcdFile(const std::string& filepath);
        void FittingSurface(uint32_t order = 3, uint32_t refinement = 2, uint32_t iterations = 10);

        void AddToPolyscope();
        void RemoveFromPolyscope();

        void UpdateControlPoint();

        std::shared_ptr<Mesh> GetMesh() { return m_Mesh; };

    private:
        std::string m_Name;
        std::shared_ptr<Mesh> m_TargetMesh;

        std::shared_ptr<pcl::on_nurbs::NurbsDataSurface> m_SurfaceData;
        std::shared_ptr<pcl::on_nurbs::NurbsDataCurve2d> m_CurveData;
        std::shared_ptr<pcl::on_nurbs::FittingSurface> m_SurfaceFit;
        std::shared_ptr<pcl::on_nurbs::FittingCurve2dASDM> m_CurveFit;

        std::shared_ptr<Mesh> m_Mesh;
        std::vector<polyscope::PointCloud*> m_SurfaceControlPoints;
        std::vector<polyscope::PointCloud*> m_CurveControlPoints;
    };

} // namespace GemCraft