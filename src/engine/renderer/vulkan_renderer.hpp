#pragma once

#include <fstream>
#include <unordered_set>
#include <set>

#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <stb_image.h>
#include <stb_truetype.h>
#include "../helpers/arx_logger.hpp"
#include "../helpers/arx_math.hpp"
#include "../proxy/renderer_proxy.hpp"

#include "vulkan_renderer/swapchain.hpp"
#include "vulkan_renderer/texture.hpp"
#include "vulkan_renderer/material.hpp"
#include "vulkan_renderer/bitmap.hpp"
#include "vulkan_renderer/mesh_model.hpp"
#include "vulkan_renderer/ui_element.hpp"

//export module vulkan_renderer;
//export import :texture;
//export import :mesh_model;
//export import :text_model;
//export import :material;
//export import :swap_chain;
//export import :bitmap;
//export import :ui_element;

#ifdef MIRROR_WINDOW
void glfw_error_callback(int code, const char* description)
{
	log_error(std::format("GLFW ERROR [{}]:\t{}", code, description));
}
#endif

namespace arx
{
	using P = VulkanOpenXrProxy;
	//export template<typename P>
	//requires VulkanReceivingProxy<P>
	class VulkanRenderer
	{
	public:
		struct PushConstantData
		{
			glm::mat4 projectionView;
			glm::mat4 modelMatrix;
		};

		enum class DebugMode {
			NoDebug,
			OnlyDebug,
			Mixed,
		};

		const int MAX_FRAMES_IN_FLIGHT = 2;

	public: // concept: Renderer.
		auto initialize(P& proxy) -> void {
#ifdef MIRROR_WINDOW
			create_window();
#endif
			create_instance(proxy);
			pick_physical_device(proxy);
			create_logical_device(proxy);
			create_command_pool();
		}
		auto initialize_session(P& proxy) -> void {
			create_swapchains(proxy);
			create_descriptors();
			create_graphics_pipelines();
			allocate_command_buffers();
			create_default_resources();
		}
		auto clean_up_instance() -> void {
			queue.waitIdle();
		
			log_step("Vulkan", "Destroying Logical Device");
			device.destroy();
			log_success();
		
			log_step("Vulkan", "Destroying Vulkan Instance");
			instance.destroy();
			log_success();
		
#ifdef MIRROR_WINDOW
			glfwDestroyWindow(window);
			glfwTerminate();
#endif
		}
		auto clean_up_session() -> void {
			queue.waitIdle();
		
#ifdef MIRROR_WINDOW
			device.destroySemaphore(mirrorImageAvailableSemaphore);
			device.destroySwapchainKHR(mirrorVkSwapchain);
			mirrorSwapchain = nullptr;
			instance.destroySurfaceKHR(mirrorSurface);
#endif
		
			//preservedModels.clear();
		
			//defaults.empty_texture = nullptr;
		
			log_step("Vulkan", "Destroying Descriptor Pool");
			device.destroyDescriptorPool(descriptor_pool);
			log_success();
		
			log_step("Vulkan", "Destroying Descriptor Set Layout");
			device.destroyDescriptorSetLayout(descriptor_set_layouts.material_descriptor_set_layout);
			log_success();
		
			log_step("Vulkan", "Destroying Fences");
			for (auto& fence : in_flights)
			{
				device.destroyFence(fence);
			}
			log_success();
		
			log_step("Vulkan", "Destroying Command Buffers");
			device.freeCommandBuffers(command_pool, { command_buffer });
			log_success();
		
		
			log_step("Vulkan", "Destroying Command Pool");
			device.destroyCommandPool(command_pool);
			log_success();
		
			log_step("Vulkan", "Destroying Swap Chains");
			swap_chains.clear();
			log_success();
		
			log_step("Vulkan", "Destroying Pipeline");
			device.destroyPipeline(pipelines.mesh_pipeline);
			device.destroyPipeline(pipelines.ui_pipeline);
			device.destroyPipeline(pipelines.text_pipeline);
			device.destroyPipeline(pipelines.wireframe_pipeline);
			log_success();
		
			log_step("Vulkan", "Destroying Pipeline Layout");
			device.destroyPipelineLayout(pipeline_layouts.world_pipeline_layout);
			log_success();
		
		
			log_step("Vulkan", "Destroying Render Pass");
			device.destroyRenderPass(render_pass);
			log_success();
		}
		auto render_view_xr(vk::ArrayProxy<std::tuple<glm::mat4, glm::mat4, uint32_t>> xr_camera) -> void
		{
			if (device.waitForFences(1, &in_flights[0], VK_TRUE, UINT64_MAX) != vk::Result::eSuccess)	// TODO: implement FRAME_IN_FLIGHT
			{
				throw std::runtime_error("Failed to wait for fences.");
			}
			if (device.resetFences(1, &in_flights[0]) != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to reset Fences.");
			}

#ifdef MIRROR_WINDOW
			bool iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
			uint32_t mirrorImageIndex = (view == mirrorView && !iconified) ? device.acquireNextImageKHR(mirrorVkSwapchain, UINT64_MAX, mirrorImageAvailableSemaphore, {}).value : UINT32_MAX;
#endif

			command_buffer.reset();
			clear_to_delete();
			vk::CommandBufferBeginInfo beginInfo{ };
			command_buffer.begin(beginInfo);	// <======= Command Buffer Begin.

			for (size_t view = 0; view < xr_camera.size(); ++view)
			{
				const auto& [mat_projection, mat_camera_transform, image_index] = xr_camera.data()[view];

				command_buffer.beginRenderPass(swap_chains[view]->get_render_pass_begin_info(image_index), vk::SubpassContents::eInline);		// <======= Render Pass Begin. TODO: use subpasses.

				bool haveText = false;
				bool haveMesh = (debug_mode != DebugMode::OnlyDebug) && !mesh_models.empty();
				bool haveUI = !ui_elements.empty();
				bool haveDebug = (debug_mode != DebugMode::NoDebug) && !debug_mesh_models.empty();

				glm::mat4 mat_camera_view = glm::inverse(mat_camera_transform);

				//	haveText |= !(scene->debugMode == SceneDebugMode::eDebugOnly || scene->texts.empty());
				//	haveMesh |= !(scene->debugMode == SceneDebugMode::eDebugOnly || scene->models.empty());
				//	haveUI |= !(scene->uiElements.empty());	// No `scene->onlyDebug` like above because we till want to have UI even in debug view.
				//	haveDebug |= debugOn(scene->debugMode) && !(scene->debugScene == nullptr || scene->debugScene->models.empty());

				/*if (haveText)
				{
					commandBuffers[view].bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.textPipeline);			// <======= Bind Text Pipeline.

					commandBuffers[view].setViewport(0, 1, swapChains[view]->getViewport());		// <======== Set Viewports.

					commandBuffers[view].setScissor(0, 1, swapChains[view]->getScissor());		// <======== Set Scissors.

					// Draw something.
					for (auto scene : scenes)
					{
						if (scene->debugMode == SceneDebugMode::eDebugOnly || scene->texts.empty()) continue;

						XrMatrix4x4f matView;		// V
						if (scene->cameraTransform.expired())
						{
							matView = noCameraView;
						}
						else
						{
							XrMatrix4x4f invView = cnv<XrMatrix4x4f>(scene->cameraTransform.lock()->getGlobalMatrix());
							XrMatrix4x4f_InvertRigidBody(&matView, &invView);
						}

						XrMatrix4x4f matProjectionView;	// PV
						XrMatrix4x4f_Multiply(&matProjectionView, &matProjection, &matView);
						std::vector<PushConstantData> data(1);

						std::pair<hd::TextModel, hd::ITransform> last = { nullptr, nullptr };
						for (auto& pair : scene->texts)
						{
							auto text = pair.first;
							if (text != last.first)
							{
								text->bind(commandBuffers[view]);
								commandBuffers[view].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.worldPipelineLayout, 0, { text->material->descriptorSet }, { });
								commandBuffers[view].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.worldPipelineLayout, 1, { text->bitmap->descriptorSet }, { });
								last.first = text;
							}

							auto transform = pair.second;
							if (transform != last.second)
							{
								auto matTransform = pair.second->getGlobalMatrix();	// M
								data[0].modelMatrix = matTransform;
								XrMatrix4x4f_Multiply(&(data[0].projectionView), &matProjectionView, (XrMatrix4x4f*)&matTransform);	// PVM
								commandBuffers[view].pushConstants<PushConstantData>(pipelineLayouts.worldPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, data);
							}

							text->draw(commandBuffers[view]);
						}
					}
					// End Draw.
				}*/

				if (haveMesh)
				{
					command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.mesh_pipeline);			// <======= Bind Mesh Pipeline.

					command_buffer.setViewport(0, 1, swap_chains[view]->getViewport());		// <======== Set Viewports.

					command_buffer.setScissor(0, 1, swap_chains[view]->getScissor());		// <======== Set Scissors.

					// Draw something.
					struct { Material* material; MeshModel* mesh_model; glm::mat4* model_transform; } last = { nullptr, nullptr, nullptr };
					for (auto& models : mesh_models) {
						for (auto [material, mesh_model, model_transform] : *models) {
							if (material != last.material) {
								last.material = material;
								command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.world_pipeline_layout, 0, { material->descriptor_set }, { });
							}
							if (mesh_model != last.mesh_model) {
								last.mesh_model = mesh_model;
								command_buffer.bindVertexBuffers(0, { mesh_model->vertex_buffer }, { vk::DeviceSize(0) });
								command_buffer.bindIndexBuffer(mesh_model->index_buffer, 0, vk::IndexType::eUint32);
							}
							if (model_transform != last.model_transform) {
								last.model_transform = model_transform;
								std::array<PushConstantData, 1> data;
								data[0].modelMatrix = *model_transform;
								data[0].projectionView = mat_projection * mat_camera_view * (*model_transform);
								command_buffer.pushConstants<PushConstantData>(pipeline_layouts.world_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, data);
							}
							command_buffer.drawIndexed(mesh_model->index_count, 1, 0, 0, 0);
						}
					}
					// End Draw.
				}

