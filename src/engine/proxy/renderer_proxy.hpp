#pragma once

#define XR_USE_GRAPHICS_API_VULKAN
#include <vulkan/vulkan.hpp>
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>

namespace arx
{
	template<typename P>
	concept VulkanReceivingProxy = requires(P proxy, vk::InstanceCreateInfo const& instance_info, vk::DeviceCreateInfo const& device_info)
	{
		{ proxy.create_instance(instance_info) } -> std::same_as<vk::Instance>;
		{ proxy.get_physical_device() } -> std::same_as<vk::PhysicalDevice>;
		{ proxy.create_logical_device(device_info) } -> std::same_as<vk::Device>;
		proxy.passin_queue_info(0ui32, 0ui32);
		{ proxy.get_swapchain_images() } -> std::same_as<std::vector<std::vector<vk::Image>>>;
		{ proxy.get_swapchain_rects() } -> std::same_as<std::vector<vk::Rect2D>>;
		{ proxy.get_swapchain_format() } -> std::same_as<vk::Format>;
	};

	template<typename P>
	concept OpenXrProvidingProxy = requires(P proxy, std::vector<xr::Swapchain>&swapchains, std::vector<xr::Rect2Di>&rects, int64_t swapchain_format)
	{
		typename P::GraphicsBindingType;
			requires std::derived_from<typename P::GraphicsBindingType, xr::impl::InputStructBase>;
		P{ xr::Instance{ }, xr::SystemId{ } };
		{ proxy.get_graphics_binding() } -> std::same_as<typename P::GraphicsBindingType>;
		proxy.passin_xr_swapchains(swapchains, rects, swapchain_format);
	};

	struct VulkanOpenXrProxy
	{
	public: // Vulkan
		auto create_instance(vk::InstanceCreateInfo const& create_info) -> vk::Instance {
			VkInstanceCreateInfo inst_info = create_info;
			xr::VulkanInstanceCreateInfoKHR xr_create_info(xr_system_id, {}, &vkGetInstanceProcAddr, &inst_info, nullptr);
			VkResult vkResult;
			xr_instance.createVulkanInstanceKHR(xr_create_info, &vk_instance, &vkResult, dispatcher);
			if (vkResult != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Vulkan instance.");
			}
			return vk_instance;
		}
		auto get_physical_device() -> vk::PhysicalDevice {
			xr::VulkanGraphicsDeviceGetInfoKHR get_info(xr_system_id, vk_instance);
			vk_physical_device = xr_instance.getVulkanGraphicsDevice2KHR(get_info, dispatcher);
			return vk_physical_device;
		}
		auto create_logical_device(vk::DeviceCreateInfo const& create_info) -> vk::Device {
			VkDeviceCreateInfo device_info = create_info;

			xr::VulkanDeviceCreateInfoKHR xr_create_info(
				xr_system_id,
				{ },
				&vkGetInstanceProcAddr,
				vk_physical_device,
				&device_info,
				nullptr
			);

			VkResult vkResult;
			xr_instance.createVulkanDeviceKHR(xr_create_info, &vk_logical_device, &vkResult, dispatcher);
			if (vkResult != VK_SUCCESS)
			{
				throw std::runtime_error("Can't create logical device.");
			}

			return vk_logical_device;
		}
		auto passin_queue_info(uint32_t queue_family_index, uint32_t queue_index) -> void {
			vk_queue_family_index = queue_family_index;
			vk_queue_index = queue_index;
		}
		auto get_swapchain_images() -> std::vector<std::vector<vk::Image>> {
			return vk_swapchain_images;
		}
		auto get_swapchain_rects() -> std::vector<vk::Rect2D> {
			return vk_swapchain_rects;
		}
		auto get_swapchain_format() -> vk::Format {
			return vk_swapchain_format;
		}

	public: // OpenXR
		using GraphicsBindingType = xr::GraphicsBindingVulkanKHR;
		VulkanOpenXrProxy(xr::Instance instance, xr::SystemId system_id) : xr_instance{ instance }, xr_system_id{ system_id }, dispatcher{ instance } {
		}
		static auto get_renderer_specific_xr_extensions() -> std::vector<const char*> {
			return { XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME };
		}
		auto get_graphics_binding() -> GraphicsBindingType {
			// Get VulkanGraphicsRequirements.
			xr::GraphicsRequirementsVulkanKHR requirement = xr_instance.getVulkanGraphicsRequirements2KHR(xr_system_id, dispatcher);
			log_info("Vulkan", "Vulkan API Version Information: ", 0);
			log_info("Vulkan",
				std::format("Lowest supported API version = {}.{}.{} ",
					requirement.minApiVersionSupported.major(),
					requirement.minApiVersionSupported.minor(),
					requirement.minApiVersionSupported.patch()
				),
				1);
			log_info("Vulkan",
				std::format("Highest verified API version = {}.{}.{} ",
					requirement.maxApiVersionSupported.major(),
					requirement.maxApiVersionSupported.minor(),
					requirement.maxApiVersionSupported.patch()
				),
				1);

			return xr::GraphicsBindingVulkanKHR(vk_instance, vk_physical_device, vk_logical_device, vk_queue_family_index, vk_queue_index);
		}
		auto passin_xr_swapchains(std::vector<xr::Swapchain>& swapchains, std::vector<xr::Rect2Di>& rects, int64_t swapchain_format) -> void {
			assert(swapchains.size() == rects.size());
			vk_swapchain_images.resize(swapchains.size());
			vk_swapchain_rects.resize(rects.size());
			for (size_t i = 0; i < swapchains.size(); ++i) {
				auto& swapchain = swapchains[i];
				auto& rect = rects[i];
				auto& vk_swapchain = vk_swapchain_images[i];
				auto& vk_rect = vk_swapchain_rects[i];
				auto xr_images = swapchain.enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>();
				for (auto& image : xr_images)
				{
					vk_swapchain.push_back(image.image);
				}
				vk_rect = vk::Rect2D{ { rect.offset.x, rect.offset.y }, { static_cast<uint32_t>(rect.extent.width), static_cast<uint32_t>(rect.extent.height) } };
			}
			vk_swapchain_format = static_cast<vk::Format>(swapchain_format);
		}

	public:
		VulkanOpenXrProxy(VulkanOpenXrProxy const&) = delete;
		VulkanOpenXrProxy& operator=(VulkanOpenXrProxy const&) = delete;
		VulkanOpenXrProxy(VulkanOpenXrProxy&&) = default;
		VulkanOpenXrProxy& operator=(VulkanOpenXrProxy&&) = default;

	private:
		VkInstance vk_instance;
		VkPhysicalDevice vk_physical_device;
		VkDevice vk_logical_device;
		uint32_t vk_queue_family_index;
		uint32_t vk_queue_index;
		xr::Instance xr_instance;
		xr::SystemId xr_system_id;
		std::vector<std::vector<vk::Image>> vk_swapchain_images;
		std::vector<vk::Rect2D> vk_swapchain_rects;
		vk::Format vk_swapchain_format;
		xr::DispatchLoaderDynamic dispatcher;
	};
}