#ifndef _RENDER_H_
#define _RENDER_H_

#include <SDL3/SDL.h>
#include <volk/volk.h>
#include "mb/mb_arena.h"
#include <vma/vk_mem_alloc.h>

#define VK_MAX_FRAMES_IN_FLIGHT 2

typedef struct VulkanResourceEngine {
  VkAllocationCallbacks    *allocator;
  VkInstance               instance;
  VkDebugUtilsMessengerEXT debug;
  VkPhysicalDevice         physical_device;
  uint32_t                 queue_family_index;

  VkDevice                 device;
  VkQueue                  queue;
  VmaAllocator             vma;
} VulkanResourceEngine;

extern VulkanResourceEngine vk_engine;

typedef struct WindowDynData {
  VkFence fence;
  VkSemaphore present_sem;
  VkSemaphore render_sem;

  VkCommandBuffer cmd_buf;
} WindowDynData;

typedef struct Window {
  SDL_Window     *sdl;
  mb_Arena       *arena;
  VkSurfaceKHR    surface;
  VkSwapchainKHR  sc;

  VkImage        *images;
  VkImageView    *image_views;
  uint32_t        v_count;

  VkCommandPool   cmd_pool;

  WindowDynData   d[VK_MAX_FRAMES_IN_FLIGHT];
  uint32_t        curr_frame_idx;
} Window;

void initialize_vulkan();
void cleanup_vulkan();

Window create_window(SDL_Window *sdl);
void   destroy_window(Window    *window);

void  window_resize(Window *window);

// misc
const char *vk_result_to_string(VkResult result);

#define VK_EXPECT_SUCCESS(result) do {                                              \
  VkResult __result = (result);                                                     \
  if(__result != VK_SUCCESS) {                                                      \
    printf("Vk call `" #result "`: failed with %s", vk_result_to_string(__result)); \
    exit(1);                                                                        \
  }                                                                                 \
} while(0)

#endif //  _RENDER_H_
