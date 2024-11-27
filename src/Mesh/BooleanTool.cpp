#include "Mesh/BooleanTool.h"

#include "Mesh/Utils.h"
#include "Core/Scene.h"
#include "Core/ResourceManager.h"

namespace GemCraft {

	std::shared_ptr<Mesh> BooleanTool::DifferenceOperation(std::shared_ptr<Mesh> ring, GemLine& gemLine)
	{
		GemSettingType settingType = gemLine.GetGemSettingType();
		const std::vector<std::shared_ptr<Mesh>>& gems = gemLine.GetGems();
		const std::vector<std::shared_ptr<Mesh>>& gemSettings = gemLine.GetGemSettings();
		if (gems.empty() && gemSettings.empty()) return ring;
		if (gemLine.GetBooleanOpLock()) return ring;

		gemLine.SetBooleanOpLock(true);
		glm::mat4 transform = ring->GetPsTransform();
		glm::mat4 inverseTransform = glm::inverse(transform);

		std::shared_ptr<CGALMesh> mesh1 = Utils::MeshToCGALMesh(ring, ring->GetPsTransform());
		Exact_point_map mesh1_exact_points = mesh1->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		Exact_vertex_point_map mesh1_vpm(mesh1_exact_points, *mesh1);

		std::shared_ptr<CGALMesh> mesh2 = std::make_shared<CGALMesh>();
		Exact_point_map mesh2_exact_points = mesh2->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
		Exact_vertex_point_map mesh2_vpm(mesh2_exact_points, *mesh2);

		// Gems
		for (size_t i = 0; i < gems.size(); i++) {
			std::shared_ptr<CGALMesh> mesh3 = Utils::MeshToCGALMesh(gems[i], gems[i]->GetPsTransform());

			Exact_point_map mesh3_exact_points = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
			Exact_vertex_point_map mesh3_vpm(mesh3_exact_points, *mesh3);

			CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
												CGALparams::vertex_point_map(mesh2_vpm),
												CGALparams::vertex_point_map(mesh3_vpm),
												CGALparams::vertex_point_map(mesh2_vpm));
		}

