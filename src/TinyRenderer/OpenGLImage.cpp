#include "TinyRenderer/OpenGLImage.h"

namespace GemCraft {

	OpenGLImage::OpenGLImage(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		glGenTextures(1, &m_RendererID);

		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	OpenGLImage::~OpenGLImage()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLImage::SetData(void* data)
	{
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}

	void OpenGLImage::Bind(uint32_t slot) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);
		glActiveTexture(GL_TEXTURE0);
	}

	std::vector<std::array<float, 4>> OpenGLImage::ReadBuffer() const
	{
		size_t buffSize = m_Width * m_Height * 4;
		std::vector<std::array<float, 4>> outData;
		outData.resize(buffSize);
		Bind();
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, static_cast<void*>(&outData.front()));
		return outData;
	}

} // namespace GemCraft