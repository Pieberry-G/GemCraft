#include "Mesh/PlacementTool.h"

#include "Core/Scene.h"
#include "Core/ResourceManager.h"

#include "Mesh/FormatTool.h"
#include "Mesh/GeodesicTool.h"

namespace GemCraft {

	static std::pair<CGALPoint2, float> CalculateBoundingCircle(std::vector<CGALPoint2>& points);
	static std::vector<CGALPoint2> GenerateSquarePacking(CGALPoint2 center, int gridStep, float cellRadius, float gridRotation);
	static std::vector<CGALPoint2> GenerateHexagonalPacking(CGALPoint2 center, int gridStep, float cellRadius, float gridRotation);
	static std::vector<CGALPoint2> ClipToUVBoundary(const std::vector<CGALPoint2>& points, const std::vector<CGALPoint2>& boundary, float radius);
	static double ComputeSignedArea(const std::vector<CGALPoint2>& polygon);

	void PlacementTool::Clean()
	{
		m_Submesh = nullptr;
		m_BooleanMesh = nullptr;
		m_CGALSubmesh = nullptr;
		m_SubmeshBoundary.clear();
		m_SubmeshUVMap.reset();
		m_SubmeshUVBoundary.clear();
		m_GeodesicDistance.clear();
	}

	void PlacementTool::BuildSubmeshForSelectedRegion()
	{
		std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
		std::vector<glm::vec3> originVertices = ring->GetVertices();
		std::vector<std::vector<size_t>> originFaces = ring->GetFaces();
		std::vector<std::vector<size_t>> newFaces;
		for (auto& index : polyscope::state::selectedRegion.Faces()) {
			newFaces.push_back(originFaces[index]);
		}
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>("", originVertices, newFaces);
		m_CGALSubmesh = FormatTool::MeshToCGALMesh(newMesh, newMesh->GetPsTransform());
		CGALpmp::remove_isolated_vertices(*m_CGALSubmesh);
		m_CGALSubmesh->collect_garbage();

		m_Submesh = FormatTool::CGALMeshToMesh(m_CGALSubmesh, glm::mat4(1.0f));
		m_Submesh->SetName("Submesh");
	}

	void PlacementTool::ParameterizeSubmesh()
	{
		halfedge_descriptor bhd = CGALpmp::longest_border(*m_CGALSubmesh).first;
		m_SubmeshUVMap = m_CGALSubmesh->add_property_map<vertex_descriptor, CGALPoint2>("h:uv").first;
		SMP::ARAP_parameterizer_3<CGALMesh> parameterizer;
		SMP::Error_code err = SMP::parameterize(*m_CGALSubmesh, parameterizer, bhd, m_SubmeshUVMap);
		std::ofstream out("parameterize.off");
		SMP::IO::output_uvmap_to_off(*m_CGALSubmesh, bhd, m_SubmeshUVMap, out);
		//CGAL::IO::write_polygon_mesh("Submesh.obj", *cgalSubmesh, CGAL::parameters::stream_precision(17));

		// Save UV boundary
		for (halfedge_descriptor h : halfedges_around_face(bhd, *m_CGALSubmesh)) {
			CGALMesh::Vertex_index v = target(h, *m_CGALSubmesh);
			m_SubmeshBoundary.push_back(v);
			m_SubmeshUVBoundary.push_back(m_SubmeshUVMap[v]);
		}
	}

	void PlacementTool::CalculateGeodesicDistance()
	{
		halfedge_descriptor bhd = CGALpmp::longest_border(*m_CGALSubmesh).first;
		Vertex_distance_map vertex_distance = m_CGALSubmesh->add_property_map<vertex_descriptor, double>("v:distance", 0).first;
		Heat_method hm(*m_CGALSubmesh);
		for (halfedge_descriptor hed : halfedges_around_face(bhd, *m_CGALSubmesh)) {
			vertex_descriptor source1 = source(hed, *m_CGALSubmesh);
			hm.add_source(source1);
		}
		hm.estimate_geodesic_distances(vertex_distance);
		for (vertex_descriptor vd : vertices(*m_CGALSubmesh)) {
			m_GeodesicDistance.push_back(get(vertex_distance, vd));
		}
	}

	void PlacementTool::CreateBooleanMeshForSink()
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

