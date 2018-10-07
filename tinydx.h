/*
 Copyright 2017 Google Inc.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.


 Copyright (c) 2017, The Cinder Project, All rights reserved.

 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

/*

NOTES:
 - Requires C++
 - tinyvk/tinydx is written for experimentation and fun-having - not performance
 - For simplicity, only one descriptor set can be bound at once
   - In D3D12, this means two descriptor heaps (CBVSRVUAVs and samplers)
   - For Vulkan shaders the 'set' parameter for 'layout' should always be 0
   - For D3D12 shaders the 'space' parameter for resource bindings should always be 0
 - Vulkan like idioms are used primarily with some D3D12 wherever it makes sense
 - Storage buffers created with tr_create_storage_buffer are not host visible. 
   - This was done to align the behavior on Vulkan and D3D12. Vulkan's storage 
     buffers can be host visible, but D3D12's UAV buffers are not permitted to 
     be host visible.

COMPILING & LINKING
   In one C++ file that #includes this file, do this:
      #define TINY_RENDERER_IMPLEMENTATION
   before the #include. That will create the implementation in that file.

*/

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if ! defined(_WIN32)
    #error "Windows is needed!"
#endif

//
// C++ is the only language supported by D3D12:
//     https://msdn.microsoft.com/en-us/library/windows/desktop/dn899120(v=vs.85).aspx
//
#if ! defined(__cplusplus)
    #error "D3D12 requires C++! Sorry!"
#endif 

#define TINY_RENDERER_MSW
// Pull in minimal Windows headers
#if ! defined(NOMINMAX)
    #define NOMINMAX
#endif
#if ! defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_5.h>

