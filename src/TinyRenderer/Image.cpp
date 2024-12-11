#include "TinyRenderer/Image.h"

#include "TinyRenderer/OpenGLImage.h"

namespace GemCraft {

	std::shared_ptr<Image> Image::Create(uint32_t width, uint32_t height)
	{
		return std::make_shared<OpenGLImage>(width, height);
	}

	std::shared_ptr<Image> Image::Create(uint32_t width, uint32_t height, void* data)
	{
		std::shared_ptr<Image> image = Image::Create(width, height);
		if (data) {
			image->SetData(data);
		}
		return image;
	}

} // namespace GemCraft