		// Mandrels
		for (size_t i = 0; i < gems.size(); i++) {
			std::shared_ptr<Mesh> mandrel = ResourceManager::Get()->CreateMandrel();
			std::shared_ptr<CGALMesh> mesh3 = Utils::MeshToCGALMesh(mandrel, gems[i]->GetPsTransform());

			Exact_point_map mesh3_exact_points = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
			Exact_vertex_point_map mesh3_vpm(mesh3_exact_points, *mesh3);

			CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
												CGALparams::vertex_point_map(mesh2_vpm),
												CGALparams::vertex_point_map(mesh3_vpm),
												CGALparams::vertex_point_map(mesh2_vpm));
		}

		// GemSettings
		for (size_t i = 0; i < gemSettings.size(); i++) {
			std::shared_ptr<CGALMesh> mesh3 = Utils::MeshToCGALMesh(gemSettings[i], gemSettings[i]->GetPsTransform());
			mesh3 = ConstructContexHull(mesh3);

			Exact_point_map mesh3_exact_points = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
			Exact_vertex_point_map mesh3_vpm(mesh3_exact_points, *mesh3);

			CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
												CGALparams::vertex_point_map(mesh2_vpm),
												CGALparams::vertex_point_map(mesh3_vpm),
												CGALparams::vertex_point_map(mesh2_vpm));
		}

		switch (settingType)
		{
			case GemSettingType::Shovel:
			{
				const std::unique_ptr<Geodesic>& geodesic = m_Scene->m_Geodesic;
				float gemScale = gemLine.GetGemScale();
				std::vector<CGALPoint> groovePoints = CalculateGroove(gemLine.GetPath(), 0.05f, 0.7f * gemScale, -0.35f, 0.6f * gemScale);

				const size_t step = 5;
				for (size_t i = 0; i < gemLine.GetPath().Length(); i += step) {
					std::shared_ptr<CGALMesh> mesh3 = std::make_shared<CGALMesh>();
					Exact_point_map mesh3_exact_points = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
					Exact_vertex_point_map mesh3_vpm(mesh3_exact_points, *mesh3);
					for (size_t j = 0; j < std::min(step, gemLine.GetPath().Length()); j++) {
						std::shared_ptr<CGALMesh> mesh4 = std::make_shared<CGALMesh>();
						CGAL::convex_hull_3(groovePoints.begin() + (i + j) * 4, groovePoints.begin() + (i + j + 2) * 4, *mesh4);
						Exact_point_map mesh4_exact_points = mesh4->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
						Exact_vertex_point_map mesh4_vpm(mesh4_exact_points, *mesh4);
						CGALpmp::corefine_and_compute_union(*mesh3, *mesh4, *mesh3,
															CGALparams::vertex_point_map(mesh3_vpm),
															CGALparams::vertex_point_map(mesh4_vpm),
															CGALparams::vertex_point_map(mesh3_vpm));
					}
					CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
														CGALparams::vertex_point_map(mesh2_vpm),
														CGALparams::vertex_point_map(mesh3_vpm),
														CGALparams::vertex_point_map(mesh2_vpm));
				}
				break;
			}	
			case GemSettingType::Channel:
			{
				const std::unique_ptr<Geodesic>& geodesic = m_Scene->m_Geodesic;
				float gemScale = gemLine.GetGemScale();

				const size_t step = 5;
				std::vector<CGALPoint> groovePoints = CalculateGroove(gemLine.GetPath(), 0.05f, 0.43f * gemScale, -0.25f, 0.43f * gemScale);
				for (size_t i = 0; i < gemLine.GetPath().Length(); i += step) {
					std::shared_ptr<CGALMesh> mesh3 = std::make_shared<CGALMesh>();
					Exact_point_map mesh3_exact_points = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
					Exact_vertex_point_map mesh3_vpm(mesh3_exact_points, *mesh3);
					for (size_t j = 0; j < std::min(step, gemLine.GetPath().Length()); j++) {
						std::shared_ptr<CGALMesh> mesh4 = std::make_shared<CGALMesh>();
						CGAL::convex_hull_3(groovePoints.begin() + (i + j) * 4, groovePoints.begin() + (i + j + 2) * 4, *mesh4);
						Exact_point_map mesh4_exact_points = mesh4->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
						Exact_vertex_point_map mesh4_vpm(mesh4_exact_points, *mesh4);
						CGALpmp::corefine_and_compute_union(*mesh3, *mesh4, *mesh3,
															CGALparams::vertex_point_map(mesh3_vpm),
															CGALparams::vertex_point_map(mesh4_vpm),
															CGALparams::vertex_point_map(mesh3_vpm));
					}
					CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
														CGALparams::vertex_point_map(mesh2_vpm),
														CGALparams::vertex_point_map(mesh3_vpm),
														CGALparams::vertex_point_map(mesh2_vpm));
				}

				//groovePoints = CalculateGroove(gemLine.GetPath(), -0.05f, 0.5f * gemScale, -0.3f, 0.5f * gemScale);
				//for (size_t i = 0; i < gemLine.GetPath().Length(); i += step) {
				//	std::shared_ptr<CGALMesh> mesh3 = std::make_shared<CGALMesh>();
				//	Exact_point_map mesh3_exact_points = mesh3->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
				//	Exact_vertex_point_map mesh3_vpm(mesh3_exact_points, *mesh3);
				//	for (size_t j = 0; j < std::min(step, gemLine.GetPath().Length()); j++) {
				//		std::shared_ptr<CGALMesh> mesh4 = std::make_shared<CGALMesh>();
				//		CGAL::convex_hull_3(groovePoints.begin() + (i + j) * 4, groovePoints.begin() + (i + j + 2) * 4, *mesh4);
				//		Exact_point_map mesh4_exact_points = mesh4->add_property_map<vertex_descriptor, ExactKernel::Point_3>("v:exact_point").first;
				//		Exact_vertex_point_map mesh4_vpm(mesh4_exact_points, *mesh4);
				//		CGALpmp::corefine_and_compute_union(*mesh3, *mesh4, *mesh3,
				//											CGALparams::vertex_point_map(mesh3_vpm),
				//											CGALparams::vertex_point_map(mesh4_vpm),
				//											CGALparams::vertex_point_map(mesh3_vpm));
				//	}
				//	CGALpmp::corefine_and_compute_union(*mesh2, *mesh3, *mesh2,
				//										CGALparams::vertex_point_map(mesh2_vpm),
				//										CGALparams::vertex_point_map(mesh3_vpm),
				//										CGALparams::vertex_point_map(mesh2_vpm));
				//}
				break;
			}
		}

		bool valid_difference = CGALpmp::corefine_and_compute_difference(*mesh1, *mesh2, *mesh1,
																		CGALparams::vertex_point_map(mesh1_vpm),
																		CGALparams::vertex_point_map(mesh2_vpm),
																		CGALparams::vertex_point_map(mesh1_vpm));

		if (valid_difference) {
			GC_CORE_INFO("Difference was successfully computed.");
		}

		return Utils::CGALMeshToMesh(mesh1, inverseTransform);
	}

	std::shared_ptr<CGALMesh> BooleanTool::ConstructContexHull(std::shared_ptr<CGALMesh> mesh)
	{
		std::shared_ptr<CGALMesh> contexHull = std::make_shared<CGALMesh>();
		CGAL::convex_hull_3(mesh->points().begin(), mesh->points().end(), *contexHull);

		return contexHull;
	}

	std::vector<CGALPoint> BooleanTool::CalculateGroove(const Path& path, float distanceUP, float topWidth, float distanceDown, float bottomWidth)
	{
		const std::unique_ptr<Geodesic>& geodesic = m_Scene->m_Geodesic;
		const std::vector<glm::vec3>& points = path.Points();

		// Calculate forward direction
		size_t len = path.Length();
		std::vector<glm::vec3> forwardDirections;
		forwardDirections.push_back(glm::normalize(points[1] - points[0]));
		for (size_t i = 1; i < len - 1; i++) {
			forwardDirections.push_back(glm::normalize(points[i + 1] - points[i - 1]));
		}
		forwardDirections.push_back(glm::normalize(points[len - 1] - points[len - 2]));

		std::vector<CGALPoint> groovePoints;
		for (size_t i = 0; i < len; i++) {
			for (int k = -1; k <= 1; k += 2) {
				CGALPoint start_point(points[i].x, points[i].y, points[i].z);
				Face_location start_point_loc = geodesic->LocatePoint(start_point);
				glm::vec3 traceResult = geodesic->TracePath(start_point_loc, forwardDirections[i], k * topWidth);
				glm::vec3 normal = geodesic->CalculateNormal(points[i]);
				glm::vec3 point = traceResult + normal * distanceUP;
				groovePoints.push_back({ point.x, point.y, point.z });

				traceResult = geodesic->TracePath(start_point_loc, forwardDirections[i], k * bottomWidth);
				point = traceResult + normal * distanceDown;
				groovePoints.push_back({ point.x, point.y, point.z });
			}
		}
		return groovePoints;
	}

} // namespace GemCraft