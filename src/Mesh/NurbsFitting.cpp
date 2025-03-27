#include "Mesh/NurbsFitting.h"

#include "Core/State.h"

#include <pcl/io/io.h>
#include <pcl/io/obj_io.h>

namespace GemCraft {

    static void PointCloud2Vector3d(pcl::PointCloud<PCLPoint>::Ptr cloud, pcl::on_nurbs::vector_vec3d& data);

    NurbsFitting::NurbsFitting(const std::string& name)
        : m_Name(name)
    {
        m_SurfaceData = std::make_shared<pcl::on_nurbs::NurbsDataSurface>();
        std::string pcdFile = "../assets/bunny.pcd";
        LoadPcdFile(pcdFile);
        FittingSurface(3, 1, 10);
    }

    NurbsFitting::NurbsFitting(const std::string& name, const std::vector<glm::vec3>& points)
        : m_Name(name)
    {
        m_SurfaceData = std::make_shared<pcl::on_nurbs::NurbsDataSurface>();
        for (auto& point : points) {
            m_SurfaceData->interior.push_back(Eigen::Vector3d(point.x, point.y, point.z));
        }
        FittingSurface(3, 1, 10);
    }

    NurbsFitting::NurbsFitting(const std::string& name, std::shared_ptr<Mesh>& targetMesh)
        : m_Name(name), m_TargetMesh(targetMesh)
    {
        m_SurfaceData = std::make_shared<pcl::on_nurbs::NurbsDataSurface>();
        const std::vector<glm::vec3>& points = targetMesh->GetVertices();
        for (auto& point : points) {
            m_SurfaceData->interior.push_back(Eigen::Vector3d(point.x, point.y, point.z));
        }
        FittingSurface(3, 1, 10);
    }

    void NurbsFitting::LoadPcdFile(const std::string& filepath)
    {
        // load point cloud
        GC_CORE_TRACE("loading {0}", filepath.c_str());
        pcl::PointCloud<PCLPoint>::Ptr cloud(new pcl::PointCloud<PCLPoint>);
        pcl::PCLPointCloud2 cloud2;
        if (pcl::io::loadPCDFile(filepath, cloud2) == -1) {
            GC_CORE_ASSERT(false, "PCD file not found.");
        }
        fromPCLPointCloud2(cloud2, *cloud);
        PointCloud2Vector3d(cloud, m_SurfaceData->interior);
        GC_CORE_TRACE("{0} points in data set", cloud->size());
    }

    void NurbsFitting::FittingSurface(uint32_t order, uint32_t refinement, uint32_t iterations)
    {
        // fit B-spline surface
        pcl::on_nurbs::FittingSurface::Parameter params;
        params.interior_smoothness = 0.2;
        params.interior_weight = 1.0;
        params.boundary_smoothness = 0.2;
        params.boundary_weight = 0.0;
        // initialize
        printf("  surface fitting ...\n");
        ON_NurbsSurface nurbs = pcl::on_nurbs::FittingSurface::initNurbsPCABoundingBox(order, m_SurfaceData.get());
        m_SurfaceFit = std::make_shared<pcl::on_nurbs::FittingSurface>(m_SurfaceData.get(), nurbs);
        // surface refinement
        for (unsigned i = 0; i < refinement; i++) {
            m_SurfaceFit->refine(0);
            m_SurfaceFit->refine(1);
            m_SurfaceFit->assemble(params);
            m_SurfaceFit->solve();
        }
        // surface fitting with final refinement level
        for (unsigned i = 0; i < iterations; i++) {
            m_SurfaceFit->assemble(params);
            m_SurfaceFit->solve();
        }

        // fit B-spline curve
        // parameters
        pcl::on_nurbs::FittingCurve2dAPDM::FitParameter curveParams;
        curveParams.addCPsAccuracy = 5e-2;
        curveParams.addCPsIteration = 3;
        curveParams.maxCPs = 200;
        curveParams.accuracy = 1e-3;
        curveParams.iterations = 100;
        curveParams.param.closest_point_resolution = 0;
        curveParams.param.closest_point_weight = 1.0;
        curveParams.param.closest_point_sigma2 = 0.1;
        curveParams.param.interior_sigma2 = 0.00001;
        curveParams.param.smooth_concavity = 1.0;
        curveParams.param.smoothness = 1.0;
        // initialisation (circular)
        printf("  curve fitting ...\n");
        m_CurveData = std::make_shared<pcl::on_nurbs::NurbsDataCurve2d>();
        m_CurveData->interior = m_SurfaceData->interior_param;
        m_CurveData->interior_weight_function.push_back(true);
        ON_NurbsCurve curve_nurbs = pcl::on_nurbs::FittingCurve2dAPDM::initNurbsCurve2D(order, m_CurveData->interior);
        // curve fitting
        m_CurveFit = std::make_shared<pcl::on_nurbs::FittingCurve2dASDM>(m_CurveData.get(), curve_nurbs);
        m_CurveFit->fitting(curveParams);

        //// triangulation of trimmed surface
        //printf("  triangulate trimmed surface ...\n");
        //pcl::on_nurbs::Triangulation::convertTrimmedSurface2PolygonMesh(m_SurfaceFit->m_nurbs, m_CurveFit->m_nurbs, mesh,
        //    mesh_resolution);

        printf("  ... done.\n");
    }