#if defined(__cplusplus) && defined(TINY_RENDERER_CPP_NAMESPACE)
namespace TINY_RENDERER_CPP_NAMESPACE {
#endif

#if ! defined(TINY_RENDERER_CUSTOM_MAX)
enum {
    tr_max_instance_extensions       = 256,
    tr_max_device_extensions         = 256,
    tr_max_gpus                      = 4,
    tr_max_descriptors               = 32,
    tr_max_descriptor_sets           = 8,
    tr_max_render_target_attachments = 8,
    tr_max_submit_cmds               = 8,
    tr_max_submit_wait_semaphores    = 8,
    tr_max_submit_signal_semaphores  = 8,
    tr_max_present_wait_semaphores   = 8,
    tr_max_vertex_bindings           = 15,
    tr_max_vertex_attribs            = 15,
    tr_max_semantic_name_length      = 128,
    tr_max_descriptor_entries        = 256,
    tr_max_mip_levels                = 0xFFFFFFFF,
};
#endif

typedef enum tr_api {
    tr_api_d3d12 = 0,
    tr_api_vulkan
} tr_api;

typedef enum tr_log_type {
    tr_log_type_info = 0,
    tr_log_type_warn,
    tr_log_type_debug,
    tr_log_type_error
} tr_log_type;

typedef void(*tr_log_fn)(tr_log_type, const char*, const char*);

/*

There's a lot of things to get right in D3D12 for it to work at all. So for now, the renderer 
will just assert in all the places where something goes wrong. Keep in mind that any memory 
allocated prior to the assert is not freed.

In the future, maybe add a error handling system so keep these values around.

*/ 
/*
typedef enum tr_result {
    // No errors
    tr_result_ok                = 0x00000000,
    // Errors
    tr_result_error_mask        = 0x0000FFFF,
    tr_result_unknown           = 0x00000001,
    tr_result_bad_ptr           = 0x00000002,
    tr_result_alloc_failed      = 0x00000004,
    tr_result_exceeded_max      = 0x00000008,
    // Components
    tr_result_component_mask    = 0x0FFF0000,
    tr_result_general           = 0x00000000,
    tr_result_renderer          = 0x00010000,
    tr_result_environment       = 0x00020000,
    tr_result_device            = 0x00040000,
    tr_result_queue             = 0x00080000,
    tr_result_surface           = 0x00100000,
    tr_result_swapchain         = 0x00200000,
    tr_result_render_target     = 0x00400000,
    tr_result_buffer            = 0x00800000,
    tr_result_texture           = 0x01000000,
    tr_result_cmd               = 0x02000000,
    tr_result_fence             = 0x04000000,
    tr_result_semaphore         = 0x08000000,
    // Internal API
    tr_result_internal_api_mask = 0xF0000000,
    tr_result_internal_api      = 0x10000000,
} tr_result;
*/

typedef enum tr_buffer_usage {
    tr_buffer_usage_index                       = 0x00000001,
    tr_buffer_usage_vertex                      = 0x00000002,
    tr_buffer_usage_indirect                    = 0x00000004,
    tr_buffer_usage_transfer_src                = 0x00000008,
    tr_buffer_usage_transfer_dst                = 0x00000010,
    tr_buffer_usage_uniform_cbv                 = 0x00000020,
    tr_buffer_usage_storage_srv                 = 0x00000040,
    tr_buffer_usage_storage_uav                 = 0x00000080,
    tr_buffer_usage_uniform_texel_srv           = 0x00000100,
    tr_buffer_usage_storage_texel_uav           = 0x00000200,
    tr_buffer_usage_counter_uav                 = 0x00000400,
} tr_buffer_usage;

typedef enum tr_texture_type {
    tr_texture_type_1d,
    tr_texture_type_2d,
    tr_texture_type_3d,
    tr_texture_type_cube,
} tr_texture_type;

typedef enum tr_texture_usage {
    tr_texture_usage_undefined                  = 0x00000000,
    tr_texture_usage_transfer_src               = 0x00000001,
    tr_texture_usage_transfer_dst               = 0x00000002,
    tr_texture_usage_sampled_image              = 0x00000004,
    tr_texture_usage_storage_image              = 0x00000008,
    tr_texture_usage_color_attachment           = 0x00000010,
    tr_texture_usage_depth_stencil_attachment   = 0x00000020,
    tr_texture_usage_resolve_src                = 0x00000040,
    tr_texture_usage_resolve_dst                = 0x00000080,
    tr_texture_usage_present                    = 0x00000100,
} tr_texture_usage;

typedef uint32_t tr_texture_usage_flags;

typedef enum tr_format {
    tr_format_undefined = 0,
    // 1 channel
    tr_format_r8_unorm,
    tr_format_r16_unorm,
    tr_format_r16_float,
    tr_format_r32_uint,
    tr_format_r32_float,
    // 2 channel
    tr_format_r8g8_unorm,
    tr_format_r16g16_unorm,
    tr_format_r16g16_float,
    tr_format_r32g32_uint,
    tr_format_r32g32_float,
    // 3 channel
    tr_format_r8g8b8_unorm,
    tr_format_r16g16b16_unorm,
    tr_format_r16g16b16_float,
    tr_format_r32g32b32_uint,
    tr_format_r32g32b32_float,
    // 4 channel
    tr_format_b8g8r8a8_unorm,
    tr_format_r8g8b8a8_unorm,
    tr_format_r16g16b16a16_unorm,
    tr_format_r16g16b16a16_float,
    tr_format_r32g32b32a32_uint,
    tr_format_r32g32b32a32_float,
    // Depth/stencil
    tr_format_d16_unorm,
    tr_format_x8_d24_unorm_pack32,
    tr_format_d32_float,
    tr_format_s8_uint,
    tr_format_d16_unorm_s8_uint,
    tr_format_d24_unorm_s8_uint,
    tr_format_d32_float_s8_uint,
} tr_format;

typedef enum tr_descriptor_type {
    tr_descriptor_type_undefined = 0,
    tr_descriptor_type_sampler,
    tr_descriptor_type_uniform_buffer_cbv,       // CBV | VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    tr_descriptor_type_storage_buffer_srv,       // SRV | VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    tr_descriptor_type_storage_buffer_uav,       // UAV | VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    tr_descriptor_type_uniform_texel_buffer_srv, // SRV | VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
    tr_descriptor_type_storage_texel_buffer_uav, // UAV | VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
    tr_descriptor_type_texture_srv,              // SRV | VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    tr_descriptor_type_texture_uav,              // UAV | VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
} tr_descriptor_type;

typedef enum tr_sample_count {
    tr_sample_count_1  =  1,
    tr_sample_count_2  =  2,
    tr_sample_count_4  =  4,
    tr_sample_count_8  =  8,
    tr_sample_count_16 = 16,
} tr_sample_count;

typedef enum tr_shader_stage {
    tr_shader_stage_vert          = 0x00000001,
    tr_shader_stage_tesc          = 0x00000002,
    tr_shader_stage_tese          = 0x00000004,
    tr_shader_stage_geom          = 0x00000008,
    tr_shader_stage_frag          = 0x00000010,
    tr_shader_stage_comp          = 0x00000020,
    tr_shader_stage_all_graphics  = 0x0000001F,
    tr_shader_stage_hull          = tr_shader_stage_tesc,
    tr_shader_stage_domn          = tr_shader_stage_tese,
    tr_shader_stage_count         = 6,
} tr_shader_stage;

typedef enum tr_primitive_topo {
    tr_primitive_topo_point_list = 0,
    tr_primitive_topo_line_list,
    tr_primitive_topo_line_strip,
    tr_primitive_topo_tri_list,
    tr_primitive_topo_tri_strip,
    tr_primitive_topo_tri_fan,
    tr_primitive_topo_1_point_patch,
    tr_primitive_topo_2_point_patch,
    tr_primitive_topo_3_point_patch,
    tr_primitive_topo_4_point_patch,
} tr_primitive_topo;

typedef enum tr_index_type {
    tr_index_type_uint32 = 0,
    tr_index_type_uint16,
} tr_index_type;

typedef enum tr_semantic {
    tr_semantic_undefined = 0,
    tr_semantic_position,
    tr_semantic_normal,
    tr_semantic_color,
    tr_semantic_tangent,
    tr_semantic_bitangent,
    tr_semantic_texcoord0,
    tr_semantic_texcoord1,
    tr_semantic_texcoord2,
    tr_semantic_texcoord3,
    tr_semantic_texcoord4,
    tr_semantic_texcoord5,
    tr_semantic_texcoord6,
    tr_semantic_texcoord7,
    tr_semantic_texcoord8,
    tr_semantic_texcoord9,
} tr_semantic;

typedef enum tr_cull_mode {
    tr_cull_mode_none = 0,
    tr_cull_mode_back,
    tr_cull_mode_front,
    tr_cull_mode_both
} tr_cull_mode;

typedef enum tr_front_face {
    tr_front_face_ccw = 0,
    tr_front_face_cw
} tr_front_face;

// Has no effect in DX12, just here for consistency
typedef enum tr_tessellation_domain_origin {
    tr_tessellation_domain_origin_upper_left = 0,
    tr_tessellation_domain_origin_lower_left = 1,
} tr_tessellation_domain_origin;

typedef enum tr_pipeline_type {
  tr_pipeline_type_undefined = 0,
  tr_pipeline_type_compute,
  tr_pipeline_type_graphics
} tr_pipeline_type;

typedef enum tr_dx_shader_target {
    tr_dx_shader_target_5_0 = 0,
    tr_dx_shader_target_5_1,
    tr_dx_shader_target_6_0,
} tr_dx_shader_target;

// Forward declarations
typedef struct tr_renderer tr_renderer;
typedef struct tr_render_target tr_render_target;
typedef struct tr_buffer tr_buffer;
typedef struct tr_texture tr_texture;
typedef struct tr_sampler tr_sampler;

typedef struct tr_clear_value {
    union {
        struct {
            float                       r;
            float                       g;
            float                       b;
            float                       a;
        };
        struct {
            float                       depth;
            uint32_t                    stencil;
        };
    };
} tr_clear_value;

typedef struct tr_platform_handle {
#if defined(TINY_RENDERER_MSW)
    HINSTANCE                           hinstance;
    HWND                                hwnd;
#endif
} tr_platform_handle;

typedef struct tr_swapchain_settings {
    uint32_t                            image_count;
    tr_sample_count                     sample_count;
    uint32_t                            sample_quality;
    tr_format                           color_format;
    tr_clear_value                      color_clear_value;
    tr_format                           depth_stencil_format;
    tr_clear_value                      depth_stencil_clear_value;
} tr_swapchain_settings;

typedef struct tr_string_list {
    uint32_t                            count;
    const char**                        names;
} tr_string_list;

typedef struct tr_renderer_settings {
    tr_platform_handle                  handle;
    uint32_t                            width;
    uint32_t                            height;
    tr_swapchain_settings               swapchain;
    tr_log_fn                           log_fn;
    D3D_FEATURE_LEVEL                   dx_feature_level;
    tr_dx_shader_target                 dx_shader_target;
} tr_renderer_settings;

typedef struct tr_fence {
    ID3D12Fence*                        dx_fence;
} tr_fence;

typedef struct tr_semaphore {
    void*                               dx_semaphore;
} tr_semaphore;

typedef struct tr_queue {
    tr_renderer*                        renderer;
    ID3D12CommandQueue*                 dx_queue;       
    HANDLE                              dx_wait_idle_fence_event;
    ID3D12Fence*                        dx_wait_idle_fence;
    UINT64                              dx_wait_idle_fence_value;
} tr_queue;

typedef struct tr_renderer {
    tr_api                              api;
    tr_renderer_settings                settings;
    tr_render_target**                  swapchain_render_targets;
    uint32_t                            swapchain_image_index;
    tr_queue*                           graphics_queue;
    tr_queue*                           present_queue;
    tr_fence**                          image_acquired_fences;
    tr_semaphore**                      image_acquired_semaphores;
    tr_semaphore**                      render_complete_semaphores;
#if defined(_DEBUG)
    ID3D12Debug*                        dx_debug_ctrl;
#endif
    // Use IDXGIFactory4 for now since IDXGIFactory5
    // creates problems for the Visual Studio graphics
    // debugger.
    IDXGIFactory4*                      dx_factory;
    uint32_t                            dx_gpu_count;
    IDXGIAdapter3*                      dx_gpus[tr_max_gpus];
    IDXGIAdapter3*                      dx_active_gpu;
    ID3D12Device*                       dx_device;
    // Use IDXGISwapChain3 for now since IDXGISwapChain4
    // isn't supported by older devices.
    IDXGISwapChain3*                    dx_swapchain;
} tr_renderer;

typedef struct tr_descriptor {
    tr_descriptor_type                  type;
    uint32_t                            binding;
    uint32_t                            count;
    tr_shader_stage                     shader_stages;
    tr_buffer*                          uniform_buffers[tr_max_descriptor_entries];
    tr_texture*                         textures[tr_max_descriptor_entries];
    tr_sampler*                         samplers[tr_max_descriptor_entries];
    tr_buffer*                          buffers[tr_max_descriptor_entries];
    uint32_t                            dx_heap_offset;
    uint32_t                            dx_root_parameter_index;
} tr_descriptor;

typedef struct tr_descriptor_set {
    uint32_t                            descriptor_count;
    tr_descriptor*                      descriptors;
    ID3D12DescriptorHeap*               dx_cbvsrvuav_heap;
    ID3D12DescriptorHeap*               dx_sampler_heap;
} tr_descriptor_set;

typedef struct tr_cmd_pool {
    tr_renderer*                        renderer;
    ID3D12CommandAllocator*             dx_cmd_alloc;
} tr_cmd_pool;

typedef struct tr_cmd {
    tr_cmd_pool*                        cmd_pool;
    ID3D12GraphicsCommandList*          dx_cmd_list;
} tr_cmd;

typedef struct tr_buffer {
    tr_renderer*                        renderer;
    tr_buffer_usage                     usage;
    uint64_t                            size;
    bool                                host_visible;
    tr_index_type                       index_type;
    uint32_t                            vertex_stride;
    tr_format                           format;
    uint64_t                            first_element;
    uint64_t                            element_count;
    uint64_t                            struct_stride;
    bool                                raw;
    void*                               cpu_mapped_address;
    ID3D12Resource*                     dx_resource;
    D3D12_CONSTANT_BUFFER_VIEW_DESC     dx_cbv_view_desc;
    D3D12_SHADER_RESOURCE_VIEW_DESC     dx_srv_view_desc;
    D3D12_UNORDERED_ACCESS_VIEW_DESC    dx_uav_view_desc;
    D3D12_INDEX_BUFFER_VIEW             dx_index_buffer_view;  
    D3D12_VERTEX_BUFFER_VIEW            dx_vertex_buffer_view;
    // Counter buffer
    tr_buffer*                          counter_buffer;
} tr_buffer;

typedef struct tr_texture {
    tr_renderer*                        renderer;
    tr_texture_type                     type;
    tr_texture_usage_flags              usage;
    uint32_t                            width;
    uint32_t                            height;
    uint32_t                            depth;
    tr_format                           format;
    uint32_t                            mip_levels;
    tr_sample_count                     sample_count;
    uint32_t                            sample_quality;
    tr_clear_value                      clear_value;
    bool                                host_visible;
    void*                               cpu_mapped_address;
    uint32_t                            owns_image;
    ID3D12Resource*                     dx_resource;
    D3D12_SHADER_RESOURCE_VIEW_DESC     dx_srv_view_desc;
    D3D12_UNORDERED_ACCESS_VIEW_DESC    dx_uav_view_desc;
} tr_texture;

typedef struct tr_sampler {
    tr_renderer*                        renderer;
    D3D12_SAMPLER_DESC	                dx_sampler_desc;
} tr_sampler;

typedef struct tr_shader_program {
    tr_renderer*                        renderer;
    uint32_t                            shader_stages;
    ID3DBlob*                           dx_vert;
    ID3DBlob*                           dx_hull;
    ID3DBlob*                           dx_domn;
    ID3DBlob*                           dx_geom;
    ID3DBlob*                           dx_frag;
    ID3DBlob*                           dx_comp;
} tr_shader_program;

typedef struct tr_vertex_attrib {
    tr_semantic                         semantic;
    uint32_t                            semantic_name_length;
    char                                semantic_name[tr_max_semantic_name_length];
    tr_format                           format;
    uint32_t                            binding;
    uint32_t                            location;
    uint32_t                            offset;
} tr_vertex_attrib;

typedef struct tr_vertex_layout {
    uint32_t                            attrib_count;
    tr_vertex_attrib                    attribs[tr_max_vertex_attribs];
} tr_vertex_layout;

typedef struct tr_pipeline_settings {
    tr_primitive_topo                   primitive_topo;
    tr_cull_mode                        cull_mode;
    tr_front_face                       front_face;
    bool                                depth;    
    tr_tessellation_domain_origin       tessellation_domain_origin; // Has no effect in DX, here for consistency
} tr_pipeline_settings;

typedef struct tr_pipeline {
    tr_renderer*                        renderer;
    tr_pipeline_settings                settings;
    tr_pipeline_type                    type;
    ID3D12RootSignature*                dx_root_signature;
    ID3D12PipelineState*                dx_pipeline_state;
} tr_pipeline;

typedef struct tr_render_target {
    tr_renderer*                        renderer;
    uint32_t                            width;
    uint32_t                            height;
    tr_sample_count                     sample_count;
    tr_format                           color_format;
    uint32_t                            color_attachment_count;
    tr_texture*                         color_attachments[tr_max_render_target_attachments];
    tr_texture*                         color_attachments_multisample[tr_max_render_target_attachments];
    tr_format                           depth_stencil_format;
    tr_texture*                         depth_stencil_attachment;
    tr_texture*                         depth_stencil_attachment_multisample;
    ID3D12DescriptorHeap*               dx_rtv_heap;
    ID3D12DescriptorHeap*               dx_dsv_heap;
} tr_render_target;

typedef struct tr_mesh {
    tr_renderer*                        renderer;
    tr_buffer*                          uniform_buffer;
    tr_buffer*                          index_buffer;
    tr_buffer*                          vertex_buffer;
    tr_shader_program*                  shader_program;
    tr_pipeline*                        pipeline;
} tr_mesh;

typedef bool(*tr_image_resize_uint8_fn)(uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* src_data, 
                                        uint32_t dst_width, uint32_t dst_height, uint32_t dst_row_stride, uint8_t* dst_data,
                                        uint32_t channel_cout, void* user_data);

typedef bool(*tr_image_resize_float_fn)(uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* src_data, 
                                        uint32_t dst_width, uint32_t dst_height, uint32_t dst_row_stride, float* dst_data,
                                        uint32_t channel_cout, void* user_data);

// API functions
 void tr_create_renderer(const char* app_name, const tr_renderer_settings* p_settings, tr_renderer** pp_renderer);
 void tr_destroy_renderer(tr_renderer* p_renderer);

