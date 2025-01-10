#include "Mesh/PlacementTool.h"

#include "Core/Scene.h"
#include "Core/ResourceManager.h"

#include "Mesh/FormatTool.h"

#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/repair.h>

namespace GemCraft {

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

	std::shared_ptr<Mesh> PlacementTool::PlaceGem(const std::string& name, const std::string& filepath, const glm::mat4& transform)
	{
		std::shared_ptr<Mesh> gem = ResourceManager::Get()->CreateGem(filepath);
		if (!gem) {
			gem = std::make_shared<Mesh>("Gem", filepath);
		}

		gem->SetName(name);
		gem->AddToPolyscope(transform);
		gem->GetPsMesh()->setMaterial("candy");
		gem->GetPsMesh()->setSurfaceColor(glm::vec3(0.270, 0.110, 0.890));

		return gem;
	}

	std::shared_ptr<Mesh> PlacementTool::PlaceGem(GemSpecification spec)
	{
		glm::vec3 shiftedPosition = spec.Position + spec.ExposureDepth * spec.Normal;
		glm::mat4 transform = glm::inverse(glm::lookAt(shiftedPosition, shiftedPosition + spec.Forward, spec.Normal));
		transform = glm::rotate(transform, glm::radians(-90.0f), { 1.0f, 0.0f, 0.0f });
		transform = glm::scale(transform, glm::vec3(spec.Scale));

		return PlaceGem(spec.Name, spec.Filepath, transform);
	}

	std::shared_ptr<Mesh> PlacementTool::PlaceGemSetting(const std::string& name, GemSettingType settingType, const glm::mat4& transform)
	{
		std::shared_ptr<Mesh> gemSetting = ResourceManager::Get()->CreateGemSetting(settingType);

		gemSetting->SetName(name);
		gemSetting->AddToPolyscope(transform);
		gemSetting->GetPsMesh()->setMaterial("clay");
		gemSetting->GetPsMesh()->setSurfaceColor(glm::vec3(0.750, 0.750, 0.750));

		return gemSetting;
	}

	std::shared_ptr<Mesh> PlacementTool::PlaceGemSetting(GemSettingSpecification spec)
	{
		glm::vec3 shiftedPosition = spec.Position + spec.ExposureDepth * spec.Normal;
		glm::mat4 transform = glm::inverse(glm::lookAt(shiftedPosition, shiftedPosition + spec.Forward, spec.Normal));
		transform = glm::rotate(transform, glm::radians(-90.0f), { 1.0f, 0.0f, 0.0f });
		transform = glm::scale(transform, glm::vec3(spec.Scale));

		return PlaceGemSetting(spec.Name, spec.SettingType, transform);
	}

	struct PositionsAndForwards
	{
		std::vector<glm::vec3> GemPositions;
		std::vector<glm::vec3> GemForwards;
		std::vector<glm::vec3> BasePositions;
		std::vector<glm::vec3> BaseForwards;
	};

	static PositionsAndForwards CalculatePositionsAndForwards(const Path& path, float gemSpacing)
	{
		const std::vector<glm::vec3>& points = path.Points();
		size_t len = path.Length();
		std::vector<glm::vec3> originForwards;
		originForwards.push_back(glm::normalize(points[1] - points[0]));
		for (size_t i = 1; i < len - 1; i++) {
			originForwards.push_back(glm::normalize(points[i + 1] - points[i - 1]));
		}
		originForwards.push_back(glm::normalize(points[len - 1] - points[len - 2]));

		float pathLength = 0.0f;
		for (size_t i = 1; i < points.size(); i++) {
			pathLength += glm::distance(points[i - 1], points[i]);
		}

		PositionsAndForwards result;
		int count = 0;
		for (float distanceAlongPath = 0.0f; distanceAlongPath < pathLength; distanceAlongPath += gemSpacing / 2) {
			size_t currentSegment = 0;
			float currentDistance = 0.0f;
			while (currentSegment + 1 < points.size()) {
				float segmentDistance = glm::distance(points[currentSegment], points[currentSegment + 1]);
				if (currentDistance + segmentDistance >= distanceAlongPath) {
					break;
				}
				currentDistance += segmentDistance;
				++currentSegment;
			}
			float remainingDistance = distanceAlongPath - currentDistance;
			float t = remainingDistance / glm::distance(points[currentSegment], points[currentSegment + 1]);
			glm::vec3 position = glm::mix(points[currentSegment], points[currentSegment + 1], t);
			glm::vec3 forward = glm::mix(originForwards[currentSegment], originForwards[currentSegment + 1], t);
			forward = glm::normalize(forward);

			if (count % 2 == 0) {
				result.BasePositions.push_back(position);
				result.BaseForwards.push_back(forward);
			}
			else {
				result.GemPositions.push_back(position);
				result.GemForwards.push_back(forward);
			}
			count++;
		}
		return result;
	}

