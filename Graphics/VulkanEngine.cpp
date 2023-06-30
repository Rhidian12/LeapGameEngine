#include "VulkanEngine.h"

// VMA
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// GLFW Window
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

// GLM
#include <gtx/transform.hpp>

// Bootstrap library
#include <VkBootstrap.h>

// STL includes
#include <iostream>
#include <iomanip>
#include <fstream>

#pragma region macros
// Vulkan error macro
using namespace std;
#define VK_CHECK(x)													\
	do																\
	{																\
		VkResult err = x;											\
		if (err)													\
		{															\
			std::cout << "Detected Vulkan error: " << err << "\n";	\
			abort();												\
		}															\
	} while (0)
#pragma endregion

// Vulkan includes
#include "VulkanInitializers.h"
#include "VulkanPipelineBuilder.h"


using namespace leap::graphics;

VulkanEngine::VulkanEngine(GLFWwindow* pWindow)
	: m_pWindow{ pWindow }
{}

VulkanEngine::~VulkanEngine()
{
	Cleanup();
}

void VulkanEngine::Initialize()
{
	// Load the core Vulkan structures
	InitializeVulkan();
	InitializeSwapChain();
	InitCommands();
	InitializeDefaultRenderPass();
	InitializeFramebuffers();
	InitializeSyncStructures();
	InitDescriptors();
	InitializePipelines();
	LoadMeshes();
	InitializeScene();

	m_IsInitialized = true;
	std::cout << "VulkanEngine initialized\n";
}

void VulkanEngine::Draw()
{
	// Wait for the GPU to be done with the last frame
	VK_CHECK(vkWaitForFences(m_Device, 1, &GetCurrentFrame().renderFence, true, UINT64_MAX));
	VK_CHECK(vkResetFences(m_Device, 1, &GetCurrentFrame().renderFence));

	// Request image from the swap chain
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, GetCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex));

	// Commands have finished executing, so reset the buffer
	VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().mainCommandBuffer, 0));

	// Record commands
	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;

	cmdBeginInfo.pInheritanceInfo = nullptr;
	// We record each frame to the command buffer, so we need to let Vulkan know that we're only executing it once
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Begin recording
	VK_CHECK(vkBeginCommandBuffer(GetCurrentFrame().mainCommandBuffer, &cmdBeginInfo));

	// Clear color
	VkClearValue clearValue;
	clearValue.color = { {0.1f, 0.1f, 0.85f, 1.0f} };

	// Depth clear value
	VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.f;

	VkClearValue clearValues[] = { clearValue, depthClear };

	// Begin the render pass
	VkRenderPassBeginInfo rpInfo{};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.pNext = nullptr;

	// Use the default render pass on the image with the index that we acquired
	rpInfo.renderPass = m_RenderPass;
	rpInfo.renderArea.offset.x = 0;
	rpInfo.renderArea.offset.y = 0;
	rpInfo.renderArea.extent = m_WindowExtent;
	rpInfo.framebuffer = m_Framebuffers[swapchainImageIndex];

	// Connect clear values
	rpInfo.clearValueCount = 2;
	rpInfo.pClearValues = &clearValues[0];

	// Send the begin command to Vulkan
	vkCmdBeginRenderPass(GetCurrentFrame().mainCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	// RENDER STUFF HERE
	// ...

	DrawObjects(GetCurrentFrame().mainCommandBuffer, m_Renderables.data(), m_Renderables.size());

	// End the render pass
	vkCmdEndRenderPass(GetCurrentFrame().mainCommandBuffer);

	// End recording
	VK_CHECK(vkEndCommandBuffer(GetCurrentFrame().mainCommandBuffer));

	// Prepare to submit to the queue
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	submit.pWaitDstStageMask = &waitStage;

	// Wait for image to be ready for rendering before rendering to it
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &GetCurrentFrame().presentSemaphore;

	// Signal ready when finished rendering
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &GetCurrentFrame().renderSemaphore;

	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &GetCurrentFrame().mainCommandBuffer;

	// Submit to the graphics queue
	VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submit, GetCurrentFrame().renderFence));

	// Present the image to the window when the rendering semaphore has signaled that rendering is done
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;

	presentInfo.pSwapchains = &m_SwapChain;
	presentInfo.swapchainCount = 1;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;

	// Which image to present
	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

	m_FrameNumber++;
}

