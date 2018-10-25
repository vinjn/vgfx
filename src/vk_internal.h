#pragma once

#include "internal.h"

// Internal init functions
void tr_internal_vk_create_renderer(const char* app_name, const tr_renderer_settings* settings,
                                    tr_renderer** pp_renderer);
void tr_internal_vk_create_instance(const char* app_name, tr_renderer* p_renderer);
void tr_internal_vk_create_surface(tr_renderer* p_renderer);
void tr_internal_vk_create_device(tr_renderer* p_renderer);
void tr_internal_vk_create_swapchain(tr_renderer* p_renderer);
void tr_internal_create_swapchain_renderpass(tr_renderer* p_renderer);
void tr_internal_vk_create_swapchain_renderpass(tr_renderer* p_renderer);
void tr_internal_vk_destroy_instance(tr_renderer* p_renderer);
void tr_internal_vk_destroy_surface(tr_renderer* p_renderer);
void tr_internal_vk_destroy_device(tr_renderer* p_renderer);
void tr_internal_vk_destroy_swapchain(tr_renderer* p_renderer);

// Internal create functions
void tr_internal_vk_create_fence(tr_renderer* p_renderer, tr_fence* p_fence);
void tr_internal_vk_destroy_fence(tr_renderer* p_renderer, tr_fence* p_fence);
void tr_internal_vk_create_semaphore(tr_renderer* p_renderer, tr_semaphore* p_semaphore);
void tr_internal_vk_destroy_semaphore(tr_renderer* p_renderer, tr_semaphore* p_semaphore);
void tr_internal_vk_create_descriptor_set(tr_renderer* p_renderer,
                                          tr_descriptor_set* p_descriptor_set);
void tr_internal_vk_destroy_descriptor_set(tr_renderer* p_renderer,
                                           tr_descriptor_set* p_descriptor_set);
void tr_internal_vk_create_cmd_pool(tr_renderer* p_renderer, tr_queue* p_queue, bool transient,
                                    tr_cmd_pool* p_cmd_pool);
void tr_internal_vk_destroy_cmd_pool(tr_renderer* p_renderer, tr_cmd_pool* p_cmd_pool);
void tr_internal_vk_create_cmd(tr_cmd_pool* p_cmd_pool, bool secondary, tr_cmd* p_cmd);
void tr_internal_vk_destroy_cmd(tr_cmd_pool* p_cmd_pool, tr_cmd* p_cmd);
void tr_internal_vk_create_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);
void tr_internal_vk_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);
void tr_internal_vk_create_texture(tr_renderer* p_renderer, tr_texture* p_texture);
void tr_internal_vk_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture);
void tr_internal_vk_create_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);
void tr_internal_vk_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);
void tr_internal_vk_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program,
                                    const tr_vertex_layout* p_vertex_layout,
                                    tr_descriptor_set* pp_descriptor_set,
                                    tr_render_target* p_render_target,
                                    const tr_pipeline_settings* p_pipeline_settings,
                                    tr_pipeline* p_pipeline);
void tr_internal_vk_create_compute_pipeline(tr_renderer* p_renderer,
                                            tr_shader_program* p_shader_program,
                                            tr_descriptor_set* p_descriptor_set,
                                            const tr_pipeline_settings* p_pipeline_settings,
                                            tr_pipeline* p_pipeline);
void tr_internal_vk_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline);
void tr_internal_vk_create_shader_program(
    tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt,
    uint32_t tesc_size, const void* tesc_code, const char* tesc_enpt, uint32_t tese_size,
    const void* tese_code, const char* tese_enpt, uint32_t geom_size, const void* geom_code,
    const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt,
    uint32_t comp_size, const void* comp_code, const char* comp_enpt,
    tr_shader_program* p_shader_program);
void tr_internal_vk_destroy_shader_program(tr_renderer* p_renderer,
                                           tr_shader_program* p_shader_program);
void tr_internal_vk_create_render_target(tr_renderer* p_renderer, bool is_swapchain,
                                         tr_render_target* p_render_target);
void tr_internal_vk_destroy_render_target(tr_renderer* p_renderer,
                                          tr_render_target* p_render_target);

// Internal descriptor set functions
void tr_internal_vk_update_descriptor_set(tr_renderer* p_renderer,
                                          tr_descriptor_set* p_descriptor_set);

// Internal command buffer functions
void tr_internal_vk_begin_cmd(tr_cmd* p_cmd);
void tr_internal_vk_end_cmd(tr_cmd* p_cmd);
void tr_internal_vk_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target);
void tr_internal_vk_cmd_end_render(tr_cmd* p_cmd);
void tr_internal_vk_cmd_set_viewport(tr_cmd* p_cmd, float x, float, float width, float height,
                                     float min_depth, float max_depth);
void tr_internal_vk_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width,
                                    uint32_t height);
void tr_internal_vk_cmd_set_line_width(tr_cmd* p_cmd, float line_width);
void tr_cmd_internal_vk_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index,
                                                   const tr_clear_value* clear_value);
void tr_cmd_internal_vk_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd,
                                                           const tr_clear_value* clear_value);
void tr_internal_vk_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline);
void tr_internal_vk_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline,
                                             tr_descriptor_set* p_descriptor_set);
void tr_internal_vk_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer);
void tr_internal_vk_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count,
                                            tr_buffer** pp_buffers);
void tr_internal_vk_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex);
void tr_internal_vk_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index);
void tr_internal_vk_cmd_draw_mesh(tr_cmd* p_cmd, const tr_mesh* p_mesh);
void tr_internal_vk_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer,
                                          tr_buffer_usage old_usage, tr_buffer_usage new_usage);
void tr_internal_vk_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture,
                                         tr_texture_usage old_usage, tr_texture_usage new_usage);
void tr_internal_vk_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target,
                                                 tr_texture_usage old_usage,
                                                 tr_texture_usage new_usage);
void tr_internal_vk_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y,
                                 uint32_t group_count_z);
void tr_internal_vk_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height,
                                                 uint32_t row_pitch, uint64_t buffer_offset,
                                                 uint32_t mip_level, tr_buffer* p_buffer,
                                                 tr_texture* p_texture);

// Internal queue/swapchain functions
void tr_internal_vk_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore,
                                       tr_fence* p_fence);
void tr_internal_vk_queue_submit(tr_queue* p_queue, uint32_t cmd_count, tr_cmd** pp_cmds,
                                 uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores,
                                 uint32_t signal_semaphore_count,
                                 tr_semaphore** pp_signal_semaphores);
void tr_internal_vk_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count,
                                  tr_semaphore** pp_wait_semaphores);
void tr_internal_vk_queue_wait_idle(tr_queue* p_queue);

VkFormat tr_util_to_vk_format(tr_format format);

tr_format tr_util_from_vk_format(VkFormat format);

VkShaderStageFlags tr_util_to_vk_shader_stages(tr_shader_stage shader_stages);