				if (haveUI)
				{
					command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.ui_pipeline);			// <======= Bind Mesh Pipeline.

					command_buffer.setViewport(0, 1, swap_chains[view]->getViewport());		// <======== Set Viewports.

					command_buffer.setScissor(0, 1, swap_chains[view]->getScissor());		// <======== Set Scissors.

					// Draw something.
					struct { Bitmap* bitmap; UIElement* ui_element; glm::mat4* model_transform; } last = { nullptr, nullptr, nullptr };
					for (auto& elements : ui_elements) {
						for (auto [bitmap, ui_element, model_transform] : *elements) {
							if (bitmap != last.bitmap) {
								last.bitmap = bitmap;
								command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.world_pipeline_layout, 1, { bitmap->descriptor_set }, { });
							}
							if (ui_element != last.ui_element) {
								last.ui_element = ui_element;
								command_buffer.bindVertexBuffers(0, { ui_element->vertex_buffer }, { vk::DeviceSize(0) });
								command_buffer.bindIndexBuffer(ui_element->index_buffer, 0, vk::IndexType::eUint32);
							}
							if (model_transform != last.model_transform) {
								last.model_transform = model_transform;
								std::array<PushConstantData, 1> data;
								data[0].modelMatrix = *model_transform;
								data[0].projectionView = mat_projection * mat_camera_view * (*model_transform);
								command_buffer.pushConstants<PushConstantData>(pipeline_layouts.world_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, data);
							}
							command_buffer.drawIndexed(ui_element->index_count, 1, 0, 0, 0);
						}
					}
					// End Draw.
				}

				if (haveDebug)
				{
					command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.wireframe_pipeline);			// <======= Bind Mesh Pipeline.

					command_buffer.setViewport(0, 1, swap_chains[view]->getViewport());		// <======== Set Viewports.

					command_buffer.setScissor(0, 1, swap_chains[view]->getScissor());		// <======== Set Scissors.

					// Draw something.
					struct { Material* material; MeshModel* mesh_model; glm::mat4* model_transform; } last = { nullptr, nullptr, nullptr };
					for (auto& models : debug_mesh_models) {
						for (auto [material, mesh_model, model_transform] : *models) {
							if (material != last.material) {
								last.material = material;
								command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layouts.world_pipeline_layout, 0, { material->descriptor_set }, { });
							}
							if (mesh_model != last.mesh_model) {
								last.mesh_model = mesh_model;
								command_buffer.bindVertexBuffers(0, { mesh_model->vertex_buffer }, { vk::DeviceSize(0) });
								command_buffer.bindIndexBuffer(mesh_model->index_buffer, 0, vk::IndexType::eUint32);
							}
							if (model_transform != last.model_transform) {
								last.model_transform = model_transform;
								std::array<PushConstantData, 1> data;
								data[0].modelMatrix = *model_transform;
								data[0].projectionView = mat_projection * mat_camera_view * (*model_transform);
								command_buffer.pushConstants<PushConstantData>(pipeline_layouts.world_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, data);
							}
							command_buffer.drawIndexed(mesh_model->index_count, 1, 0, 0, 0);
						}
					}
					// End Draw.
				}

				command_buffer.endRenderPass();		// <========= Render Pass End.
			}

#ifdef MIRROR_WINDOW
			if (view == mirrorView && !iconified)
			{
				vk::ImageMemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eTransferDstOptimal, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, mirrorSwapchain->images[mirrorImageIndex], { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
				barrier.image = mirrorSwapchain->images[mirrorImageIndex];
				barrier.old_layout = vk::ImageLayout::eUndefined;
				barrier.new_layout = vk::ImageLayout::eTransferDstOptimal;
				commandBuffers[view].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, { }, { }, { }, { barrier });

				barrier.image = swapChains[view]->images[imageIndex];
				barrier.old_layout = vk::ImageLayout::eColorAttachmentOptimal;
				barrier.new_layout = vk::ImageLayout::eTransferSrcOptimal;
				commandBuffers[view].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, { }, { }, { }, { barrier });

				vk::ImageBlit blit(
					{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
					std::array<vk::Offset3D, 2>({ { 0, 0, 0 }, { static_cast<int32_t>(swapChains[view]->rect.extent.width) , static_cast<int32_t>(swapChains[view]->rect.extent.height), 1 } }),
					{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
					std::array<vk::Offset3D, 2>({ { 0, 0, 0 }, { static_cast<int32_t>(mirrorSwapchain->rect.extent.width) , static_cast<int32_t>(mirrorSwapchain->rect.extent.height), 1 } })
				);
				commandBuffers[view].blitImage(swapChains[view]->images[imageIndex], vk::ImageLayout::eTransferSrcOptimal, mirrorSwapchain->images[mirrorImageIndex], vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

				barrier.image = mirrorSwapchain->images[mirrorImageIndex];
				barrier.old_layout = vk::ImageLayout::eTransferDstOptimal;
				barrier.new_layout = vk::ImageLayout::ePresentSrcKHR;
				commandBuffers[view].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, { }, { }, { }, { barrier });

				barrier.image = swapChains[view]->images[imageIndex];
				barrier.old_layout = vk::ImageLayout::eTransferSrcOptimal;
				barrier.new_layout = vk::ImageLayout::eColorAttachmentOptimal;
				commandBuffers[view].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, { }, { }, { }, { barrier });
			}
#endif // MIRROR_WINDOW

			command_buffer.end();		// <========= Command Buffer End.

			vk::SubmitInfo submit_info{ }; submit_info.commandBufferCount = 1; submit_info.pCommandBuffers = &command_buffer;
#ifdef MIRROR_WINDOW
			if (view == mirrorView && !iconified)
			{
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &mirrorImageAvailableSemaphore;
			}
#endif // MIRROR_WINDOW

			queue.submit(submit_info, in_flights[0]);

#ifdef MIRROR_WINDOW
			if (view == mirrorView && !iconified)
			{
				vk::PresentInfoKHR presentInfo(0, nullptr, 1, &mirrorVkSwapchain, &mirrorImageIndex, nullptr);
				if (queue.presentKHR(presentInfo) != vk::Result::eSuccess)
				{
					throw std::runtime_error("Failed to present to mirror window.");
				}
			}
