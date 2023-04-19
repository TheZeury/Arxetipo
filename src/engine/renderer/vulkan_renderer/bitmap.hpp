#pragma once

namespace arx
{
	struct Bitmap
	{
	public:
		using CharInfo = stbtt_bakedchar;

	public:
		Bitmap(const Bitmap&) = delete;
		Bitmap& operator=(const Bitmap&) = delete;
		Bitmap(Bitmap&&) = delete;
		Bitmap& operator=(Bitmap&&) = delete;

		Bitmap(vk::DescriptorSet descriptor_set, Texture* map) : descriptor_set{ descriptor_set }, map{ map } {
		}
		Bitmap(vk::DescriptorSet descriptor_set, Texture* map, const std::array<CharInfo, 128>& char_infos) : descriptor_set{ descriptor_set }, map{ map }, char_infos{ char_infos } {
		}

		static auto get_descriptor_set_layout_bindings() -> std::array<vk::DescriptorSetLayoutBinding, 1> {
			vk::DescriptorSetLayoutBinding bitmap_layout_binding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, { });

			return { bitmap_layout_binding };
		}

	public:
		vk::DescriptorSet descriptor_set;
		Texture* map;
		std::array<CharInfo, 128> char_infos = { };
		glm::vec2 map_extent = { 1024.f, 1024.f };
		float height = 150.f;
	};
}