	GemLine PlacementTool::PlaceGemsOnPath(const Path& path)
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		const GemSelectionUI& gemSelectionUI = m_Scene->m_GemSelectionUI;
		const GemSettingSelectionUI& gemSettingSelectionUI = m_Scene->m_GemSettingSelectionUI;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

		GemSettingType settingType = gemSettingSelectionUI.GetCurSelectedGemSetting();
		float exposureDepth = gemPatternUI.GetExposureDepth();
		if (settingType == GemSettingType::Shovel || settingType == GemSettingType::Channel) exposureDepth = 0.0f;

		float gemScale = gemPatternUI.GetGemScale();
		float spacing = (GetParams(settingType).GemSpacing + 1.0f) * gemScale;

		std::vector<std::shared_ptr<Mesh>> gems;
		std::vector<std::shared_ptr<Mesh>> gemSettings;

		PositionsAndForwards result = CalculatePositionsAndForwards(path, spacing);
		for (size_t j = 1; j < result.BasePositions.size(); j++) {
			GemSpecification spec;
			spec.Filepath = gemSelectionUI.GetCurSelectedGem();
			spec.Position = result.GemPositions[j - 1];
			spec.Forward = result.GemForwards[j - 1];
			spec.Normal = geodesic->CalculateNormal(spec.Position);
			spec.ExposureDepth = exposureDepth;
			spec.Scale = gemScale;
			std::stringstream ss;
			ss << std::fixed << std::setprecision(3) << "Gem (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
			spec.Name = ss.str();
			gems.push_back(PlaceGem(spec));
		}

		switch (settingType)
		{
		case GemSettingType::Pave:
		case GemSettingType::Shovel:
			for (size_t j = 1; j < result.BasePositions.size(); j++) {
				GemSettingSpecification spec;
				if (j == 1) {
					spec.SettingType = settingType;
					glm::vec3 position = result.BasePositions[0];
					spec.Forward = result.BaseForwards[0];
					spec.Normal = geodesic->CalculateNormal(position);
					spec.ExposureDepth = exposureDepth;
					spec.Scale = gemScale;
					glm::vec3 right = glm::cross(spec.Forward, spec.Normal);
					for (int k = -1; k <= 1; k += 2) {
						spec.Position = position + k * 0.3f * spec.Scale * right;
						std::stringstream ss;
						ss << std::fixed << std::setprecision(3) << "GemSetting (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
						spec.Name = ss.str();
						gemSettings.push_back(PlaceGemSetting(spec));
					}
				}
				spec.SettingType = settingType;
				glm::vec3 position = result.BasePositions[j];
				spec.Forward = result.BaseForwards[j];
				spec.Normal = geodesic->CalculateNormal(position);
				spec.ExposureDepth = exposureDepth;
				spec.Scale = gemScale;
				glm::vec3 right = glm::cross(spec.Forward, spec.Normal);
				for (int k = -1; k <= 1; k += 2) {
					spec.Position = position + k * 0.3f * spec.Scale * right;
					std::stringstream ss;
					ss << std::fixed << std::setprecision(3) << "GemSetting (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
					spec.Name = ss.str();
					gemSettings.push_back(PlaceGemSetting(spec));
				}
			}
			break;
		default:
			for (size_t j = 1; j < result.BasePositions.size(); j++) {
				GemSettingSpecification spec;
				spec.SettingType = settingType;
				spec.Position = result.GemPositions[j - 1];
				spec.Forward = result.GemForwards[j - 1];
				spec.Normal = geodesic->CalculateNormal(spec.Position);
				spec.ExposureDepth = exposureDepth;
				spec.Scale = gemScale;
				std::stringstream ss;
				ss << std::fixed << std::setprecision(3) << "GemSetting (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
				spec.Name = ss.str();
				gemSettings.push_back(PlaceGemSetting(spec));
			}
		}

