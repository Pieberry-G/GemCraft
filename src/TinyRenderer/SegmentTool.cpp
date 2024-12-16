#include "TinyRenderer/SegmentTool.h"

#include <filesystem>

namespace GemCraft {

	void SegmentTool::Segment()
	{
		std::filesystem::create_directories("../dataIO/InputImages");
		std::filesystem::create_directories("../dataIO/OutputMasks");

		GC_CORE_WARN("Performing segmentation prediction with SAM-Adapter.");
		GC_CORE_TRACE("Waiting...");
		system("..\\deps\\sam-adapter\\SegmentInfer.bat");
		GC_CORE_INFO("Segmentation prediction completed!\n");
	}

} // namespace GemCraft