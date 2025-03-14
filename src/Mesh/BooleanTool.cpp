#include "Mesh/BooleanTool.h"

#include "Mesh/FormatTool.h"
#include "Core/Scene.h"
#include "Core/ResourceManager.h"

#include "Mesh/GeodesicTool.h"

namespace GemCraft {

	std::shared_ptr<Mesh> BooleanTool::DifferenceOperation(std::shared_ptr<Mesh>& ring, std::shared_ptr<Mesh>& meshToSubtract)
	{
		glm::mat4 transform = ring->GetPsTransform();
		glm::mat4 inverseTransform = glm::inverse(transform);

		std::shared_ptr<CGALMesh> mesh1 = FormatTool::MeshToCGALMesh(ring, ring->GetPsTransform());
		Exact_point_map mesh1ExactPoints = mesh1->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		Exact_vertex_point_map mesh1Vpm(mesh1ExactPoints, *mesh1);

		std::shared_ptr<CGALMesh> mesh2 = FormatTool::MeshToCGALMesh(meshToSubtract, meshToSubtract->GetPsTransform());
		Exact_point_map mesh2ExactPoints = mesh2->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		Exact_vertex_point_map mesh2Vpm(mesh2ExactPoints, *mesh2);

		bool success = CGALpmp::corefine_and_compute_difference(*mesh1, *mesh2, *mesh1,
			CGALparams::vertex_point_map(mesh1Vpm),
			CGALparams::vertex_point_map(mesh2Vpm),
			CGALparams::vertex_point_map(mesh1Vpm));
		mesh1->collect_garbage();

		if (success) {
			GC_CORE_INFO("Difference was successfully computed.");
		}
		else {
			GC_CORE_INFO("Difference crashed.");
		}

		return FormatTool::CGALMeshToMesh(mesh1, inverseTransform);
	}

	std::shared_ptr<Mesh> BooleanTool::DifferenceOperation(std::shared_ptr<Mesh>& ring, GemLine& gemLine)
	{
		GemSettingType settingType = gemLine.GetGemSettingType();
		const std::vector<std::shared_ptr<Mesh>>& gems = gemLine.GetGems();
		const std::vector<std::shared_ptr<Mesh>>& gemSettings = gemLine.GetGemSettings();
		if (gems.empty() && gemSettings.empty()) return ring;
		if (gemLine.GetBooleanOpLock()) return ring;

		gemLine.SetBooleanOpLock(true);
		glm::mat4 transform = ring->GetPsTransform();
		glm::mat4 inverseTransform = glm::inverse(transform);

		std::shared_ptr<CGALMesh> mesh1 = FormatTool::MeshToCGALMesh(ring, ring->GetPsTransform());
		Exact_point_map mesh1ExactPoints = mesh1->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		Exact_vertex_point_map mesh1Vpm(mesh1ExactPoints, *mesh1);

		std::shared_ptr<CGALMesh> mesh2 = std::make_shared<CGALMesh>();
		Exact_point_map mesh2ExactPoints = mesh2->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		Exact_vertex_point_map mesh2Vpm(mesh2ExactPoints, *mesh2);

		// Gems
		for (size_t i = 0; i < gems.size(); i++) {
			std::shared_ptr<CGALMesh> mesh3 = FormatTool::MeshToCGALMesh(gems[i], gems[i]->GetPsTransform());

			Exact_point_map mesh3ExactPoints = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
			Exact_vertex_point_map mesh3Vpm(mesh3ExactPoints, *mesh3);

			CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
												CGALparams::vertex_point_map(mesh2Vpm),
												CGALparams::vertex_point_map(mesh3Vpm),
												CGALparams::vertex_point_map(mesh2Vpm));
		}

		//// Mandrels
		//for (size_t i = 0; i < gems.size(); i++) {
		//	std::shared_ptr<Mesh> mandrel = ResourceManager::Get()->CreateMandrel();
		//	std::shared_ptr<CGALMesh> mesh3 = FormatTool::MeshToCGALMesh(mandrel, gems[i]->GetPsTransform());