 void tr_create_fence(tr_renderer* p_renderer, tr_fence** pp_fence);
 void tr_destroy_fence(tr_renderer* p_renderer, tr_fence* p_fence);

 void tr_create_semaphore(tr_renderer* p_renderer, tr_semaphore** pp_semaphore);
 void tr_destroy_semaphore(tr_renderer* p_renderer, tr_semaphore* p_semaphore);

 void tr_create_descriptor_set(tr_renderer* p_renderer, uint32_t descriptor_count, const tr_descriptor* descriptors, tr_descriptor_set** pp_descriptor_set);
 void tr_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);

 void tr_create_cmd_pool(tr_renderer* p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool** pp_cmd_pool);
 void tr_destroy_cmd_pool(tr_renderer* p_renderer, tr_cmd_pool* p_cmd_pool);

 void tr_create_cmd(tr_cmd_pool* p_cmd_pool, bool secondary, tr_cmd** pp_cmd);
 void tr_destroy_cmd(tr_cmd_pool* p_cmd_pool, tr_cmd* p_cmd);

 void tr_create_cmd_n(tr_cmd_pool* p_cmd_pool, bool secondary, uint32_t cmd_count, tr_cmd*** ppp_cmd);
 void tr_destroy_cmd_n(tr_cmd_pool* p_cmd_pool, uint32_t cmd_count, tr_cmd** pp_cmd);

 void tr_create_buffer(tr_renderer* p_renderer, tr_buffer_usage usage, uint64_t size, bool host_visible, tr_buffer** pp_buffer);
 void tr_create_index_buffer(tr_renderer*p_renderer, uint64_t size, bool host_visible, tr_index_type index_type, tr_buffer** pp_buffer);
 void tr_create_uniform_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_buffer** pp_buffer);
 void tr_create_vertex_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, uint32_t vertex_stride, tr_buffer** pp_buffer);
 void tr_create_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element, uint64_t element_count, uint64_t struct_stride, bool raw, tr_buffer** pp_buffer);
 void tr_create_rw_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element, uint64_t element_count, uint64_t struct_stride, bool raw, tr_buffer** pp_counter_buffer, tr_buffer** pp_buffer);
 void tr_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);

 void tr_create_texture(tr_renderer* p_renderer, tr_texture_type type, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, uint32_t mip_levels, const tr_clear_value* p_clear_value, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture);
 void tr_create_texture_1d(tr_renderer* p_renderer, uint32_t width, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
 void tr_create_texture_2d(tr_renderer* p_renderer, uint32_t width, uint32_t height, tr_sample_count sample_count, tr_format format, uint32_t mip_levels, const tr_clear_value* p_clear_value, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
 void tr_create_texture_3d(tr_renderer* p_renderer, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
 void tr_destroy_texture(tr_renderer* p_renderer, tr_texture*p_texture);

 void tr_create_sampler(tr_renderer* p_renderer, tr_sampler** pp_sampler);
 void tr_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);

 void tr_create_shader_program_n(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t hull_size, const void* hull_code, const char* hull_enpt, uint32_t domn_size, const void* domn_code, const char* domn_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program);
 void tr_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, tr_shader_program** pp_shader_program);
 void tr_create_shader_program_compute(tr_renderer* p_renderer, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program);
 void tr_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program);

 void tr_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline);
 void tr_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline);
 void tr_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline);

 void tr_create_render_target(tr_renderer*p_renderer, uint32_t width, uint32_t height, tr_sample_count sample_count, tr_format color_format, uint32_t color_attachment_count, tr_format depth_stencil_format, tr_render_target** pp_render_target);
 void tr_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target);

 void tr_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);

 void tr_begin_cmd(tr_cmd* p_cmd);
 void tr_end_cmd(tr_cmd* p_cmd);
 void tr_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target);
 void tr_cmd_end_render(tr_cmd* p_cmd);
 void tr_cmd_set_viewport(tr_cmd* p_cmd, float x, float, float width, float height, float min_depth, float max_depth);
 void tr_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
 void tr_cmd_set_line_width(tr_cmd* p_cmd, float line_width);
 void tr_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value);
 void tr_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value);
 void tr_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline);
 void tr_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set);
 void tr_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer);
 void tr_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers);
 void tr_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex);
 void tr_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index);
 void tr_cmd_draw_mesh(tr_cmd* p_cmd, const tr_mesh* p_mesh);
 void tr_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage);
 void tr_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage);
 void tr_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage);
 void tr_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage);
 void tr_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
 void tr_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture);

 void tr_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence);
 void tr_queue_submit(tr_queue* p_queue, uint32_t cmd_count, tr_cmd** pp_cmds, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores, uint32_t signal_semaphore_count, tr_semaphore** pp_signal_semaphores);
 void tr_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores);
 void tr_queue_wait_idle(tr_queue* p_queue);

 void tr_render_target_set_color_clear_value(tr_render_target* p_render_target, uint32_t attachment_index, float r, float g, float b, float a);
 void tr_render_target_set_depth_stencil_clear_value(tr_render_target* p_render_target, float depth, uint8_t stencil);

 bool      tr_vertex_layout_support_format(tr_format format);
 uint32_t  tr_vertex_layout_stride(const tr_vertex_layout* p_vertex_layout);

