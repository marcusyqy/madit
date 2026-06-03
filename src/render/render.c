#include "render.h"
#include "mb/mb_arena.h"
#include <stdio.h>

#include <vulkan/vulkan.h>
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <SDL3/SDL_vulkan.h>
#include <vma/vk_mem_alloc.h>


#define VK_RESULT_LIST                                     \
  X(VK_SUCCESS)                                            \
  X(VK_NOT_READY)                                          \
  X(VK_TIMEOUT)                                            \
  X(VK_EVENT_SET)                                          \
  X(VK_EVENT_RESET)                                        \
  X(VK_INCOMPLETE)                                         \
  X(VK_ERROR_OUT_OF_HOST_MEMORY)                           \
  X(VK_ERROR_OUT_OF_DEVICE_MEMORY)                         \
  X(VK_ERROR_INITIALIZATION_FAILED)                        \
  X(VK_ERROR_DEVICE_LOST)                                  \
  X(VK_ERROR_MEMORY_MAP_FAILED)                            \
  X(VK_ERROR_LAYER_NOT_PRESENT)                            \
  X(VK_ERROR_EXTENSION_NOT_PRESENT)                        \
  X(VK_ERROR_FEATURE_NOT_PRESENT)                          \
  X(VK_ERROR_INCOMPATIBLE_DRIVER)                          \
  X(VK_ERROR_TOO_MANY_OBJECTS)                             \
  X(VK_ERROR_FORMAT_NOT_SUPPORTED)                         \
  X(VK_ERROR_FRAGMENTED_POOL)                              \
  X(VK_ERROR_UNKNOWN)                                      \
  X(VK_ERROR_VALIDATION_FAILED)                            \
  X(VK_ERROR_OUT_OF_POOL_MEMORY)                           \
  X(VK_ERROR_INVALID_EXTERNAL_HANDLE)                      \
  X(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)               \
  X(VK_ERROR_FRAGMENTATION)                                \
  X(VK_PIPELINE_COMPILE_REQUIRED)                          \
  X(VK_ERROR_NOT_PERMITTED)                                \
  X(VK_ERROR_SURFACE_LOST_KHR)                             \
  X(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)                     \
  X(VK_SUBOPTIMAL_KHR)                                     \
  X(VK_ERROR_OUT_OF_DATE_KHR)                              \
  X(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)                     \
  X(VK_ERROR_INVALID_SHADER_NV)                            \
  X(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)                \
  X(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)       \
  X(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)    \
  X(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)       \
  X(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)        \
  X(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)          \
  X(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT) \
  X(VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT)                \
  X(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)          \
  X(VK_THREAD_IDLE_KHR)                                    \
  X(VK_THREAD_DONE_KHR)                                    \
  X(VK_OPERATION_DEFERRED_KHR)                             \
  X(VK_OPERATION_NOT_DEFERRED_KHR)                         \
  X(VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)             \
  X(VK_ERROR_COMPRESSION_EXHAUSTED_EXT)                    \
  X(VK_INCOMPATIBLE_SHADER_BINARY_EXT)                     \
  X(VK_PIPELINE_BINARY_MISSING_KHR)                        \
  X(VK_ERROR_NOT_ENOUGH_SPACE_KHR)                         \
  X(VK_RESULT_MAX_ENUM)

#define VK_MAX_QUEUES 3


VulkanResourceEngine vk_engine = {0};

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback,
    void *user_data) {
  (void)severity;
  (void)type;
  (void)user_data;
  // @TODO: add some cool logging?
  fprintf(stderr, "Validation layer: %s\n", callback->pMessage);
  return VK_FALSE;
}

