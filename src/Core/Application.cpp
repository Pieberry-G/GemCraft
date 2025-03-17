#include "Core/Application.h"
#include "Core/EntryPoint.h"

#include "EventSystem/Event.h"
#include "TinyRenderer/RenderTool.h"

#include "Mesh/GeodesicTool.h"
#include "Mesh/SurfaceCleanerTool.h"
#include "Mesh/RegionSelectionTool.h"
#include "Mesh/GeometryTool.h"
#include "Mesh/PlacementTool.h"
#include "Mesh/BooleanTool.h"

#include "Tools/DatasetBuilder.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>

#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/surface/on_nurbs/fitting_surface_tdm.h>
#include <pcl/surface/on_nurbs/fitting_curve_2d_asdm.h>
#include <pcl/surface/on_nurbs/triangulation.h>

typedef pcl::PointXYZ PCLPoint;

void
PointCloud2Vector3d(pcl::PointCloud<PCLPoint>::Ptr cloud, pcl::on_nurbs::vector_vec3d& data);

void
visualizeCurve(ON_NurbsCurve& curve,
    ON_NurbsSurface& surface,
    pcl::visualization::PCLVisualizer& viewer);

void
PointCloud2Vector3d(pcl::PointCloud<PCLPoint>::Ptr cloud, pcl::on_nurbs::vector_vec3d& data)
{
    for (unsigned i = 0; i < cloud->size(); i++)
    {
        PCLPoint& p = cloud->at(i);
        if (!std::isnan(p.x) && !std::isnan(p.y) && !std::isnan(p.z))
            data.push_back(Eigen::Vector3d(p.x, p.y, p.z));
    }
}