    void NurbsFitting::AddToPolyscope()
    {
        if (polyscope::state::groups.find(m_Name + "_Nurf Surface") != polyscope::state::groups.end()) {
            GC_CORE_ASSERT(false, "Mesh has already been registered to polyscope!");
        }
        polyscope::registerGroup(m_Name + "_Nurf Surface");

        // mesh
        pcl::PolygonMesh mesh;
        pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
        std::vector<pcl::Vertices> meshVertices;
        unsigned meshResolution(256);
        pcl::on_nurbs::Triangulation::convertSurface2Vertices(m_SurfaceFit->m_nurbs, meshCloud, meshVertices, meshResolution);
        // triangulation of trimmed surface
        printf("  triangulate trimmed surface ...\n");
        pcl::on_nurbs::Triangulation::convertTrimmedSurface2PolygonMesh(m_SurfaceFit->m_nurbs, m_CurveFit->m_nurbs, mesh, meshResolution);
        pcl::fromPCLPointCloud2(mesh.cloud, *meshCloud);
        meshVertices = mesh.polygons;
        pcl::io::saveOBJFile("output.obj", mesh);

        std::vector<glm::vec3> vertices;
        std::vector<std::vector<size_t>> faces;
        for (const auto& point : *meshCloud) {
            vertices.push_back(glm::vec3(point.x, point.y, point.z));
        }
        for (const auto& face : meshVertices) {
            std::vector<size_t> face_indices(face.vertices.begin(), face.vertices.end());
            faces.push_back(face_indices);
        }
        m_Mesh = std::make_shared<Mesh>(m_Name, vertices, faces);
        m_Mesh->AddToPolyscope();
        polyscope::setParentGroupOfStructure(m_Mesh->GetPsMesh(), m_Name + "_Nurf Surface");

        // surface control points
        int cvCountU = m_SurfaceFit->m_nurbs.CVCount(0);
        int cvCountV = m_SurfaceFit->m_nurbs.CVCount(1);
        m_SurfaceControlPoints.resize(cvCountU * cvCountV);
        for (int i = 0; i < cvCountU; i++) {
            for (int j = 0; j < cvCountV; j++) {
                std::string name = m_Name + "_Surface Control Point (" + std::to_string(i) + ", " + std::to_string(j) + ")";
                std::vector<glm::vec3> singlePointCloud = { { 0.0f, 0.0f, 0.0f } };
                polyscope::PointCloud* pointCloud = polyscope::registerPointCloud(name, singlePointCloud);
                ON_3dPoint cv;
                m_SurfaceFit->m_nurbs.GetCV(i, j, cv);
                glm::vec3 controlPoint = { cv.x, cv.y, cv.z };
                pointCloud->setTransform(glm::translate(glm::mat4(1.0f), controlPoint));
                pointCloud->setPointColor({ 0.0f, 0.0f, 0.0f });
                pointCloud->setPointRadius(0.01f);
                polyscope::setParentGroupOfStructure(pointCloud, m_Name + "_Nurf Surface");
                m_SurfaceControlPoints[i * cvCountV + j] = pointCloud;
                State::controlPointToNurbs[pointCloud] = this;
            }
        }

        //// curve control points
        //int cvCount = m_CurveFit->m_nurbs.CVCount();
        //m_CurveControlPoints.resize(cvCount);
        //for (int i = 0; i < cvCount; i++) {
        //    std::string name = m_Name + "_Curve Control Point (" + std::to_string(i) + ")";
        //    std::vector<glm::vec3> singlePointCloud = { { 0.0f, 0.0f, 0.0f } };
        //    polyscope::PointCloud* pointCloud = polyscope::registerPointCloud(name, singlePointCloud);
        //    ON_3dPoint cv;
        //    m_CurveFit->m_nurbs.GetCV(i, cv);
        //    glm::vec3 controlPoint = { cv.x, cv.y, cv.z };
        //    pointCloud->setTransform(glm::translate(glm::mat4(1.0f), controlPoint));
        //    pointCloud->setPointColor({ 1.0f, 0.0f, 0.0f });
        //    pointCloud->setPointRadius(0.01f);
        //    polyscope::setParentGroupOfStructure(pointCloud, m_Name + "_Nurf Surface");
        //    m_CurveControlPoints[i] = pointCloud;
        //    State::controlPointToNurbs[pointCloud] = this;
        //}
    }

