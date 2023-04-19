#pragma once

namespace arx
{
	struct UIVertex
	{
		glm::vec2 position;
		glm::vec2 uv;
		glm::vec4 color;

		static auto get_binding_descriptions() -> std::array<vk::VertexInputBindingDescription, 1>
		{
			return {
				vk::VertexInputBindingDescription{ 0, sizeof(UIVertex), vk::VertexInputRate::eVertex },
			};
		}

		static auto get_attribute_descriptions() -> std::array<vk::VertexInputAttributeDescription, 3>
		{
			return {
				vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32Sfloat, offsetof(UIVertex, position) },
				vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32Sfloat, offsetof(UIVertex,uv) },
				vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(UIVertex, color) },
			};
		}
	};

	struct UIElement
	{
	public:
		using Vertex = UIVertex;

	public:
		UIElement(
			uint32_t vertex_count, uint32_t index_count,
			vk::Buffer vertex_buffer, vk::DeviceMemory vertex_buffer_memory,
			vk::Buffer index_buffer, vk::DeviceMemory index_buffer_memory
		) :
			vertex_count{ vertex_count }, index_count{ index_count },
			vertex_buffer{ vertex_buffer }, vertex_buffer_memory{ vertex_buffer_memory },
			index_buffer{ index_buffer }, index_buffer_memory{ index_buffer_memory } {
		}

	public:
		uint32_t vertex_count;
		uint32_t index_count;
		vk::Buffer vertex_buffer;
		vk::DeviceMemory vertex_buffer_memory;
		vk::Buffer index_buffer;
		vk::DeviceMemory index_buffer_memory;
	};
}