void
visualizeCurve(ON_NurbsCurve& curve, ON_NurbsSurface& surface, pcl::visualization::PCLVisualizer& viewer)
{
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr curve_cloud(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::on_nurbs::Triangulation::convertCurve2PointCloud(curve, surface, curve_cloud, 4);
    for (std::size_t i = 0; i < curve_cloud->size() - 1; i++)
    {
        pcl::PointXYZRGB& p1 = curve_cloud->at(i);
        pcl::PointXYZRGB& p2 = curve_cloud->at(i + 1);
        std::ostringstream os;
        os << "line" << i;
        viewer.removeShape(os.str());
        viewer.addLine<pcl::PointXYZRGB>(p1, p2, 1.0, 0.0, 0.0, os.str());
    }

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr curve_cps(new pcl::PointCloud<pcl::PointXYZRGB>);
    for (int i = 0; i < curve.CVCount(); i++)
    {
        ON_3dPoint p1;
        curve.GetCV(i, p1);

        double pnt[3];
        surface.Evaluate(p1.x, p1.y, 0, 3, pnt);
        pcl::PointXYZRGB p2;
        p2.x = float(pnt[0]);
        p2.y = float(pnt[1]);
        p2.z = float(pnt[2]);

        p2.r = 255;
        p2.g = 0;
        p2.b = 0;

        curve_cps->push_back(p2);
    }
    viewer.removePointCloud("cloud_cps");
    viewer.addPointCloud(curve_cps, "cloud_cps");
}

namespace GemCraft {

    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        GC_CORE_WARN("GemCraft Application Launch!");

        GC_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        polyscope::init();
        polyscope::render::engine->setEventCallback(GC_BIND_EVENT_FN(Application::OnEvent));
        polyscope::registerGroup("Gems");
        polyscope::registerGroup("GemSettings");

        TinyRenderer::RenderTool::Init();

        m_ResourceManager = ResourceManager::Get();
        m_ResourceManager->PreloadGemSettings();
        m_ResourceManager->PreloadMandrel();
        m_ResourceManager->PreloadCylinder();

        polyscope::state::edgeLengthScale = 0.3;

        m_Scene = std::make_unique<Scene>();

        //Tools::DatasetBuilder builder;
        //builder.BuildDataset();
    }

    void Application::Run()
    {
        std::string pcd_file, file_3dm;

        pcd_file = "../assets/bunny.pcd";
        file_3dm = "out.3dm";

        // ############################################################################
        // load point cloud

        printf("  loading %s\n", pcd_file.c_str());
        pcl::PointCloud<PCLPoint>::Ptr cloud(new pcl::PointCloud<PCLPoint>);
        pcl::PCLPointCloud2 cloud2;
        pcl::on_nurbs::NurbsDataSurface data;

        if (pcl::io::loadPCDFile(pcd_file, cloud2) == -1)
            throw std::runtime_error("  PCD file not found.");

        fromPCLPointCloud2(cloud2, *cloud);
        PointCloud2Vector3d(cloud, data.interior);
        pcl::visualization::PointCloudColorHandlerCustom<PCLPoint> handler(cloud, 0, 255, 0);
        printf("  %lu points in data set\n", cloud->size());

        // ############################################################################
        // fit B-spline surface

        // parameters
        unsigned order(3);
        unsigned refinement(5);
        unsigned iterations(10);
        unsigned mesh_resolution(256);

        pcl::on_nurbs::FittingSurface::Parameter params;
        params.interior_smoothness = 0.2;
        params.interior_weight = 1.0;
        params.boundary_smoothness = 0.2;
        params.boundary_weight = 0.0;

        // initialize
        printf("  surface fitting ...\n");
        ON_NurbsSurface nurbs = pcl::on_nurbs::FittingSurface::initNurbsPCABoundingBox(order, &data);
        pcl::on_nurbs::FittingSurface fit(&data, nurbs);
        //  fit.setQuiet (false); // enable/disable debug output

        // mesh for visualization
        pcl::PolygonMesh mesh;
        pcl::PointCloud<pcl::PointXYZ>::Ptr mesh_cloud(new pcl::PointCloud<pcl::PointXYZ>);
        std::vector<pcl::Vertices> mesh_vertices;
        std::string mesh_id = "mesh_nurbs";
        pcl::on_nurbs::Triangulation::convertSurface2PolygonMesh(fit.m_nurbs, mesh, mesh_resolution);

        // surface refinement
        for (unsigned i = 0; i < refinement; i++)
        {
            fit.refine(0);
            fit.refine(1);
            fit.assemble(params);
            fit.solve();
            pcl::on_nurbs::Triangulation::convertSurface2Vertices(fit.m_nurbs, mesh_cloud, mesh_vertices, mesh_resolution);
        }

        // surface fitting with final refinement level
        for (unsigned i = 0; i < iterations; i++)
        {
            fit.assemble(params);
            fit.solve();
            pcl::on_nurbs::Triangulation::convertSurface2Vertices(fit.m_nurbs, mesh_cloud, mesh_vertices, mesh_resolution);
        }

        // ############################################################################
        // fit B-spline curve

        // parameters
        pcl::on_nurbs::FittingCurve2dAPDM::FitParameter curve_params;
        curve_params.addCPsAccuracy = 5e-2;
        curve_params.addCPsIteration = 3;
        curve_params.maxCPs = 200;
        curve_params.accuracy = 1e-3;
        curve_params.iterations = 100;

        curve_params.param.closest_point_resolution = 0;
        curve_params.param.closest_point_weight = 1.0;
        curve_params.param.closest_point_sigma2 = 0.1;
        curve_params.param.interior_sigma2 = 0.00001;
        curve_params.param.smooth_concavity = 1.0;
        curve_params.param.smoothness = 1.0;

        // initialisation (circular)
        printf("  curve fitting ...\n");
        pcl::on_nurbs::NurbsDataCurve2d curve_data;
        curve_data.interior = data.interior_param;
        curve_data.interior_weight_function.push_back(true);
        ON_NurbsCurve curve_nurbs = pcl::on_nurbs::FittingCurve2dAPDM::initNurbsCurve2D(order, curve_data.interior);

        // curve fitting
        pcl::on_nurbs::FittingCurve2dASDM curve_fit(&curve_data, curve_nurbs);
        // curve_fit.setQuiet (false); // enable/disable debug output
        curve_fit.fitting(curve_params);

        // ############################################################################
        // triangulation of trimmed surface

        printf("  triangulate trimmed surface ...\n");
        pcl::on_nurbs::Triangulation::convertTrimmedSurface2PolygonMesh(fit.m_nurbs, curve_fit.m_nurbs, mesh,
            mesh_resolution);


        // save trimmed B-spline surface
        if (fit.m_nurbs.IsValid())
        {
            ONX_Model model;
            ONX_Model_Object& surf = model.m_object_table.AppendNew();
            surf.m_object = new ON_NurbsSurface(fit.m_nurbs);
            surf.m_bDeleteObject = true;
            surf.m_attributes.m_layer_index = 1;
            surf.m_attributes.m_name = "surface";

            ONX_Model_Object& curv = model.m_object_table.AppendNew();
            curv.m_object = new ON_NurbsCurve(curve_fit.m_nurbs);
            curv.m_bDeleteObject = true;
            curv.m_attributes.m_layer_index = 2;
            curv.m_attributes.m_name = "trimming curve";

            model.Write(file_3dm.c_str());
            printf("  model saved: %s\n", file_3dm.c_str());
        }

        printf("  ... done.\n");


        std::vector<glm::vec3> vertices;
        std::vector<std::vector<size_t>> faces;
        for (const auto& point : *mesh_cloud)
        {
            vertices.push_back(glm::vec3(point.x, point.y, point.z));
        }
        for (const auto& face : mesh_vertices)
        {
            std::vector<size_t> face_indices(face.vertices.begin(), face.vertices.end());
            faces.push_back(face_indices);
        }
        std::shared_ptr<Mesh> mesh1 = std::make_shared<Mesh>("Nurbs", vertices, faces);
        m_Scene->AddMesh(mesh1);

        // Give control to the polyscope gui
        polyscope::show();
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(GC_BIND_EVENT_FN(OnWindowClose));
        dispatcher.Dispatch<KeyReleasedEvent>(GC_BIND_EVENT_FN(Application::OnKeyReleased));
        dispatcher.Dispatch<AppRenderEvent>(GC_BIND_EVENT_FN(Application::OnAppRender));
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        polyscope::popContext();
        return true;
    }

    bool Application::OnKeyReleased(KeyReleasedEvent& e)
    {
        //if (e.IsRepeat()) return false;

        if (m_MainMenu.OnKeyReleased(e)) return true;
        if (m_Scene->OnKeyReleased(e)) return true;
        return false;
    }

    bool Application::OnAppRender(AppRenderEvent& e)
    {
        if (m_Scene->OnRender(e)) return true;
        return false;
    }

} // namespace GemCraft