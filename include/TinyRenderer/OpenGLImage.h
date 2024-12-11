#pragma once

#include "TinyRenderer/Image.h"

#include <glad/glad.h>

namespace GemCraft {

	class OpenGLImage : public Image
	{
	public:
		OpenGLImage(uint32_t width, uint32_t height);
		virtual ~OpenGLImage();

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetRendererID() const override { return m_RendererID; }

		virtual void SetData(void* data) override;

		virtual void Bind(uint32_t slot = 0) const override;

		virtual bool IsLoaded() const override { return m_IsLoaded; }

		virtual bool operator==(const Image& other) const override
		{
			return m_RendererID == ((OpenGLImage&)other).m_RendererID;
		}
	private:
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
	};

} // namespace GemCraft