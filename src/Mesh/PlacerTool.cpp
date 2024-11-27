#include "Mesh/PlacerTool.h"

#include "Core/Scene.h"
#include "Core/ResourceManager.h"

namespace GemCraft {

	std::shared_ptr<Mesh> PlacerTool::PlaceGem(const std::string& name, const std::string& filepath, const glm::mat4& transform)
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

	std::shared_ptr<Mesh> PlacerTool::PlaceGem(GemSpecification spec)
	{
		glm::vec3 shiftedPosition = spec.Position + spec.ExposureDepth * spec.Normal;
		glm::quat rotation = glm::angleAxis(glm::radians(spec.Rotation + 90.0f), spec.Normal);
		glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
		glm::vec3 rotatedForward = glm::vec3(rotationMatrix * glm::vec4(spec.Forward, 0.0f));
		glm::mat4 transform = glm::inverse(glm::lookAt(shiftedPosition, shiftedPosition + spec.Forward, spec.Normal));
		transform = glm::rotate(transform, glm::radians(-90.0f), { 1.0f, 0.0f, 0.0f });
		transform = glm::scale(transform, glm::vec3(spec.Scale));

		return PlaceGem(spec.Name, spec.Filepath, transform);
	}

	std::shared_ptr<Mesh> PlacerTool::PlaceGemSetting(const std::string& name, GemSettingType settingType, const glm::mat4& transform)
	{
		std::shared_ptr<Mesh> gemSetting = ResourceManager::Get()->CreateGemSetting(settingType);

		gemSetting->SetName(name);
		gemSetting->AddToPolyscope(transform);
		gemSetting->GetPsMesh()->setMaterial("clay");
		gemSetting->GetPsMesh()->setSurfaceColor(glm::vec3(0.750, 0.750, 0.750));

		return gemSetting;
	}

	std::shared_ptr<Mesh> PlacerTool::PlaceGemSetting(GemSettingSpecification spec)
	{
		glm::vec3 shiftedPosition = spec.Position + spec.ExposureDepth * spec.Normal;
		glm::quat rotation = glm::angleAxis(glm::radians(spec.Rotation + 90.0f), spec.Normal);
		glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
		glm::vec3 rotatedForward = glm::vec3(rotationMatrix * glm::vec4(spec.Forward, 0.0f));
		glm::mat4 transform = glm::inverse(glm::lookAt(shiftedPosition, shiftedPosition + rotatedForward, spec.Normal));
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

	GemLine PlacerTool::PlaceGemsOnPath(const Path& path)
	{
		const std::unique_ptr<Geodesic>& geodesic = m_Scene->m_Geodesic;
		const GemSelectionUI& gemSelectionUI = m_Scene->m_GemSelectionUI;
		const GemSettingSelectionUI& gemSettingSelectionUI = m_Scene->m_GemSettingSelectionUI;
		const GemPatternUI& gemPatternUI = m_Scene->m_GemPatternUI;

		GemSettingType settingType = gemSettingSelectionUI.GetCurSelectedGemSetting();
		float exposureDepth = gemPatternUI.GetExposureDepth();
		if (settingType == GemSettingType::Shovel || settingType == GemSettingType::Channel) exposureDepth = 0.0f;

		float gemScale = gemPatternUI.GetGemScale();
		float gemRotation = gemPatternUI.GetGemRotation();
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
			spec.Rotation = gemRotation;
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
					spec.Rotation = gemRotation;
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
				spec.Rotation = gemRotation;
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
				spec.Rotation = gemRotation;
				std::stringstream ss;
				ss << std::fixed << std::setprecision(3) << "GemSetting (" << spec.Position.x << "," << spec.Position.y << "," << spec.Position.z << ")";
				spec.Name = ss.str();
				gemSettings.push_back(PlaceGemSetting(spec));
			}
		}

		return GemLine(settingType, gems, gemSettings, path, gemScale);
	}

} // namespace GemCraft