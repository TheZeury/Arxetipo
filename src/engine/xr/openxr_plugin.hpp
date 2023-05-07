#pragma once

#define XR_USE_GRAPHICS_API_VULKAN
#include <vulkan/vulkan.hpp>
#include <openxr/openxr_platform.h>
#include <openxr/openxr.hpp>
#include <OpenXR-SDK/src/common/xr_linear.h>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "../helpers/arx_logger.hpp"
#include "../helpers/arx_math.hpp"
#include "../proxy/renderer_proxy.hpp"
#include "../renderer/vulkan_renderer.hpp"


namespace arx
{
	struct InputAction
	{
		xr::ActionSet action_set;
		xr::Action pose_action;
		xr::Action trigger_action;
		xr::Action grip_action;
		xr::Action primary_button_action;
		xr::Action secondary_button_action;
		xr::Action thumbstick_x_action;
		xr::Action thumbstick_y_action;
		xr::Action vibrate_action;
		std::array<xr::Path, 2> hand_subaction_path;
		std::array<xr::Space, 2> hand_space;
	};

	struct InputState
	{
		std::array<xr::Bool32, 2> hand_active;
		std::array<xr::SpaceLocation, 2> hand_locations;
		std::array<xr::ActionStateFloat, 2> trigger_states;
		std::array<xr::ActionStateFloat, 2> grip_states;
		std::array<xr::ActionStateBoolean, 2> primary_button_states;
		std::array<xr::ActionStateBoolean, 2> secondary_button_states;
		std::array<xr::ActionStateFloat, 2> thumbstick_x_states;
		std::array<xr::ActionStateFloat, 2> thumbstick_y_states;
	};

	// MAKE_HANDLE(OpenXrInstance);

