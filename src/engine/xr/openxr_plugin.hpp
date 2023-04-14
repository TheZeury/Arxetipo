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
		xr::ActionSet actionSet;
		xr::Action poseAction;
		xr::Action triggerAction;
		xr::Action gripAction;
		xr::Action primaryButtonAction;
		xr::Action secondaryButtonAction;
		xr::Action thumbstickXAction;
		xr::Action thumbstickYAction;
		xr::Action vibrateAction;
		std::array<xr::Path, 2> handSubactionPath;
		std::array<xr::Space, 2> handSpace;
	};

	struct InputState
	{
		std::array<xr::Bool32, 2> handActive;
		std::array<xr::SpaceLocation, 2> handLocations;
		std::array<xr::ActionStateFloat, 2> triggerStates;
		std::array<xr::ActionStateFloat, 2> gripStates;
		std::array<xr::ActionStateBoolean, 2> primaryButtonStates;
		std::array<xr::ActionStateBoolean, 2> secondaryButtonStates;
		std::array<xr::ActionStateFloat, 2> thumbstickXStates;
		std::array<xr::ActionStateFloat, 2> thumbstickYStates;
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
						xr::EventDataSessionStateChanged eventDataSessionStateChanged(*reinterpret_cast<xr::EventDataSessionStateChanged*>(&event_data_buffer));
						go_on = handle_session_state_changed_event(eventDataSessionStateChanged);
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
			input_state.handActive = { XR_FALSE, XR_FALSE };

			xr::ActiveActionSet activeActionSet(input_action.actionSet, { });
			xr::ActionsSyncInfo syncInfo(1, &activeActionSet);
			session.syncActions(syncInfo);

			for (int hand : { 0, 1 })
			{
				xr::ActionStateGetInfo getInfo(input_action.poseAction, input_action.handSubactionPath[hand]);

				xr::ActionStatePose poseState = session.getActionStatePose(getInfo);
				input_state.handActive[hand] = poseState.isActive;

				getInfo.action = input_action.triggerAction;
				xr::ActionStateFloat triggerState = session.getActionStateFloat(getInfo);
				input_state.triggerStates[hand] = triggerState;

				getInfo.action = input_action.gripAction;
				xr::ActionStateFloat gripState = session.getActionStateFloat(getInfo);
				input_state.gripStates[hand] = gripState;

				getInfo.action = input_action.primaryButtonAction;
				xr::ActionStateBoolean primaryButtonState = session.getActionStateBoolean(getInfo);
				input_state.primaryButtonStates[hand] = primaryButtonState;

				getInfo.action = input_action.secondaryButtonAction;
				xr::ActionStateBoolean secondaryButtonState = session.getActionStateBoolean(getInfo);
				input_state.secondaryButtonStates[hand] = secondaryButtonState;

				getInfo.action = input_action.thumbstickXAction;
				xr::ActionStateFloat thumbstickXState = session.getActionStateFloat(getInfo);
				input_state.thumbstickXStates[hand] = thumbstickXState;

				getInfo.action = input_action.thumbstickYAction;
				xr::ActionStateFloat thumbstickYState = session.getActionStateFloat(getInfo);
				input_state.thumbstickYStates[hand] = thumbstickYState;
			}
		}
		auto update(const glm::mat4& camera_offset = glm::mat4{ 1.f }) -> void {
			auto frameState = session.waitFrame({ });
			session.beginFrame({ });

			std::vector<xr::CompositionLayerBaseHeader*> layers = { };
			xr::CompositionLayerProjection layer{ };
			std::vector<xr::CompositionLayerProjectionView> projectionLayerViews;

			if (frameState.shouldRender == XR_TRUE)
			{
				for (int hand : { 0, 1 })
				{
					input_state.handLocations[hand] = input_action.handSpace[hand].locateSpace(app_space, frameState.predictedDisplayTime);
				}

				xr::ViewState viewState{ };
				xr::ViewLocateInfo locateInfo(xr::ViewConfigurationType::PrimaryStereo, frameState.predictedDisplayTime, app_space);
				auto views = session.locateViewsToVector(locateInfo, reinterpret_cast<XrViewState*>(&viewState));
				if ((viewState.viewStateFlags & xr::ViewStateFlagBits::PositionValid) && (viewState.viewStateFlags & xr::ViewStateFlagBits::OrientationValid))
				{
					projectionLayerViews.resize(views.size());
					std::vector<std::tuple<glm::mat4, glm::mat4, uint32_t>> xr_camera(views.size());
					for (uint32_t i = 0; i < views.size(); ++i)
					{
						auto& [mat_projection, mat_camera_transform, image_index] = xr_camera[i];

						image_index = swap_chains[i].acquireSwapchainImage({});
						swap_chains[i].waitSwapchainImage({xr::Duration::infinite()});

						projectionLayerViews[i] = xr::CompositionLayerProjectionView{ views[i].pose, views[i].fov, { swap_chains[i], swap_chain_rects[i], 0}};

						XrMatrix4x4f_CreateProjectionFov(&cnv<XrMatrix4x4f>(mat_projection), GRAPHICS_VULKAN, views[i].fov, DEFAULT_NEAR_Z, INFINITE_FAR_Z);
						glm::mat4 eye_pose;
						glm::vec3 identity{ 1.f, 1.f, 1.f };
						XrMatrix4x4f_CreateTranslationRotationScale(
							&cnv<XrMatrix4x4f>(eye_pose), 
							&(projectionLayerViews[i].pose.get()->position), 
							&(projectionLayerViews[i].pose.get()->orientation), 
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
					layer.viewCount = (uint32_t)projectionLayerViews.size();
					layer.views = projectionLayerViews.data();
					layers.push_back(&layer);
				}
			}

			xr::FrameEndInfo endInfo(frameState.predictedDisplayTime, xr::EnvironmentBlendMode::Opaque, static_cast<uint32_t>(layers.size()), layers.data());
			session.endFrame(endInfo);
		}

	public:
		OpenXRPlugin(R& renderer) : renderer{ renderer } {
		}

		auto create_instance() -> void {
			// Logging Extensions and Layers -----------------------------------------------------
			log_step("OpenXR", "Enumerating Supported OpenXR Extensions");
			auto allExtensions = xr::enumerateInstanceExtensionPropertiesToVector(nullptr);
			log_success();
			log_info("OpenXR", std::format("List of total {} OpenXR Extension(s):", allExtensions.size()), 0);
			for (const auto& ext : allExtensions)
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
			xr::ApplicationInfo appInfo(
				"Arxetipo",
				1,
				"Arxetipo Engine",
				1,
				xr::Version::current()
			);

			xr::InstanceCreateFlags flags = { };
			xr::InstanceCreateInfo instanceCreateInfo(
				{ },
				appInfo,
				0,
				nullptr,
				static_cast<uint32_t>(extensions.size()),
				extensions.data()
			);

			instance = xr::createInstance(instanceCreateInfo);
			log_success();

			// Logging Instance Properties -------------------------------------------------------
			auto instanceProperty = instance.getInstanceProperties();
			log_info(
				"OpenXR",
				std::format("OpenXR Runtime = {}, RuntimeVersion = {}.{}.{}",
					instanceProperty.runtimeName,
					instanceProperty.runtimeVersion.major(),
					instanceProperty.runtimeVersion.minor(),
					instanceProperty.runtimeVersion.patch()
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

			xr::SystemGetInfo systemGetInfo(
				xr::FormFactor::HeadMountedDisplay
			);

			system_id = instance.getSystem(systemGetInfo);

			log_success();
		}

		auto create_session(P::GraphicsBindingType graphicsBinding) -> void {
			log_step("OpenXR", "Creating OpenXR Session");
			xr::SessionCreateInfo createInfo({}, system_id, &graphicsBinding);
			session = instance.createSession(createInfo);
			log_success();
		}
		auto create_space() -> void {
			log_step("OpenXR", "Creating Reference Space");
			xr::ReferenceSpaceCreateInfo refSpaceCreateInfo(xr::ReferenceSpaceType::Stage, { });
			app_space = session.createReferenceSpace(refSpaceCreateInfo);
			log_success();
		}
		auto create_swap_chains(P& proxy) -> void {
			if (!system_id)
			{
				throw std::runtime_error("No system ID.");
			}

			auto systemPorperties = instance.getSystemProperties(system_id);
			log_info("OpenXR", "System Properties: ", 0);
			log_info("OpenXR", std::format("System Name : {}, Vendor ID : {}", systemPorperties.systemName, systemPorperties.vendorId), 1);
			log_info("OpenXR", std::format("Graphics Properties: MaxSwapChainImageWidth = {}, MaxSwapChainImageHeight = {}, MaxLayers = {}", systemPorperties.graphicsProperties.maxSwapchainImageWidth, systemPorperties.graphicsProperties.maxSwapchainImageHeight, systemPorperties.graphicsProperties.maxLayerCount), 1);
			log_info("OpenXR", std::format("Tracking Properties: OrientationTracking = {}, PositionTracking = {}", systemPorperties.trackingProperties.orientationTracking == XR_TRUE, systemPorperties.trackingProperties.positionTracking == XR_TRUE), 1);

			config_views = instance.enumerateViewConfigurationViewsToVector(system_id, xr::ViewConfigurationType::PrimaryStereo);
			log_info("OpenXR", std::format("Number of Views: {}", config_views.size()), 0);

			auto swapChainFormats = session.enumerateSwapchainFormatsToVector();
			swap_chain_format = vk::Format::eB8G8R8A8Srgb;
			int64_t i64format;
			bool find = false;
			log_info("OpenXR", std::format("Searching for Image Format, Desired Format : {}, All available Formats: ", vk::to_string(swap_chain_format)), 0);
			for (auto format : swapChainFormats)
			{
				vk::Format vkFormat = (vk::Format)format;
				if (vkFormat == swap_chain_format)
				{
					find = true;
					i64format = format;
					log_info("OpenXR", std::format("{} <= Found.", vk::to_string(vkFormat)), 1);
				}
				else
				{
					log_info("OpenXR", vk::to_string(vkFormat), 1);
				}
			}
			if (!find)
			{
				swap_chain_format = (vk::Format)swapChainFormats[0];
				i64format = swapChainFormats[0];
				log_info("OpenXR", std::format("Cannot find desired format, falling back to first available format : {}", vk::to_string(swap_chain_format)), 0);
			}

			swap_chains.clear();
			swap_chain_rects.clear();
			for (const auto& view : config_views)
			{
				log_step("OpenXR", "Creating XR Side SwapChain");
				xr::SwapchainCreateInfo swapChainInfo({}, xr::SwapchainUsageFlagBits::Sampled | xr::SwapchainUsageFlagBits::ColorAttachment | xr::SwapchainUsageFlagBits::TransferSrc, i64format, view.recommendedSwapchainSampleCount, view.recommendedImageRectWidth, view.recommendedImageRectHeight, 1, 1, 1);
				auto swapChain = session.createSwapchain(swapChainInfo);
				swap_chains.push_back(swapChain);
				swap_chain_rects.push_back({ { 0, 0 }, { static_cast<int32_t>(view.recommendedImageRectWidth), static_cast<int32_t>(view.recommendedImageRectHeight) } });
				log_success();
				//log_info("OpenXR", std::format("SwapChainImageType = {}", xr::to_string(swapChainImages[0][0].type)), 0);
				log_info("OpenXR", std::format("SwapChainImageExtent = ({}, {}, {})", view.recommendedSwapchainSampleCount, view.recommendedImageRectWidth, view.recommendedImageRectHeight), 0);
			}
			proxy.passin_xr_swapchains(swap_chains, swap_chain_rects, i64format);
			//graphics.create_swap_chain(proxy);
		}
		auto create_actions() -> void {
			xr::ActionSetCreateInfo setInfo("gameplay", "Gameplay", 0);
			input_action.actionSet = instance.createActionSet(setInfo);

			input_action.handSubactionPath[0] = instance.stringToPath("/user/hand/left");
			input_action.handSubactionPath[1] = instance.stringToPath("/user/hand/right");

			xr::ActionCreateInfo poseActionInfo("hand_pose", xr::ActionType::PoseInput, static_cast<uint32_t>(input_action.handSubactionPath.size()), input_action.handSubactionPath.data(), "Hand Pose");
			input_action.poseAction = input_action.actionSet.createAction(poseActionInfo);

			std::array<xr::Path, 2> posePath = {
				instance.stringToPath("/user/hand/left/input/grip/pose"),
				instance.stringToPath("/user/hand/right/input/grip/pose"),
			};

			xr::ActionCreateInfo triggerActionInfo("trigger", xr::ActionType::FloatInput, static_cast<uint32_t>(input_action.handSubactionPath.size()), input_action.handSubactionPath.data(), "Trigger");
			input_action.triggerAction = input_action.actionSet.createAction(triggerActionInfo);

			std::array<xr::Path, 2> triggerPath = {
				instance.stringToPath("/user/hand/left/input/trigger/value"),
				instance.stringToPath("/user/hand/right/input/trigger/value"),
			};

			xr::ActionCreateInfo gripActionInfo("grip", xr::ActionType::FloatInput, static_cast<uint32_t>(input_action.handSubactionPath.size()), input_action.handSubactionPath.data(), "Grip");
			input_action.gripAction = input_action.actionSet.createAction(gripActionInfo);

			std::array<xr::Path, 2> gripPath = {
				instance.stringToPath("/user/hand/left/input/squeeze/value"),
				instance.stringToPath("/user/hand/right/input/squeeze/value"),
			};

			xr::ActionCreateInfo primaryButtonActionInfo("primary_button", xr::ActionType::BooleanInput, static_cast<uint32_t>(input_action.handSubactionPath.size()), input_action.handSubactionPath.data(), "Primary Button");
			input_action.primaryButtonAction = input_action.actionSet.createAction(primaryButtonActionInfo);

			std::array<xr::Path, 2> primaryButtonPath = {
				instance.stringToPath("/user/hand/left/input/x/click"),
				instance.stringToPath("/user/hand/right/input/a/click"),
			};

			xr::ActionCreateInfo secondaryButtonActionInfo("secondary_button", xr::ActionType::BooleanInput, static_cast<uint32_t>(input_action.handSubactionPath.size()), input_action.handSubactionPath.data(), "Secondary Button");
			input_action.secondaryButtonAction = input_action.actionSet.createAction(secondaryButtonActionInfo);

			std::array<xr::Path, 2> secondaryButtonPath = {
				instance.stringToPath("/user/hand/left/input/y/click"),
				instance.stringToPath("/user/hand/right/input/b/click"),
			};

			xr::ActionCreateInfo thumbstickXActionInfo("thumbstick_x", xr::ActionType::FloatInput, static_cast<uint32_t>(input_action.handSubactionPath.size()), input_action.handSubactionPath.data(), "ThumbStick X");
			input_action.thumbstickXAction = input_action.actionSet.createAction(thumbstickXActionInfo);

			std::array<xr::Path, 2> thumbstickXPath = {
				instance.stringToPath("/user/hand/left/input/thumbstick/x"),
				instance.stringToPath("/user/hand/right/input/thumbstick/x"),
			};

			xr::ActionCreateInfo thumbstickYActionInfo("thumbstick_y", xr::ActionType::FloatInput, static_cast<uint32_t>(input_action.handSubactionPath.size()), input_action.handSubactionPath.data(), "ThumbStick Y");
			input_action.thumbstickYAction = input_action.actionSet.createAction(thumbstickYActionInfo);

			std::array<xr::Path, 2> thumbstickYPath = {
				instance.stringToPath("/user/hand/left/input/thumbstick/y"),
				instance.stringToPath("/user/hand/right/input/thumbstick/y"),
			};

			xr::ActionCreateInfo vibrateActionInfo("vibrate", xr::ActionType::VibrationOutput, static_cast<uint32_t>(input_action.handSubactionPath.size()), input_action.handSubactionPath.data(), "Virbate");
			input_action.vibrateAction = input_action.actionSet.createAction(vibrateActionInfo);

			std::array<xr::Path, 2> vibratePath = {
				instance.stringToPath("/user/hand/left/output/haptic"),
				instance.stringToPath("/user/hand/right/output/haptic"),
			};

			xr::Path oculusTouchInteractionProfilePath = instance.stringToPath("/interaction_profiles/oculus/touch_controller");

			std::vector<xr::ActionSuggestedBinding> binding = {
				xr::ActionSuggestedBinding{ input_action.poseAction, posePath[0] },
				xr::ActionSuggestedBinding{ input_action.poseAction, posePath[1] },
				xr::ActionSuggestedBinding{ input_action.triggerAction, triggerPath[0] },
				xr::ActionSuggestedBinding{ input_action.triggerAction, triggerPath[1] },
				xr::ActionSuggestedBinding{ input_action.gripAction, gripPath[0] },
				xr::ActionSuggestedBinding{ input_action.gripAction, gripPath[1] },
				xr::ActionSuggestedBinding{ input_action.primaryButtonAction, primaryButtonPath[0] },
				xr::ActionSuggestedBinding{ input_action.primaryButtonAction, primaryButtonPath[1] },
				xr::ActionSuggestedBinding{ input_action.secondaryButtonAction, secondaryButtonPath[0] },
				xr::ActionSuggestedBinding{ input_action.secondaryButtonAction, secondaryButtonPath[1] },
				xr::ActionSuggestedBinding{ input_action.thumbstickXAction, thumbstickXPath[0] },
				xr::ActionSuggestedBinding{ input_action.thumbstickXAction, thumbstickXPath[1] },
				xr::ActionSuggestedBinding{ input_action.thumbstickYAction, thumbstickYPath[0] },
				xr::ActionSuggestedBinding{ input_action.thumbstickYAction, thumbstickYPath[1] },
				xr::ActionSuggestedBinding{ input_action.vibrateAction, vibratePath[0] },
				xr::ActionSuggestedBinding{ input_action.vibrateAction, vibratePath[1] },
			};
			xr::InteractionProfileSuggestedBinding suggestedBinding(oculusTouchInteractionProfilePath, static_cast<uint32_t>(binding.size()), binding.data());
			instance.suggestInteractionProfileBindings(suggestedBinding);

			xr::ActionSpaceCreateInfo actionSpaceInfo(input_action.poseAction, { }, { });
			actionSpaceInfo.subactionPath = input_action.handSubactionPath[0];
			input_action.handSpace[0] = session.createActionSpace(actionSpaceInfo);
			actionSpaceInfo.subactionPath = input_action.handSubactionPath[1];
			input_action.handSpace[1] = session.createActionSpace(actionSpaceInfo);

			xr::SessionActionSetsAttachInfo attachInfo(1, &input_action.actionSet);
			session.attachSessionActionSets(attachInfo);
		}

		auto handle_session_state_changed_event(xr::EventDataSessionStateChanged eventDataSessionStateChanged) -> bool {
			auto oldState = session_state;
			session_state = eventDataSessionStateChanged.state;
			log_info("OpenXR", std::format("Session State Changed: {} -> {}", xr::to_string(oldState), xr::to_string(session_state)), 0);

			switch (session_state)
			{
			case xr::SessionState::Ready:
			{
				xr::SessionBeginInfo sessionBeginInfo(xr::ViewConfigurationType::PrimaryStereo);
				session.beginSession(sessionBeginInfo);
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
			xr::HapticActionInfo hapticInfo(input_action.vibrateAction, input_action.handSubactionPath[hand]);
			if (session.applyHapticFeedback(hapticInfo, virbation.get_base()) != xr::Result::Success)
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