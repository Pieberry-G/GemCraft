#pragma once

namespace GemCraft {

	class Scene;
	class PreprocessTool
	{
	public:
		PreprocessTool(Scene* scene)
			: m_Scene(scene) {}

		void PreprocessRing();

	private:
		void RenderMultiviewImages();
		void Segment();
		void InverseProjection();

	private:
		Scene* m_Scene;
	};

} // namespace GemCraft