void VulkanEngine::Cleanup()
{
	if (m_IsInitialized)
	{
		// Wait for the GPU to be done with the last frame
		vkDeviceWaitIdle(m_Device);

		m_MainDeletionQueue.Flush();

		vkDestroyDevice(m_Device, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
		vkDestroyInstance(m_Instance, nullptr);
	}
}

void VulkanEngine::InitializeVulkan()
{
	std::cout << "Initializing Vulkan\n";
	vkb::InstanceBuilder builder;

	// Make the Vulkan instance, with basic debug features
	auto instanceDesc = builder.set_app_name("Leap Vulkan Engine")
		.request_validation_layers(false)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkbInstance = instanceDesc.value();

	// Store the instance
	m_Instance = vkbInstance.instance;

	// Store the debug messenger
	m_DebugMessenger = vkbInstance.debug_messenger;

	if (glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_Surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

	// Get the window size to use for the swap chain
	int width, height;
	glfwGetWindowSize(m_pWindow, &width, &height);
	m_WindowExtent.width = static_cast<uint32_t>(width);
	m_WindowExtent.height = static_cast<uint32_t>(height);

	// use vkbootstrap to select a GPU.
	// We want a GPU that can write to the SDL surface and supports Vulkan 1.1
	vkb::PhysicalDeviceSelector selector{ vkbInstance };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 1)
		.set_surface(m_Surface)
		.select()
		.value();

	// create the final Vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a Vulkan application
	m_Device = vkbDevice.device;
	m_PhysicalDevice = physicalDevice.physical_device;

	// Get the graphics queue for the rest of the Vulkan application
	m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = m_PhysicalDevice;
	allocatorInfo.device = m_Device;
	allocatorInfo.instance = m_Instance;
	vmaCreateAllocator(&allocatorInfo, &m_Allocator);

	m_MainDeletionQueue.deletors.emplace_back([=]() {
		vmaDestroyAllocator(m_Allocator);
		});

	m_DeviceProperties = vkbDevice.physical_device.properties;
	std::cout << "The GPU has a minimum buffer alignment of " << m_DeviceProperties.limits.minUniformBufferOffsetAlignment << "\n";
}

void VulkanEngine::InitializeSwapChain()
{
	std::cout << "Initializing Swap Chain\n";

	vkb::SwapchainBuilder swapChainBuilder{ m_PhysicalDevice, m_Device, m_Surface };

	vkb::Swapchain vkbSwapChain = swapChainBuilder
		.use_default_format_selection()
		// use VSync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
		.set_desired_extent(m_WindowExtent.width, m_WindowExtent.height)
		.build()
		.value();

	// store swap chain and its related images
	m_SwapChain = vkbSwapChain.swapchain;
	m_SwapChainImages = vkbSwapChain.get_images().value();
	m_SwapChainImageViews = vkbSwapChain.get_image_views().value();

	m_SwapChainImageFormat = vkbSwapChain.image_format;

	m_MainDeletionQueue.deletors.emplace_back([=]() {
			vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr); 
		});

	// Depth buffer image matches window extents
	VkExtent3D depthImageExtent 
	{
		m_WindowExtent.width,
		m_WindowExtent.height,
		1
	};

	// Depth format (must be changed to support stencil)
	m_DepthFormat = VK_FORMAT_D32_SFLOAT;

	VkImageCreateInfo depthImageCreateInfo = vkinit::ImageCreateInfo(m_DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
	
	// Allocate image on GPU
	VmaAllocationCreateInfo depthImageAllocInfo{};
	depthImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	depthImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Allocate mem and create image
	vmaCreateImage(m_Allocator, &depthImageCreateInfo, &depthImageAllocInfo, &m_DepthImage.image, &m_DepthImage.allocation, nullptr);

	// Build depth buffer image view
	VkImageViewCreateInfo depthImageViewCreateInfo = vkinit::ImageViewCreateInfo(m_DepthFormat, m_DepthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(m_Device, &depthImageViewCreateInfo, nullptr, &m_DepthImageView));

	m_MainDeletionQueue.deletors.emplace_back([=]() {
		vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
		vmaDestroyImage(m_Allocator, m_DepthImage.image, m_DepthImage.allocation);
		});
}

void VulkanEngine::InitCommands()
{
	std::cout << "Initializing Commands\n";

	// Create command pool for commands submitted to the graphics queue
	// We also want the pool to allow for resetting of individual command buffers
	auto commandPoolInfo = vkinit::CommandPoolCreateInfo(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].commandPool));

		// Allocate the default command buffer that we will use for rendering
		auto commandBufferInfo = vkinit::CommandBufferAllocateInfo(m_Frames[i].commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(m_Device, &commandBufferInfo, &m_Frames[i].mainCommandBuffer));

		m_MainDeletionQueue.deletors.emplace_back([=]() {
			vkDestroyCommandPool(m_Device, m_Frames[i].commandPool, nullptr);
			});
	}
}

