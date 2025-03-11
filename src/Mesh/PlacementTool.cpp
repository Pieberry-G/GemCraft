#include "Mesh/PlacementTool.h"

#include "Core/Scene.h"
#include "Core/ResourceManager.h"

#include "Mesh/FormatTool.h"
#include "Mesh/GeodesicTool.h"
#include "Mesh/Packing2D.h"

namespace GemCraft {

	static double ComputeSignedArea(const std::vector<CGALPoint2>& polygon);

	void PlacementTool::Clean()
	{
		m_Submesh = nullptr;
		m_BooleanMesh = nullptr;
		m_UVCoords.clear();
		m_GeodesicDistances.clear();
	}

	void PlacementTool::BuildSubmeshForSelectedRegion()
	{
		GC_CORE_WARN("Building submesh.");

		std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
		std::vector<glm::vec3> originVertices = ring->GetVertices();
		std::vector<std::vector<size_t>> originFaces = ring->GetFaces();
		std::vector<std::vector<size_t>> newFaces;
		for (auto& index : polyscope::state::selectedRegion.Faces()) {
			newFaces.push_back(originFaces[index]);
		}
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>("", originVertices, newFaces);
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(newMesh, newMesh->GetPsTransform());
		CGALpmp::remove_isolated_vertices(*cgalSubmesh);
		cgalSubmesh->collect_garbage();

		m_Submesh = FormatTool::CGALMeshToMesh(cgalSubmesh, glm::mat4(1.0f));
		m_Submesh->SetName("Submesh");

		GC_CORE_INFO("Completed!");
	}

	void PlacementTool::ParameterizeSubmesh()
	{
		GC_CORE_WARN("Parameterizing submesh.");

		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());
		halfedge_descriptor bhd = CGALpmp::longest_border(*cgalSubmesh).first;
		UV_pmap uvMap = cgalSubmesh->add_property_map<vertex_descriptor, CGALPoint2>("h:uv").first;
		SMP::ARAP_parameterizer_3<CGALMesh> parameterizer;
		SMP::Error_code err = SMP::parameterize(*cgalSubmesh, parameterizer, bhd, uvMap);
		for (vertex_descriptor v : vertices(*cgalSubmesh)) {
			m_UVCoords.push_back({ uvMap[v].x(), uvMap[v].y()});
		}

		std::ofstream out("parameterize.off");
		SMP::IO::output_uvmap_to_off(*cgalSubmesh, bhd, uvMap, out);

