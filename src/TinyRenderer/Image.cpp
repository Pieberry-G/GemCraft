#include "TinyRenderer/Image.h"

#include "TinyRenderer/OpenGLImage.h"

#include <stb_image.h>

namespace GemCraft {

	std::shared_ptr<Image> Image::Create(const std::string& filepath)
	{
        unsigned char* data;
        int width, height, component;
        stbi_set_flip_vertically_on_load(1);
        data = stbi_load(filepath.c_str(), &width, &height, &component, 0);

        std::shared_ptr<Image> image;
        if (data) {
            // Get the image data from stb_image
            unsigned char* buffer = nullptr;
            int32_t bufferSize = 0;
            bool deleteBuffer = false;
            // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in OpenGL
            if (component == 1) {
                bufferSize = width * height * 4;
                buffer = new unsigned char[bufferSize];
                unsigned char* rgba = buffer;
                unsigned char* rgb = data;
                for (size_t i = 0; i < width * height; ++i) {
                    *(rgba) = *(rgb);
                    *(rgba + 1) = *(rgb);
                    *(rgba + 2) = *(rgb);
                    *(rgba + 3) = 255;
                    rgba += 4;
                    rgb += 1;
                }
                deleteBuffer = true;
            }
            else if (component == 3) {
                bufferSize = width * height * 4;
                buffer = new unsigned char[bufferSize];
                unsigned char* rgba = buffer;
                unsigned char* rgb = data;
                for (size_t i = 0; i < width * height; ++i) {
                    memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                    *(rgba + 3) = 255;
                    rgba += 4;
                    rgb += 3;
                }
                deleteBuffer = true;
            }
            else {
                buffer = data;
                bufferSize = width * height * 4;
            }

            image = Image::Create(width, height, buffer);

            stbi_image_free(data);
            if (deleteBuffer) {
                delete[] buffer;
            }
        }
        return image;
	}

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