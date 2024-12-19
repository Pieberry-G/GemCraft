#pragma once

#include "Core/Base.h"

namespace GemCraft {

	class Image
	{
	public:
		virtual ~Image() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual void SetData(void* data) = 0;
		
		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual std::vector<std::array<float, 4>> ReadBuffer() const = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const Image& other) const = 0;

		static std::shared_ptr<Image> Create(const std::string& filepath);
		static std::shared_ptr<Image> Create(uint32_t width, uint32_t height);
		static std::shared_ptr<Image> Create(uint32_t width, uint32_t height, void* data);
	};

} // namespace GemCraft