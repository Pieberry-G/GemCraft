#include "Mesh/RegionSubmesh.h"

#include "Mesh/FormatTool.h"
#include "Mesh/GeodesicTool.h"

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

	static double ComputeSignedArea(const std::vector<CGALPoint2>& polygon);

	RegionSubmesh::RegionSubmesh(std::shared_ptr<Mesh>& mesh, MeshSubset selectedRegion)
	{
		BuildSubmesh(mesh, selectedRegion);
		ParameterizeSubmesh();
		CalculateGeodesicDistance();
	}

	std::vector<glm::vec2> RegionSubmesh::GetBoundary()
	{
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());
		std::vector<glm::vec2> boundary;
		halfedge_descriptor bhd = CGALpmp::longest_border(*cgalSubmesh).first;
		for (halfedge_descriptor h : halfedges_around_face(bhd, *cgalSubmesh)) {
			CGALMesh::Vertex_index v = target(h, *cgalSubmesh);
			boundary.push_back(m_UVCoords[v]);
		}
		return boundary;
	}

	std::vector<glm::vec3> RegionSubmesh::Map2DPointsTo3D(std::vector<glm::vec2>& points)
	{
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());
		Triangulation t;
		std::map<CGALPoint2, vertex_descriptor> vertexMap;
		for (auto v : vertices(*cgalSubmesh)) {
			t.insert({ m_UVCoords[v].x, m_UVCoords[v].y });
			vertexMap[{ m_UVCoords[v].x, m_UVCoords[v].y }] = v;
		}
		GeodesicTool submeshGeodesic(m_Submesh);
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
			positions.push_back({ targetPoint.x(), targetPoint.y(), targetPoint.z() });
		}
		return positions;
	}

	std::shared_ptr<Mesh>& RegionSubmesh::CreateMeshForBooleanHole(float holeDepth, float shrinkLength)
	{
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());
		GeodesicTool submeshGeodesic(m_Submesh);

		std::vector<glm::vec3> sortedVertices = ExtractBooleanMeshBoundary(shrinkLength);

		std::shared_ptr<CGALMesh> cgalBooleanMesh = std::make_shared<CGALMesh>();
		glm::vec3 normal;
		int len = sortedVertices.size();
		for (int i = 0; i < len; i++) {
			normal = submeshGeodesic.CalculateNormal(sortedVertices[i]);
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

		double targetEdgeLength = 0.1;
		std::vector<edge_descriptor> border;
		CGALpmp::border_halfedges(faces(*cgalBooleanMesh), *cgalBooleanMesh, boost::make_function_output_iterator(HalfedgeToEdge(*cgalBooleanMesh, border)));
		CGALpmp::split_long_edges(border, targetEdgeLength, *cgalBooleanMesh);
		CGALpmp::isotropic_remeshing(faces(*cgalBooleanMesh), targetEdgeLength, *cgalBooleanMesh,
			CGAL::parameters::number_of_iterations(10)
			.protect_constraints(true));
		cgalBooleanMesh->collect_garbage();

		std::ofstream out("boolean_mesh.obj");
		CGAL::IO::write_OBJ(out, *cgalBooleanMesh);
		out.close();

		m_BooleanMesh = FormatTool::CGALMeshToMesh(cgalBooleanMesh, glm::mat4(1.0f));
		m_BooleanMesh->SetName("Boolean Mesh");

		return m_BooleanMesh;
	}

	void RegionSubmesh::BuildSubmesh(std::shared_ptr<Mesh>& mesh, MeshSubset selectedRegion)
	{
		GC_CORE_WARN("Building submesh.");

		const std::vector<glm::vec3>& originVertices = mesh->GetVertices();
		const std::vector<std::vector<size_t>>& originFaces = mesh->GetFaces();
		std::vector<std::vector<size_t>> newFaces;
		for (auto& index : selectedRegion.Faces()) {
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

	void RegionSubmesh::ParameterizeSubmesh()
	{
		GC_CORE_WARN("Parameterizing submesh.");

		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());
		halfedge_descriptor bhd = CGALpmp::longest_border(*cgalSubmesh).first;
		UV_pmap uvMap = cgalSubmesh->add_property_map<vertex_descriptor, CGALPoint2>("h:uv").first;
		CGALsmp::ARAP_parameterizer_3<CGALMesh> parameterizer;
		CGALsmp::Error_code err = CGALsmp::parameterize(*cgalSubmesh, parameterizer, bhd, uvMap);
		for (vertex_descriptor v : vertices(*cgalSubmesh)) {
			m_UVCoords.push_back({ uvMap[v].x(), uvMap[v].y() });
		}

		std::ofstream out("parameterize.off");
		CGALsmp::IO::output_uvmap_to_off(*cgalSubmesh, bhd, uvMap, out);

		GC_CORE_INFO("Completed!");
	}

	void RegionSubmesh::CalculateGeodesicDistance()
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

	std::vector<glm::vec3> RegionSubmesh::ExtractBooleanMeshBoundary(float shrinkLength)
	{
		std::shared_ptr<CGALMesh> cgalSubmesh = FormatTool::MeshToCGALMesh(m_Submesh, m_Submesh->GetPsTransform());

		std::vector<glm::vec3> positions;
		std::vector<std::array<size_t, 2>> edgeInds;
		if (shrinkLength == 0.0f) {
			halfedge_descriptor bhd = CGALpmp::longest_border(*cgalSubmesh).first;
			for (halfedge_descriptor hed : halfedges_around_face(bhd, *cgalSubmesh)) {
				vertex_descriptor p0 = source(hed, *cgalSubmesh);
				vertex_descriptor p1 = target(hed, *cgalSubmesh);
				positions.push_back({ cgalSubmesh->point(p0).x(), cgalSubmesh->point(p0).y(), cgalSubmesh->point(p0).z() });
				positions.push_back({ cgalSubmesh->point(p1).x(), cgalSubmesh->point(p1).y(), cgalSubmesh->point(p1).z() });
				edgeInds.push_back({ positions.size() - 2, positions.size() - 1 });
			}
		}
		else {
			for (auto& face : m_Submesh->GetFaces()) {
				std::vector<glm::vec3> pos;
				for (size_t i = 0; i < face.size(); i++) {
					float vs = m_GeodesicDistances[face[i]];
					float vd = m_GeodesicDistances[face[(i + 1) % face.size()]];
					int region1 = floor(vs / shrinkLength);
					int region2 = floor(vd / shrinkLength);
					if ((region1 == 0 && region2 == 1) || (region1 == 1 && region2 == 0)) {
						double val = region1 > region2 ? region1 * shrinkLength : region2 * shrinkLength;
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
					if (std::min(dis0, dis1) < minDistance) {
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

		GeodesicTool submeshGeodesic(m_Submesh);
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

	void RegionSubmesh::ShowResult(Scene* scene)
	{
		// Submesh
		{
			scene->AddMesh(m_Submesh);
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

	void RegionSubmesh::ShowBooleanMesh(Scene* scene)
	{
		// Boolean mesh
		{
			scene->AddMesh(m_BooleanMesh);
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