// Utility functions
 uint64_t    tr_util_calc_storage_counter_offset(uint64_t buffer_size);
 uint32_t    tr_util_calc_mip_levels(uint32_t width, uint32_t height);
 DXGI_FORMAT tr_util_to_dx_format(tr_format format);
 tr_format   tr_util_from_dx_format(DXGI_FORMAT fomat);
 uint32_t    tr_util_format_stride(tr_format format);
 uint32_t    tr_util_format_channel_count(tr_format format);
 void        tr_util_transition_buffer(tr_queue* p_queue, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage);
 void        tr_util_transition_image(tr_queue* p_queue, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage);
 void        tr_util_set_storage_buffer_count(tr_queue* p_queue, uint64_t count_offset, uint32_t count, tr_buffer* p_buffer);
 void        tr_util_clear_buffer(tr_queue* p_queue, tr_buffer* p_buffer);
 void        tr_util_update_buffer(tr_queue* p_queue, uint64_t size, const void* p_src_data, tr_buffer* p_buffer);
 void        tr_util_update_texture_uint8(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* p_src_data, uint32_t src_channel_count, tr_texture* p_texture, tr_image_resize_uint8_fn resize_fn, void* p_user_data);
 void        tr_util_update_texture_float(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* p_src_data, uint32_t channels, tr_texture* p_texture, tr_image_resize_float_fn resize_fn, void* p_user_data);