	using P = VulkanOpenXrProxy;
	using R = VulkanRenderer;
	//export template <typename R, typename P>
	//requires OpenXrProvidingProxy<P>
	class OpenXRPlugin
	{
	public: // concept: XRPlugin
		auto initialize() -> P {
			create_instance();
			initialize_system();
			return P{ instance, system_id };
		}
		auto initialize_session(P& proxy) -> void {
			create_session(proxy.get_graphics_binding());
			create_space();
			create_swap_chains(proxy);
			create_actions();
		}
		auto clean_up_instance() -> void {
			if (instance != nullptr) instance.destroy();
		}
		auto clean_up_session() -> void {
			for (auto& swapchain : swap_chains)
			{
				//		swapchain.destroy();
			}

			if (app_space != nullptr) app_space.destroy();

			if (session != nullptr) session.destroy();
		}
		auto poll_events() -> bool {
			bool go_on = true;

			xr::Result result = xr::Result::Success;

			while (result == xr::Result::Success)
			{
				event_data_buffer.type = xr::StructureType::EventDataBuffer;
				result = instance.pollEvent(event_data_buffer);
				if (result == xr::Result::Success)
				{
					//log_info("OpenXR", std::format("Event Type : {}", xr::to_string(eventDataBuffer.type)), 0);
					switch (event_data_buffer.type)
					{
					case xr::StructureType::EventDataInstanceLossPending:
					{
						return false;
						break;
					}
					case xr::StructureType::EventDataSessionStateChanged:
					{
						xr::EventDataSessionStateChanged event_data_session_state_changed(*reinterpret_cast<xr::EventDataSessionStateChanged*>(&event_data_buffer));
						go_on = handle_session_state_changed_event(event_data_session_state_changed);
						break;
					}
					default:
						break;
					}

					continue;
				}
			}

			if (result != xr::Result::EventUnavailable)
			{
				throw std::runtime_error(std::format("Unable to read Event. Result : {}", xr::to_string(result)));
			}

			return go_on;
		}
		auto running() -> bool {
			return session_running;
		}
		auto poll_actions() -> void {
			input_state.hand_active = { XR_FALSE, XR_FALSE };

			xr::ActiveActionSet active_action_set(input_action.action_set, { });
			xr::ActionsSyncInfo sync_info(1, &active_action_set);
			session.syncActions(sync_info);

			for (int hand : { 0, 1 })
			{
				xr::ActionStateGetInfo get_info(input_action.pose_action, input_action.hand_subaction_path[hand]);

				xr::ActionStatePose pose_state = session.getActionStatePose(get_info);
				input_state.hand_active[hand] = pose_state.isActive;

				get_info.action = input_action.trigger_action;
				xr::ActionStateFloat trigger_state = session.getActionStateFloat(get_info);
				input_state.trigger_states[hand] = trigger_state;

				get_info.action = input_action.grip_action;
				xr::ActionStateFloat grip_state = session.getActionStateFloat(get_info);
				input_state.grip_states[hand] = grip_state;

				get_info.action = input_action.primary_button_action;
				xr::ActionStateBoolean primary_button_state = session.getActionStateBoolean(get_info);
				input_state.primary_button_states[hand] = primary_button_state;

				get_info.action = input_action.secondary_button_action;
				xr::ActionStateBoolean secondary_button_state = session.getActionStateBoolean(get_info);
				input_state.secondary_button_states[hand] = secondary_button_state;

				get_info.action = input_action.thumbstick_x_action;
				xr::ActionStateFloat thumbstick_x_state = session.getActionStateFloat(get_info);
				input_state.thumbstick_x_states[hand] = thumbstick_x_state;

				get_info.action = input_action.thumbstick_y_action;
				xr::ActionStateFloat thumbstick_y_state = session.getActionStateFloat(get_info);
				input_state.thumbstick_y_states[hand] = thumbstick_y_state;
			}
		}
		auto update(const glm::mat4& camera_offset = glm::mat4{ 1.f }) -> void {
			auto frame_state = session.waitFrame({ });
			session.beginFrame({ });

			std::vector<xr::CompositionLayerBaseHeader*> layers = { };
			xr::CompositionLayerProjection layer{ };
			std::vector<xr::CompositionLayerProjectionView> projection_layer_views;

			if (frame_state.shouldRender == XR_TRUE)
			{
				for (int hand : { 0, 1 })
				{
					input_state.hand_locations[hand] = input_action.hand_space[hand].locateSpace(app_space, frame_state.predictedDisplayTime);
				}

				xr::ViewState view_state{ };
				xr::ViewLocateInfo locate_info(xr::ViewConfigurationType::PrimaryStereo, frame_state.predictedDisplayTime, app_space);
				auto views = session.locateViewsToVector(locate_info, reinterpret_cast<XrViewState*>(&view_state));
				if ((view_state.viewStateFlags & xr::ViewStateFlagBits::PositionValid) && (view_state.viewStateFlags & xr::ViewStateFlagBits::OrientationValid))
				{
					projection_layer_views.resize(views.size());
					std::vector<std::tuple<glm::mat4, glm::mat4, uint32_t>> xr_camera(views.size());
					for (uint32_t i = 0; i < views.size(); ++i)
					{
						auto& [mat_projection, mat_camera_transform, image_index] = xr_camera[i];

						image_index = swap_chains[i].acquireSwapchainImage({});
						swap_chains[i].waitSwapchainImage({xr::Duration::infinite()});

						projection_layer_views[i] = xr::CompositionLayerProjectionView{ views[i].pose, views[i].fov, { swap_chains[i], swap_chain_rects[i], 0}};

						XrMatrix4x4f_CreateProjectionFov(&cnv<XrMatrix4x4f>(mat_projection), GRAPHICS_VULKAN, views[i].fov, DEFAULT_NEAR_Z, INFINITE_FAR_Z);
						glm::mat4 eye_pose;
						glm::vec3 identity{ 1.f, 1.f, 1.f };
						XrMatrix4x4f_CreateTranslationRotationScale(
							&cnv<XrMatrix4x4f>(eye_pose), 
							&(projection_layer_views[i].pose.get()->position), 
							&(projection_layer_views[i].pose.get()->orientation), 
							&cnv<XrVector3f>(identity)
						);
						mat_camera_transform = camera_offset * eye_pose;
					}

					renderer.render_view_xr(xr_camera); // Renderer.
					
					for (uint32_t i = 0; i < views.size(); ++i)
					{
						swap_chains[i].releaseSwapchainImage({ });
					}

					layer.space = app_space;
					layer.layerFlags = { };
					layer.viewCount = (uint32_t)projection_layer_views.size();
					layer.views = projection_layer_views.data();
					layers.push_back(&layer);
				}
			}

			xr::FrameEndInfo end_info(frame_state.predictedDisplayTime, xr::EnvironmentBlendMode::Opaque, static_cast<uint32_t>(layers.size()), layers.data());
			session.endFrame(end_info);
		}

