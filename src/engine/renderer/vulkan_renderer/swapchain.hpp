#include <vulkan/vulkan.hpp>

namespace arx
{
	struct Swapchain
	{
	public:
		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;
		Swapchain(Swapchain&&) = delete;
		Swapchain& operator=(Swapchain&&) = delete;

		Swapchain(
			uint32_t length,
			const std::vector<vk::Image>& images,
			const std::vector<vk::ImageView>& image_views,
			const std::vector<vk::Framebuffer>& framebuffers,
			vk::Image depth_image,
			vk::DeviceMemory depth_image_memory,
			vk::ImageView depth_image_view,
			vk::Format format,
			vk::Rect2D rect
		) : length{ length },
			images{ images },
			image_views{ image_views },
			framebuffers{ framebuffers },
			depth_image{ depth_image },
			depth_image_memory{ depth_image_memory },
			format{ format },
			rect{ rect }
		{

		}
		~Swapchain() {
			// TODO
		}

		auto get_render_pass_begin_info(uint32_t image_index) -> vk::RenderPassBeginInfo {
			assert(render_pass != nullptr);
			std::array<vk::ClearValue, 2> clear_values = {
				vk::ClearValue{ vk::ClearColorValue{ std::array<float, 4>{ 0.3f, 0.4f, 0.7f, 1.f } } },
				vk::ClearValue{ vk::ClearDepthStencilValue{ 1.f, 0 } },
			};
			vk::RenderPassBeginInfo render_pass_info(render_pass, framebuffers[image_index], { { }, { rect.extent.width, rect.extent.height } }, clear_values);
			return render_pass_info;
		}

		auto getViewport() -> vk::Viewport* {
			viewport = vk::Viewport{ 0.f, 0.f, static_cast<float>(rect.extent.width), static_cast<float>(rect.extent.height), 0.f, 1.f };
			return &viewport;
		}
		auto getScissor() -> vk::Rect2D* {
			return &rect;
		}


	public:
		uint32_t length;
		std::vector<vk::Image> images;
		std::vector<vk::ImageView> image_views;
		std::vector<vk::Framebuffer> framebuffers;
		vk::Image depth_image;
		vk::DeviceMemory depth_image_memory;
		vk::ImageView depth_image_view;
		vk::Format format = vk::Format::eUndefined;
		vk::Rect2D rect = { };
		vk::Viewport viewport = { };

		vk::RenderPass render_pass;
	};
}