static void vk_create_instance(void) {
  VkApplicationInfo app_info = {
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext              = 0,
    .pApplicationName   = "Game",
    .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
    .pEngineName        = "Engine",
    .engineVersion      = VK_MAKE_VERSION(0, 0, 0),
    .apiVersion         = VK_API_VERSION_1_4,
  };

  uint32_t max_layer_count = 0;
  uint32_t extension_count = 0;
  vkEnumerateInstanceLayerProperties(&max_layer_count, 0);

  uint32_t layer_count = 0;
  const char *layer_names[1] = {"VK_LAYER_KHRONOS_validation"};

  mb_TempArena arena = mb_begin_temp_arena(0);
  VkLayerProperties *properties = mb_arena_push(arena.arena, VkLayerProperties, .count = max_layer_count);
  vkEnumerateInstanceLayerProperties(&max_layer_count, properties);


  for (uint32_t i = 0; i < max_layer_count; ++i) {
    fprintf(stdout, "VkInstance Layers %2u) %s : %s\n", i + 1, properties[i].layerName, properties[i].description);
    if (strcmp(properties[i].layerName, layer_names[0]) == 0) { layer_count = 1; }
  }

  // @NOTE: should we remove this. Feels abit error prone to end temp arena twice.
  mb_end_temp_arena(&arena);

  // For win32 surface and stuff.
  const char * const *sdl_instance_extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);
  const char **instance_extensions = mb_arena_push(arena.arena, const char *, .count = extension_count + 1);
  MemoryCopy(instance_extensions, sdl_instance_extensions, sizeof(const char *) * extension_count);
  instance_extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  for(uint32_t i = 0; i < extension_count; ++i) { fprintf(stdout, "VkInstance Extension %u) %s\n", i + 1, instance_extensions[i]); }

  VkInstanceCreateInfo instance_create_info = {
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext                   = 0,
    .flags                   = 0,
    .pApplicationInfo        = &app_info,
    .enabledLayerCount       = layer_count,
    .ppEnabledLayerNames     = layer_names,
    .enabledExtensionCount   = extension_count,
    .ppEnabledExtensionNames = instance_extensions,
  };

  VK_EXPECT_SUCCESS(vkCreateInstance(&instance_create_info, vk_engine.allocator, &vk_engine.instance));

  mb_end_temp_arena(&arena);

}

static void vk_create_debug_utils_messenger(void) {
  VkDebugUtilsMessengerCreateInfoEXT create_info = {
    .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debug_utils_callback,
    .pUserData       = 0
  };
  VK_EXPECT_SUCCESS(vkCreateDebugUtilsMessengerEXT(vk_engine.instance, &create_info, vk_engine.allocator, &vk_engine.debug));
}


VkSurfaceKHR vk_create_surface(SDL_Window *sdl) {
  VkSurfaceKHR surface = 0;
  b8 result = SDL_Vulkan_CreateSurface(sdl, vk_engine.instance, vk_engine.allocator, &surface);
  assert(result);
  return surface;
}