AllocatedBuffer VulkanEngine::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	VkBufferCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.pNext = nullptr;
	
	info.size = allocSize;
	info.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo{};
	vmaallocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer{};

	VK_CHECK(vmaCreateBuffer(m_Allocator, &info, &vmaallocInfo,
		&newBuffer.buffer,
		&newBuffer.allocation,
		nullptr));

	return newBuffer;
}

void VulkanEngine::InitDescriptors()
{
	// DescriptorSetLayout defines how the descriptor data is layed out in the descriptor set

	// Bindig 0: Uniform buffer (Camera data)
	VkDescriptorSetLayoutBinding camBufferBinding{ vkinit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT, 0) };

	// Binding 1: Uniform buffer (Scene data)
	VkDescriptorSetLayoutBinding sceneBufferBinding{ vkinit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1) };

	VkDescriptorSetLayoutBinding bindings[] = { camBufferBinding, sceneBufferBinding };

	VkDescriptorSetLayoutCreateInfo setInfo{};
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.pNext = nullptr;

	// We currently only have one binding, the camera buffer
	setInfo.bindingCount = 2;
	setInfo.pBindings = bindings;
	setInfo.flags = 0;

	vkCreateDescriptorSetLayout(m_Device, &setInfo, nullptr, &m_GlobalSetLayout);

	// Create descriptor pool that will hold 10 uniform buffers and 10 dynamic uniform buffers
	std::vector<VkDescriptorPoolSize> sizes
	{
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10}
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;

	poolInfo.flags = 0;
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
	poolInfo.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool);

	const size_t sceneDataBufferSize = MAX_FRAMES_IN_FLIGHT * PadUniformBufferSize(sizeof(GPUSceneData));
	m_SceneDataBuffer = CreateBuffer(sceneDataBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_Frames[i].cameraBuffer = CreateBuffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// Allocate one descriptor set for each frame
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;

		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_GlobalSetLayout;

		vkAllocateDescriptorSets(m_Device, &allocInfo, &m_Frames[i].globalDescriptor);

		// Info about the camera buffer we want to point at in the descriptor
		VkDescriptorBufferInfo cameraInfo{};
		cameraInfo.buffer = m_Frames[i].cameraBuffer.buffer;
		// Offset into the buffer
		cameraInfo.offset = 0;
		// Size of data to pass to the shader
		cameraInfo.range = sizeof(GPUCameraData);

		VkDescriptorBufferInfo sceneInfo{};
		sceneInfo.buffer = m_SceneDataBuffer.buffer;
		sceneInfo.offset = 0;
		sceneInfo.range = sizeof(GPUSceneData);

		VkWriteDescriptorSet cameraWrite{vkinit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_Frames[i].globalDescriptor, &cameraInfo, 0)};

		VkWriteDescriptorSet sceneWrite{ vkinit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			m_Frames[i].globalDescriptor, &sceneInfo, 1) };

		VkWriteDescriptorSet setWrites[] = { cameraWrite, sceneWrite };

		vkUpdateDescriptorSets(m_Device, 2, setWrites, 0, nullptr);
	}

	m_MainDeletionQueue.deletors.emplace_back([&]() {
		vkDestroyDescriptorSetLayout(m_Device, m_GlobalSetLayout, nullptr);
		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

		// Add buffers to deletion queue
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vmaDestroyBuffer(m_Allocator, m_Frames[i].cameraBuffer.buffer, m_Frames[i].cameraBuffer.allocation);
		}

		vmaDestroyBuffer(m_Allocator, m_SceneDataBuffer.buffer, m_SceneDataBuffer.allocation);
		});
}