#endif
		}

	public: // concept: RenderingObjectManager.
		auto create_texture(const std::string& path) -> Texture* {
			int width, height, channels;
			stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			if (!pixels) throw std::runtime_error("Failed to load texture image from file \"" + path + "\".");
			auto texture = create_texture(pixels, width, height, 4);
			stbi_image_free(pixels);
			return texture;
		}
		auto create_texture(stbi_uc* pixels, int width, int height, int channels) -> Texture* {
			auto mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
			vk::DeviceSize image_size = sizeof(stbi_uc) * width * height * channels;
			auto [staging_buffer, staging_buffer_memory] =
				create_buffer(sizeof(stbi_uc), width * height * channels, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			auto mapping_memory = map_memory(staging_buffer_memory);
			std::memcpy(mapping_memory, pixels, static_cast<size_t>(image_size));
			unmap_memory(staging_buffer_memory);

			vk::Format image_format;
			if (channels == 1)
			{
				image_format = vk::Format::eR8Unorm;
			}
			else if (channels == 4)
			{
				image_format = vk::Format::eR8G8B8A8Srgb;
			}
			else
			{
				throw std::runtime_error(std::format("Unsupported image channels({}) for a texture. Must be 1 for grayscale or 4 for rgba.", channels));
			}

			std::array<uint32_t, 1> queue_family_indices = { 0 };
			vk::ImageCreateInfo image_info({ }, vk::ImageType::e2D, image_format, { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 }, mip_levels, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::SharingMode::eExclusive, queue_family_indices, vk::ImageLayout::eUndefined);

			auto [texture_image, texture_image_memory] = create_image(image_info, vk::MemoryPropertyFlagBits::eDeviceLocal);

			transition_image_layout(texture_image, image_format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mip_levels);
			copy_buffer_to_image(texture_image, staging_buffer, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
			destroy_buffer(staging_buffer, staging_buffer_memory);
			generate_mipmaps(texture_image, width, height, mip_levels);

			auto texture_image_view = create_image_view(texture_image, image_format, mip_levels);

			vk::SamplerCreateInfo sampler_info({ }, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.f, VK_TRUE, max_anisotrophy, VK_FALSE, vk::CompareOp::eAlways, 0.f, static_cast<float>(mip_levels), vk::BorderColor::eIntOpaqueBlack, VK_FALSE);
			auto texture_sampler = device.createSampler(sampler_info);

			return new Texture{ mip_levels, texture_image_view, texture_sampler, texture_image, texture_image_memory };
		}
		auto create_material(Texture* diffuse_map, Texture* normal_map, glm::vec4 color) -> Material* {
			MaterialPropertyBufferObject property_buffer_object{ static_cast<vk::Bool32>(diffuse_map != nullptr), static_cast<vk::Bool32>(normal_map != nullptr), color };

			auto [staging_buffer, staging_buffer_memory] =
				create_buffer(sizeof(property_buffer_object), 1, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

			void* mapping_memory = map_memory(staging_buffer_memory);
			std::memcpy(mapping_memory, &property_buffer_object, sizeof(MaterialPropertyBufferObject));
			unmap_memory(staging_buffer_memory);

			auto [property_buffer, property_buffer_memory] =
				create_buffer(sizeof(property_buffer_object), 1, vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

			copy_buffer(property_buffer, staging_buffer, sizeof(MaterialPropertyBufferObject));
			destroy_buffer(staging_buffer, staging_buffer_memory);

			std::array<vk::DescriptorSetLayout, 1> layouts{ descriptor_set_layouts.material_descriptor_set_layout };
			vk::DescriptorSetAllocateInfo allo_info(descriptor_pool, layouts);
			auto descriptor_set = device.allocateDescriptorSets(allo_info)[0];

			vk::DescriptorBufferInfo property_buffer_info(property_buffer, { }, sizeof(MaterialPropertyBufferObject));
			vk::DescriptorImageInfo diffuse_map_info;
			vk::DescriptorImageInfo normal_map_info;

			std::vector<vk::WriteDescriptorSet> descriptor_writes;

			{
				descriptor_writes.push_back({ descriptor_set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, { }, &property_buffer_info });
			}

			if (property_buffer_object.enable_diffuse_map)
			{
				diffuse_map_info = { diffuse_map->texture_sampler, diffuse_map->texture_image_view, vk::ImageLayout::eShaderReadOnlyOptimal };
				descriptor_writes.push_back({ descriptor_set, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &diffuse_map_info });
			}
			else
			{
				diffuse_map_info = { defaults.empty_texture->texture_sampler, defaults.empty_texture->texture_image_view, vk::ImageLayout::eShaderReadOnlyOptimal };
				descriptor_writes.push_back({ descriptor_set, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &diffuse_map_info });
			}

			if (property_buffer_object.enable_normal_map)
			{
				normal_map_info = { normal_map->texture_sampler, normal_map->texture_image_view, vk::ImageLayout::eShaderReadOnlyOptimal };
				descriptor_writes.push_back({ descriptor_set, 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &normal_map_info });
			}
			else
			{
				normal_map_info = { defaults.empty_texture->texture_sampler, defaults.empty_texture->texture_image_view, vk::ImageLayout::eShaderReadOnlyOptimal };
				descriptor_writes.push_back({ descriptor_set, 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &normal_map_info });
			}

			device.updateDescriptorSets(descriptor_writes, { });

			return new Material{
				descriptor_set,
				property_buffer_object,
				property_buffer,
				property_buffer_memory,
				diffuse_map,
				normal_map,
			};
		}
		auto create_bitmap(Texture* map) -> Bitmap* {
			std::array<vk::DescriptorSetLayout, 1> layouts{ descriptor_set_layouts.bitmap_descriptor_set_layout };
			vk::DescriptorSetAllocateInfo allo_info(descriptor_pool, layouts);
			auto descriptor_set = device.allocateDescriptorSets(allo_info)[0];

			vk::DescriptorImageInfo map_info{ map->texture_sampler, map->texture_image_view, vk::ImageLayout::eShaderReadOnlyOptimal };
			std::vector<vk::WriteDescriptorSet> descriptor_writes = {
				{ descriptor_set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &map_info },
			};

			device.updateDescriptorSets(descriptor_writes, { });

			return new Bitmap{
				descriptor_set,
				map,
			};
		}
		auto create_bitmap(const std::string& font_path, uint32_t width = 1024, uint32_t height = 1024, float pixelHeight = 150.f) -> Bitmap* {
			auto font_buffer = readFile(font_path);
			std::vector<stbi_uc> single_channel_bitmap_data(static_cast<size_t>(width * height));
			std::vector<stbi_uc> rgba_bitmap_data(static_cast<size_t>(width * height) * 4);
			std::array<Bitmap::CharInfo, 128> char_infos;
			stbtt_BakeFontBitmap((unsigned char*)font_buffer.data(), 0, pixelHeight, single_channel_bitmap_data.data(), width, height, 0, 128, char_infos.data());
			for (size_t i = 0; i < single_channel_bitmap_data.size(); ++i)
			{
				for (size_t j = 0; j < 4; ++j)
				{
					rgba_bitmap_data[4 * i + j] = single_channel_bitmap_data[i];
				}
			}
			auto texture = create_texture(rgba_bitmap_data.data(), static_cast<int32_t>(width), static_cast<int32_t>(height), 4);
			auto bitmap = create_bitmap(texture);
			bitmap->char_infos = char_infos;
			bitmap->height = pixelHeight;
			bitmap->map_extent = { width, height };
			return bitmap;
		}
		/*auto create_mesh_model(const std::string& path) -> MeshModel* {
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string warn, err;

			if (!tinyobj::loadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), "models"))
			{
				throw std::runtime_error(warn + err);
			}

			std::vector<MeshModel::Vertex> vertices;
			std::vector<uint32_t> indices;
			std::unordered_map<MeshModel::Vertex, uint32_t> uniqueVertices;

			for (const auto& shape : shapes)
			{
				for (const auto& index : shape.mesh.indices)
				{
					MeshModel::Vertex vertex{ };

					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};

					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.f - attrib.texcoords[2 * index.texcoord_index + 1],
					};

					if (uniqueVertices.count(vertex) == 0)
					{
						uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
						vertices.push_back(vertex);
					}

					indices.push_back(uniqueVertices[vertex]);
				}
			}

			return create_mesh_model(vertices, indices);
		}*/
		auto create_mesh_model(std::vector<MeshModel::Vertex>& vertices, const std::vector<uint32_t> indices, bool calculate_tbn = true) -> MeshModel* {
			if (calculate_tbn) {
				calculate_tangent_bitangent(vertices, indices);
			}
			// VertexBuffer
			auto vertex_count = static_cast<uint32_t>(vertices.size());
			vk::DeviceSize buffer_size = sizeof(MeshModel::Vertex) * vertex_count;
			uint32_t vertex_size = sizeof(MeshModel::Vertex);

			auto [staging_buffer, staging_buffer_memory] =
				create_buffer(vertex_size, vertex_count, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			void* mapping_memory = map_memory(staging_buffer_memory);
			std::memcpy(mapping_memory, vertices.data(), buffer_size);
			unmap_memory(staging_buffer_memory);

			auto [vertex_buffer, vertex_buffer_memory] =
				create_buffer(vertex_size, vertex_count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

			copy_buffer(vertex_buffer, staging_buffer, buffer_size);
			destroy_buffer(staging_buffer, staging_buffer_memory);

			// VertexBuffer
			auto index_count = static_cast<uint32_t>(indices.size());
			buffer_size = sizeof(indices[0]) * index_count;
			uint32_t index_size = sizeof(indices[0]);

			std::tie(staging_buffer, staging_buffer_memory) =
				create_buffer(index_size, index_count, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			mapping_memory = map_memory(staging_buffer_memory);
			std::memcpy(mapping_memory, indices.data(), buffer_size);
			unmap_memory(staging_buffer_memory);

			auto [index_buffer, index_buffer_memory] =
				create_buffer(index_size, index_count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

			copy_buffer(index_buffer, staging_buffer, buffer_size);
			destroy_buffer(staging_buffer, staging_buffer_memory);

			return new MeshModel{ vertex_count, index_count, vertex_buffer, vertex_buffer_memory, index_buffer, index_buffer_memory };
		}
		auto create_mesh_model(const MeshModel::Builder& builder) -> MeshModel* {
			auto [build_vertices, build_indices] = builder.build();
			return create_mesh_model(build_vertices, build_indices);
		}
		/*auto create_text_model(const std::string& text, Material* material, Bitmap* bitmap, float height = 0.1f) -> TextModel* {
			float scale = height / 150.f;

			std::vector<TextModel::Vertex> text_vertices;
			std::vector<uint32_t> indices;

			uint32_t index = 0;

			glm::vec2 current_point{ };
			for (auto c : text)
			{
				index = static_cast<uint32_t>(text_vertices.size());

				if (c == '\n')
				{
					current_point.x = 0.f;
					current_point.y -= height;
					continue;
				}

				std::array<TextVertex, 4> char_vertices = {
					TextVertex{ { }, { 0.f, 0.f }, { }, {  0.f,  0.f, -1.f }, { }, { } },
					TextVertex{ { }, { 0.f, 1.f }, { }, {  0.f,  0.f, -1.f }, { }, { } },
					TextVertex{ { }, { 1.f, 1.f }, { }, {  0.f,  0.f, -1.f }, { }, { } },
					TextVertex{ { }, { 1.f, 0.f }, { }, {  0.f,  0.f, -1.f }, { }, { } },
				};

				const auto& info = bitmap->charInfos[c];
				glm::vec2 extent{ info.x1 - info.x0, info.y1 - info.y0 };

				char_vertices[0].position = glm::vec3(current_point + scale * glm::vec2(info.xoff, -info.yoff), 0.f); // top left
				char_vertices[0].bitmapUv = glm::vec2(info.x0, info.y0) / 1024.f;

				char_vertices[1].position = glm::vec3(current_point + scale * glm::vec2(info.xoff, -info.yoff - extent.y), 0.f); // bottom left
				char_vertices[1].bitmapUv = glm::vec2(info.x0, info.y1) / 1024.f;

				char_vertices[2].position = glm::vec3(current_point + scale * glm::vec2(info.xoff + extent.x, -info.yoff - extent.y), 0.f); // bottom right
				char_vertices[2].bitmapUv = glm::vec2(info.x1, info.y1) / 1024.f;

				char_vertices[3].position = glm::vec3(current_point + scale * glm::vec2(info.xoff + extent.x, -info.yoff), 0.f); // top right
				char_vertices[3].bitmapUv = glm::vec2(info.x1, info.y0) / 1024.f;

				for (auto& v : char_vertices)
				{
					text_vertices.push_back(v);
				}

				indices.push_back(index + 0);
				indices.push_back(index + 1);
				indices.push_back(index + 2);


				indices.push_back(index + 2);
				indices.push_back(index + 3);
				indices.push_back(index + 0);

				current_point.x += scale * info.xadvance;
			}

			// Vertex Buffer
			auto vertex_count = static_cast<uint32_t>(text_vertices.size());
			vk::DeviceSize buffer_size = sizeof(TextModel::Vertex) * vertex_count;
			uint32_t vertex_size = sizeof(TextModel::Vertex);

			auto [staging_buffer, staging_buffer_memory] =
				create_buffer(vertex_size, vertex_count, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			void* mapping_memory = map_memory(staging_buffer_memory);
			std::memcpy(mapping_memory, text_vertices.data(), buffer_size);
			unmap_memory(staging_buffer_memory);

			auto [vertex_buffer, vertex_buffer_memory] =
				create_buffer(vertex_size, vertex_count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

			copy_buffer(vertex_buffer, staging_buffer, buffer_size);
			destroy_buffer(staging_buffer, staging_buffer_memory);

			// Index Buffer
			auto index_count = static_cast<uint32_t>(indices.size());
			buffer_size = sizeof(indices[0]) * index_count;
			uint32_t index_size = sizeof(indices[0]);

			std::tie(staging_buffer, staging_buffer_memory) =
				create_buffer(index_size, index_count, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			mapping_memory = map_memory(staging_buffer_memory);
			std::memcpy(mapping_memory, indices.data(), buffer_size);
			unmap_memory(staging_buffer_memory);

			auto [index_buffer, index_buffer_memory] =
				create_buffer(index_size, index_count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

			copy_buffer(index_buffer, staging_buffer, buffer_size);
			destroy_buffer(staging_buffer, staging_buffer_memory);

			return new TextModel{
				material, bitmap,
				vertex_count, index_count,
				vertex_buffer, vertex_buffer_memory,
				index_buffer, index_buffer_memory,
			};
		}*/
		auto create_ui_element(std::vector<UIVertex>& vertices, std::vector<uint32_t>& indices) -> UIElement* {
			// VertexBuffer
			auto vertex_count = static_cast<uint32_t>(vertices.size());
			vk::DeviceSize buffer_size = sizeof(UIVertex) * vertex_count;
			uint32_t vertex_size = sizeof(UIVertex);

			auto [staging_buffer, staging_buffer_memory] =
				create_buffer(vertex_size, vertex_count, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			void* mapping_memory = map_memory(staging_buffer_memory);
			std::memcpy(mapping_memory, vertices.data(), buffer_size);
			unmap_memory(staging_buffer_memory);

			auto [vertex_buffer, vertex_buffer_memory] =
				create_buffer(vertex_size, vertex_count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

			copy_buffer(vertex_buffer, staging_buffer, buffer_size);
			destroy_buffer(staging_buffer, staging_buffer_memory);

			// VertexBuffer
			auto index_count = static_cast<uint32_t>(indices.size());
			buffer_size = sizeof(indices[0]) * index_count;
			uint32_t index_size = sizeof(indices[0]);

			std::tie(staging_buffer, staging_buffer_memory) =
				create_buffer(index_size, index_count, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			mapping_memory = map_memory(staging_buffer_memory);
			std::memcpy(mapping_memory, indices.data(), buffer_size);
			unmap_memory(staging_buffer_memory);

			auto [index_buffer, index_buffer_memory] =
				create_buffer(index_size, index_count, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

			copy_buffer(index_buffer, staging_buffer, buffer_size);
			destroy_buffer(staging_buffer, staging_buffer_memory);

			return new UIElement{
				vertex_count, index_count,
				vertex_buffer, vertex_buffer_memory,
				index_buffer, index_buffer_memory,
			};
		}
		auto create_ui_panel(glm::vec2 extent, glm::vec4 color = { 1.f, 1.f, 1.f, 1.f }, glm::vec2 anchor = { 0.5f, 0.5f }) -> UIElement* {
			std::vector<UIVertex> vertices = {
				UIVertex{ extent * (glm::vec2{ 0.f, 0.f } - anchor), { 0.f, 1.f }, color },
				UIVertex{ extent * (glm::vec2{ 1.f, 0.f } - anchor), { 1.f, 1.f }, color },
				UIVertex{ extent * (glm::vec2{ 1.f, 1.f } - anchor), { 1.f, 0.f }, color },
				UIVertex{ extent * (glm::vec2{ 0.f, 1.f } - anchor), { 0.f, 0.f }, color },
			};
			std::vector<uint32_t> indices = {
				0, 1, 2,
				2, 3, 0,
			};
			return create_ui_element(vertices, indices);
		}
		auto create_ui_text(const std::string& text, float height, Bitmap* bitmap, glm::vec4 color = { 0.f, 0.f, 0.f, 1.f }, glm::vec2 anchor = { 0.5f, 0.5f }) -> UIElement* {
			float scale = height / bitmap->height;

			uint32_t lineCount = 1;
			glm::vec2 textExtent = { 0.f, height };
			std::vector<UIVertex> textVertices;
			std::vector<uint32_t> indices;

			uint32_t index = 0;

			glm::vec2 currentPoint{ };
			for (auto c : text)
			{
				index = static_cast<uint32_t>(textVertices.size());

				if (c == '\n')
				{
					currentPoint.x = 0.f;
					currentPoint.y -= height;
					textExtent.y += height;
					++lineCount;
					continue;
				}

				std::array<UIVertex, 4> charVertices;

				const auto& info = bitmap->char_infos[c];
				glm::vec2 charExtent{ info.x1 - info.x0, info.y1 - info.y0 };

				charVertices[0].position = currentPoint + scale * glm::vec2(info.xoff, -info.yoff); // top left
				charVertices[0].uv = glm::vec2(info.x0, info.y0) / 1024.f;
				charVertices[0].color = color;

				charVertices[1].position = currentPoint + scale * glm::vec2(info.xoff, -info.yoff - charExtent.y); // bottom left
				charVertices[1].uv = glm::vec2(info.x0, info.y1) / 1024.f;
				charVertices[1].color = color;

				charVertices[2].position = currentPoint + scale * glm::vec2(info.xoff + charExtent.x, -info.yoff - charExtent.y); // bottom right
				charVertices[2].uv = glm::vec2(info.x1, info.y1) / 1024.f;
				charVertices[2].color = color;

				charVertices[3].position = currentPoint + scale * glm::vec2(info.xoff + charExtent.x, -info.yoff); // top right
				charVertices[3].uv = glm::vec2(info.x1, info.y0) / 1024.f;
				charVertices[3].color = color;

				for (auto& v : charVertices)
				{
					textVertices.push_back(v);
				}

				indices.push_back(index + 0);
				indices.push_back(index + 1);
				indices.push_back(index + 2);


				indices.push_back(index + 2);
				indices.push_back(index + 3);
				indices.push_back(index + 0);

				currentPoint.x += scale * info.xadvance;
				textExtent.x = glm::max(textExtent.x, currentPoint.x);
			}

			glm::vec2 newOrigin = textExtent * anchor;
			glm::vec2 oldOrigin = { 0.f, height * (lineCount - 1) };
			glm::vec2 offset = oldOrigin - newOrigin;
			for (auto& vertex : textVertices)
			{
				vertex.position += offset;
			}

			return create_ui_element(textVertices, indices);
		}
		
		auto delete_ui_element(UIElement* ui_element) -> void {
			ui_elements_to_delete.insert(ui_element);
		}

		auto delete_ui_element_immediate(UIElement* ui_element) -> void {
			destroy_buffer(ui_element->index_buffer, ui_element->index_buffer_memory);
			destroy_buffer(ui_element->vertex_buffer, ui_element->vertex_buffer_memory);
		}

		auto clear_to_delete() -> void {
			for (auto ui_element : ui_elements_to_delete)
			{
				delete_ui_element_immediate(ui_element);
			}
			ui_elements_to_delete.clear();
		}
		
	public:
		VulkanRenderer() {
			queue_family_index = 0;
		}

#ifdef MIRROR_WINDOW
		auto create_window() -> void {
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
			window = glfwCreateWindow(1920, 1080, "Arxetipo", nullptr, nullptr);
			glfwSetErrorCallback(glfw_error_callback);
		}
		auto get_window() -> GLFWwindow* {
			return window;
		}
#endif
		auto create_instance(P& proxy) -> void {
			// Logging Extensions -----------------------------------------------------------
			auto allExtensions = vk::enumerateInstanceExtensionProperties();
			log_info("Vulkan", std::format("List of total {} Vulkan Instance Extension(s):", allExtensions.size()), 0);
			for (const auto& ext : allExtensions)
			{
				log_info("Vulkan", ext.extensionName, 1);
			}

			// Creating Instance ------------------------------------------------------------
			if (instance)
			{
				throw std::runtime_error("Can't create a new Vulkan instance when there's already one!");
			}

			std::vector<const char*> extensions;
#ifdef MIRROR_WINDOW
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
			for (uint32_t i = 0; i < glfwExtensionCount; ++i) extensions.push_back(glfwExtensions[i]);
#endif
			log_info("Vulkan", std::format("List of total {} required Vulkan Instance Extension(s):", extensions.size()), 0);
			for (const auto& ext : extensions)
			{
				log_info("Vulkan", ext, 1);
			}

			//// Get VulkanGraphicsRequirements.
			//xr::GraphicsRequirementsVulkanKHR requirement = openXrInstance.getVulkanGraphicsRequirements2KHR(openXrSystemId, dispatcher);
			//log_info("Vulkan", "Vulkan API Version Information: ", 0);
			//log_info("Vulkan",
			//	std::format("Lowest supported API version = {}.{}.{} ",
			//		requirement.minApiVersionSupported.major(),
			//		requirement.minApiVersionSupported.minor(),
			//		requirement.minApiVersionSupported.patch()
			//	),
			//	1);
			//log_info("Vulkan",
			//	std::format("Highest verified API version = {}.{}.{} ",
			//		requirement.maxApiVersionSupported.major(),
			//		requirement.maxApiVersionSupported.minor(),
			//		requirement.maxApiVersionSupported.patch()
			//	),
			//	1);

			const std::vector<const char*> validationLayers = {
				"VK_LAYER_KHRONOS_validation"
			};

#define ENABLE_VALIDATION_LAYERS
#if !defined(ENABLE_VALIDATION_LAYERS)
#if !defined(NDEBUG)
#define ENABLE_VALIDATION_LAYERS
#endif
#endif

			log_step("Vulkan", "Creating Vulkan Instance");
			// App Info.
			vk::ApplicationInfo appInfo("Arxetipo", 1, "Arxetipo Engine", 1, VK_API_VERSION_1_3);

			// Instance Info.
#if !defined(ENABLE_VALIDATION_LAYERS)
			vk::InstanceCreateInfo instanceInfo({}, &appInfo, nullptr, extensions);
#else
			vk::InstanceCreateInfo instanceInfo({}, &appInfo, validationLayers, extensions);
#endif

			instance = proxy.create_instance(instanceInfo);

			log_success();
		}
		auto pick_physical_device(P& proxy) -> void {
			log_step("Vulkan", "Picking Physical Device");
			physical_device = proxy.get_physical_device();
			log_success();
			auto properties = physical_device.getProperties();
			log_info("Vulkan", ("Physical Device Name : " + static_cast<std::string>(properties.deviceName.data())), 0);
			log_info("Vulkan", std::format("Push Constant Limit : {}", properties.limits.maxPushConstantsSize), 0);
			log_info("Vulkan", std::format("Max Anisotropy : {}", properties.limits.maxSamplerAnisotropy), 0);
			max_anisotrophy = properties.limits.maxSamplerAnisotropy;
			auto features = physical_device.getFeatures();
		}
		auto create_logical_device(P& proxy) -> void {
			// Finding for a suitable Queue Family.
			log_step("Vulkan", "Finding for a suitable Queue Family");
			auto queueFamilyProperties = physical_device.getQueueFamilyProperties();
			queue_family_index = -1;
			for (int i = 0; i < queueFamilyProperties.size(); ++i)
			{
				if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)/* && physicalDevice.getSurfaceSupportKHR(i, mirrorSurface) // we'll check it later. */)
				{
					queue_family_index = i;
					break;
				}
			}
			if (queue_family_index < 0)
			{
				throw std::runtime_error("Can't find a suitable queue family.");
			}
			log_success();
			log_info("Vulkan", std::format("Queue Family Index = {}", queue_family_index), 0);

			std::vector<float> queuePiorities(1, { 0.f });
			vk::DeviceQueueCreateInfo queueInfo({ }, queue_family_index, queuePiorities);

			// Logging Device Extensions
			auto allExtensions = physical_device.enumerateDeviceExtensionProperties();
			log_info("Vulkan", std::format("List of total {} Vulkan Device Extension(s): ", allExtensions.size()), 0);
			for (auto& ext : allExtensions)
			{
				log_info("Vulkan", std::string(ext.extensionName.data()), 1);
			}

			std::vector<const char*> requiredExtensions(0);
			requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

			log_info("Vulkan", std::format("List of total {} required Vulkan Device Extension(s): ", requiredExtensions.size()), 0);
			for (auto& ext : requiredExtensions)
			{
				log_info("Vulkan", ext, 1);
			}

			vk::PhysicalDeviceFeatures deviceFeatures{ };
			deviceFeatures.samplerAnisotropy = VK_TRUE;
			std::vector<vk::DeviceQueueCreateInfo> queueInfos(1, { queueInfo });

			vk::DeviceCreateInfo deviceInfo(
				{ },
				queueInfos,
				{ },
				requiredExtensions,
				&deviceFeatures
			);

			log_step("Vulkan", "Creating Vulkan Logical Device");
			device = proxy.create_logical_device(deviceInfo);
			log_success();

			// Getting Queue.
			log_step("Vulkan", "Getting Queue");
			queue = device.getQueue(queue_family_index, 0);
			proxy.passin_queue_info(queue_family_index, 0);
			log_success();
		}
		auto create_command_pool() -> void {
			log_step("Vulkan", "Creating Command Pool");
			vk::CommandPoolCreateInfo createInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family_index);
			command_pool = device.createCommandPool(createInfo);
			log_success();
		}

		auto create_swapchains(P& proxy) -> void {
			std::vector<std::vector<vk::Image>> swap_chain_images = proxy.get_swapchain_images();
			std::vector<vk::Rect2D> rects = proxy.get_swapchain_rects();
			vk::Format format = proxy.get_swapchain_format();
			create_render_pass(format);
			log_step("Vulkan", "Creating Vulkan Side Swap Chains");
			swap_chains.clear();
			for (int i = 0; i < swap_chain_images.size(); ++i) {
				swap_chains.push_back(create_swap_chain(render_pass, swap_chain_images[i], format, rects[i]));
			}
			log_success();
			log_info("Vulkan", std::format("Swapchain length : {}", swap_chain_images[0].size()), 0);

#ifdef MIRROR_WINDOW
			log_step("Vulkan", "Creating Mirror Window Swap Chain");

			glfwSetWindowSize(window, rects[0].extent.width * mirrorImageHeight / rects[0].extent.height, mirrorImageHeight);
			VkSurfaceKHR surface;
			if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			{
				throw std::runtime_error("Unable to create mirror window surface.");
			}
			mirrorSurface = surface;
			if (physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface) != VK_TRUE)
			{
				throw std::runtime_error("Presentation not supported.");
			}

			vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(mirrorSurface);
			auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(mirrorSurface);
			auto presentModes = physicalDevice.getSurfacePresentModesKHR(mirrorSurface);
			if (surfaceFormats.empty() || presentModes.empty())
			{
				throw std::runtime_error("Swapchain not supported.");
			}
			vk::SurfaceFormatKHR choosedMirrorSurfaceFormat; bool surfaceFormatFound = false;
			for (const auto& surfaceFormat : surfaceFormats)
			{
				if (surfaceFormat.format == format && surfaceFormat.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear)
				{
					choosedMirrorSurfaceFormat = surfaceFormat;
					surfaceFormatFound = true;
					break;
				}
			}
			if (!surfaceFormatFound)
			{
				throw std::runtime_error(std::format("Can's find desired surface format({}, {}) for mirror window.", vk::to_string(format), vk::to_string(vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear)));
			}
			vk::PresentModeKHR choosedPresentMode; bool presentModefound = false;
			for (const auto& presentMode : presentModes)
			{
				if (presentMode == vk::PresentModeKHR::eMailbox)
				{
					choosedPresentMode = presentMode;
					presentModefound = true;
					break;
				}
			}
			if (!presentModefound) for (const auto& presentMode : presentModes)
			{
				if (presentMode == vk::PresentModeKHR::eImmediate)
				{
					choosedPresentMode = presentMode;
					presentModefound = true;
					break;
				}
			}
			if (!presentModefound)
			{
				throw std::runtime_error(std::format("Can't find desired present mode({} or {}) for mirror window", vk::to_string(vk::PresentModeKHR::eMailbox), vk::to_string(vk::PresentModeKHR::eImmediate)));
			}
			vk::Extent2D mirrorWindowExtent;
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			{
				mirrorWindowExtent = capabilities.currentExtent;
			}
			else
			{
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);
				mirrorWindowExtent.width = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				mirrorWindowExtent.height = std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}
			std::cout << "Extent: [" << mirrorWindowExtent.width << ", " << mirrorWindowExtent.height << "]\t";
			uint32_t imageCount = capabilities.maxImageCount ? std::min(capabilities.maxImageCount, capabilities.minImageCount + 1) : (capabilities.minImageCount + 1);

			vk::SwapchainCreateInfoKHR mirrorSwapchainCreateInfo({ }, mirrorSurface, imageCount, choosedMirrorSurfaceFormat.format, choosedMirrorSurfaceFormat.colorSpace, mirrorWindowExtent,
				1, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, { },
				capabilities.currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque, choosedPresentMode, VK_FALSE, { });
			mirrorVkSwapchain = device.createSwapchainKHR(mirrorSwapchainCreateInfo);

			auto mirrorSwapchainImages = device.getSwapchainImagesKHR(mirrorVkSwapchain);
			mirrorSwapchain = std::make_shared<Swapchain>(device, nullptr, mirrorSwapchainImages, choosedMirrorSurfaceFormat.format, vk::Rect2D{ { }, mirrorWindowExtent });

			vk::SemaphoreCreateInfo semaphoreInfo{ };
			mirrorImageAvailableSemaphore = device.createSemaphore(semaphoreInfo);

			log_success();
#endif // MIRROR_WINDOW

		}
		auto create_descriptors() -> void {
			auto material_bindings = Material::get_descriptor_set_layout_bindings();
			auto bitmap_bindings = Bitmap::get_descriptor_set_layout_bindings();
			descriptor_set_layouts.material_descriptor_set_layout = device.createDescriptorSetLayout({ { }, material_bindings });
			descriptor_set_layouts.bitmap_descriptor_set_layout = device.createDescriptorSetLayout({ { }, bitmap_bindings });
			std::array<vk::DescriptorPoolSize, 2> poolSizes = {
				vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 10 },
				vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 30 },
			};

			vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 100, poolSizes);	// TODO performance concern.
			descriptor_pool = device.createDescriptorPool(poolInfo);
		}
		auto create_graphics_pipelines() -> void {
			log_step("Vulkan", "Loading Shader Modules");
			auto meshVertShaderCode = readFile("shaders/mesh.vert.spv");
			auto meshFragShaderCode = readFile("shaders/mesh.frag.spv");
			vk::ShaderModuleCreateInfo meshVertInfo({ }, meshVertShaderCode);
			vk::ShaderModuleCreateInfo meshFragInfo({ }, meshFragShaderCode);
			auto meshVertShaderModule = device.createShaderModule(meshVertInfo);
			auto meshFragShaderModule = device.createShaderModule(meshFragInfo);

			auto textVertShaderCode = readFile("shaders/text.vert.spv");
			auto textFragShaderCode = readFile("shaders/text.frag.spv");
			vk::ShaderModuleCreateInfo textVertInfo({ }, textVertShaderCode);
			vk::ShaderModuleCreateInfo textFragInfo({ }, textFragShaderCode);
			auto textVertShaderModule = device.createShaderModule(textVertInfo);
			auto textFragShaderModule = device.createShaderModule(textFragInfo);

			auto uiVertShaderCode = readFile("shaders/ui.vert.spv");
			auto uiFragShaderCode = readFile("shaders/ui.frag.spv");
			vk::ShaderModuleCreateInfo uiVertInfo({ }, uiVertShaderCode);
			vk::ShaderModuleCreateInfo uiFragInfo({ }, uiFragShaderCode);
			auto uiVertShaderModule = device.createShaderModule(uiVertInfo);
			auto uiFragShaderModule = device.createShaderModule(uiFragInfo);
			log_success();

			vk::PipelineShaderStageCreateInfo meshVertexStageInfo({ }, vk::ShaderStageFlagBits::eVertex, meshVertShaderModule, "main");
			vk::PipelineShaderStageCreateInfo meshFragmentStageInfo({ }, vk::ShaderStageFlagBits::eFragment, meshFragShaderModule, "main");
			std::vector<vk::PipelineShaderStageCreateInfo> meshStageInfos = {
				meshVertexStageInfo,
				meshFragmentStageInfo,
			};

			vk::PipelineShaderStageCreateInfo textVertexStageInfo({ }, vk::ShaderStageFlagBits::eVertex, textVertShaderModule, "main");
			vk::PipelineShaderStageCreateInfo textFragmentStageInfo({ }, vk::ShaderStageFlagBits::eFragment, textFragShaderModule, "main");
			std::vector<vk::PipelineShaderStageCreateInfo> textStageInfos = {
				textVertexStageInfo,
				textFragmentStageInfo,
			};

			vk::PipelineShaderStageCreateInfo uiVertexStageInfo({ }, vk::ShaderStageFlagBits::eVertex, uiVertShaderModule, "main");
			vk::PipelineShaderStageCreateInfo uiFragmentStageInfo({ }, vk::ShaderStageFlagBits::eFragment, uiFragShaderModule, "main");
			std::vector<vk::PipelineShaderStageCreateInfo> uiStageInfos = {
				uiVertexStageInfo,
				uiFragmentStageInfo,
			};

			std::vector<vk::DynamicState> dynamicStates = {
				vk::DynamicState::eViewport,
				vk::DynamicState::eScissor,
			};

			vk::PipelineDynamicStateCreateInfo dynamicStateInfo({ }, dynamicStates);

			auto meshVertexBindingDescriptions = MeshModel::Vertex::get_binding_descriptions();
			auto meshVertexAttributeDescriptions = MeshModel::Vertex::get_attribute_descriptions();
			vk::PipelineVertexInputStateCreateInfo meshVertexInputInfo{ { }, meshVertexBindingDescriptions, meshVertexAttributeDescriptions };

			//auto textVertexBindingDescriptions = std::vector<vk::VertexInputBindingDescription>{ };//TextModel::TextVertex::getBindingDescriptions();
			//auto textVertexAttributeDescriptions = std::vector<vk::VertexInputAttributeDescription>{ };//TextModel::TextVertex::getAttributeDescriptions();
			//vk::PipelineVertexInputStateCreateInfo textVertexInputInfo{ { }, textVertexBindingDescriptions, textVertexAttributeDescriptions };

			auto uiVertexBindingDescriptions = UIElement::Vertex::get_binding_descriptions();
			auto uiVertexAttributeDescriptions = UIElement::Vertex::get_attribute_descriptions();
			vk::PipelineVertexInputStateCreateInfo uiVertexInputInfo{ { }, uiVertexBindingDescriptions, uiVertexAttributeDescriptions };

			vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);

			vk::PipelineViewportStateCreateInfo viewportInfo({ }, 1, nullptr, 1, nullptr);

			vk::PipelineRasterizationStateCreateInfo fillRasterizationInfo({ }, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, VK_FALSE, { }, { }, { }, 1.f);

			vk::PipelineRasterizationStateCreateInfo wireRasterizationInfo({ }, VK_FALSE, VK_FALSE, vk::PolygonMode::eLine, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, VK_FALSE, { }, { }, { }, 1.f);

			vk::PipelineMultisampleStateCreateInfo multisampleInfo{ };

			vk::PipelineDepthStencilStateCreateInfo depthStencilInfo({ }, VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE, VK_FALSE);

			vk::PipelineColorBlendAttachmentState colorBlendAttachment{ VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };

			vk::PipelineColorBlendStateCreateInfo colorBlendInfo{ }; colorBlendInfo.attachmentCount = 1; colorBlendInfo.pAttachments = &colorBlendAttachment;

			std::array<vk::DescriptorSetLayout, 2> setLayouts = {
				descriptor_set_layouts.material_descriptor_set_layout,
				descriptor_set_layouts.bitmap_descriptor_set_layout,
			};

			std::vector<vk::PushConstantRange> pushConstantRanges = {
				{ vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantData) }
			};

			log_step("Vulkan", "Creating Pipeline Layout");
			vk::PipelineLayoutCreateInfo pipelineLayoutInfo({ }, setLayouts, pushConstantRanges);
			pipeline_layouts.world_pipeline_layout = device.createPipelineLayout(pipelineLayoutInfo);
			log_success();

			log_step("Vulkan", "Creating Graphics Pipelines");
			/*vk::GraphicsPipelineCreateInfo textPipelineCreateInfo({ }, textStageInfos, &textVertexInputInfo, &inputAssemblyInfo, { }, &viewportInfo, &fillRasterizationInfo, &multisampleInfo, &depthStencilInfo, &colorBlendInfo, &dynamicStateInfo, pipelineLayouts.worldPipelineLayout, renderPass, 0, { }, -1);
			auto result = device.createGraphicsPipeline({ }, textPipelineCreateInfo);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create text graphics pipeline.");
			}
			pipelines.textPipeline = result.value;*/

			vk::GraphicsPipelineCreateInfo meshPipelineCreateInfo({ }, meshStageInfos, &meshVertexInputInfo, &inputAssemblyInfo, { }, &viewportInfo, &fillRasterizationInfo, &multisampleInfo, &depthStencilInfo, &colorBlendInfo, &dynamicStateInfo, pipeline_layouts.world_pipeline_layout, render_pass, 0, { }, -1);
			auto result = device.createGraphicsPipeline({ }, meshPipelineCreateInfo);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create mesh graphics pipeline.");
			}
			pipelines.mesh_pipeline = result.value;

			vk::GraphicsPipelineCreateInfo wireframePipelineCreateInfo({ }, meshStageInfos, &meshVertexInputInfo, &inputAssemblyInfo, { }, &viewportInfo, &wireRasterizationInfo, &multisampleInfo, &depthStencilInfo, &colorBlendInfo, &dynamicStateInfo, pipeline_layouts.world_pipeline_layout, render_pass, 0, { }, -1);
			result = device.createGraphicsPipeline({ }, wireframePipelineCreateInfo);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create wireframe graphics pipeline.");
			}
			pipelines.wireframe_pipeline = result.value;

			vk::GraphicsPipelineCreateInfo uiPipelineCreateInfo({ }, uiStageInfos, &uiVertexInputInfo, &inputAssemblyInfo, { }, &viewportInfo, &fillRasterizationInfo, &multisampleInfo, &depthStencilInfo, &colorBlendInfo, &dynamicStateInfo, pipeline_layouts.world_pipeline_layout, render_pass, 0, { }, -1);
			result = device.createGraphicsPipeline({ }, uiPipelineCreateInfo);
			if (result.result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to create UI graphics pipeline.");
			}
			pipelines.ui_pipeline = result.value;
			log_success();

			device.destroyShaderModule(meshVertShaderModule);
			device.destroyShaderModule(meshFragShaderModule);
			device.destroyShaderModule(textVertShaderModule);
			device.destroyShaderModule(textFragShaderModule);
			device.destroyShaderModule(uiVertShaderModule);
			device.destroyShaderModule(uiFragShaderModule);
		}
		auto allocate_command_buffers() -> void {
			log_step("Vulkan", "Allocating Command Buffers");
			vk::CommandBufferAllocateInfo allocateInfo(command_pool, vk::CommandBufferLevel::ePrimary, 1ui32/*static_cast<uint32_t>(swap_chains.size())*/);
			command_buffer = device.allocateCommandBuffers(allocateInfo)[0];
			log_success();

			//preservedModels.clear();
			//preservedModels.resize(swapChains.size());

			log_step("Vulkan", "Creating synchronizers");
			vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);
			in_flights.resize(swap_chains.size());
			for (int i = 0; i < swap_chains.size(); ++i)
			{
				in_flights[i] = device.createFence(fenceInfo);
			}
			log_success();
		}
		auto create_default_resources() -> void {
			log_step("Vulkan", "Creating default resources");
			std::array<stbi_uc, 4> pixels = { 0, 0, 0, 0 };
			defaults.empty_texture = create_texture(pixels.data(), 1, 1, 4);
			pixels = { 255, 255, 255, 255 };
			defaults.white_texture = create_texture(pixels.data(), 1, 1, 4);
			defaults.default_material = create_material(nullptr, nullptr, glm::vec4{ 0.4f, 0.4f, 0.4f, 1.0f });
			defaults.white_bitmap = create_bitmap(defaults.white_texture);
			defaults.sample_sphere = create_mesh_model(MeshBuilder::Icosphere(0.2f, 3));
			log_success();
		}

		auto create_render_pass(vk::Format format) -> void {
			vk::AttachmentDescription colorAttachmentDescription({ }, format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal);
			vk::AttachmentDescription depthAttachmentDescription({ }, find_depth_format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
			std::array<vk::AttachmentDescription, 2> attachments = { colorAttachmentDescription, depthAttachmentDescription };

			vk::AttachmentReference colorAttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
			std::array<vk::AttachmentReference, 1> colorReferences = { colorAttachmentReference };
			vk::AttachmentReference depthAttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

			vk::SubpassDescription subpass({ }, vk::PipelineBindPoint::eGraphics, { }, colorReferences, { }, &depthAttachmentReference);
			std::array<vk::SubpassDescription, 1> subpasses = { subpass };

			vk::SubpassDependency dependency(VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests, { }, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
			std::array<vk::SubpassDependency, 1> dependencies = { dependency };

			log_step("Vulkan", "Creating Render Pass");
			vk::RenderPassCreateInfo createInfo({ }, attachments, subpasses, dependencies);
			render_pass = device.createRenderPass(createInfo);
			log_success();
		}

		/*auto load_game_object_from_files(std::string name) -> hd::GameObject {	// May creates multiple textures and models, but only a single gameObject.
			std::string modelDirectory = "models/" + name;
			std::string textureDirectory = modelDirectory + "/textures";
			std::string modelPath = modelDirectory + '/' + name + ".obj";

			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			std::vector<tinyobj::material_t> materials;
			std::string warn, err;

			log_step("Vulkan", "Loading Model File");
			if (!tinyobj::loadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str(), modelDirectory.c_str()))
			{
				throw std::runtime_error(warn + err);
			}
			log_success();

			std::vector<hd::Material> mates;
			std::unordered_set<hd::MeshModel> modls;

			log_step("Vulkan", "Creating Textures");
			for (auto& material : materials)
			{
				auto diffuseTex = material.diffuse_texname == "" ? nullptr : std::make_shared<Texture>(textureDirectory + '/' + material.diffuse_texname);
				auto normalTex = material.bump_texname == "" ? nullptr : std::make_shared<Texture>(textureDirectory + '/' + material.bump_texname);
				mates.push_back(std::make_shared<Material>(diffuseTex, normalTex));
			}
			log_success();

			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			std::unordered_map<Vertex, uint32_t> uniqueVertices;

			int shapeCount = 0;
			int materialId = shapes[0].mesh.material_ids[0];	// assume that all faces in a shape have the same material.

			log_step("Vulkan", "Creating Models");
			for (const auto& shape : shapes)
			{
				for (const auto& index : shape.mesh.indices)
				{
					Vertex vertex{ };

					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};

					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.f - attrib.texcoords[2 * index.texcoord_index + 1],
					};

					if (uniqueVertices.count(vertex) == 0)
					{
						uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
						vertices.push_back(vertex);
					}

					indices.push_back(uniqueVertices[vertex]);
				}

				if (++shapeCount >= shapes.size())	// last shape.
				{
					modls.insert(std::make_shared<MeshModel>(vertices, indices, mates[materialId]));	// make current model.
					// no need to manually clear the containers.
				}
				else if (materialId != shapes[shapeCount].mesh.material_ids[0])	// the next shape has different texture.
				{
					modls.insert(std::make_shared<MeshModel>(vertices, indices, mates[materialId]));	// make current model.
					vertices.clear();	// clear.
					indices.clear();
					uniqueVertices.clear();
					materialId = shapes[shapeCount].mesh.material_ids[0];	// set material for the next shape.
				}
			}
			log_success();

			log_info("Vulkan", std::format("Loaded {} textures and {} models", mates.size(), modls.size()), 0);

			hd::GameObject obj = std::make_shared<GameObject>();
			obj->models = std::move(modls);
			return obj;
		}*/

	public: // helper functions.
		auto create_buffer(vk::DeviceSize instance_size, uint32_t instance_count,
			vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_property_flags,
			vk::DeviceSize min_offset_alignment = 1) -> std::tuple<vk::Buffer, vk::DeviceMemory>
		{
			vk::DeviceSize buffer_size = get_alignment(instance_size, min_offset_alignment) * (instance_count ? instance_count : 1);

			// Buffer
			vk::BufferCreateInfo buffer_info({ }, buffer_size, usage_flags, vk::SharingMode::eExclusive);
			vk::Buffer buffer = device.createBuffer(buffer_info);

			// Memory
			vk::MemoryRequirements memory_requirements = device.getBufferMemoryRequirements(buffer);
			vk::MemoryAllocateInfo allo_info(memory_requirements.size, find_memory_type(memory_requirements.memoryTypeBits, memory_property_flags));
			vk::DeviceMemory memory = device.allocateMemory(allo_info);

			// Bind
			device.bindBufferMemory(buffer, memory, 0);

			// Return
			return std::make_tuple(buffer, memory);
		}
		auto create_image(vk::ImageCreateInfo& image_info, vk::MemoryPropertyFlags memory_property_flags) -> std::tuple<vk::Image, vk::DeviceMemory>
		{
			// Image
			vk::Image image = device.createImage(image_info);

			// Memory
			vk::MemoryRequirements memory_requirement = device.getImageMemoryRequirements(image);
			vk::MemoryAllocateInfo allo_info(memory_requirement.size, find_memory_type(memory_requirement.memoryTypeBits, memory_property_flags));
			vk::DeviceMemory memory = device.allocateMemory(allo_info);

			// Bind
			device.bindImageMemory(image, memory, 0);

			// Return
			return std::make_tuple(image, memory);
		}
		auto copy_buffer(vk::Buffer& dst_buffer, vk::Buffer& src_buffer, vk::DeviceSize size) -> void
		{
			auto command_buffer = begin_single_time_command_buffer();

			std::array<vk::BufferCopy, 1> buffer_copies = { vk::BufferCopy{ 0, 0, size ? size : 1 } };
			command_buffer.copyBuffer(src_buffer, dst_buffer, buffer_copies);

			end_single_time_command_buffer(command_buffer);
		}
		auto copy_buffer_to_image(vk::Image& dst_image, vk::Buffer& src_buffer, uint32_t width, uint32_t height) -> void
		{
			auto command_buffer = begin_single_time_command_buffer();

			std::array<vk::BufferImageCopy, 1> image_copy = {
				vk::BufferImageCopy{ 0, 0, 0, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, { 0, 0, 0 }, { width, height, 1 } },
			};
			command_buffer.copyBufferToImage(src_buffer, dst_image, vk::ImageLayout::eTransferDstOptimal, image_copy);

			end_single_time_command_buffer(command_buffer);
		}
		auto destroy_buffer(vk::Buffer& buffer, vk::DeviceMemory& memory) -> void
		{
			device.destroyBuffer(buffer);
			device.freeMemory(memory);
		}
		auto destroyImage(vk::Image& image, vk::DeviceMemory& memory) -> void
		{
			device.destroyImage(image);
			device.freeMemory(memory);
		}

		auto get_alignment(vk::DeviceSize instance_size, vk::DeviceSize min_offset_alignment) -> vk::DeviceSize {
			if (min_offset_alignment > 0) {
				return (instance_size + min_offset_alignment - 1) & ~(min_offset_alignment - 1);
			}
			return instance_size;
		}
		auto find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags memory_property_flags) -> uint32_t
		{
			auto memory_properties = physical_device.getMemoryProperties();
			for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
			{
				if ((type_filter & (1 << i)) && ((memory_properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags))
				{
					return i;
				}
			}
			throw std::runtime_error("Failed to find memory type.");
			return 0;
		}
		auto map_memory(vk::DeviceMemory memory, vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0) -> void*
		{
			return device.mapMemory(memory, offset, size);
		}
		auto unmap_memory(vk::DeviceMemory memory) -> void
		{
			device.unmapMemory(memory);
		}

		auto transition_image_layout(vk::Image image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout, uint32_t mip_levels) -> void
		{
			auto commandBuffer = begin_single_time_command_buffer();

			vk::ImageMemoryBarrier barrier({ }, { }, old_layout, new_layout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, { vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1 });
			vk::PipelineStageFlags source_stage, destination_stage;
			if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
			{
				barrier.srcAccessMask = { };
				barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
				source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
				destination_stage = vk::PipelineStageFlagBits::eTransfer;
			}
			else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
			{
				barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
				barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
				source_stage = vk::PipelineStageFlagBits::eTransfer;
				destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
			}
			else
			{
				throw std::invalid_argument("Unsupported layout transition.");
			}
			std::array<vk::ImageMemoryBarrier, 1> barriers = {
				barrier,
			};

			commandBuffer.pipelineBarrier(source_stage, destination_stage, { }, { }, { }, barriers);

			end_single_time_command_buffer(commandBuffer);
		}
		auto generate_mipmaps(vk::Image image, int32_t tex_width, int32_t tex_height, uint32_t mip_levels) -> void
		{
			auto command_buffer = begin_single_time_command_buffer();

			vk::ImageMemoryBarrier barrier({ }, { }, { }, { }, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

			int32_t mip_width = tex_width;
			int32_t mip_height = tex_height;

			for (uint32_t i = 1; i < mip_levels; ++i)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
				barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
				barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
				barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

				command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, { }, { }, { }, { barrier });

				vk::ImageBlit blit(
					{ vk::ImageAspectFlagBits::eColor, i - 1, 0, 1 },
					std::array<vk::Offset3D, 2>({ { 0, 0, 0 }, { mip_width, mip_height, 1 } }),
					{ vk::ImageAspectFlagBits::eColor, i, 0, 1 },
					std::array<vk::Offset3D, 2>({ { 0, 0, 0 }, { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 } })
				);

				command_buffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

				barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
				barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
				barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

				command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, { }, { }, { }, { barrier });

				if (mip_width > 1) mip_width /= 2;
				if (mip_height > 1) mip_height /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mip_levels - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, { }, { }, { }, { barrier });

			end_single_time_command_buffer(command_buffer);
		}
		auto create_image_view(vk::Image& image, vk::Format format, uint32_t mip_levels = 1, vk::ImageAspectFlags aspect_flags = vk::ImageAspectFlagBits::eColor) -> vk::ImageView
		{
			vk::ImageViewCreateInfo view_info({}, image, vk::ImageViewType::e2D, format, { }, { aspect_flags, 0, mip_levels, 0, 1 });
			return device.createImageView(view_info);
		}
		auto find_depth_format() -> vk::Format {
			return find_supported_format({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
		}
		auto find_supported_format(const std::vector<vk::Format> candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) -> vk::Format {
			for (auto format : candidates)
			{
				auto properties = physical_device.getFormatProperties(format);
				if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features)
				{
					return format;
				}
				if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features)
				{
					return format;
				}
			}
			throw std::runtime_error("Failed to find supported format.");
		}

		auto create_swap_chain(vk::RenderPass render_pass, std::vector<vk::Image>& images, vk::Format format, vk::Rect2D rect) -> Swapchain* {
			auto length = static_cast<uint32_t>(images.size());
			std::vector<vk::ImageView> image_views{ };
			vk::Image depth_image;
			vk::DeviceMemory depth_image_memory;
			vk::ImageView depth_image_view;
			std::vector<vk::Framebuffer> framebuffers{ };

			// ImageViews.
			for (auto& image : images) {
				vk::ImageViewCreateInfo create_info{ { }, image, vk::ImageViewType::e2D, format, { }, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
				image_views.push_back(device.createImageView(create_info));
			}

			if (render_pass) {
				// Depth.
				auto depth_format = find_depth_format();
				std::array<uint32_t, 1> queue_family_indices = { 0 };
				vk::ImageCreateInfo image_info({ }, vk::ImageType::e2D, depth_format, { static_cast<uint32_t>(rect.extent.width), static_cast<uint32_t>(rect.extent.height), 1 }, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::SharingMode::eExclusive, queue_family_indices, vk::ImageLayout::eUndefined);
				auto [depth_image, depth_image_memory] = create_image(image_info, vk::MemoryPropertyFlagBits::eDeviceLocal);
				auto depth_image_view = create_image_view(depth_image, depth_format, 1, vk::ImageAspectFlagBits::eDepth);

				// Framebuffer.
				for (auto& view : image_views) {
					std::array<vk::ImageView, 2> attachments = { view, depth_image_view };
					vk::FramebufferCreateInfo create_info({ }, render_pass, attachments, rect.extent.width, rect.extent.height, 1);
					framebuffers.push_back(device.createFramebuffer(create_info));
				}
			}

			return new Swapchain{ length, images, image_views, framebuffers, depth_image, depth_image_memory, depth_image_view, format, rect, render_pass };
		}

		auto calculate_tangent_bitangent(std::vector<MeshModel::Vertex>& vertices, const std::vector<uint32_t>& indices) -> void
		{
			if (indices.size() % 3 != 0)
			{
				throw std::runtime_error("Not a complete triangle mesh.");
			}
			std::vector<uint32_t> count(vertices.size(), 0);

			for (size_t i = 0; i < indices.size(); i += 3)
			{
				uint32_t index0 = indices[i + 0];
				uint32_t index1 = indices[i + 1];
				uint32_t index2 = indices[i + 2];

				++count[index0];
				++count[index1];
				++count[index2];

				MeshModel::Vertex& v0 = vertices[index0];
				MeshModel::Vertex& v1 = vertices[index1];
				MeshModel::Vertex& v2 = vertices[index2];

				glm::vec3& p0 = v0.position;
				glm::vec3& p1 = v1.position;
				glm::vec3& p2 = v2.position;

				glm::vec2& u0 = v0.uv;
				glm::vec2& u1 = v1.uv;
				glm::vec2& u2 = v2.uv;

				glm::vec3 deltaP1 = p1 - p0;
				glm::vec3 deltaP2 = p2 - p0;

				glm::vec2 deltaU1 = u1 - u0;
				glm::vec2 deltaU2 = u2 - u0;

				float r = 1.0f / (deltaU1.x * deltaU2.y - deltaU2.x * deltaU1.y);
				glm::vec3 tangent = (deltaP1 * deltaU2.y - deltaP2 * deltaU1.y) * r;
				glm::vec3 bitangent = (deltaP2 * deltaU1.x - deltaP1 * deltaU2.x) * r;

				v0.tangent += tangent;
				v0.bitangent += bitangent;

				v1.tangent += tangent;
				v1.bitangent += bitangent;

				v2.tangent += tangent;
				v2.bitangent += bitangent;
			}

			for (int i = 0; i < vertices.size(); ++i)
			{
				vertices[i].tangent /= count[i];
				vertices[i].bitangent /= count[i];
			}
		}

		auto begin_single_time_command_buffer() -> vk::CommandBuffer
		{
			vk::CommandBufferAllocateInfo allo_info(command_pool, vk::CommandBufferLevel::ePrimary, 1);
			auto command_buffer = device.allocateCommandBuffers(allo_info)[0];

			vk::CommandBufferBeginInfo begin_info({ });
			command_buffer.begin(begin_info);

			return command_buffer;
		}
		auto end_single_time_command_buffer(vk::CommandBuffer& command_buffer) -> void
		{
			command_buffer.end();
			std::array<vk::CommandBuffer, 1> command_buffers = { command_buffer };

			vk::SubmitInfo submit_info({ }, { }, command_buffers);
			std::array<vk::SubmitInfo, 1> infos = { submit_info };
			queue.submit(infos);
			queue.waitIdle();

			device.freeCommandBuffers(command_pool, command_buffers);
		}

		static auto readFile(const std::string& filepath) -> std::vector<uint32_t> {
			std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

			if (!file.is_open())
			{
				throw std::runtime_error("Failed to open file: " + filepath);
			}

			size_t fileSize = static_cast<size_t>(file.tellg());
			std::vector<uint32_t> buffer((fileSize / 4) + (bool)(fileSize % 4));

			file.seekg(0);
			file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

			file.close();
			return buffer;
		}
		
		DebugMode debug_mode = DebugMode::Mixed;
		struct {
			Texture* empty_texture;
			Texture* white_texture;
			Material* default_material;
			Bitmap* white_bitmap;
			MeshModel* sample_sphere;
		} defaults;

	private: // Owning. Responsible to destroy them. Or value types.
		vk::Instance instance;
		vk::PhysicalDevice physical_device;
		float max_anisotrophy;
		vk::Device device;
		uint32_t queue_family_index;
		vk::Queue queue;
		vk::RenderPass render_pass;
		vk::DescriptorPool descriptor_pool;
		struct {
			vk::DescriptorSetLayout material_descriptor_set_layout;
			vk::DescriptorSetLayout bitmap_descriptor_set_layout;
		} descriptor_set_layouts;
		struct {
			vk::PipelineLayout world_pipeline_layout;
		} pipeline_layouts;
		struct {
			vk::Pipeline mesh_pipeline;
			vk::Pipeline text_pipeline;
			vk::Pipeline wireframe_pipeline;
			vk::Pipeline ui_pipeline;
			vk::Pipeline shadow_pipeline;
		} pipelines;
		vk::CommandPool command_pool;
		vk::CommandBuffer command_buffer;
		// std::vector<std::vector<hd::MeshModel>> preservedModels;	// models are preserved by a commandBuffer when they are being drawn.
		vk::Semaphore draw_done;
		std::vector<vk::Fence> in_flights;
		std::vector<Swapchain*> swap_chains;

		std::unordered_set<UIElement*> ui_elements_to_delete;

#ifdef MIRROR_WINDOW
		GLFWwindow* window = nullptr;
		vk::SurfaceKHR mirrorSurface;
		vk::SwapchainKHR mirrorVkSwapchain;
		Swapchain* mirrorSwapchain;
		vk::Semaphore mirrorImageAvailableSemaphore;
		uint32_t mirrorView = 0;
		uint32_t mirrorImageHeight = 1440ui32;
#endif

	public: // Not Owning. Don't try to destroy them.
		std::unordered_set<std::multiset<std::tuple<Material*, MeshModel*, glm::mat4*>>*> mesh_models;	// What a crazy type :D
		std::unordered_set<std::list<std::tuple<Bitmap*, UIElement*, glm::mat4*>>*> ui_elements;
		std::unordered_set<std::multiset<std::tuple<Material*, MeshModel*, glm::mat4*>>*> debug_mesh_models;
	};
}