    void NurbsFitting::RemoveFromPolyscope()
    {
        if (polyscope::state::groups.find(m_Name + "_Nurf Surface") != polyscope::state::groups.end()) {
            GC_CORE_ASSERT(false, "Mesh has already been registered to polyscope!");
        }

        m_Mesh->RemoveFromPolyscope();

        int cvCountU = m_SurfaceFit->m_nurbs.CVCount(0);
        int cvCountV = m_SurfaceFit->m_nurbs.CVCount(1);
        m_SurfaceControlPoints.resize(cvCountU * cvCountV);
        for (int i = 0; i < cvCountU; i++) {
            for (int j = 0; j < cvCountV; j++) {
                polyscope::PointCloud* pointCloud = m_SurfaceControlPoints[i * cvCountV + j];
                pointCloud->remove();
                State::controlPointToNurbs.erase(pointCloud);
            }
        }
        m_SurfaceControlPoints.clear();

        polyscope::removeGroup(m_Name + "_Nurf Surface");
    }

    void NurbsFitting::UpdateControlPoint()
    {
        int cvCountU = m_SurfaceFit->m_nurbs.CVCount(0);
        int cvCountV = m_SurfaceFit->m_nurbs.CVCount(1);
        for (int i = 0; i < cvCountU; i++) {
            for (int j = 0; j < cvCountV; j++) {
                glm::mat4 transform = m_SurfaceControlPoints[i * cvCountV + j]->getTransform();
                glm::vec3 controlPoint = glm::vec3(transform[3]);
                ON_3dPoint cv = { controlPoint.x, controlPoint.y, controlPoint.z };
                m_SurfaceFit->m_nurbs.SetCV(i, j, cv);
            }
        }

        pcl::PolygonMesh mesh;
        pcl::PointCloud<pcl::PointXYZ>::Ptr meshCloud(new pcl::PointCloud<pcl::PointXYZ>);
        std::vector<pcl::Vertices> meshVertices;
        unsigned meshResolution(256);
        pcl::on_nurbs::Triangulation::convertSurface2Vertices(m_SurfaceFit->m_nurbs, meshCloud, meshVertices, meshResolution);
        printf("  triangulate trimmed surface ...\n");
        pcl::on_nurbs::Triangulation::convertTrimmedSurface2PolygonMesh(m_SurfaceFit->m_nurbs, m_CurveFit->m_nurbs, mesh,
            meshResolution);
        pcl::fromPCLPointCloud2(mesh.cloud, *meshCloud);
        meshVertices = mesh.polygons;
        pcl::io::saveOBJFile("output2.obj", mesh);

        std::vector<glm::vec3> vertices;
        std::vector<std::vector<size_t>> faces;
        for (const auto& point : *meshCloud) {
            vertices.push_back(glm::vec3(point.x, point.y, point.z));
        }
        for (const auto& face : meshVertices) {
            std::vector<size_t> face_indices(face.vertices.begin(), face.vertices.end());
            faces.push_back(face_indices);
        }
        m_Mesh->RemoveFromPolyscope();
        m_Mesh = std::make_shared<Mesh>(m_Name, vertices, faces);
        m_Mesh->AddToPolyscope();
        polyscope::setParentGroupOfStructure(m_Mesh->GetPsMesh(), m_Name + "_Nurf Surface");
    }

    static void PointCloud2Vector3d(pcl::PointCloud<PCLPoint>::Ptr cloud, pcl::on_nurbs::vector_vec3d& data)
    {
        for (unsigned i = 0; i < cloud->size(); i++) {
            PCLPoint& p = cloud->at(i);
            if (!std::isnan(p.x) && !std::isnan(p.y) && !std::isnan(p.z)) {
                data.push_back(Eigen::Vector3d(p.x, p.y, p.z));
            }
        }
    }


} // namespace GemCraft