void VulkanEngine::InitializeDefaultRenderPass()
{
	std::cout << "Initializing Default Render Pass\n";

	// Description of the image that we will be writing rendering commands to
	VkAttachmentDescription colorAttachment{};

	// format should be the same as the swap chain images
	colorAttachment.format = m_SwapChainImageFormat;
	// MSAA samples, set to 1 (no MSAA) by default
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// Clear when render pass begins
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// Keep the attachment stored when render pass ends
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// Don't care about stencil data
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Image data layout before render pass starts (undefined = don't care)
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// Image data layout after render pass (to change to), set to present by default
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	// Attachment number will index into the pAttachments array in the parent render pass itself
	colorAttachmentRef.attachment = 0;
	// Optimal layout for writing to the image
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Description of the depth image
	VkAttachmentDescription depthAttachment{};
	depthAttachment.flags = 0;
	depthAttachment.format = m_DepthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Create one sub pass (minimum one sub pass required)
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	// Connect depth attachment to subpass
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

	// Connect the color attachment description to the info
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = &attachments[0];

	// Connect the subpass(es) to the info
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	// These dependencies tell Vulkan that the attachment cannot be used before the previous renderpasses have finished using it
	VkSubpassDependency colorDependency{};
	colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	colorDependency.dstSubpass = 0;
	
	colorDependency.srcAccessMask = 0;
	colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubpassDependency depthDependency{};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;

	depthDependency.srcAccessMask = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

	VkSubpassDependency dependencies[2] = { colorDependency, depthDependency };

	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = &dependencies[0];

	VK_CHECK(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));

	m_MainDeletionQueue.deletors.emplace_back([=]() {
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
		});
}

void VulkanEngine::InitializeFramebuffers()
{
	std::cout << "Initializing Framebuffers\n";

	// Create the framebuffers for the swap chain images.
	// This will connect the render pass to the images for rendering
	VkFramebufferCreateInfo fbInfo{};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

	// Connect the render pass to the framebuffer
	fbInfo.renderPass = m_RenderPass;
	fbInfo.attachmentCount = 1;
	fbInfo.width = m_WindowExtent.width;
	fbInfo.height = m_WindowExtent.height;
	fbInfo.layers = 1;

	// Get the number of swap chain image views
	const uint32_t swapChainImageViewCount = static_cast<uint32_t>(m_SwapChainImageViews.size());
	m_Framebuffers = std::vector<VkFramebuffer>(swapChainImageViewCount);

	// Create the framebuffers for each image view
	for (uint32_t i = 0; i < swapChainImageViewCount; i++)
	{
		VkImageView attachments[2] = { m_SwapChainImageViews[i], m_DepthImageView };

		fbInfo.attachmentCount = 2;
		fbInfo.pAttachments = attachments;

		VK_CHECK(vkCreateFramebuffer(m_Device, &fbInfo, nullptr, &m_Framebuffers[i]));

		m_MainDeletionQueue.deletors.emplace_back([=]() {
			vkDestroyFramebuffer(m_Device, m_Framebuffers[i], nullptr);
			vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);
			});
	}
}