		// Extract boundary
		std::vector<glm::vec3> positions;
		std::vector<std::array<size_t, 2>> edgeInds;
		if (gemPatternUI.GetEnableHoleShrink()) {
			float holeShrinkLength = gemPatternUI.GetHoleShrinkLength();
			for (auto& face : m_Submesh->GetFaces()) {
				std::vector<glm::vec3> pos;
				for (size_t i = 0; i < face.size(); i++) {
					float vs = m_GeodesicDistance[face[i]];
					float vd = m_GeodesicDistance[face[(i + 1) % face.size()]];
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
			halfedge_descriptor bhd = CGALpmp::longest_border(*m_CGALSubmesh).first;
			for (halfedge_descriptor hed : halfedges_around_face(bhd, *m_CGALSubmesh)) {
				vertex_descriptor p0 = source(hed, *m_CGALSubmesh);
				vertex_descriptor p1 = target(hed, *m_CGALSubmesh);
				positions.push_back({ m_CGALSubmesh->point(p0).x(), m_CGALSubmesh->point(p0).y(), m_CGALSubmesh->point(p0).z() });
				positions.push_back({ m_CGALSubmesh->point(p1).x(), m_CGALSubmesh->point(p1).y(), m_CGALSubmesh->point(p1).z() });
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
			glm::vec2 sum{ 0.0f, 0.0f };
			int count = 0;
			for (boost::tie(vbegin, vend) = m_CGALSubmesh->vertices_around_face(m_CGALSubmesh->halfedge(face_descriptor(faceID))); vbegin != vend; ++vbegin) {
				sum += glm::vec2{ m_SubmeshUVMap[*vbegin].x() * faceLoc.second[count], m_SubmeshUVMap[*vbegin].y() * faceLoc.second[count] };
				count++;
			}
			polygon.push_back(CGALPoint2(sum.x, sum.y));
		}
		if (ComputeSignedArea(polygon) > 0) {
			std::reverse(sortedVertices.begin(), sortedVertices.end());
		}

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
		// collect one halfedge per boundary cycle
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
		
		// Generate Hexagonal Packing
		auto [center, boundingRadius] = CalculateBoundingCircle(m_SubmeshUVBoundary);
		float gemScale = gemPatternUI.GetGemScale();
		float holeShrinkLength = gemPatternUI.GetHoleShrinkLength();
		int gridStep = 2.0 * boundingRadius / gemScale;
		float cellRadius = 0.55f * gemScale;
		float gridRotation = glm::radians(gemPatternUI.GetGridRotation());

		std::vector<CGALPoint2> targetPoints;
		
		if (gemPatternUI.GetCurSelectedPackingMode() == PackingMode::Hexagonal) {
			targetPoints = GenerateHexagonalPacking(center, gridStep, cellRadius, gridRotation);
		}
		else if (gemPatternUI.GetCurSelectedPackingMode() == PackingMode::Square) {
			targetPoints = GenerateSquarePacking(center, gridStep, cellRadius, gridRotation);
		}

		std::vector<CGALPoint2> clippedPoints = ClipToUVBoundary(targetPoints, m_SubmeshUVBoundary, cellRadius + holeShrinkLength);

		Triangulation t;
		std::map<CGALPoint2, vertex_descriptor> vertexMap;
		for (auto v : vertices(*m_CGALSubmesh)) {
			t.insert(m_SubmeshUVMap[v]);
			vertexMap[m_SubmeshUVMap[v]] = v;
		}
		std::vector<glm::vec3> positions;
		for (CGALPoint2 query : clippedPoints) {
			Triangulation::Face_handle fh = t.locate(query);
			std::vector<std::pair<CGALPoint2, double>> coords;
			double norm = CGAL::natural_neighbor_coordinates_2(t, query, std::back_inserter(coords)).second;
			CGALPoint targetPoint(0, 0, 0);
			for (const auto& pair : coords) {
				vertex_descriptor v = vertexMap[pair.first];
				double weight = pair.second / norm;
				targetPoint += weight * CGALVector(m_CGALSubmesh->point(v).x(), m_CGALSubmesh->point(v).y(), m_CGALSubmesh->point(v).z());
			}
			size_t faceID = geodesic->LocatePoint(targetPoint).first;
			std::set<size_t> selectedRegion = polyscope::state::selectedRegion.Faces();
			if (selectedRegion.find(faceID) != selectedRegion.end()) {
				positions.push_back({ targetPoint.x(), targetPoint.y(), targetPoint.z() });
			}
		}








		//Triangulation t;
		//std::vector<CGALPoint2> points;
		//std::map<CGALPoint2, vertex_descriptor> vertexMap;
		//for (auto v : vertices(*cgalSubmesh)) {
		//	t.insert(uvMap[v]);
		//	vertexMap[uvMap[v]] = v;
		//	points.push_back(uvMap[v]);
		//}

		//double min_x = std::numeric_limits<double>::max();
		//double max_x = std::numeric_limits<double>::min();
		//double min_y = std::numeric_limits<double>::max();
		//double max_y = std::numeric_limits<double>::min();
		//for (const auto& p : points) {
		//	min_x = std::min(min_x, p.x());
		//	max_x = std::max(max_x, p.x());
		//	min_y = std::min(min_y, p.y());
		//	max_y = std::max(max_y, p.y());
		//}
		//CGALPoint2 center((min_x + max_x) / 2.0, (min_y + max_y) / 2.0);
		//double maxDistance = 0.0;
		//for (const auto& p : points) {
		//	double distance = CGAL::sqrt(CGAL::squared_distance(center, p));
		//	maxDistance = std::max(maxDistance, distance);
		//}
		//int nGrid = 1.5 * maxDistance / 2.0;

		//std::vector<glm::vec3> positions;
		//for (int i = -nGrid; i <= nGrid; i++) {
		//	for (int j = -nGrid; j <= nGrid; j++) {
		//		glm::vec2 offset = { i * 2.0f, j * 2.0f };
		//		float angleRad = glm::radians(gemPatternUI.GetGridRotation());
		//		float cosTheta = std::cos(angleRad);
		//		float sinTheta = std::sin(angleRad);
		//		glm::vec2 rotatedOffset;
		//		rotatedOffset.x = offset.x * cosTheta - offset.y * sinTheta;
		//		rotatedOffset.y = offset.x * sinTheta + offset.y * cosTheta;

		//		CGALPoint2 query(center.x() + rotatedOffset.x, center.x() + rotatedOffset.y);
		//		Triangulation::Face_handle fh = t.locate(query);
		//		std::vector<std::pair<CGALPoint2, double>> coords;
		//		double norm = CGAL::natural_neighbor_coordinates_2(t, query, std::back_inserter(coords)).second;
		//		CGALPoint targetPoint(0, 0, 0);
		//		for (const auto& pair : coords) {
		//			vertex_descriptor v = vertexMap[pair.first];
		//			double weight = pair.second / norm;
		//			CGALVector tempVector = { cgalSubmesh->point(v).x(), cgalSubmesh->point(v).y(), cgalSubmesh->point(v).z() };
		//			tempVector = weight * tempVector;
		//			targetPoint += tempVector;
		//		}

		//		size_t faceID = geodesic->LocatePoint(targetPoint).first;
		//		std::set<size_t> selectedRegion = polyscope::state::selectedRegion.Faces();
		//		if (selectedRegion.find(faceID) != selectedRegion.end()) {
		//			positions.push_back({ targetPoint.x(), targetPoint.y(), targetPoint.z() });
		//		}
		//	}
		//}

		return PlaceGemsOnPositions(positions);
	}

	GemGroup PlacementTool::PlaceGemsAtTargets()
	{
		const std::unique_ptr<GeodesicTool>& geodesic = m_Scene->m_GeodesicTool;
		const GemSettingSelectionUI& gemSettingSelectionUI = m_Scene->m_GemSettingSelectionUI;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

		GemSettingType settingType = gemSettingSelectionUI.GetCurSelectedGemSetting();
		float gemExposureDepth = gemPatternUI.GetGemExposureDepth();

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
		float gemExposureDepth = gemPatternUI.GetGemExposureDepth();
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
			auto maxIter = std::max_element(m_GeodesicDistance.begin(), m_GeodesicDistance.end());
			float maxValue = *maxIter;

			std::vector<glm::vec3> vertexColors(m_Submesh->GetVertices().size());
			for (size_t i = 0; i < m_Submesh->GetVertices().size(); i++) {
				float value = m_GeodesicDistance[i] / maxValue;
				vertexColors[i] = { value, value, value };
			}
			polyscope::SurfaceVertexColorQuantity* showFaces = m_Submesh->GetPsMesh()->addVertexColorQuantity("Geodesic Distance", vertexColors);
			showFaces->setEnabled(true);
		}

		// Boolean mesh
		{
			m_Scene->AddMesh(m_BooleanMesh);
			m_BooleanMesh->GetPsMesh()->setEnabled(false);
		}


		//polyscope::SurfaceGraphQuantity* isolines = m_Submesh->GetPsMesh()->addSurfaceGraphQuantity("Isolines", positions, edgeInds);
		//isolines->setEnabled(true);
		//isolines->setRadius(0.002f);
		//isolines->setColor({ 0.0, 0.0, 0.0 });
	}

	static std::pair<CGALPoint2, float> CalculateBoundingCircle(std::vector<CGALPoint2>& points)
	{
		double min_x = std::numeric_limits<double>::max();
		double max_x = std::numeric_limits<double>::min();
		double min_y = std::numeric_limits<double>::max();
		double max_y = std::numeric_limits<double>::min();
		for (const auto& p : points) {
			min_x = std::min(min_x, p.x());
			max_x = std::max(max_x, p.x());
			min_y = std::min(min_y, p.y());
			max_y = std::max(max_y, p.y());
		}
		CGALPoint2 center((min_x + max_x) / 2.0, (min_y + max_y) / 2.0);
		double radius = 0.0;
		for (const auto& p : points) {
			double distance = CGAL::sqrt(CGAL::squared_distance(center, p));
			radius = std::max(radius, distance);
		}
		return { center, radius };
	}

	static std::vector<CGALPoint2> GenerateSquarePacking(CGALPoint2 center, int gridStep, float cellRadius, float gridRotation)
	{
		std::vector<CGALPoint2> points;
		float dx = 2 * cellRadius;	// horizontal spacing
		float dy = 2 * cellRadius;	// vertical spacing
		float cosTheta = std::cos(gridRotation);
		float sinTheta = std::sin(gridRotation);

		for (int i = -gridStep; i <= gridStep; i++) {
			for (int j = -gridStep; j <= gridStep; j++) {
				glm::vec2 offset{ i * dx, j * dy };
				glm::vec2 rotatedOffset;
				rotatedOffset.x = offset.x * cosTheta - offset.y * sinTheta;
				rotatedOffset.y = offset.x * sinTheta + offset.y * cosTheta;
				points.push_back(CGALPoint2(center.x() + rotatedOffset.x, center.y() + rotatedOffset.y));
			}
		}
		return points;
	}

	static std::vector<CGALPoint2> GenerateHexagonalPacking(CGALPoint2 center, int gridStep, float cellRadius, float gridRotation)
	{
		std::vector<CGALPoint2> points;
		float dx = cellRadius * sqrt(3) * 2.0f;	// horizontal spacing
		float dy = cellRadius;					// vertical spacing
		float cosTheta = std::cos(gridRotation);
		float sinTheta = std::sin(gridRotation);

		for (int i = -gridStep; i <= gridStep; i++) {
			for (int j = -gridStep; j <= gridStep; j++) {
				glm::vec2 offset{ i * dx, j * dy };
				if (j % 2 != 0) {
					offset.x += dx * 0.5f; // even row offset
				}
				glm::vec2 rotatedOffset;
				rotatedOffset.x = offset.x * cosTheta - offset.y * sinTheta;
				rotatedOffset.y = offset.x * sinTheta + offset.y * cosTheta;
				points.push_back(CGALPoint2(center.x() + rotatedOffset.x, center.y() + rotatedOffset.y));
			}
		}
		return points;
	}

	static std::vector<CGALPoint2> ClipToUVBoundary(const std::vector<CGALPoint2>& points, const std::vector<CGALPoint2>& boundary, float radius)
	{
		std::vector<CGALPoint2> clippedPoints;
		CGAL::Polygon_2<Kernel> polygon(boundary.begin(), boundary.end());

		for (const auto& point : points) {
			bool flag = true;
			int nSamples = 100;
			for (int i = 0; i < nSamples; i++) {
				float angle = 2.0f * glm::pi<float>() * i / nSamples;
				CGALPoint2 pointOnCircle{ point.x() + radius * std::cos(angle), point.y() + radius * std::sin(angle) };
				if (polygon.bounded_side(pointOnCircle) != CGAL::ON_BOUNDED_SIDE) {
					flag = false;
					break;
				}
			}
			//if (polygon.bounded_side(point) != CGAL::ON_BOUNDED_SIDE) {
			//	flag = false;
			//}
			if (flag) {
				clippedPoints.push_back(point);
			}
		}
		return clippedPoints;
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