		//	Exact_point_map mesh3ExactPoints = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		//	Exact_vertex_point_map mesh3Vpm(mesh3ExactPoints, *mesh3);

		//	CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
		//										CGALparams::vertex_point_map(mesh2Vpm),
		//										CGALparams::vertex_point_map(mesh3Vpm),
		//										CGALparams::vertex_point_map(mesh2Vpm));
		//}

		// GemSettings
		for (size_t i = 0; i < gemSettings.size(); i++) {
			std::shared_ptr<CGALMesh> mesh3 = FormatTool::MeshToCGALMesh(gemSettings[i], gemSettings[i]->GetPsTransform());
			mesh3 = ConstructContexHull(mesh3);

			Exact_point_map mesh3ExactPoints = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
			Exact_vertex_point_map mesh3Vpm(mesh3ExactPoints, *mesh3);

			CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
												CGALparams::vertex_point_map(mesh2Vpm),
												CGALparams::vertex_point_map(mesh3Vpm),
												CGALparams::vertex_point_map(mesh2Vpm));
		}

		bool success = CGALpmp::corefine_and_compute_difference(*mesh1, *mesh2, *mesh1,
																		CGALparams::vertex_point_map(mesh1Vpm),
																		CGALparams::vertex_point_map(mesh2Vpm),
																		CGALparams::vertex_point_map(mesh1Vpm));
		mesh1->collect_garbage();

		if (success) {
			GC_CORE_INFO("Difference was successfully computed.");
		}
		else {
			GC_CORE_INFO("Difference crashed.");
		}

		return FormatTool::CGALMeshToMesh(mesh1, inverseTransform);
	}

	std::shared_ptr<Mesh> BooleanTool::DifferenceOperation(std::shared_ptr<Mesh>& ring, GemGroup& gemGroup)
	{
		GemSettingType settingType = gemGroup.GetGemSettingType();
		const std::vector<std::shared_ptr<Mesh>>& gems = gemGroup.GetGems();
		const std::vector<std::shared_ptr<Mesh>>& gemSettings = gemGroup.GetGemSettings();
		if (gems.empty() && gemSettings.empty()) return ring;
		if (gemGroup.GetBooleanOpLock()) return ring;

		gemGroup.SetBooleanOpLock(true);
		glm::mat4 transform = ring->GetPsTransform();
		glm::mat4 inverseTransform = glm::inverse(transform);

		std::shared_ptr<CGALMesh> mesh1 = FormatTool::MeshToCGALMesh(ring, ring->GetPsTransform());
		Exact_point_map mesh1ExactPoints = mesh1->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		Exact_vertex_point_map mesh1Vpm(mesh1ExactPoints, *mesh1);

		std::shared_ptr<CGALMesh> mesh2 = std::make_shared<CGALMesh>();
		Exact_point_map mesh2ExactPoints = mesh2->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		Exact_vertex_point_map mesh2Vpm(mesh2ExactPoints, *mesh2);

		// Gems
		for (size_t i = 0; i < gems.size(); i++) {
			std::shared_ptr<CGALMesh> mesh3 = FormatTool::MeshToCGALMesh(gems[i], gems[i]->GetPsTransform());

			Exact_point_map mesh3ExactPoints = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
			Exact_vertex_point_map mesh3Vpm(mesh3ExactPoints, *mesh3);

			CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
				CGALparams::vertex_point_map(mesh2Vpm),
				CGALparams::vertex_point_map(mesh3Vpm),
				CGALparams::vertex_point_map(mesh2Vpm));
		}

		// Cylinder
		for (size_t i = 0; i < gems.size(); i++) {
			std::shared_ptr<Mesh> cylinder = ResourceManager::Get()->CreateCylinder();
			std::shared_ptr<CGALMesh> mesh3 = FormatTool::MeshToCGALMesh(cylinder, gems[i]->GetPsTransform());

			Exact_point_map mesh3ExactPoints = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
			Exact_vertex_point_map mesh3Vpm(mesh3ExactPoints, *mesh3);

			CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
				CGALparams::vertex_point_map(mesh2Vpm),
				CGALparams::vertex_point_map(mesh3Vpm),
				CGALparams::vertex_point_map(mesh2Vpm));
		}

		//// Mandrels
		//for (size_t i = 0; i < gems.size(); i++) {
		//	std::shared_ptr<Mesh> mandrel = ResourceManager::Get()->CreateMandrel();
		//	std::shared_ptr<CGALMesh> mesh3 = FormatTool::MeshToCGALMesh(mandrel, gems[i]->GetPsTransform());

		//	Exact_point_map mesh3ExactPoints = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		//	Exact_vertex_point_map mesh3Vpm(mesh3ExactPoints, *mesh3);

		//	CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
		//		CGALparams::vertex_point_map(mesh2Vpm),
		//		CGALparams::vertex_point_map(mesh3Vpm),
		//		CGALparams::vertex_point_map(mesh2Vpm));
		//}

		// GemSettings
		for (size_t i = 0; i < gemSettings.size(); i++) {
			std::shared_ptr<CGALMesh> mesh3 = FormatTool::MeshToCGALMesh(gemSettings[i], gemSettings[i]->GetPsTransform());
			mesh3 = ConstructContexHull(mesh3);

			Exact_point_map mesh3ExactPoints = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
			Exact_vertex_point_map mesh3Vpm(mesh3ExactPoints, *mesh3);

			CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
				CGALparams::vertex_point_map(mesh2Vpm),
				CGALparams::vertex_point_map(mesh3Vpm),
				CGALparams::vertex_point_map(mesh2Vpm));
		}

		bool success = CGALpmp::corefine_and_compute_difference(*mesh1, *mesh2, *mesh1,
			CGALparams::vertex_point_map(mesh1Vpm),
			CGALparams::vertex_point_map(mesh2Vpm),
			CGALparams::vertex_point_map(mesh1Vpm));
		mesh1->collect_garbage();

		if (success) {
			GC_CORE_INFO("Difference was successfully computed.");
		}
		else {
			GC_CORE_INFO("Difference crashed.");
		}

		return FormatTool::CGALMeshToMesh(mesh1, inverseTransform);
	}

	struct HalfedgeToEdge
	{
		HalfedgeToEdge(const CGALMesh& cgalmesh, std::vector<edge_descriptor>& edges)
			: m_CGALmesh(cgalmesh), m_Edges(edges) {}

		void operator()(const halfedge_descriptor& h) const
		{
			m_Edges.push_back(edge(h, m_CGALmesh));
		}

		const CGALMesh& m_CGALmesh;
		std::vector<edge_descriptor>& m_Edges;
	};

	std::shared_ptr<CGALMesh> BooleanTool::ConstructContexHull(std::shared_ptr<CGALMesh> mesh)
	{
		std::shared_ptr<CGALMesh> contexHull = std::make_shared<CGALMesh>();
		CGAL::convex_hull_3(mesh->points().begin(), mesh->points().end(), *contexHull);

		//auto vnormals = contexHull->add_property_map<vertex_descriptor, CGALVector>("v:normal", CGAL::NULL_VECTOR).first;
		//CGALpmp::compute_vertex_normals(*contexHull, vnormals);

		//for (auto& vertex : contexHull->vertices())
		//{
		//	CGALPoint& point = contexHull->point(vertex);
		//	CGALVector normal = vnormals[vertex];
		//	point = point + normal * 1.0f;
		//}

		//double targetEdgeLength = 0.1;
		//std::vector<edge_descriptor> border;
		//CGALpmp::border_halfedges(faces(*contexHull), *contexHull, boost::make_function_output_iterator(HalfedgeToEdge(*contexHull, border)));
		//CGALpmp::split_long_edges(border, targetEdgeLength, *contexHull);
		//CGALpmp::isotropic_remeshing(faces(*contexHull), targetEdgeLength, *contexHull,
		//	CGAL::parameters::number_of_iterations(10)
		//	.protect_constraints(true));
		//contexHull->collect_garbage();

		//std::ofstream out("contexHull.obj");
		//CGAL::IO::write_OBJ(out, *contexHull);
		//out.close();

		return contexHull;
	}

} // namespace GemCraft