FrameData& VulkanEngine::GetCurrentFrame()
{
	return m_Frames[m_FrameNumber % MAX_FRAMES_IN_FLIGHT];
}

void VulkanEngine::InitializeSyncStructures()
{
	std::cout << "Initializing Sync Structures\n";

	VkFenceCreateInfo fenceCreateInfo{ vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT) };
	
	// Sempahore needs no flags
	VkSemaphoreCreateInfo semaphoreCreateInfo{ vkinit::SemaphoreCreateInfo() };
	
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].renderFence));

		m_MainDeletionQueue.deletors.emplace_back([=]() {
			vkDestroyFence(m_Device, m_Frames[i].renderFence, nullptr);
			});

		VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].presentSemaphore));
		VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].renderSemaphore));

		m_MainDeletionQueue.deletors.emplace_back([=]()
			{
				vkDestroySemaphore(m_Device, m_Frames[i].presentSemaphore, nullptr);
				vkDestroySemaphore(m_Device, m_Frames[i].renderSemaphore, nullptr);
			});
	}
}

bool VulkanEngine::LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule)
{
	// Open the file stream, seek to the end of the file
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		return false;

	// Get the file size
	size_t fileSize = static_cast<size_t>(file.tellg());

	// SPRIV expects the buffer to be on uint32_t, so make sure to reserve a buffer big enough for that
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	// Put cursor at the beginning of the file
	file.seekg(0);

	// Load the entire file into the buffer
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	// Close the file stream
	file.close();

	// Create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	// Code size has to be in bytes, so multiply the int in the buffer by the size of an int
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	// Validate creation
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		return false;

	// Set the outShaderModule to the newly created shader module
	*outShaderModule = shaderModule;

	return true;
}

