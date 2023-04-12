#pragma once

namespace arx
{
	struct Texture
	{
	public:
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		Texture(Texture&& rhs) {
			if (rhs.moved) {
				moved = true;
				return;
			}
			mip_levels = std::move(rhs.mip_levels);
			texture_image_view = std::move(rhs.texture_image_view);
			texture_sampler = std::move(rhs.texture_sampler);
			texture_image = std::move(rhs.texture_image);
			texture_image_memory = std::move(rhs.texture_image_memory);
			moved = false;
			rhs.moved = true;
		}
		Texture& operator=(Texture&& rhs) {
			if (rhs.moved) {
				moved = true;
				return *this;
			}
			mip_levels = std::move(rhs.mip_levels);
			texture_image_view = std::move(rhs.texture_image_view);
			texture_sampler = std::move(rhs.texture_sampler);
			texture_image = std::move(rhs.texture_image);
			texture_image_memory = std::move(rhs.texture_image_memory);
			moved = false;
			rhs.moved = true;
			return *this;
		}

		Texture(uint32_t mip_levels, vk::ImageView texture_image_view, vk::Sampler texture_sampler, vk::Image texture_image, vk::DeviceMemory texture_image_memory)
			: mip_levels{ mip_levels }, texture_image_view{ texture_image_view }, texture_sampler{ texture_sampler }, texture_image{ texture_image }, texture_image_memory{ texture_image_memory }, moved{ false } {

		}
		~Texture() {
			if (moved) return;
			// TODO
		}
		// auto clone() noexcept -> Texture; TODO
	public:
		uint32_t mip_levels;
		vk::ImageView texture_image_view;
		vk::Sampler texture_sampler;
		vk::Image texture_image;
		vk::DeviceMemory texture_image_memory;
	private:
		bool moved = false;
	};
}