	public:
		OpenXRPlugin(R& renderer) : renderer{ renderer } {
		}

		auto create_instance() -> void {
			// Logging Extensions and Layers -----------------------------------------------------
			log_step("OpenXR", "Enumerating Supported OpenXR Extensions");
			auto all_extensions = xr::enumerateInstanceExtensionPropertiesToVector(nullptr);
			log_success();
			log_info("OpenXR", std::format("List of total {} OpenXR Extension(s):", all_extensions.size()), 0);
			for (const auto& ext : all_extensions)
			{
				log_info("OpenXR", ext.extensionName, 1);
			}

			log_step("OpenXR", "Enumerating API Layers");
			auto layers = xr::enumerateApiLayerPropertiesToVector();
			log_success();
			log_info("OpenXR", std::format("List of total {} API Layer(s):", layers.size()), 0);
			for (const auto& layer : layers)
			{
				log_info("OpenXR", layer.layerName, 1);
			}

			// Creating Instance ------------------------------------------------------------------
			if (instance)
			{
				throw std::runtime_error("Can't create a new OpenXR instance when there's already one!");
			}

			std::vector<const char*> extensions;
			auto render_extensions = P::get_renderer_specific_xr_extensions();
			extensions.insert(extensions.end(), render_extensions.begin(), render_extensions.end());
			log_info("OpenXR", std::format("List of total {} required Extension(s)", extensions.size()), 0);
			for (const auto& ext : extensions)
			{
				log_info("OpenXR", ext, 1);
			}

			log_step("OpenXR", "Creating OpenXR Instance");
			xr::ApplicationInfo app_info(
				"Arxetipo",
				1,
				"Arxetipo Engine",
				1,
				xr::Version::current()
			);

			xr::InstanceCreateFlags flags = { };
			xr::InstanceCreateInfo instance_create_info(
				{ },
				app_info,
				0,
				nullptr,
				static_cast<uint32_t>(extensions.size()),
				extensions.data()
			);

			instance = xr::createInstance(instance_create_info);
			log_success();

			// Logging Instance Properties -------------------------------------------------------
			auto instance_property = instance.getInstanceProperties();
			log_info(
				"OpenXR",
				std::format("OpenXR Runtime = {}, RuntimeVersion = {}.{}.{}",
					instance_property.runtimeName,
					instance_property.runtimeVersion.major(),
					instance_property.runtimeVersion.minor(),
					instance_property.runtimeVersion.patch()
				),
				0
			);
		}
		auto initialize_system() -> void {
			log_step("OpenXR", "Initializing OpenXR System");

			if (!instance)
			{
				throw std::runtime_error("Can't initialize OpenXR system when there's no instance.");
			}
			if (system_id)
			{
				throw std::runtime_error("Can't initialize OpenXR system when there's already one.");
			}

			xr::SystemGetInfo system_get_info(
				xr::FormFactor::HeadMountedDisplay
			);

			system_id = instance.getSystem(system_get_info);

			log_success();
		}