void VulkanEngine::InitializePipelines()
{
	std::cout << "Initializing Pipelines\n";

	VkShaderModule fragShader;
	if (!LoadShaderModule("../Graphics/Shaders/PosNormCol.frag.spv", &fragShader))
		std::cout << "Error building the triangle fragment shader module\n";
	else
		std::cout << "Successfully built the fragment shader module\n";

	VkShaderModule vertShader;
	if (!LoadShaderModule("../Graphics/Shaders/PosNormCol.vert.spv", &vertShader))
		std::cout << "Error building the triangle vertex shader module\n";
	else
		std::cout << "Successfully built the vertex shader module\n";

	// Build pipeline layout that controls inputs/outputs of the shader
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{ vkinit::PipelineLayoutCreateInfo() };

	// Setup push constants
	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(MeshPushConstants);
	// Push constant is only available in the vertex shader
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_GlobalSetLayout;

	VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_MeshPipelineLayout));

	// Build the stage-create-info for both vertex and fragment stages.
	// This lets the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;

	pipelineBuilder.m_ShaderStages.push_back(
		vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader)
	);

	pipelineBuilder.m_ShaderStages.push_back(
		vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader)
	);

	// Vertex input controls how to read vertices from vertex buffers
	pipelineBuilder.m_VertexInputInfo = vkinit::VertexInputStateCreateInfo();

	VertexInputDescription vertexDesc = Vertex::GetVertexDescription();

	pipelineBuilder.m_VertexInputInfo.pVertexAttributeDescriptions = vertexDesc.attributes.data();
	pipelineBuilder.m_VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDesc.attributes.size());

	pipelineBuilder.m_VertexInputInfo.pVertexBindingDescriptions = vertexDesc.bindings.data();
	pipelineBuilder.m_VertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDesc.bindings.size());

	// Set input assembly state, which controls primitive topology
	pipelineBuilder.m_InputAssembly = vkinit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// Build the viewport and scissor from the swapchain extents
	pipelineBuilder.m_Viewport.x = 0.0f;
	pipelineBuilder.m_Viewport.y = 0.0f;
	pipelineBuilder.m_Viewport.width = static_cast<float>(m_WindowExtent.width);
	pipelineBuilder.m_Viewport.height = static_cast<float>(m_WindowExtent.height);
	pipelineBuilder.m_Viewport.minDepth = 0.0f;
	pipelineBuilder.m_Viewport.maxDepth = 1.0f;

	pipelineBuilder.m_Scissor.offset = { 0, 0 };
	pipelineBuilder.m_Scissor.extent = m_WindowExtent;

	// Build rasterizer
	pipelineBuilder.m_Rasterizer = vkinit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

	// Build multisampling
	pipelineBuilder.m_Multisampling = vkinit::MultisamplingStateCreateInfo();

	// Build color blend attachment with no blending and writing to RGBA
	pipelineBuilder.m_ColorBlendAttachment = vkinit::ColorBlendAttachmentState();

	// Use the triangle layout
	pipelineBuilder.m_PipelineLayout = m_MeshPipelineLayout;

	// Depth testing
	pipelineBuilder.m_DepthStencil = vkinit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	// Build the pipeline
	m_MeshPipeline = pipelineBuilder.BuildPipeline(m_Device, m_RenderPass);

	CreateMaterial("defaultmesh", m_MeshPipeline, m_MeshPipelineLayout);

	vkDestroyShaderModule(m_Device, vertShader, nullptr);
	vkDestroyShaderModule(m_Device, fragShader, nullptr);

	m_MainDeletionQueue.deletors.emplace_back([=]() {
		vkDestroyPipeline(m_Device, m_MeshPipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_MeshPipelineLayout, nullptr);
		});
}

void VulkanEngine::LoadMeshes()
{
	m_TeapotMesh.LoadFromObj("Meshes/Teapot.obj");

	UploadMesh(m_TeapotMesh);
	m_Meshes["Teapot"] = m_TeapotMesh;
}

void VulkanEngine::UploadMesh(Mesh& mesh)
{
	// Allocate vertex buffer
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	// Total size, in bytes, of the buffer
	// Should hold all vertices
	bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);

	// This buffer is going to be used as a Vertex Buffer
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	// Let the buffer be shared between the CPU (write) and GPU (read) so that we can upload it
	VmaAllocationCreateInfo vmaallocInfo{};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaallocInfo,
		&mesh.vertexBuffer.buffer,
		&mesh.vertexBuffer.allocation,
		nullptr));

	// Add the destruction of buffer to the deletion queue
	m_MainDeletionQueue.deletors.emplace_back([=]() {
		vmaDestroyBuffer(m_Allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
		});

	// Copy vertex data into buffer
	void* data;
	vmaMapMemory(m_Allocator, mesh.vertexBuffer.allocation, &data);
	memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(m_Allocator, mesh.vertexBuffer.allocation);
}

size_t VulkanEngine::PadUniformBufferSize(size_t originalSize)
{
	size_t minUboAlignment = m_DeviceProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0)
	{
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}

Material* VulkanEngine::CreateMaterial(const std::string& name, VkPipeline pipeline, VkPipelineLayout layout)
{
	Material mat;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	m_Materials[name] = mat;
	return &m_Materials[name];
}

Material* VulkanEngine::GetMaterial(const std::string& name)
{
	auto it = m_Materials.find(name);
	if (it == end(m_Materials))
		return nullptr;
	else
		return &(*it).second;
}

Mesh* VulkanEngine::GetMesh(const std::string& name)
{
	auto it = m_Meshes.find(name);
	if (it == end(m_Meshes))
		return nullptr;
	else
		return &(*it).second;
}