		GC_CORE_INFO("Completed!");
	}

	void PlacementTool::CalculateGeodesicDistance()
	{
		GC_CORE_WARN("Calculating geodesic distance.");

		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());
		halfedge_descriptor bhd = CGALpmp::longest_border(*cgalSubmesh).first;
		Vertex_distance_map vertexDistance = cgalSubmesh->add_property_map<vertex_descriptor, double>("v:distance", 0).first;
		Heat_method hm(*cgalSubmesh);
		for (halfedge_descriptor hed : halfedges_around_face(bhd, *cgalSubmesh)) {
			vertex_descriptor sourceVertex = source(hed, *cgalSubmesh);
			hm.add_source(sourceVertex);
		}
		hm.estimate_geodesic_distances(vertexDistance);
		for (vertex_descriptor v : vertices(*cgalSubmesh)) {
			m_GeodesicDistances.push_back(vertexDistance[v]);
		}

		GC_CORE_INFO("Completed!");
	}

	void PlacementTool::CreateMeshForBooleanHole()
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());

		std::vector<glm::vec3> sortedVertices = ExtractBooleanMeshBoundary();

		std::shared_ptr<CGALMesh> cgalBooleanMesh = std::make_shared<CGALMesh>();
		glm::vec3 normal;
		float holeDepth = gemPatternUI.GetHoleDepth();
		int len = sortedVertices.size();
		for (int i = 0; i < len; i++) {
			normal = geodesic->CalculateNormal(sortedVertices[i]);
			cgalBooleanMesh->add_vertex(CGALPoint(sortedVertices[i].x + 0.2f * normal.x, sortedVertices[i].y + 0.2f * normal.y, sortedVertices[i].z + 0.2f * normal.z));
			cgalBooleanMesh->add_vertex(CGALPoint(sortedVertices[i].x - holeDepth * normal.x, sortedVertices[i].y - holeDepth * normal.y, sortedVertices[i].z - holeDepth * normal.z));
		}
		for (int i = 0; i < len; i++) {
			cgalBooleanMesh->add_face(CGALMesh::vertex_index((2 * i) % (2 * len)), CGALMesh::vertex_index((2 * i + 2) % (2 * len)), CGALMesh::vertex_index((2 * i + 3) % (2 * len)));
			cgalBooleanMesh->add_face(CGALMesh::vertex_index((2 * i) % (2 * len)), CGALMesh::vertex_index((2 * i + 3) % (2 * len)), CGALMesh::vertex_index((2 * i + 1) % (2 * len)));
		}

		unsigned int nbHoles = 0;
		std::vector<halfedge_descriptor> borderCycles;
		CGALpmp::extract_boundary_cycles(*cgalBooleanMesh, std::back_inserter(borderCycles));
		for (halfedge_descriptor h : borderCycles) {
			std::vector<face_descriptor>  patchFaces;
			std::vector<vertex_descriptor> patchVertices;
			bool success = std::get<0>(CGALpmp::triangulate_refine_and_fair_hole(*cgalBooleanMesh,
				h,
				CGAL::parameters::face_output_iterator(std::back_inserter(patchFaces))
				.vertex_output_iterator(std::back_inserter(patchVertices))
				.fairing_continuity(0)));
			++nbHoles;
		}

		CGALpmp::smooth_shape(*cgalBooleanMesh, 0.001, CGAL::parameters::number_of_iterations(10));

		std::ofstream out("boolean_mesh.obj");
		CGAL::IO::write_OBJ(out, *cgalBooleanMesh);
		out.close();

		m_BooleanMesh = FormatTool::CGALMeshToMesh(cgalBooleanMesh, glm::mat4(1.0f));
		m_BooleanMesh->SetName("Boolean Mesh");
	}

	GemGroup PlacementTool::PlaceGemsOnSelectedRegion()
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());

		// Generate Packing
		std::vector<glm::vec2> boundary;
		halfedge_descriptor bhd = CGALpmp::longest_border(*cgalSubmesh).first;
		for (halfedge_descriptor h : halfedges_around_face(bhd, *cgalSubmesh)) {
			CGALMesh::Vertex_index v = target(h, *cgalSubmesh);
			boundary.push_back(m_UVCoords[v]);
		}

		Packing2D packing2D;
		float gemScale = gemPatternUI.GetGemScale();
		float holeShrinkLength = gemPatternUI.GetHoleShrinkLength();
		float gridRotation = glm::radians(gemPatternUI.GetGridRotation());

		std::vector<glm::vec2> targetPoints;
		if (gemPatternUI.GetCurSelectedPackingMode() == PackingMode::Hexagonal) {
			float cellRadius = 0.55f * gemScale;
			float shrinkLength = holeShrinkLength + 0.1f * cellRadius;
			targetPoints = packing2D.GenerateHexagonalPacking(0.55f * gemScale, gridRotation, boundary, shrinkLength);
		}
		else if (gemPatternUI.GetCurSelectedPackingMode() == PackingMode::Square) {
			float cellRadius = 0.55f * gemScale;
			float shrinkLength = holeShrinkLength + 0.1f * cellRadius;
			targetPoints = packing2D.GenerateSquarePacking(0.55f * gemScale, gridRotation, boundary, shrinkLength);
		}
		else if (gemPatternUI.GetCurSelectedPackingMode() == PackingMode::Compact) {
			float cellRadius = 0.6f * gemScale;
			float shrinkLength = holeShrinkLength + 0.1f * cellRadius;
			targetPoints = packing2D.GenerateCompactPacking(cellRadius, gridRotation, boundary, shrinkLength, gemPatternUI.GetPackingEdgeLoopDensity(), gemPatternUI.GetPackingCenterDensity());
		}

		std::vector<glm::vec3> positions = Map2DPointsTo3D(targetPoints);
		return PlaceGemsOnPositions(positions);
	}

	GemGroup PlacementTool::PlaceGemsAtTargets()
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		const GemSettingSelectionUI& gemSettingSelectionUI = m_Scene->m_GemSettingSelectionUI;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

		GemSettingType settingType = gemSettingSelectionUI.GetCurSelectedGemSetting();
		float gemExposureDepth = gemPatternUI.GetGemExposureLength();

		float gemScale = gemPatternUI.GetGemScale();
		float spacing = (GetParams(settingType).GemSpacing + 1.0f) * gemScale;

		std::vector<std::shared_ptr<Mesh>> gems;
		std::vector<std::shared_ptr<Mesh>> gemSettings;

		std::vector<glm::vec3> positions = polyscope::state::targetPositions;
		std::vector<glm::vec3> normals = polyscope::state::targetNormals;

		for (size_t i = 0; i < positions.size(); i++) {
			GemSpecification spec;
			spec.SettingType = settingType;
			spec.Position = positions[i];
			spec.Normal = normals[i];

			glm::vec3 v;
			if (std::abs(spec.Normal.x) < std::abs(spec.Normal.y)) {
				v = glm::vec3(1.0f, 0.0f, 0.0f);
			}
			else {
				v = glm::vec3(0.0f, 1.0f, 0.0f);
			}
			spec.Forward = glm::cross(spec.Normal, v);

			spec.ExposureDepth = 0.0f;
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
			spec.Normal = normals[i];

			glm::vec3 v;
			if (std::abs(spec.Normal.x) < std::abs(spec.Normal.y)) {
				v = glm::vec3(1.0f, 0.0f, 0.0f);
			}
			else {
				v = glm::vec3(0.0f, 1.0f, 0.0f);
			}
			spec.Forward = glm::cross(spec.Normal, v);

			spec.ExposureDepth = 0.0f;
			spec.Scale = gemScale;
			std::stringstream ss;
			ss << std::fixed << std::setprecision(3) << "GemSetting (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
			spec.Name = ss.str();
			gemSettings.push_back(PlaceGemSetting(spec));
		}

		return GemGroup(settingType, gems, gemSettings);
	}

	std::vector<glm::vec3> PlacementTool::ExtractBooleanMeshBoundary()
	{
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());

		std::vector<glm::vec3> positions;
		std::vector<std::array<size_t, 2>> edgeInds;
		if (gemPatternUI.GetEnableHoleShrink()) {
			float holeShrinkLength = gemPatternUI.GetHoleShrinkLength();
			for (auto& face : m_Submesh->GetFaces()) {
				std::vector<glm::vec3> pos;
				for (size_t i = 0; i < face.size(); i++) {
					float vs = m_GeodesicDistances[face[i]];
					float vd = m_GeodesicDistances[face[(i + 1) % face.size()]];
					int region1 = floor(vs / holeShrinkLength);
					int region2 = floor(vd / holeShrinkLength);
					if ((region1 == 0 && region2 == 1) || (region1 == 1 && region2 == 0)) {
						double val = region1 > region2 ? region1 * holeShrinkLength : region2 * holeShrinkLength;
						float t = (val - vs) / (vd - vs);
						glm::vec3 ps = m_Submesh->GetVertices()[face[i]];
						glm::vec3 pd = m_Submesh->GetVertices()[face[(i + 1) % face.size()]];
						glm::vec3 p = ps + t * (pd - ps);
						pos.push_back(p);
					}
				}
				if (pos.size() == 2) {
					positions.push_back(pos[0]);
					positions.push_back(pos[1]);
					edgeInds.push_back({ positions.size() - 2, positions.size() - 1 });
				}
			}
		}
		else {
			halfedge_descriptor bhd = CGALpmp::longest_border(*cgalSubmesh).first;
			for (halfedge_descriptor hed : halfedges_around_face(bhd, *cgalSubmesh)) {
				vertex_descriptor p0 = source(hed, *cgalSubmesh);
				vertex_descriptor p1 = target(hed, *cgalSubmesh);
				positions.push_back({ cgalSubmesh->point(p0).x(), cgalSubmesh->point(p0).y(), cgalSubmesh->point(p0).z() });
				positions.push_back({ cgalSubmesh->point(p1).x(), cgalSubmesh->point(p1).y(), cgalSubmesh->point(p1).z() });
				edgeInds.push_back({ positions.size() - 2, positions.size() - 1 });
			}
		}

		// Sort boundary
		std::vector<glm::vec3> sortedVertices;
		std::unordered_set<int> visited;
		glm::vec3 endPoint;
		visited.insert(0);
		sortedVertices.push_back(positions[0]);
		sortedVertices.push_back(positions[1]);
		endPoint = positions[1];

		int count = 1;
		while (visited.size() != edgeInds.size()) {
			float minDistance = std::numeric_limits<float>::max();
			int targetEdge;
			int endIndex;
			for (int i = 0; i < edgeInds.size(); i++) {
				if (visited.find(i) == visited.end()) {
					float dis0 = glm::distance(endPoint, positions[edgeInds[i][0]]);
					float dis1 = glm::distance(endPoint, positions[edgeInds[i][1]]);
					if (min(dis0, dis1) < minDistance) {
						if (dis0 <= dis1) {
							minDistance = dis0;
							targetEdge = i;
							endIndex = 1;
						}
						else {
							minDistance = dis1;
							targetEdge = i;
							endIndex = 0;
						}
					}

				}
			}
			visited.insert(targetEdge);
			if (visited.size() != edgeInds.size()) {
				endPoint = positions[edgeInds[targetEdge][endIndex]];
				sortedVertices.push_back(endPoint);
			}
		}

		GeodesicTool submeshGeodesic(m_Submesh, m_Scene);
		std::vector<CGALPoint2> polygon;
		for (auto v : sortedVertices) {
			Face_location faceLoc = submeshGeodesic.LocatePoint(CGALPoint(v.x, v.y, v.z));
			size_t faceID = faceLoc.first;
			CGAL::Vertex_around_face_iterator<CGALMesh> vbegin, vend;
			glm::vec2 sum(0.0f, 0.0f);
			int count = 0;
			for (boost::tie(vbegin, vend) = cgalSubmesh->vertices_around_face(cgalSubmesh->halfedge(face_descriptor(faceID))); vbegin != vend; ++vbegin) {
				sum += glm::vec2{ m_UVCoords[*vbegin].x * faceLoc.second[count], m_UVCoords[*vbegin].y * faceLoc.second[count] };
				count++;
			}
			polygon.push_back(CGALPoint2(sum.x, sum.y));
		}
		if (ComputeSignedArea(polygon) > 0) {
			std::reverse(sortedVertices.begin(), sortedVertices.end());
		}

		return sortedVertices;
	}

	std::vector<glm::vec3> PlacementTool::Map2DPointsTo3D(std::vector<glm::vec2>& points)
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());
		Triangulation t;
		std::map<CGALPoint2, vertex_descriptor> vertexMap;
		for (auto v : vertices(*cgalSubmesh)) {
			t.insert({ m_UVCoords[v].x, m_UVCoords[v].y });
			vertexMap[{ m_UVCoords[v].x, m_UVCoords[v].y }] = v;
		}
		std::vector<glm::vec3> positions;
		for (auto& point : points) {
			CGALPoint2 query(point.x, point.y);
			Triangulation::Face_handle fh = t.locate(query);
			std::vector<std::pair<CGALPoint2, double>> coords;
			double norm = CGAL::natural_neighbor_coordinates_2(t, query, std::back_inserter(coords)).second;
			CGALPoint targetPoint(0, 0, 0);
			for (const auto& pair : coords) {
				vertex_descriptor v = vertexMap[pair.first];
				double weight = pair.second / norm;
				targetPoint += weight * CGALVector(cgalSubmesh->point(v).x(), cgalSubmesh->point(v).y(), cgalSubmesh->point(v).z());
			}
			size_t faceID = geodesic->LocatePoint(targetPoint).first;
			std::set<size_t> selectedRegion = polyscope::state::selectedRegion.Faces();
			if (selectedRegion.find(faceID) != selectedRegion.end()) {
				positions.push_back({ targetPoint.x(), targetPoint.y(), targetPoint.z() });
			}
		}
		return positions;
	}

	std::shared_ptr<Mesh> PlacementTool::PlaceGem(const std::string& name, GemSettingType settingType, const glm::mat4& transform)
	{
		std::shared_ptr<Mesh> gem = ResourceManager::Get()->CreateGem(settingType);

		gem->SetName(name);
		gem->AddToPolyscope(transform);
		gem->GetPsMesh()->setMaterial("candy");
		gem->GetPsMesh()->setSurfaceColor(glm::vec3(0.270, 0.110, 0.890));
		polyscope::setParentGroupOfStructure(gem->GetPsMesh(), "Gems");

		return gem;
	}

	std::shared_ptr<Mesh> PlacementTool::PlaceGem(GemSpecification spec)
	{
		glm::vec3 shiftedPosition = spec.Position + spec.ExposureDepth * spec.Normal;
		glm::mat4 transform = glm::inverse(glm::lookAt(shiftedPosition, shiftedPosition + spec.Forward, spec.Normal));
		transform = glm::rotate(transform, glm::radians(-90.0f), { 1.0f, 0.0f, 0.0f });
		transform = glm::scale(transform, glm::vec3(spec.Scale));

		return PlaceGem(spec.Name, spec.SettingType, transform);
	}

	std::shared_ptr<Mesh> PlacementTool::PlaceGemSetting(const std::string& name, GemSettingType settingType, const glm::mat4& transform)
	{
		std::shared_ptr<Mesh> gemSetting = ResourceManager::Get()->CreateGemSetting(settingType);

		gemSetting->SetName(name);
		gemSetting->AddToPolyscope(transform);
		gemSetting->GetPsMesh()->setMaterial("clay");
		gemSetting->GetPsMesh()->setSurfaceColor(glm::vec3(0.750, 0.750, 0.750));
		polyscope::setParentGroupOfStructure(gemSetting->GetPsMesh(), "GemSettings");

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

	GemGroup PlacementTool::PlaceGemsOnPositions(const std::vector<glm::vec3>& positions)
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		const GemSettingSelectionUI& gemSettingSelectionUI = m_Scene->m_GemSettingSelectionUI;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

		GemSettingType settingType = gemSettingSelectionUI.GetCurSelectedGemSetting();
		float gemExposureDepth = gemPatternUI.GetGemExposureLength();
		float gemScale = gemPatternUI.GetGemScale();
		float spacing = (GetParams(settingType).GemSpacing + 1.0f) * gemScale;

		std::vector<std::shared_ptr<Mesh>> gems;
		std::vector<std::shared_ptr<Mesh>> gemSettings;

		for (size_t i = 0; i < positions.size(); i++) {
			GemSpecification spec;
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

			spec.ExposureDepth = gemExposureDepth;
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

			spec.ExposureDepth = gemExposureDepth;
			spec.Scale = gemScale;
			std::stringstream ss;
			ss << std::fixed << std::setprecision(3) << "GemSetting (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
			spec.Name = ss.str();
			gemSettings.push_back(PlaceGemSetting(spec));
		}

		return GemGroup(settingType, gems, gemSettings);
	}

	void PlacementTool::ShowResult()
	{
		// Submesh
		{
			m_Scene->AddMesh(m_Submesh);
			m_Submesh->GetPsMesh()->setEnabled(false);
		}

		// Geodesic distance
		{
			auto maxIter = std::max_element(m_GeodesicDistances.begin(), m_GeodesicDistances.end());
			float maxValue = *maxIter;

			std::vector<glm::vec3> vertexColors(m_Submesh->GetVertices().size());
			for (size_t i = 0; i < m_Submesh->GetVertices().size(); i++) {
				float value = m_GeodesicDistances[i] / maxValue;
				vertexColors[i] = { value, value, value };
			}
			polyscope::SurfaceVertexColorQuantity* showFaces = m_Submesh->GetPsMesh()->addVertexColorQuantity("Geodesic Distance", vertexColors);
			showFaces->setEnabled(true);
		}
	}

	void PlacementTool::ShowBooleanMesh()
	{
		// Boolean mesh
		{
			m_Scene->AddMesh(m_BooleanMesh);
			m_BooleanMesh->GetPsMesh()->setEnabled(false);
		}
	}

	static double ComputeSignedArea(const std::vector<CGALPoint2>& polygon)
	{
		double area = 0.0;
		int n = polygon.size();
		for (int i = 0; i < n; ++i) {
			int j = (i + 1) % n;
			area += (polygon[i].x() * polygon[j].y() - polygon[j].x() * polygon[i].y());
		}
		return area / 2.0;
	}

} // namespace GemCraft