static void vk_choose_physical_device() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(vk_engine.instance, &count, 0);

  mb_TempArena temp = mb_begin_temp_arena(0);

  VkPhysicalDevice *pd = mb_arena_push(temp.arena, VkPhysicalDevice, .count = count);
  vkEnumeratePhysicalDevices(vk_engine.instance, &count, pd);

  for(uint32_t i = 0; i < count; ++i) {
    mb_TempArena scope_temp = mb_begin_temp_arena(temp.arena);
    uint32_t queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd[i], &queue_count, 0);

    VkQueueFamilyProperties *queue_props = mb_arena_push(scope_temp.arena, VkQueueFamilyProperties, .count = queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(pd[i], &queue_count, queue_props);

    for(uint32_t j = 0; j < queue_count; ++j) {
      if(((queue_props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT) &&
         ((queue_props[j].queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT) &&
         (SDL_Vulkan_GetPresentationSupport(vk_engine.instance, pd[i], j))) {
        vk_engine.physical_device = pd[i];
        vk_engine.queue_family_index = j;
      }
    }
    mb_end_temp_arena(&scope_temp);
  }

  mb_end_temp_arena(&temp);
  assert(vk_engine.physical_device);
}

static void vk_create_device(void) {
  mb_TempArena temp = mb_begin_temp_arena(0);

  uint32_t queue_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(vk_engine.physical_device, &queue_count, 0);

  VkQueueFamilyProperties *queue_props = mb_arena_push(temp.arena, VkQueueFamilyProperties, .count = queue_count);
  vkGetPhysicalDeviceQueueFamilyProperties(vk_engine.physical_device, &queue_count, queue_props);

  const float priority = 1.0f;
  VkDeviceQueueCreateInfo queue_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = vk_engine.queue_family_index,
    .queueCount = 1, // queue_props[i].queueCount;
    .pQueuePriorities = &priority,
  };

  const char *enabled_extension[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkPhysicalDeviceVulkan12Features enabled_vk12_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .descriptorIndexing = VK_TRUE,
    .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
    .descriptorBindingVariableDescriptorCount = VK_TRUE,
    .runtimeDescriptorArray = VK_TRUE,
    .bufferDeviceAddress = VK_TRUE
  };
  VkPhysicalDeviceVulkan13Features enabled_vk13_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext = &enabled_vk12_features,
    .synchronization2 = VK_TRUE,
    .dynamicRendering = VK_TRUE
  };
  VkPhysicalDeviceFeatures enabled_features = { .samplerAnisotropy = VK_TRUE };

  VkDeviceCreateInfo create_info = {
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                   = &enabled_vk13_features,
    .flags                   = 0,
    .queueCreateInfoCount    = 1,
    .pQueueCreateInfos       = &queue_create_info,
    // @NOTE: enabledLayerCount is legacy and should not be used
    .enabledLayerCount       = 0,
    // @NOTE: ppEnabledLayerNames is legacy and should not be used
    .ppEnabledLayerNames     = 0,
    .enabledExtensionCount   = 1,
    .ppEnabledExtensionNames = enabled_extension,
    .pEnabledFeatures        = &enabled_features,
  };

  VK_EXPECT_SUCCESS(vkCreateDevice(vk_engine.physical_device, &create_info, vk_engine.allocator, &vk_engine.device));
  assert(vk_engine.device);
  volkLoadDevice(vk_engine.device);

  vkGetDeviceQueue(vk_engine.device, vk_engine.queue_family_index, 0, &vk_engine.queue);
  mb_end_temp_arena(&temp);
}