void VulkanEngine::DrawObjects(VkCommandBuffer cmdBuffer, RenderObject* first, int count)
{
	glm::vec3 camPos{0.f, -60.f, -290.f};

	// WVP matrix
	glm::mat4 trans{glm::translate(glm::mat4{1.f}, camPos)};
	glm::mat4 rot{glm::rotate(glm::radians(46.f), glm::vec3{1, 0, 0})};
	glm::mat4 view = rot * trans;
	glm::mat4 proj{glm::perspective(glm::radians(70.f), (float)m_WindowExtent.width / (float)m_WindowExtent.height, 0.1f, 1000.f)};
	proj[1][1] *= -1;

	GPUCameraData camData;
	camData.proj = proj;
	camData.view = view;
	camData.viewProj = proj * view;
	camData.viewInv = glm::inverse(view);

	// Copy camera data into buffer
	void* data;
	vmaMapMemory(m_Allocator, GetCurrentFrame().cameraBuffer.allocation, &data);
	memcpy(data, &camData, sizeof(GPUCameraData));
	vmaUnmapMemory(m_Allocator, GetCurrentFrame().cameraBuffer.allocation);

	// Scene data
	char* sceneData;
	vmaMapMemory(m_Allocator, m_SceneDataBuffer.allocation, (void**)&sceneData);
	
	int frameIndex = m_FrameNumber % MAX_FRAMES_IN_FLIGHT;
	sceneData += PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
	
	memcpy(sceneData, &m_GPUSceneData, sizeof(GPUSceneData));
	vmaUnmapMemory(m_Allocator, m_SceneDataBuffer.allocation);

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;
	for (int i = 0; i < count; i++)
	{
		RenderObject& object = first[i];

		// Only bind pipeline if it doesn't match with already bound one
		if (object.material != lastMaterial)
		{
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
			lastMaterial = object.material;

			// Offset in scene buffer to access current frame data
			uint32_t uniformOffset = PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &GetCurrentFrame().globalDescriptor, 1, &uniformOffset);
		}

		// Final WVP
		glm::mat4 world = object.transform * glm::rotate(glm::mat4{1.f}, glm::radians(m_FrameNumber * 0.4f), glm::vec3(0, 1, 0));
		glm::mat4 wvp = proj * view * world;

		MeshPushConstants constants;
		constants.world = world;

		// Upload mesh to the GPU via push constants
		vkCmdPushConstants(cmdBuffer, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

		// Only bind mesh if it's different from last bind
		if (object.mesh != lastMesh)
		{
			// Bind the mesh vertex buffer with offset 0
			VkDeviceSize offset{ 0 };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);
			lastMesh = object.mesh;
		}

		vkCmdDraw(cmdBuffer, object.mesh->vertices.size(), 1, 0, 0);
	}
}

void VulkanEngine::InitializeScene()
{
	constexpr int numObjects = 100;

	Material* defaultMaterial = GetMaterial("defaultmesh");
	Mesh* teapotMesh = GetMesh("Teapot");
	const int vertCount = teapotMesh->vertices.size();

	for (int x = -numObjects * 0.5f; x < numObjects * 0.5f; x++)
	{
		for (int y = -numObjects * 0.5f; y < numObjects * 0.5f; y++)
		{
			RenderObject teapot;
			teapot.mesh = teapotMesh;
			teapot.material = defaultMaterial;
			teapot.transform = glm::translate(glm::mat4{1.0f}, glm::vec3(x * 6.f, 0, y * 6.f));

			m_Renderables.push_back(teapot);
		}
	}

	// Scene data
	m_GPUSceneData.ambientColor = { 0.625f, 0.0f, 0.24f, 0.1f };
	m_GPUSceneData.lightDir = { -0.577f, -0.577f, 0.577f, 1.f };

	std::cout << "Number of renderables: " << m_Renderables.size() << "\n";
	std::cout << std::setprecision(10) << "Number of triangles: " << (m_Renderables.size() * vertCount) / 3 << "\n";
}