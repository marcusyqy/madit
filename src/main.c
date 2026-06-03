#include "mb/mb_base.h"
#include "mb/mb_arena.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdint.h>
#include "render/render.h"
#include "render/shader_compiler.h"

#include <volk/volk.h>

#include "render/font.h"

typedef struct RenderPipeline {
  VkPipeline handle;
  VkPipelineLayout layout;
} RenderPipeline;

static RenderPipeline create_default_render_pipeline(void) {
  RenderPipeline rp = {0};

  // @TODO: when we need to pass information to the shader.
  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .setLayoutCount = 0,
    .pSetLayouts = 0,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = 0,
  };
  VK_EXPECT_SUCCESS(vkCreatePipelineLayout(vk_engine.device, &pipeline_layout_create_info, vk_engine.allocator, &rp.layout));

  ShadercInfo sc = shader_compiler_create();
  // compile spv
  mb_TempArena temp = mb_begin_temp_arena(0);
  mb_StringView spv_vert_shader = shader_compiler_read_glsl_and_compile_to_spv(temp.arena, &sc, mb_str_from_cstr("data/shaders/color.vert"), shaderc_glsl_vertex_shader);
  mb_StringView spv_frag_shader = shader_compiler_read_glsl_and_compile_to_spv(temp.arena, &sc, mb_str_from_cstr("data/shaders/color.frag"), shaderc_glsl_fragment_shader);
  assert(spv_vert_shader.count && spv_frag_shader.count);

  VkShaderModule vert_shader_module = 0;
  VkShaderModuleCreateInfo vert_shader_ci = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .codeSize = spv_vert_shader.count,
    .pCode = (const uint32_t *)spv_vert_shader.data,
  };
  VK_EXPECT_SUCCESS(vkCreateShaderModule(vk_engine.device, &vert_shader_ci, vk_engine.allocator, &vert_shader_module));

  VkShaderModule frag_shader_module = 0;
  VkShaderModuleCreateInfo shader_ci = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .codeSize = spv_frag_shader.count,
    .pCode = (const uint32_t *)spv_frag_shader.data,
  };
  VK_EXPECT_SUCCESS(vkCreateShaderModule(vk_engine.device, &shader_ci, vk_engine.allocator, &frag_shader_module));

  VkPipelineShaderStageCreateInfo shader_stages[] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vert_shader_module,
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = frag_shader_module,
      .pName = "main",
    }
  };

  // this is now hardcoded inside the window as well.
  const VkFormat color_format = VK_FORMAT_B8G8R8A8_SRGB;
  VkPipelineRenderingCreateInfo rendering_ci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .pNext = 0,
    .viewMask = 0,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &color_format,
    .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .vertexBindingDescriptionCount = 0,
    .pVertexBindingDescriptions = 0,
    .vertexAttributeDescriptionCount = 0,
    .pVertexAttributeDescriptions = 0,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = 0,
  };

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .viewportCount = 1,
    .pViewports = 0, // 0 because of dynamic state.
    .scissorCount = 1,
    .pScissors = 0, // 0 because of dynamic state.
  };

  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .depthClampEnable = 0,
    .rasterizerDiscardEnable = 0,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .frontFace = VK_FRONT_FACE_CLOCKWISE, // VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = 0,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
    .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = 0,
    .minSampleShading = 1.0f,
    .pSampleMask = 0,
    .alphaToCoverageEnable = 0,
    .alphaToOneEnable = 0,
  };

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .depthTestEnable = 0,
    .depthWriteEnable = 0,
    .depthCompareOp = VK_COMPARE_OP_NEVER,
    .depthBoundsTestEnable = 0,
    .stencilTestEnable = 0,
    .front = (VkStencilOpState){0},
    .back = (VkStencilOpState){0},
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f,
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
    .blendEnable = 0,
    .srcColorBlendFactor = 0,
    .dstColorBlendFactor = 0,
    .colorBlendOp = 0,
    .srcAlphaBlendFactor = 0,
    .dstAlphaBlendFactor = 0,
    .alphaBlendOp = 0,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .logicOpEnable = 0,
    .logicOp = VK_LOGIC_OP_CLEAR,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment_state,
    .blendConstants = {0.f, 0.f, 0.f, 0.f},
 };

  VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .dynamicStateCount = ArrayCount(dynamic_states),
    .pDynamicStates = dynamic_states,
  };

  VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &rendering_ci,
    .flags = 0,
    .stageCount = ArrayCount(shader_stages),
    .pStages = shader_stages,
    .pVertexInputState = &vertex_input_state_create_info,
    .pInputAssemblyState = &input_assembly_state_create_info,
    .pTessellationState = 0,
    .pViewportState = &viewport_state_create_info,
    .pRasterizationState = &rasterization_state_create_info,
    .pMultisampleState = &multisample_state_create_info,
    .pDepthStencilState = &depth_stencil_state_create_info,
    .pColorBlendState = &color_blend_state_create_info,
    .pDynamicState = &dynamic_state_create_info,
    .layout = rp.layout,
    .renderPass = 0,
    .subpass = 0,
    .basePipelineHandle = 0,
    .basePipelineIndex = 0
  };

  VK_EXPECT_SUCCESS(vkCreateGraphicsPipelines(vk_engine.device, 0, 1, &graphics_pipeline_create_info, vk_engine.allocator, &rp.handle));

  mb_end_temp_arena(&temp);
  shader_compiler_destroy(&sc);
  vkDestroyShaderModule(vk_engine.device, vert_shader_module, vk_engine.allocator);
  vkDestroyShaderModule(vk_engine.device, frag_shader_module, vk_engine.allocator);

  return rp;
}