		auto create_session(P::GraphicsBindingType graphics_binding) -> void {
			log_step("OpenXR", "Creating OpenXR Session");
			xr::SessionCreateInfo create_info({}, system_id, &graphics_binding);
			session = instance.createSession(create_info);
			log_success();
		}
		auto create_space() -> void {
			log_step("OpenXR", "Creating Reference Space");
			xr::ReferenceSpaceCreateInfo ref_space_create_info(xr::ReferenceSpaceType::Stage, { });
			app_space = session.createReferenceSpace(ref_space_create_info);
			log_success();
		}
		auto create_swap_chains(P& proxy) -> void {
			if (!system_id)
			{
				throw std::runtime_error("No system ID.");
			}

			auto system_porperties = instance.getSystemProperties(system_id);
			log_info("OpenXR", "System Properties: ", 0);
			log_info("OpenXR", std::format("System Name : {}, Vendor ID : {}", system_porperties.systemName, system_porperties.vendorId), 1);
			log_info("OpenXR", std::format("Graphics Properties: MaxSwapChainImageWidth = {}, MaxSwapChainImageHeight = {}, MaxLayers = {}", system_porperties.graphicsProperties.maxSwapchainImageWidth, system_porperties.graphicsProperties.maxSwapchainImageHeight, system_porperties.graphicsProperties.maxLayerCount), 1);
			log_info("OpenXR", std::format("Tracking Properties: OrientationTracking = {}, PositionTracking = {}", system_porperties.trackingProperties.orientationTracking == XR_TRUE, system_porperties.trackingProperties.positionTracking == XR_TRUE), 1);

			config_views = instance.enumerateViewConfigurationViewsToVector(system_id, xr::ViewConfigurationType::PrimaryStereo);
			log_info("OpenXR", std::format("Number of Views: {}", config_views.size()), 0);

			auto swap_chain_formats = session.enumerateSwapchainFormatsToVector();
			swap_chain_format = vk::Format::eB8G8R8A8Srgb;
			int64_t i64_format;
			bool find = false;
			log_info("OpenXR", std::format("Searching for Image Format, Desired Format : {}, All available Formats: ", vk::to_string(swap_chain_format)), 0);
			for (auto format : swap_chain_formats)
			{
				vk::Format vk_format = (vk::Format)format;
				if (vk_format == swap_chain_format)
				{
					find = true;
					i64_format = format;
					log_info("OpenXR", std::format("{} <= Found.", vk::to_string(vk_format)), 1);
				}
				else
				{
					log_info("OpenXR", vk::to_string(vk_format), 1);
				}
			}
			if (!find)
			{
				swap_chain_format = (vk::Format)swap_chain_formats[0];
				i64_format = swap_chain_formats[0];
				log_info("OpenXR", std::format("Cannot find desired format, falling back to first available format : {}", vk::to_string(swap_chain_format)), 0);
			}

			swap_chains.clear();
			swap_chain_rects.clear();
			for (const auto& view : config_views)
			{
				log_step("OpenXR", "Creating XR Side SwapChain");
				xr::SwapchainCreateInfo swap_chain_info({}, xr::SwapchainUsageFlagBits::Sampled | xr::SwapchainUsageFlagBits::ColorAttachment | xr::SwapchainUsageFlagBits::TransferSrc, i64_format, view.recommendedSwapchainSampleCount, view.recommendedImageRectWidth, view.recommendedImageRectHeight, 1, 1, 1);
				auto swap_chain = session.createSwapchain(swap_chain_info);
				swap_chains.push_back(swap_chain);
				swap_chain_rects.push_back({ { 0, 0 }, { static_cast<int32_t>(view.recommendedImageRectWidth), static_cast<int32_t>(view.recommendedImageRectHeight) } });
				log_success();
				//log_info("OpenXR", std::format("SwapChainImageType = {}", xr::to_string(swap_chainImages[0][0].type)), 0);
				log_info("OpenXR", std::format("SwapChainImageExtent = ({}, {}, {})", view.recommendedSwapchainSampleCount, view.recommendedImageRectWidth, view.recommendedImageRectHeight), 0);
			}
			proxy.passin_xr_swapchains(swap_chains, swap_chain_rects, i64_format);
			//graphics.create_swap_chain(proxy);
		}
		auto create_actions() -> void {
			xr::ActionSetCreateInfo set_info("gameplay", "Gameplay", 0);
			input_action.action_set = instance.createActionSet(set_info);

			input_action.hand_subaction_path[0] = instance.stringToPath("/user/hand/left");
			input_action.hand_subaction_path[1] = instance.stringToPath("/user/hand/right");

			xr::ActionCreateInfo pose_action_info("hand_pose", xr::ActionType::PoseInput, static_cast<uint32_t>(input_action.hand_subaction_path.size()), input_action.hand_subaction_path.data(), "Hand Pose");
			input_action.pose_action = input_action.action_set.createAction(pose_action_info);

			std::array<xr::Path, 2> pose_path = {
				instance.stringToPath("/user/hand/left/input/grip/pose"),
				instance.stringToPath("/user/hand/right/input/grip/pose"),
			};

			xr::ActionCreateInfo trigger_action_info("trigger", xr::ActionType::FloatInput, static_cast<uint32_t>(input_action.hand_subaction_path.size()), input_action.hand_subaction_path.data(), "Trigger");
			input_action.trigger_action = input_action.action_set.createAction(trigger_action_info);

			std::array<xr::Path, 2> trigger_path = {
				instance.stringToPath("/user/hand/left/input/trigger/value"),
				instance.stringToPath("/user/hand/right/input/trigger/value"),
			};

			xr::ActionCreateInfo grip_action_info("grip", xr::ActionType::FloatInput, static_cast<uint32_t>(input_action.hand_subaction_path.size()), input_action.hand_subaction_path.data(), "Grip");
			input_action.grip_action = input_action.action_set.createAction(grip_action_info);

			std::array<xr::Path, 2> grip_path = {
				instance.stringToPath("/user/hand/left/input/squeeze/value"),
				instance.stringToPath("/user/hand/right/input/squeeze/value"),
			};

			xr::ActionCreateInfo primary_button_action_info("primary_button", xr::ActionType::BooleanInput, static_cast<uint32_t>(input_action.hand_subaction_path.size()), input_action.hand_subaction_path.data(), "Primary Button");
			input_action.primary_button_action = input_action.action_set.createAction(primary_button_action_info);

			std::array<xr::Path, 2> primary_button_path = {
				instance.stringToPath("/user/hand/left/input/x/click"),
				instance.stringToPath("/user/hand/right/input/a/click"),
			};

			xr::ActionCreateInfo secondary_button_action_info("secondary_button", xr::ActionType::BooleanInput, static_cast<uint32_t>(input_action.hand_subaction_path.size()), input_action.hand_subaction_path.data(), "Secondary Button");
			input_action.secondary_button_action = input_action.action_set.createAction(secondary_button_action_info);

			std::array<xr::Path, 2> secondary_button_path = {
				instance.stringToPath("/user/hand/left/input/y/click"),
				instance.stringToPath("/user/hand/right/input/b/click"),
			};

			xr::ActionCreateInfo thumbstick_x_action_info("thumbstick_x", xr::ActionType::FloatInput, static_cast<uint32_t>(input_action.hand_subaction_path.size()), input_action.hand_subaction_path.data(), "ThumbStick X");
			input_action.thumbstick_x_action = input_action.action_set.createAction(thumbstick_x_action_info);

			std::array<xr::Path, 2> thumbstick_x_path = {
				instance.stringToPath("/user/hand/left/input/thumbstick/x"),
				instance.stringToPath("/user/hand/right/input/thumbstick/x"),
			};

			xr::ActionCreateInfo thumbstick_y_action_info("thumbstick_y", xr::ActionType::FloatInput, static_cast<uint32_t>(input_action.hand_subaction_path.size()), input_action.hand_subaction_path.data(), "ThumbStick Y");
			input_action.thumbstick_y_action = input_action.action_set.createAction(thumbstick_y_action_info);

			std::array<xr::Path, 2> thumbstick_y_path = {
				instance.stringToPath("/user/hand/left/input/thumbstick/y"),
				instance.stringToPath("/user/hand/right/input/thumbstick/y"),
			};

			xr::ActionCreateInfo vibrate_action_info("vibrate", xr::ActionType::VibrationOutput, static_cast<uint32_t>(input_action.hand_subaction_path.size()), input_action.hand_subaction_path.data(), "Virbate");
			input_action.vibrate_action = input_action.action_set.createAction(vibrate_action_info);

			std::array<xr::Path, 2> vibrate_path = {
				instance.stringToPath("/user/hand/left/output/haptic"),
				instance.stringToPath("/user/hand/right/output/haptic"),
			};

			xr::Path oculus_touch_interaction_profile_path = instance.stringToPath("/interaction_profiles/oculus/touch_controller");

			std::vector<xr::ActionSuggestedBinding> binding = {
				xr::ActionSuggestedBinding{ input_action.pose_action, pose_path[0] },
				xr::ActionSuggestedBinding{ input_action.pose_action, pose_path[1] },
				xr::ActionSuggestedBinding{ input_action.trigger_action, trigger_path[0] },
				xr::ActionSuggestedBinding{ input_action.trigger_action, trigger_path[1] },
				xr::ActionSuggestedBinding{ input_action.grip_action, grip_path[0] },
				xr::ActionSuggestedBinding{ input_action.grip_action, grip_path[1] },
				xr::ActionSuggestedBinding{ input_action.primary_button_action, primary_button_path[0] },
				xr::ActionSuggestedBinding{ input_action.primary_button_action, primary_button_path[1] },
				xr::ActionSuggestedBinding{ input_action.secondary_button_action, secondary_button_path[0] },
				xr::ActionSuggestedBinding{ input_action.secondary_button_action, secondary_button_path[1] },
				xr::ActionSuggestedBinding{ input_action.thumbstick_x_action, thumbstick_x_path[0] },
				xr::ActionSuggestedBinding{ input_action.thumbstick_x_action, thumbstick_x_path[1] },
				xr::ActionSuggestedBinding{ input_action.thumbstick_y_action, thumbstick_y_path[0] },
				xr::ActionSuggestedBinding{ input_action.thumbstick_y_action, thumbstick_y_path[1] },
				xr::ActionSuggestedBinding{ input_action.vibrate_action, vibrate_path[0] },
				xr::ActionSuggestedBinding{ input_action.vibrate_action, vibrate_path[1] },
			};
			xr::InteractionProfileSuggestedBinding suggested_binding(oculus_touch_interaction_profile_path, static_cast<uint32_t>(binding.size()), binding.data());
			instance.suggestInteractionProfileBindings(suggested_binding);

			xr::ActionSpaceCreateInfo action_space_info(input_action.pose_action, { }, { });
			action_space_info.subactionPath = input_action.hand_subaction_path[0];
			input_action.hand_space[0] = session.createActionSpace(action_space_info);
			action_space_info.subactionPath = input_action.hand_subaction_path[1];
			input_action.hand_space[1] = session.createActionSpace(action_space_info);

			xr::SessionActionSetsAttachInfo attach_info(1, &input_action.action_set);
			session.attachSessionActionSets(attach_info);
		}

