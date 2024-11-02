#include "Mesh/BooleanOperation.h"
#include "Mesh/Utils.h"

namespace GemCraft {

	std::shared_ptr<Mesh> BooleanOperation::DifferenceOperation(std::shared_ptr<Mesh> mainMesh, std::vector<std::shared_ptr<Mesh>> meshedToSubstract)
	{
		glm::mat4 transform = mainMesh->GetPsTransform();
		glm::mat4 inverseTransform = glm::inverse(transform);
		std::shared_ptr<CGALMesh> mesh1 = Utils::MeshToCGALMesh(mainMesh);

		for (size_t i = 0; i < meshedToSubstract.size(); i++) {
			if (meshedToSubstract[i]->GetBooleanOpLock()) continue;

			meshedToSubstract[i]->SetBooleanOpLock(true);
			std::shared_ptr<CGALMesh> mesh2 = Utils::MeshToCGALMesh(meshedToSubstract[i]);
			mesh2 = ConstructContexHull(mesh2);
			//create a property on edges to indicate whether they are constrained
			CGALMesh::Property_map<edge_descriptor, bool> is_constrained_map =
				mesh1->add_property_map<edge_descriptor, bool>("e:is_constrained", false).first;
			// update mesh1 to contain the mesh bounding the difference
			// of the two input volumes.
			bool valid_difference =
				CGALpmp::corefine_and_compute_difference(*mesh1,
					*mesh2,
					*mesh1,
					CGALparams::default_values(), // default parameters for mesh1
					CGALparams::default_values(), // default parameters for mesh2
					CGALparams::edge_is_constrained_map(is_constrained_map));

			if (valid_difference) {
				GC_CORE_INFO("Difference was successfully computed.");
			} else {
				GC_CORE_ERROR("Difference could not be computed.");
			}
		}
		return Utils::CGALMeshToMesh(mesh1, inverseTransform);
	}

	std::shared_ptr<CGALMesh> BooleanOperation::ConstructContexHull(std::shared_ptr<CGALMesh> mesh)
	{
		std::shared_ptr<CGALMesh> contexHull = std::make_shared<CGALMesh>();
		CGAL::convex_hull_3(mesh->points().begin(), mesh->points().end(), *contexHull);
		GC_CORE_INFO("The convex hull contains {0} vertices.", num_vertices(*contexHull));

		return contexHull;
	}

} // namespace GemCraft