static void vk_create_or_recreate_swapchain(Window *window) {
  VkSurfaceCapabilitiesKHR surface_capabilities = {0};
  VK_EXPECT_SUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_engine.physical_device, window->surface, &surface_capabilities));

  VkExtent2D extent = surface_capabilities.currentExtent;
  if (surface_capabilities.currentExtent.width == 0xFFFFFFFF) {
    int w, h;
    SDL_GetWindowSize(window->sdl, &w, &h);
    assert(w >= 0 && h >= 0);
    extent = (VkExtent2D){ .width = (uint32_t)w, .height = (uint32_t)h };
  }

  const VkFormat image_format = VK_FORMAT_B8G8R8A8_SRGB ;


  uint32_t present_mode_count = 0;
  VK_EXPECT_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_engine.physical_device, window->surface, &present_mode_count, NULL));
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

  if (present_mode_count > 0) {
    mb_TempArena tmp = mb_begin_temp_arena(0);
    VkPresentModeKHR* modes = mb_arena_push(tmp.arena, VkPresentModeKHR, .count = present_mode_count);
    assert(modes);
    VK_EXPECT_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_engine.physical_device, window->surface, &present_mode_count, modes));
    for (uint32_t i = 0; i < present_mode_count; ++i) {
      if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) { present_mode = VK_PRESENT_MODE_MAILBOX_KHR; break; }
    }
    mb_end_temp_arena(&tmp);
  }

  VkSwapchainCreateInfoKHR create_info = {
    .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext                 = 0,
    .flags                 = 0,
    .surface               = window->surface,
    .minImageCount         = surface_capabilities.minImageCount,
    .imageFormat           = image_format,
    .imageColorSpace       = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
    .imageExtent           = extent,
    .imageArrayLayers      = 1,
    .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode      = 0,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices   = 0,
    .preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode           = present_mode,
    .clipped               = VK_FALSE,
    .oldSwapchain          = window->sc,
  };

  VK_EXPECT_SUCCESS(vkCreateSwapchainKHR(vk_engine.device, &create_info, vk_engine.allocator, &window->sc));

  if(create_info.oldSwapchain) vkDestroySwapchainKHR(vk_engine.device, create_info.oldSwapchain, vk_engine.allocator);

  // clean up past window stuff.
  for(uint32_t i = 0; i < window->v_count; ++i) {
    vkDestroyImageView(vk_engine.device, window->image_views[i], vk_engine.allocator);
  }

  VkCommandBuffer cmd_buffers[VK_MAX_FRAMES_IN_FLIGHT];
  for(uint32_t i = 0; i < VK_MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyFence(vk_engine.device, window->d[i].fence, vk_engine.allocator);
    vkDestroySemaphore(vk_engine.device, window->d[i].render_sem, vk_engine.allocator);
    vkDestroySemaphore(vk_engine.device, window->d[i].present_sem, vk_engine.allocator);
    cmd_buffers[i] = window->d[i].cmd_buf;
  }
  if(window->cmd_pool) vkFreeCommandBuffers(vk_engine.device, window->cmd_pool, VK_MAX_FRAMES_IN_FLIGHT, cmd_buffers);

  // we want to clean up everything here as well so that we don't leak. In the future we could probably keep those stuff until it needs
  // to disappear like a bin.
  mb_arena_clear(window->arena);
  VK_EXPECT_SUCCESS(vkGetSwapchainImagesKHR(vk_engine.device, window->sc, &window->v_count,  0));

  window->images = mb_arena_push(window->arena, VkImage, .count = window->v_count);
  VK_EXPECT_SUCCESS(vkGetSwapchainImagesKHR(vk_engine.device, window->sc, &window->v_count, window->images));

  // @TODO: this needs to change when we decide to recreate stuff. But for now we can just leak lol.
  window->image_views = mb_arena_push(window->arena, VkImageView, .count = window->v_count);
  for(uint32_t i = 0; i < window->v_count; ++i) {
    VkImageViewCreateInfo v_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = 0,
      .flags = 0,
      .image = window->images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = image_format,
      .components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
      },
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      }
    };
    VK_EXPECT_SUCCESS(vkCreateImageView(vk_engine.device, &v_create_info, vk_engine.allocator, window->image_views + i));
  }

  VkCommandBufferAllocateInfo command_buffer_allocate_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = window->cmd_pool,
    .commandBufferCount = VK_MAX_FRAMES_IN_FLIGHT
  };
  VK_EXPECT_SUCCESS(vkAllocateCommandBuffers(vk_engine.device, &command_buffer_allocate_info, cmd_buffers));

  for(uint32_t i = 0; i < VK_MAX_FRAMES_IN_FLIGHT; ++i) {
    VkFenceCreateInfo fence_create_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = 0,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    VK_EXPECT_SUCCESS(vkCreateFence(vk_engine.device, &fence_create_info, vk_engine.allocator, &window->d[i].fence));
    VkSemaphoreCreateInfo semaphore_create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = 0,
      .flags = 0,
    };
    VK_EXPECT_SUCCESS(vkCreateSemaphore(vk_engine.device, &semaphore_create_info, vk_engine.allocator, &window->d[i].present_sem));
    VK_EXPECT_SUCCESS(vkCreateSemaphore(vk_engine.device, &semaphore_create_info, vk_engine.allocator, &window->d[i].render_sem));

    window->d[i].cmd_buf = cmd_buffers[i];
  }
}