		auto handle_session_state_changed_event(xr::EventDataSessionStateChanged event_data_session_state_changed) -> bool {
			auto old_state = session_state;
			session_state = event_data_session_state_changed.state;
			log_info("OpenXR", std::format("Session State Changed: {} -> {}", xr::to_string(old_state), xr::to_string(session_state)), 0);

			switch (session_state)
			{
			case xr::SessionState::Ready:
			{
				xr::SessionBeginInfo session_begin_info(xr::ViewConfigurationType::PrimaryStereo);
				session.beginSession(session_begin_info);
				session_running = true;
				break;
			}
			case xr::SessionState::Stopping:
			{
				session_running = false;
				session.endSession();
				break;
			}
			case xr::SessionState::Exiting:
			{
				return false;
				break;
			}
			case xr::SessionState::LossPending:
			{
				return false;
				break;
			}
			default:
				break;
			}
			return true;
		}

	public:
		auto vibrate(const xr::HapticVibration& virbation, int hand) -> void {
			xr::HapticActionInfo haptic_info(input_action.vibrate_action, input_action.hand_subaction_path[hand]);
			if (session.applyHapticFeedback(haptic_info, virbation.get_base()) != xr::Result::Success)
			{
				throw std::runtime_error("failed to vibrate.");
			}
		}
		InputState input_state;

	private:
		xr::Instance instance;
		xr::Session session;
		xr::Space app_space;
		xr::SystemId system_id;
		std::vector<xr::Swapchain> swap_chains;
		std::vector<xr::Rect2Di> swap_chain_rects;
		std::vector<xr::ViewConfigurationView> config_views;
		vk::Format swap_chain_format;
		xr::EventDataBuffer event_data_buffer;
		xr::SessionState session_state;
		bool session_running;

		InputAction input_action;

	private:
		R& renderer;
	};
}