		return GemLine(settingType, gems, gemSettings, path, gemScale);
	}

	GemGroup PlacementTool::PlaceGemsOnSelectedRegion()
	{
		const GemSettingSelectionUI& gemSettingSelectionUI = m_Scene->m_GemSettingSelectionUI;
		GemSettingType settingType = gemSettingSelectionUI.GetCurSelectedGemSetting();
		if (settingType == GemSettingType::Pave || settingType == GemSettingType::Shovel || settingType == GemSettingType::Channel) {
			return GemGroup(settingType, std::vector<std::shared_ptr<Mesh>>(), std::vector<std::shared_ptr<Mesh>>());
		}

		std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
		std::vector<glm::vec3> originVertices = ring->GetVertices();
		std::vector<std::vector<size_t>> originFaces = ring->GetFaces();
		std::vector<std::vector<size_t>> newFaces;
		for (auto& index : polyscope::state::selectedRegion.Faces()) {
			newFaces.push_back(originFaces[index]);
		}
		std::shared_ptr<Mesh> subMesh = std::make_shared<Mesh>("", originVertices, newFaces);
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(subMesh, subMesh->GetPsTransform());
		CGALpmp::remove_isolated_vertices(*cgalSubmesh);
		cgalSubmesh->collect_garbage();

		for (int iter = 0; iter < 5; iter++) {
			std::vector<CGALMesh::Face_index> facesToRemove;
			for (auto& f : cgalSubmesh->faces()) {
				bool isBorderFace = false;
				CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
				for (boost::tie(vbegin, vend) = cgalSubmesh->vertices_around_face(cgalSubmesh->halfedge(f)); vbegin != vend; ++vbegin) {
					for (auto h : halfedges_around_target(*vbegin, *cgalSubmesh)) {
						if (is_border(h, *cgalSubmesh)) {
							isBorderFace = true;
						}
					}
				}
				if (isBorderFace) {
					facesToRemove.push_back(f);
				}
			}
			for (const auto& f : facesToRemove) {
				CGAL::Euler::remove_face(halfedge(f, *cgalSubmesh), *cgalSubmesh);
			}
			cgalSubmesh->collect_garbage();
		}

		double target_edge_length = 2.5;
		unsigned int nb_iter = 10;

		std::vector<edge_descriptor> border;
		CGALpmp::border_halfedges(faces(*cgalSubmesh), *cgalSubmesh, boost::make_function_output_iterator(HalfedgeToEdge(*cgalSubmesh, border)));
		CGALpmp::split_long_edges(border, target_edge_length, *cgalSubmesh);

		CGALpmp::isotropic_remeshing(faces(*cgalSubmesh), target_edge_length, *cgalSubmesh,
			CGAL::parameters::number_of_iterations(nb_iter)
			.protect_constraints(true)); //i.e. protect border, here

		CGAL::IO::write_polygon_mesh("remeshed.obj", *cgalSubmesh, CGAL::parameters::stream_precision(17));

		std::shared_ptr<Mesh> remeshedMesh = std::make_shared<Mesh>("Remeshed Result 2", "remeshed.obj", true);
		std::shared_ptr<CGALMesh> remeshedCGALMesh = FormatTool::MeshToCGALMesh(remeshedMesh, remeshedMesh->GetPsTransform());

		for (int iter = 0; iter < 1; iter++) {
			std::vector<CGALMesh::Face_index> facesToRemove;
			for (auto& f : remeshedCGALMesh->faces()) {
				bool isBorderFace = false;
				CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
				for (boost::tie(vbegin, vend) = remeshedCGALMesh->vertices_around_face(remeshedCGALMesh->halfedge(f)); vbegin != vend; ++vbegin) {
					for (auto h : halfedges_around_target(*vbegin, *remeshedCGALMesh)) {
						if (is_border(h, *remeshedCGALMesh)) {
							isBorderFace = true;
						}
					}
				}
				if (isBorderFace) {
					facesToRemove.push_back(f);
				}
			}
			for (const auto& f : facesToRemove) {
				CGAL::Euler::remove_face(halfedge(f, *remeshedCGALMesh), *remeshedCGALMesh);
			}
			remeshedCGALMesh->collect_garbage();
		}

		std::vector<CGALPoint> points;
		for (auto& v : remeshedCGALMesh->vertices()) {
			bool isBorderVertex = false;
			for (auto h : halfedges_around_target(v, *remeshedCGALMesh)) {
				if (is_border(h, *remeshedCGALMesh)) {
					isBorderVertex = true;
				}
			}
			if (isBorderVertex) {
			}
			points.push_back({ remeshedCGALMesh->point(v).x(), remeshedCGALMesh->point(v).y(), remeshedCGALMesh->point(v).z() });
		}

		std::ofstream output("sampled_points.xyz");
		for (const auto& point : points) {
			output << point << std::endl;
		}

		std::vector<glm::vec3> positions;
		for (const auto& point : points) {
			positions.push_back({ point.x(), point.y(), point.z() });
		}

		PlacementTool placerTool(m_Scene);
		GemGroup gemGroup = placerTool.PlaceGemsOnPositions(positions);

		// Show remesh result.
		m_Scene->AddMesh(remeshedMesh);
		remeshedMesh->GetPsMesh()->setEnabled(false);

		return gemGroup;
	}

	GemGroup PlacementTool::PlaceGemsOnPositions(const std::vector<glm::vec3>& positions)
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		const GemSelectionUI& gemSelectionUI = m_Scene->m_GemSelectionUI;
		const GemSettingSelectionUI& gemSettingSelectionUI = m_Scene->m_GemSettingSelectionUI;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

		GemSettingType settingType = gemSettingSelectionUI.GetCurSelectedGemSetting();
		float exposureDepth = gemPatternUI.GetExposureDepth();
		if (settingType == GemSettingType::Shovel || settingType == GemSettingType::Channel) exposureDepth = 0.0f;

		float gemScale = gemPatternUI.GetGemScale();
		float spacing = (GetParams(settingType).GemSpacing + 1.0f) * gemScale;

		std::vector<std::shared_ptr<Mesh>> gems;
		std::vector<std::shared_ptr<Mesh>> gemSettings;

		for (size_t i = 0; i < positions.size(); i++) {
			GemSpecification spec;
			spec.Filepath = gemSelectionUI.GetCurSelectedGem();
			spec.Position = positions[i];
			spec.Normal = geodesic->CalculateNormal(spec.Position);

			glm::vec3 v;
			if (std::abs(spec.Normal.x) < std::abs(spec.Normal.y)) {
				v = glm::vec3(1.0f, 0.0f, 0.0f);
			}
			else {
				v = glm::vec3(0.0f, 1.0f, 0.0f);
			}
			spec.Forward = glm::cross(spec.Normal, v);

			spec.ExposureDepth = exposureDepth;
			spec.Scale = gemScale;
			std::stringstream ss;
			ss << std::fixed << std::setprecision(3) << "Gem (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
			spec.Name = ss.str();
			gems.push_back(PlaceGem(spec));
		}

		for (size_t i = 0; i < positions.size(); i++) {
			GemSettingSpecification spec;
			spec.SettingType = settingType;
			spec.Position = positions[i];
			spec.Normal = geodesic->CalculateNormal(spec.Position);

			glm::vec3 v;
			if (std::abs(spec.Normal.x) < std::abs(spec.Normal.y)) {
				v = glm::vec3(1.0f, 0.0f, 0.0f);
			}
			else {
				v = glm::vec3(0.0f, 1.0f, 0.0f);
			}
			spec.Forward = glm::cross(spec.Normal, v);

			spec.ExposureDepth = exposureDepth;
			spec.Scale = gemScale;
			std::stringstream ss;
			ss << std::fixed << std::setprecision(3) << "GemSetting (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
			spec.Name = ss.str();
			gemSettings.push_back(PlaceGemSetting(spec));
		}

		return GemGroup(settingType, gems, gemSettings);
	}

} // namespace GemCraft