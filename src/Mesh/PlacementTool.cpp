#include "Mesh/PlacementTool.h"

#include "Core/Scene.h"
#include "Core/ResourceManager.h"

#include "Mesh/GeodesicTool.h"
#include "Mesh/Packing2D.h"

namespace GemCraft {

	void PlacementTool::Clean()
	{
		m_Submesh = nullptr;
	}

	void PlacementTool::BuildSubmeshForSelectedRegion()
	{
		std::shared_ptr<Mesh>& ring = m_Scene->GetRing();
		m_Submesh = std::make_shared<RegionSubmesh>(ring, polyscope::state::selectedRegion);
	}

	std::shared_ptr<Mesh> PlacementTool::CreateMeshForBooleanHole()
	{
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;
		bool enableHoleShrink = gemPatternUI.GetEnableHoleShrink();
		float shrinkLength = gemPatternUI.GetHoleShrinkLength();
		if (!enableHoleShrink) {
			shrinkLength = 0.0f;
		}
		float holeDepth = gemPatternUI.GetHoleDepth();
		return m_Submesh->CreateMeshForBooleanHole(holeDepth, shrinkLength);
	}

	GemGroup PlacementTool::PlaceGemsOnSelectedRegion()
	{
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

		// Generate Packing
		Packing2D packing2D;
		float gemScale = gemPatternUI.GetGemScale();
		bool enableHoleShrink = gemPatternUI.GetEnableHoleShrink();
		float holeShrinkLength = gemPatternUI.GetHoleShrinkLength();
		float gridRotation = glm::radians(gemPatternUI.GetGridRotation());
		std::vector<glm::vec2> boundary = m_Submesh->GetBoundary();

		std::vector<glm::vec2> targetPoints;
		float cellRadius = 0.6f * gemScale;
		float shrinkLength = holeShrinkLength + 0.1f * cellRadius;
		if (!enableHoleShrink) {
			shrinkLength = 0.1f * cellRadius;
		}
		if (gemPatternUI.GetCurSelectedPackingMode() == PackingMode::Hexagonal) {
			targetPoints = packing2D.GenerateHexagonalPacking(cellRadius, gridRotation, boundary, 0.0f);
		}
		else if (gemPatternUI.GetCurSelectedPackingMode() == PackingMode::Square) {
			targetPoints = packing2D.GenerateSquarePacking(cellRadius, gridRotation, boundary, shrinkLength);
		}
		else if (gemPatternUI.GetCurSelectedPackingMode() == PackingMode::Compact) {
			targetPoints = packing2D.GenerateCompactPacking(cellRadius, gridRotation, boundary, shrinkLength, gemPatternUI.GetPackingEdgeLoopDensity(), gemPatternUI.GetPackingCenterDensity());
		}

		std::vector<glm::vec3> positions = m_Submesh->Map2DPointsTo3D(targetPoints);
		return PlaceGemsOnPositions(positions);
	}

	GemGroup PlacementTool::PlaceGemsAtTargets()
	{
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
		m_Submesh->ShowResult(m_Scene);
	}

	void PlacementTool::ShowBooleanMesh()
	{
		m_Submesh->ShowBooleanMesh(m_Scene);
	}

} // namespace GemCraft