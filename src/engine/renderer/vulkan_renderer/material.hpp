#pragma once

namespace arx
{
	struct MaterialPropertyBufferObject
	{
		alignas(4) vk::Bool32 enable_diffuse_map;
		alignas(4) vk::Bool32 enable_normal_map;
		alignas(16) glm::vec4 default_color;
	};

	struct Material
	{
	public:
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		Material(vk::DescriptorSet descriptor_set, MaterialPropertyBufferObject property_buffer_object, vk::Buffer property_buffer, vk::DeviceMemory property_buffer_memory, Texture* diffuse_map, Texture* normal_map) : descriptor_set{ descriptor_set },
			property_buffer_object{ property_buffer_object },
			property_buffer{ property_buffer },
			property_buffer_memory{ property_buffer_memory },
			diffuse_map{ diffuse_map },
			normal_map{ normal_map } {
		}
		~Material() {
			// TODO
		}

	public:
		static auto get_descriptor_set_layout_bindings() -> std::array<vk::DescriptorSetLayoutBinding, 3> {
			vk::DescriptorSetLayoutBinding property_layout_binding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment, { });
			vk::DescriptorSetLayoutBinding diffuse_map_layout_binding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, { });
			vk::DescriptorSetLayoutBinding normal_map_layout_binding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, { });

			return { property_layout_binding, diffuse_map_layout_binding, normal_map_layout_binding };
			//return std::make_tuple(vk::DescriptorSetLayoutCreateInfo{ { }, bindings });
		}

	public:
		vk::DescriptorSet descriptor_set;
		MaterialPropertyBufferObject property_buffer_object;
		vk::Buffer property_buffer;
		vk::DeviceMemory property_buffer_memory;
		Texture* diffuse_map;
		Texture* normal_map;
	};
}