void initialize_vulkan(void) {
  SDL_Vulkan_LoadLibrary(NULL);
  VK_EXPECT_SUCCESS(volkInitialize());
  vk_create_instance();
  volkLoadInstance(vk_engine.instance);
  vk_create_debug_utils_messenger();
  vk_choose_physical_device();

  VkPhysicalDeviceProperties physical_device_properties = {0};
  vkGetPhysicalDeviceProperties(vk_engine.physical_device, &physical_device_properties);
  fprintf(stdout, "Chosen physical device :%s\n", physical_device_properties.deviceName);
  vk_create_device();

  VmaVulkanFunctions vk_functions = {
    .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
    .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    .vkCreateImage = vkCreateImage
  };
  VmaAllocatorCreateInfo vma_create_info = {
    .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice = vk_engine.physical_device,
    .device = vk_engine.device,
    .pVulkanFunctions = &vk_functions,
    .instance = vk_engine.instance,
    .vulkanApiVersion = VK_API_VERSION_1_4,
  };
  VK_EXPECT_SUCCESS(vmaCreateAllocator(&vma_create_info, &vk_engine.vma));
}

void cleanup_vulkan(void) {
  VK_EXPECT_SUCCESS(vkDeviceWaitIdle(vk_engine.device));
  vmaDestroyAllocator(vk_engine.vma);
  vkDestroyDevice(vk_engine.device, vk_engine.allocator);
  vkDestroyDebugUtilsMessengerEXT(vk_engine.instance, vk_engine.debug, vk_engine.allocator);
  vkDestroyInstance(vk_engine.instance, vk_engine.allocator);
}

// @TODO: we can change this to just create window and then from there initialize vulkan if we have not.
Window create_window(SDL_Window *sdl) {
  // figure out when we should initialize (create_window right now cannot be called twice).
  Window window = {0};
  window.sdl = sdl;
  window.arena = mb_arena_create(0, KB(1));
  window.surface = vk_create_surface(sdl);

  VkCommandPoolCreateInfo command_pool_create_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = 0,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = vk_engine.queue_family_index,
  };
  VK_EXPECT_SUCCESS(vkCreateCommandPool(vk_engine.device, &command_pool_create_info, vk_engine.allocator, &window.cmd_pool));

  vk_create_or_recreate_swapchain(&window);
  return window;
}

void destroy_window(Window *window) {
  assert(window);
  // @TODO: figure out a good way to cleanup.
  VK_EXPECT_SUCCESS(vkDeviceWaitIdle(vk_engine.device));


  // clean up past window stuff.
  for(uint32_t i = 0; i < window->v_count; ++i) {
    vkDestroyImageView(vk_engine.device, window->image_views[i], vk_engine.allocator);
  }

  VkCommandBuffer cmd_buffers[VK_MAX_FRAMES_IN_FLIGHT];
  for(uint32_t i = 0; i < VK_MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyFence(vk_engine.device, window->d[i].fence, vk_engine.allocator);
    vkDestroySemaphore(vk_engine.device, window->d[i].render_sem, vk_engine.allocator);
    vkDestroySemaphore(vk_engine.device, window->d[i].present_sem, vk_engine.allocator);
    cmd_buffers[i] = window->d[i].cmd_buf;
  }
  vkFreeCommandBuffers(vk_engine.device, window->cmd_pool, VK_MAX_FRAMES_IN_FLIGHT, cmd_buffers);

  vkDestroyCommandPool(vk_engine.device, window->cmd_pool, vk_engine.allocator);
  vkDestroySwapchainKHR(vk_engine.device, window->sc, vk_engine.allocator);
  SDL_Vulkan_DestroySurface(vk_engine.instance, window->surface, vk_engine.allocator);
  mb_arena_destroy(window->arena);
}

const char *vk_result_to_string(VkResult result) {
  switch(result) {
#define X(result) case result: return #result;
    VK_RESULT_LIST
#undef X
  }
  return "VkResultUnknown";
}


void window_resize(Window *window) {
  VK_EXPECT_SUCCESS(vkDeviceWaitIdle(vk_engine.device));
  vk_create_or_recreate_swapchain(window);
}