void render_pipeline_destroy(RenderPipeline *rp) {
  vkDestroyPipeline(vk_engine.device, rp->handle, vk_engine.allocator);
  vkDestroyPipelineLayout(vk_engine.device, rp->layout, vk_engine.allocator);
}

static int window_application() {
  if(!SDL_Init(SDL_INIT_VIDEO)) fprintf(stderr, "Failed to initialize SDL3: %s\n", SDL_GetError());
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");
  initialize_vulkan();

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  fprintf(stdout, "Main scale %f\n", main_scale);

  SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE; // | SDL_WINDOW_MAXIMIZED;

  // @TODO: figure out this (probably good for high dpi stuff).
  // SDL_Rect rect = {};
  // SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &rect);

  uint32_t window_width = 1200;
  uint32_t window_height = 800;

  SDL_Window *sdl = SDL_CreateWindow("Madit", window_width, window_height, window_flags);
  if(!sdl) {
    fprintf(stdout, "Failed to create SDL3 window: %s\n", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  Window window = create_window(sdl);
  RenderPipeline rp = create_default_render_pipeline();

  SDL_Event event = {0};
  b8 running = 1;
  b8 need_resize = 0;
  while(running) {

    // to make things better we may have to make this threaded? Unless we don't care about how fast the window resizes
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_EVENT_QUIT) running = 0;
      if(event.window.windowID == SDL_GetWindowID(window.sdl)) {
        if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = 0;
        if(event.type == SDL_EVENT_WINDOW_RESIZED) {
            window_width  = (uint32_t)event.window.data1;
            window_height = (uint32_t)event.window.data2;
            need_resize = 1;
        }
        if(event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_Q) running = 0;
      }
    }

    if(need_resize) {
      window_resize(&window);
      need_resize = 0;
      continue;
    }

    WindowDynData *curr_data = &window.d[window.curr_frame_idx];
    VK_EXPECT_SUCCESS(vkWaitForFences(vk_engine.device, 1, &curr_data->fence, 1, UINT64_MAX));
    VK_EXPECT_SUCCESS(vkResetFences(vk_engine.device, 1, &curr_data->fence));

    uint32_t image_idx = 0;
    VK_EXPECT_SUCCESS(vkAcquireNextImageKHR(vk_engine.device, window.sc, UINT64_MAX, curr_data->present_sem, VK_NULL_HANDLE, &image_idx));

    VK_EXPECT_SUCCESS(vkResetCommandBuffer(curr_data->cmd_buf, 0));

    VkCommandBufferBeginInfo cmd_begin_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    VK_EXPECT_SUCCESS(vkBeginCommandBuffer(curr_data->cmd_buf, &cmd_begin_info));

    VkImageMemoryBarrier2 output_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .image = window.images[image_idx],
      .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
    };
    VkDependencyInfo output_barrier_dependency_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &output_barrier
    };
    vkCmdPipelineBarrier2(curr_data->cmd_buf, &output_barrier_dependency_info);

    VkRenderingAttachmentInfo render_color_attachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .pNext = 0,
      .imageView = window.image_views[image_idx],
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .resolveMode = 0,
      .resolveImageView = VK_NULL_HANDLE,
      .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = { .color = { 0.f, 0.f, 0.f, 1.f } },
    };
    VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .pNext = 0,
      .flags = 0,
      .renderArea = { .extent = { .width = window_width, .height = window_height } },
      .layerCount = 1,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachments = &render_color_attachment,
      .pDepthAttachment = 0,
      .pStencilAttachment= 0,
    };

    DeferScope(vkCmdBeginRendering(curr_data->cmd_buf, &rendering_info),
               vkCmdEndRendering(curr_data->cmd_buf))
    {
      VkViewport vp = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)window_width,
        .height = (float)window_height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      };
      vkCmdSetViewport(curr_data->cmd_buf, 0, 1, &vp);

      VkRect2D scissors = {
        .offset = { .x = 0, .y = 0 },
        .extent = { .width = (int32_t)window_width, .height = (int32_t)window_height }
      };
      vkCmdSetScissor(curr_data->cmd_buf, 0, 1, &scissors);

      vkCmdBindPipeline(curr_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, rp.handle);
      vkCmdDraw(curr_data->cmd_buf, 6, 1, 0, 0);
    }

    VkImageMemoryBarrier2 present_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE, // VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = window.images[image_idx],
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
    };
    VkDependencyInfo present_barrier_dependency_info = { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &present_barrier };
    vkCmdPipelineBarrier2(curr_data->cmd_buf, &present_barrier_dependency_info);
    VK_EXPECT_SUCCESS(vkEndCommandBuffer(curr_data->cmd_buf));

    VkPipelineStageFlags wait_stages = (VkPipelineStageFlags)VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &window.d[window.curr_frame_idx].present_sem,
        .pWaitDstStageMask = &wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &curr_data->cmd_buf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &window.d[window.curr_frame_idx].render_sem,
    };
    VK_EXPECT_SUCCESS(vkQueueSubmit(vk_engine.queue, 1, &submit_info, window.d[window.curr_frame_idx].fence));

    VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &window.d[window.curr_frame_idx].render_sem,
      .swapchainCount = 1,
      .pSwapchains = &window.sc,
      .pImageIndices = &image_idx
    };

    // we need to take into account VK_ERROR_OUT_OF_DATE_KHR for resize next time. For now we don't need resizing.
    VkResult present_result = vkQueuePresentKHR(vk_engine.queue, &present_info);
    if(present_result == VK_ERROR_OUT_OF_DATE_KHR) need_resize = 1;
    else if(present_result != VK_SUCCESS)          printf("Vk call `vkQueuePresentKHR`: failed with %s", vk_result_to_string(present_result));
    else                                           window.curr_frame_idx = (window.curr_frame_idx + 1) % VK_MAX_FRAMES_IN_FLIGHT;
  }

  destroy_window(&window);
  render_pipeline_destroy(&rp);
  SDL_DestroyWindow(window.sdl);
  cleanup_vulkan();
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();
  return 0;
}

// #define TESTING
int main() {
#ifndef TESTING
  return window_application();
#else
  font_test();
#endif
}

#define MB_INCLUDE_IMPLEMENTATION
#include "mb/mb.h"

// build c files.
#include "render/render.c"
#include "render/shader_compiler.c"
#include "render/font.c"
