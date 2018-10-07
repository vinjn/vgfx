#include "dx_internal.h"
#include "internal.h"
#include "vk_internal.h"

using namespace std;

void tr_destroy_renderer(tr_renderer* p_renderer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != s_tr_internal);

    // Destroy the swapchain render targets
    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        tr_destroy_render_target(p_renderer, p_renderer->swapchain_render_targets[i]);
    }

    // Destroy render sync objects
    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        tr_destroy_fence(p_renderer, p_renderer->image_acquired_fences[i]);
    }
    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        tr_destroy_semaphore(p_renderer, p_renderer->image_acquired_semaphores[i]);
    }
    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        tr_destroy_semaphore(p_renderer, p_renderer->render_complete_semaphores[i]);
    }

    if (p_renderer->api == tr_api_vulkan)
    {
        tr_internal_vk_destroy_swapchain(p_renderer);
        tr_internal_vk_destroy_surface(p_renderer);
        tr_internal_vk_destroy_device(p_renderer);
        tr_internal_vk_destroy_instance(p_renderer);

        delete s_tr_internal->renderer->present_queue;
    }
    else
    {
        tr_internal_dx_destroy_swapchain(p_renderer);
        // tr_internal_dx_destroy_surface(p_renderer);
        tr_internal_dx_destroy_device(p_renderer);
        // tr_internal_dx_destroy_instance(p_renderer);

        // No need to destroy the present queue since it's just a pointer to the graphics queue
    }
    // Destroy the Vulkan bits

    // Free all the renderer components!
    delete s_tr_internal->renderer->graphics_queue;
    delete s_tr_internal->renderer;
    delete s_tr_internal;
}

void tr_create_fence(tr_renderer* p_renderer, tr_fence** pp_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_fence* p_fence = new tr_fence();
    assert(NULL != p_fence);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_fence(p_renderer, p_fence);
    else
        tr_internal_dx_create_fence(p_renderer, p_fence);

    *pp_fence = p_fence;
}

void tr_destroy_fence(tr_renderer* p_renderer, tr_fence* p_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_fence);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_fence(p_renderer, p_fence);
    else
        tr_internal_dx_destroy_fence(p_renderer, p_fence);

    delete p_fence;
}

void tr_create_semaphore(tr_renderer* p_renderer, tr_semaphore** pp_semaphore)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_semaphore* p_semaphore = new tr_semaphore();
    assert(NULL != p_semaphore);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_semaphore(p_renderer, p_semaphore);
    else
        tr_internal_dx_create_semaphore(p_renderer, p_semaphore);

    *pp_semaphore = p_semaphore;
}

void tr_destroy_semaphore(tr_renderer* p_renderer, tr_semaphore* p_semaphore)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_semaphore);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_semaphore(p_renderer, p_semaphore);
    else
        tr_internal_dx_destroy_semaphore(p_renderer, p_semaphore);

    delete p_semaphore;
}

void tr_create_descriptor_set(tr_renderer* p_renderer, uint32_t descriptor_count,
                              const tr_descriptor* p_descriptors,
                              tr_descriptor_set** pp_descriptor_set)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_descriptor_set* p_descriptor_set = new tr_descriptor_set();
    assert(NULL != p_descriptor_set);

    p_descriptor_set->descriptors = new tr_descriptor[descriptor_count]();

    p_descriptor_set->descriptor_count = descriptor_count;
    memcpy(p_descriptor_set->descriptors, p_descriptors,
           descriptor_count * sizeof(*(p_descriptor_set->descriptors)));

    for (uint32_t i = 0; i < descriptor_count; ++i)
    {
        p_descriptor_set->descriptors[i].dx_root_parameter_index = 0xFFFFFFFF;
    }

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_descriptor_set(p_renderer, p_descriptor_set);
    else
        tr_internal_dx_create_descriptor_set(p_renderer, p_descriptor_set);

    *pp_descriptor_set = p_descriptor_set;
}

void tr_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_descriptor_set);

    delete[] p_descriptor_set->descriptors;

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_descriptor_set(p_renderer, p_descriptor_set);
    else
        tr_internal_dx_destroy_descriptor_set(p_renderer, p_descriptor_set);

    delete p_descriptor_set;
}

void tr_create_cmd_pool(tr_renderer* p_renderer, tr_queue* p_queue, bool transient,
                        tr_cmd_pool** pp_cmd_pool)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_cmd_pool* p_cmd_pool = new tr_cmd_pool();
    assert(NULL != p_cmd_pool);

    p_cmd_pool->renderer = p_renderer;

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_cmd_pool(p_renderer, p_queue, transient, p_cmd_pool);
    else
        tr_internal_dx_create_cmd_pool(p_renderer, p_queue, transient, p_cmd_pool);

    *pp_cmd_pool = p_cmd_pool;
}

void tr_destroy_cmd_pool(tr_renderer* p_renderer, tr_cmd_pool* p_cmd_pool)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_cmd_pool);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_cmd_pool(p_renderer, p_cmd_pool);
    else
        tr_internal_dx_destroy_cmd_pool(p_renderer, p_cmd_pool);

    delete p_cmd_pool;
}

void tr_create_cmd(tr_cmd_pool* p_cmd_pool, bool secondary, tr_cmd** pp_cmd)
{
    assert(NULL != p_cmd_pool);

    tr_cmd* p_cmd = new tr_cmd();
    assert(NULL != p_cmd);

    p_cmd->cmd_pool = p_cmd_pool;

    if (p_cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_create_cmd(p_cmd_pool, secondary, p_cmd);
    else
        tr_internal_dx_create_cmd(p_cmd_pool, secondary, p_cmd);

    *pp_cmd = p_cmd;
}

void tr_destroy_cmd(tr_cmd_pool* p_cmd_pool, tr_cmd* p_cmd)
{
    assert(NULL != p_cmd_pool);
    assert(NULL != p_cmd);

    if (p_cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_cmd(p_cmd_pool, p_cmd);
    else
        tr_internal_dx_destroy_cmd(p_cmd_pool, p_cmd);

    delete p_cmd;
}

void tr_create_cmd_n(tr_cmd_pool* p_cmd_pool, bool secondary, uint32_t cmd_count, tr_cmd*** ppp_cmd)
{
    assert(NULL != ppp_cmd);

    tr_cmd** pp_cmd = new tr_cmd*[cmd_count]();
    assert(NULL != pp_cmd);

    for (uint32_t i = 0; i < cmd_count; ++i)
    {
        tr_create_cmd(p_cmd_pool, secondary, &(pp_cmd[i]));
    }

    *ppp_cmd = pp_cmd;
}

void tr_destroy_cmd_n(tr_cmd_pool* p_cmd_pool, uint32_t cmd_count, tr_cmd** pp_cmd)
{
    assert(NULL != pp_cmd);

    for (uint32_t i = 0; i < cmd_count; ++i)
    {
        tr_destroy_cmd(p_cmd_pool, pp_cmd[i]);
    }

    delete[] pp_cmd;
}

void tr_create_buffer(tr_renderer* p_renderer, tr_buffer_usage usage, uint64_t size,
                      bool host_visible, tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0);

    tr_buffer* p_buffer = new tr_buffer();
    assert(NULL != p_buffer);

    p_buffer->renderer = p_renderer;
    p_buffer->usage = usage;
    p_buffer->size = size;
    p_buffer->host_visible = host_visible;

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_buffer(p_renderer, p_buffer);
    else
        tr_internal_dx_create_buffer(p_renderer, p_buffer);

    *pp_buffer = p_buffer;
}

void tr_create_index_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible,
                            tr_index_type index_type, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_index, size, host_visible, pp_buffer);
    (*pp_buffer)->index_type = index_type;
    (*pp_buffer)->dx_index_buffer_view.Format =
        (tr_index_type_uint16 == index_type) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

void tr_create_uniform_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible,
                              tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_uniform_cbv, size, host_visible, pp_buffer);
}

void tr_create_vertex_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible,
                             uint32_t vertex_stride, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_vertex, size, host_visible, pp_buffer);
    (*pp_buffer)->vertex_stride = vertex_stride;
    (*pp_buffer)->dx_vertex_buffer_view.StrideInBytes = vertex_stride;
}

void tr_create_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element,
                                 uint64_t element_count, uint64_t struct_stride, bool raw,
                                 tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0);

    tr_buffer* p_buffer = new tr_buffer();
    assert(NULL != p_buffer);

    p_buffer->renderer = p_renderer;
    p_buffer->usage = tr_buffer_usage_storage_srv;
    p_buffer->size = size;
    p_buffer->host_visible = false;
    p_buffer->format = tr_format_undefined;
    p_buffer->first_element = first_element;
    p_buffer->element_count = element_count;
    p_buffer->struct_stride = raw ? 0 : struct_stride;
    p_buffer->raw = raw;

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_buffer(p_renderer, p_buffer);
    else
        tr_internal_dx_create_buffer(p_renderer, p_buffer);

    *pp_buffer = p_buffer;
}

void tr_create_rw_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element,
                                    uint64_t element_count, uint64_t struct_stride, bool raw,
                                    tr_buffer** pp_counter_buffer, tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0);

    // Create counter buffer
    if (pp_counter_buffer != NULL)
    {
        tr_buffer* p_counter_buffer = new tr_buffer();
        assert(NULL != p_counter_buffer);

        p_counter_buffer->renderer = p_renderer;
        p_counter_buffer->usage = tr_buffer_usage_counter_uav;
        p_counter_buffer->size = 4;
        p_counter_buffer->host_visible = false;
        p_counter_buffer->format = tr_format_undefined;
        p_counter_buffer->first_element = 0;
        p_counter_buffer->element_count = 1;
        p_counter_buffer->struct_stride = 4;

        if (p_renderer->api == tr_api_vulkan)
            tr_internal_vk_create_buffer(p_renderer, p_counter_buffer);
        else
            tr_internal_dx_create_buffer(p_renderer, p_counter_buffer);

        *pp_counter_buffer = p_counter_buffer;
    }

    // Create data buffer
    {
        tr_buffer* p_buffer = new tr_buffer();
        assert(NULL != p_buffer);

        p_buffer->renderer = p_renderer;
        p_buffer->usage = tr_buffer_usage_storage_uav;
        p_buffer->size = size;
        p_buffer->host_visible = false;
        p_buffer->format = tr_format_undefined;
        p_buffer->first_element = first_element;
        p_buffer->element_count = element_count;
        p_buffer->struct_stride = raw ? 0 : struct_stride;
        p_buffer->raw = raw;

        if (pp_counter_buffer != NULL)
        {
            p_buffer->counter_buffer = *pp_counter_buffer;
        }

        if (p_renderer->api == tr_api_vulkan)
            tr_internal_vk_create_buffer(p_renderer, p_buffer);
        else
            tr_internal_dx_create_buffer(p_renderer, p_buffer);

        *pp_buffer = p_buffer;
    }
}

void tr_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_buffer);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_buffer(p_renderer, p_buffer);
    else
        tr_internal_dx_destroy_buffer(p_renderer, p_buffer);

    delete p_buffer;
}

void tr_create_texture(tr_renderer* p_renderer, tr_texture_type type, uint32_t width,
                       uint32_t height, uint32_t depth, tr_sample_count sample_count,
                       tr_format format, uint32_t mip_levels, const tr_clear_value* p_clear_value,
                       bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert((width > 0) && (height > 0) && (depth > 0));

    tr_texture* p_texture = new tr_texture();
    assert(NULL != p_texture);

    p_texture->renderer = p_renderer;
    p_texture->type = type;
    p_texture->usage = usage;
    p_texture->width = width;
    p_texture->height = height;
    p_texture->depth = depth;
    p_texture->format = format;
    p_texture->mip_levels = mip_levels;
    p_texture->sample_count = sample_count;
    p_texture->host_visible = false;
    p_texture->cpu_mapped_address = NULL;
    p_texture->owns_image = false;
    if (NULL != p_clear_value)
    {
        if (tr_texture_usage_depth_stencil_attachment ==
            (usage & tr_texture_usage_depth_stencil_attachment))
        {
            p_texture->clear_value.depth = p_clear_value->depth;
            p_texture->clear_value.stencil = p_clear_value->stencil;
        }
        else
        {
            p_texture->clear_value.r = p_clear_value->r;
            p_texture->clear_value.g = p_clear_value->g;
            p_texture->clear_value.b = p_clear_value->b;
            p_texture->clear_value.a = p_clear_value->a;
        }
    }

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_texture(p_renderer, p_texture);
    else
        tr_internal_dx_create_texture(p_renderer, p_texture);

    *pp_texture = p_texture;

    if (host_visible)
    {
        tr_internal_log(tr_log_type_warn,
                        "D3D12 does not support host visible textures, memory of resulting texture "
                        "will not be mapped for CPU visibility",
                        "tr_create_texture");
    }
}

void tr_create_texture_1d(tr_renderer* p_renderer, uint32_t width, tr_sample_count sample_count,
                          tr_format format, bool host_visible, tr_texture_usage_flags usage,
                          tr_texture** pp_texture)
{
    tr_create_texture(p_renderer, tr_texture_type_1d, width, 1, 1, sample_count, format, 1, NULL,
                      host_visible, usage, pp_texture);
}

void tr_create_texture_2d(tr_renderer* p_renderer, uint32_t width, uint32_t height,
                          tr_sample_count sample_count, tr_format format, uint32_t mip_levels,
                          const tr_clear_value* p_clear_value, bool host_visible,
                          tr_texture_usage_flags usage, tr_texture** pp_texture)
{
    if (tr_max_mip_levels == mip_levels)
    {
        mip_levels = tr_util_calc_mip_levels(width, height);
    }

    tr_create_texture(p_renderer, tr_texture_type_2d, width, height, 1, sample_count, format,
                      mip_levels, p_clear_value, host_visible, usage, pp_texture);
}

void tr_create_texture_3d(tr_renderer* p_renderer, uint32_t width, uint32_t height, uint32_t depth,
                          tr_sample_count sample_count, tr_format format, bool host_visible,
                          tr_texture_usage_flags usage, tr_texture** pp_texture)
{
    tr_create_texture(p_renderer, tr_texture_type_3d, width, height, depth, sample_count, format, 1,
                      NULL, host_visible, usage, pp_texture);
}

void tr_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_texture);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_texture(p_renderer, p_texture);
    else
        tr_internal_dx_destroy_texture(p_renderer, p_texture);

    delete p_texture;
}

void tr_create_sampler(tr_renderer* p_renderer, tr_sampler** pp_sampler)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_sampler* p_sampler = new tr_sampler();
    assert(NULL != p_sampler);

    p_sampler->renderer = p_renderer;

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_sampler(p_renderer, p_sampler);
    else
        tr_internal_dx_create_sampler(p_renderer, p_sampler);

    *pp_sampler = p_sampler;
}

void tr_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_sampler);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_sampler(p_renderer, p_sampler);
    else
        tr_internal_dx_destroy_sampler(p_renderer, p_sampler);

    delete p_sampler;
}

void tr_create_shader_program_n(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code,
                                const char* vert_enpt, uint32_t hull_size, const void* hull_code,
                                const char* hull_enpt, uint32_t domn_size, const void* domn_code,
                                const char* domn_enpt, uint32_t geom_size, const void* geom_code,
                                const char* geom_enpt, uint32_t frag_size, const void* frag_code,
                                const char* frag_enpt, uint32_t comp_size, const void* comp_code,
                                const char* comp_enpt, tr_shader_program** pp_shader_program)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    if (vert_size > 0)
    {
        assert(NULL != vert_code);
    }
    if (hull_size > 0)
    {
        assert(NULL != hull_code);
    }
    if (domn_size > 0)
    {
        assert(NULL != domn_code);
    }
    if (geom_size > 0)
    {
        assert(NULL != geom_code);
    }
    if (frag_size > 0)
    {
        assert(NULL != frag_code);
    }
    if (comp_size > 0)
    {
        assert(NULL != comp_code);
    }

    tr_shader_program* p_shader_program = new tr_shader_program();
    assert(NULL != p_shader_program);

    p_shader_program->renderer = p_renderer;
    p_shader_program->shader_stages |= (vert_size > 0) ? tr_shader_stage_vert : 0;
    p_shader_program->shader_stages |= (hull_size > 0) ? tr_shader_stage_tesc : 0;
    p_shader_program->shader_stages |= (domn_size > 0) ? tr_shader_stage_tese : 0;
    p_shader_program->shader_stages |= (geom_size > 0) ? tr_shader_stage_geom : 0;
    p_shader_program->shader_stages |= (frag_size > 0) ? tr_shader_stage_frag : 0;
    p_shader_program->shader_stages |= (comp_size > 0) ? tr_shader_stage_comp : 0;

    if (p_renderer->api == tr_api_vulkan)
    {
        tr_internal_vk_create_shader_program(
            p_renderer, vert_size, vert_code, vert_enpt, hull_size, hull_code, hull_enpt, domn_size,
            domn_code, domn_enpt, geom_size, geom_code, geom_enpt, frag_size, frag_code, frag_enpt,
            comp_size, comp_code, comp_enpt, p_shader_program);

        if (vert_enpt != NULL)
        {
            p_shader_program->vert_entry_point = vert_enpt;
        }

        if (hull_enpt != NULL)
        {
            p_shader_program->tesc_entry_point = hull_enpt;
        }

        if (domn_enpt != NULL)
        {
            p_shader_program->tese_entry_point = domn_enpt;
        }

        if (geom_enpt != NULL)
        {
            p_shader_program->geom_entry_point = geom_enpt;
        }

        if (frag_enpt != NULL)
        {
            p_shader_program->frag_entry_point = frag_enpt;
        }

        if (comp_enpt != NULL)
        {
            p_shader_program->comp_entry_point = comp_enpt;
        }
    }
    else
        tr_internal_dx_create_shader_program(
            p_renderer, vert_size, vert_code, vert_enpt, hull_size, hull_code, hull_enpt, domn_size,
            domn_code, domn_enpt, geom_size, geom_code, geom_enpt, frag_size, frag_code, frag_enpt,
            comp_size, comp_code, comp_enpt, p_shader_program);

    *pp_shader_program = p_shader_program;
}

void tr_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code,
                              const char* vert_enpt, uint32_t frag_size, const void* frag_code,
                              const char* frag_enpt, tr_shader_program** pp_shader_program)
{
    tr_create_shader_program_n(p_renderer, vert_size, vert_code, vert_enpt, 0, NULL, NULL, 0, NULL,
                               NULL, 0, NULL, NULL, frag_size, frag_code, frag_enpt, 0, NULL, NULL,
                               pp_shader_program);
}

void tr_create_shader_program_compute(tr_renderer* p_renderer, uint32_t comp_size,
                                      const void* comp_code, const char* comp_enpt,
                                      tr_shader_program** pp_shader_program)
{
    tr_create_shader_program_n(p_renderer, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL,
                               NULL, 0, NULL, NULL, comp_size, comp_code, comp_enpt,
                               pp_shader_program);
}

void tr_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_shader_program(p_renderer, p_shader_program);
    else
        tr_internal_dx_destroy_shader_program(p_renderer, p_shader_program);
}

void tr_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program,
                        const tr_vertex_layout* p_vertex_layout,
                        tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target,
                        const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_shader_program);
    assert(NULL != p_render_target);
    assert(NULL != p_pipeline_settings);

    tr_pipeline* p_pipeline = new tr_pipeline();
    assert(NULL != p_pipeline);

    memcpy(&(p_pipeline->settings), p_pipeline_settings, sizeof(*p_pipeline_settings));

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_pipeline(p_renderer, p_shader_program, p_vertex_layout,
                                       p_descriptor_set, p_render_target, p_pipeline_settings,
                                       p_pipeline);
    else
        tr_internal_dx_create_pipeline(p_renderer, p_shader_program, p_vertex_layout,
                                       p_descriptor_set, p_render_target, p_pipeline_settings,
                                       p_pipeline);
    p_pipeline->type = tr_pipeline_type_graphics;

    *pp_pipeline = p_pipeline;
}

void tr_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program,
                                tr_descriptor_set* p_descriptor_set,
                                const tr_pipeline_settings* p_pipeline_settings,
                                tr_pipeline** pp_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_shader_program);
    assert(NULL != p_pipeline_settings);

    tr_pipeline* p_pipeline = new tr_pipeline();
    assert(NULL != p_pipeline);

    memcpy(&(p_pipeline->settings), p_pipeline_settings, sizeof(*p_pipeline_settings));

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_compute_pipeline(p_renderer, p_shader_program, p_descriptor_set,
                                               p_pipeline_settings, p_pipeline);
    else
        tr_internal_dx_create_compute_pipeline(p_renderer, p_shader_program, p_descriptor_set,
                                               p_pipeline_settings, p_pipeline);
    p_pipeline->type = tr_pipeline_type_compute;

    *pp_pipeline = p_pipeline;
}

void tr_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_pipeline);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_destroy_pipeline(p_renderer, p_pipeline);
    else
        tr_internal_dx_destroy_pipeline(p_renderer, p_pipeline);

    delete p_pipeline;
}

void tr_create_render_target(tr_renderer* p_renderer, uint32_t width, uint32_t height,
                             tr_sample_count sample_count, tr_format color_format,
                             uint32_t color_attachment_count,
                             const tr_clear_value* p_color_clear_values,
                             tr_format depth_stencil_format,
                             const tr_clear_value* p_depth_stencil_clear_value,
                             tr_render_target** pp_render_target)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_render_target* p_render_target = new tr_render_target();
    assert(NULL != p_render_target);

    p_render_target->renderer = p_renderer;
    p_render_target->width = width;
    p_render_target->height = height;
    p_render_target->sample_count = sample_count;
    p_render_target->color_format = color_format;
    p_render_target->color_attachment_count = color_attachment_count;
    p_render_target->depth_stencil_format = depth_stencil_format;

    // Create attachments
    {
        // Color
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i)
        {
            const tr_clear_value* clear_value =
                (NULL != p_color_clear_values) ? &p_color_clear_values[i] : NULL;

            tr_create_texture_2d(p_renderer, p_render_target->width, p_render_target->height,
                                 tr_sample_count_1, p_render_target->color_format, 1, clear_value,
                                 false,
                                 (tr_texture_usage)(tr_texture_usage_color_attachment |
                                                    tr_texture_usage_sampled_image),
                                 &(p_render_target->color_attachments[i]));

            if (p_render_target->sample_count > tr_sample_count_1)
            {
                tr_create_texture_2d(p_renderer, p_render_target->width, p_render_target->height,
                                     p_render_target->sample_count, p_render_target->color_format,
                                     1, clear_value, false,
                                     (tr_texture_usage)(tr_texture_usage_color_attachment |
                                                        tr_texture_usage_sampled_image),
                                     &(p_render_target->color_attachments_multisample[i]));
            }
        }

        // Depth/stencil
        if (tr_format_undefined != p_render_target->depth_stencil_format)
        {
            tr_create_texture_2d(p_renderer, p_render_target->width, p_render_target->height,
                                 p_render_target->sample_count, p_render_target->color_format, 1,
                                 p_depth_stencil_clear_value, false,
                                 (tr_texture_usage)(tr_texture_usage_depth_stencil_attachment |
                                                    tr_texture_usage_sampled_image),
                                 &(p_render_target->depth_stencil_attachment));
        }
    }

    // Create Vulkan specific objects for the render target
    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_create_render_target(p_renderer, false, p_render_target);
    else
        tr_internal_dx_create_render_target(p_renderer, false, p_render_target);

    *pp_render_target = p_render_target;
}

void tr_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_render_target);

    if ((s_tr_internal->renderer == p_renderer) && (NULL != p_render_target))
    {
        // Destroy color attachments
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i)
        {
            tr_destroy_texture(p_renderer, p_render_target->color_attachments[i]);
            if (NULL != p_render_target->color_attachments_multisample[i])
            {
                tr_destroy_texture(p_renderer, p_render_target->color_attachments_multisample[i]);
            }
        }

        // Destroy depth attachment
        if (NULL != p_render_target->depth_stencil_attachment_multisample)
        {
            tr_destroy_texture(p_renderer, p_render_target->depth_stencil_attachment_multisample);
        }
        if (NULL != p_render_target->depth_stencil_attachment)
        {
            tr_destroy_texture(p_renderer, p_render_target->depth_stencil_attachment);
        }

        /*
        // Destroy VkRenderPass object
        if (VK_NULL_HANDLE != p_render_target->vk_render_pass) {
        vkDestroyRenderPass(p_renderer->vk_device, p_render_target->vk_render_pass, NULL);
        }

        // Destroy VkFramebuffer object
        if (VK_NULL_HANDLE != p_render_target->vk_framebuffer) {
        vkDestroyFramebuffer(p_renderer->vk_device, p_render_target->vk_framebuffer, NULL);
        }
        */
    }

    delete p_render_target;
}

// -------------------------------------------------------------------------------------------------
// Descriptor set functions
// -------------------------------------------------------------------------------------------------
void tr_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_renderer);
    assert(NULL != p_descriptor_set);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_update_descriptor_set(p_renderer, p_descriptor_set);
    else
        tr_internal_dx_update_descriptor_set(p_renderer, p_descriptor_set);
}

// -------------------------------------------------------------------------------------------------
// Command buffer functions
// -------------------------------------------------------------------------------------------------
void tr_begin_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_begin_cmd(p_cmd);
    else
        tr_internal_dx_begin_cmd(p_cmd);
}

void tr_end_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_end_cmd(p_cmd);
    else
        tr_internal_dx_end_cmd(p_cmd);
}

void tr_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
    assert(NULL != p_cmd);
    assert(NULL != p_render_target);

    s_tr_internal->bound_render_target = p_render_target;

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_begin_render(p_cmd, p_render_target);
    else
        tr_internal_dx_cmd_begin_render(p_cmd, p_render_target);
}

void tr_cmd_end_render(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_end_render(p_cmd);
    else
        tr_internal_dx_cmd_end_render(p_cmd);

    s_tr_internal->bound_render_target = NULL;
}

void tr_cmd_set_viewport(tr_cmd* p_cmd, float x, float y, float width, float height,
                         float min_depth, float max_depth)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_set_viewport(p_cmd, x, y, width, height, min_depth, max_depth);
    else
        tr_internal_dx_cmd_set_viewport(p_cmd, x, y, width, height, min_depth, max_depth);
}

void tr_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_set_scissor(p_cmd, x, y, width, height);
    else
        tr_internal_dx_cmd_set_scissor(p_cmd, x, y, width, height);
}

void tr_cmd_set_line_width(tr_cmd* p_cmd, float line_width)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_set_line_width(p_cmd, line_width);
}

void tr_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index,
                                   const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_cmd_internal_vk_cmd_clear_color_attachment(p_cmd, attachment_index, clear_value);
    else
        tr_cmd_internal_dx_cmd_clear_color_attachment(p_cmd, attachment_index, clear_value);
}

void tr_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_cmd_internal_vk_cmd_clear_depth_stencil_attachment(p_cmd, clear_value);
    else
        tr_cmd_internal_dx_cmd_clear_depth_stencil_attachment(p_cmd, clear_value);
}

void tr_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline)
{
    assert(NULL != p_cmd);
    assert(NULL != p_pipeline);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_bind_pipeline(p_cmd, p_pipeline);
    else
        tr_internal_dx_cmd_bind_pipeline(p_cmd, p_pipeline);
}

void tr_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline,
                                 tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_cmd);
    assert(NULL != p_pipeline);
    assert(NULL != p_descriptor_set);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_bind_descriptor_sets(p_cmd, p_pipeline, p_descriptor_set);
    else
        tr_internal_dx_cmd_bind_descriptor_sets(p_cmd, p_pipeline, p_descriptor_set);
}

void tr_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer)
{
    assert(NULL != p_cmd);
    assert(NULL != p_buffer);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_bind_index_buffer(p_cmd, p_buffer);
    else
        tr_internal_dx_cmd_bind_index_buffer(p_cmd, p_buffer);
}

void tr_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers)
{
    assert(NULL != p_cmd);
    assert(0 != buffer_count);
    assert(NULL != pp_buffers);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_bind_vertex_buffers(p_cmd, buffer_count, pp_buffers);
    else
        tr_internal_dx_cmd_bind_vertex_buffers(p_cmd, buffer_count, pp_buffers);
}

void tr_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_draw(p_cmd, vertex_count, first_vertex);
    else
        tr_internal_dx_cmd_draw(p_cmd, vertex_count, first_vertex);
}

void tr_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index)
{
    assert(NULL != p_cmd);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_draw_indexed(p_cmd, index_count, first_index);
    else
        tr_internal_dx_cmd_draw_indexed(p_cmd, index_count, first_index);
}

void tr_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage,
                              tr_buffer_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_buffer);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, old_usage, new_usage);
    else
        tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, old_usage, new_usage);
}

void tr_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage,
                             tr_texture_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_texture);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
    {
        // Vulkan doesn't have an VkImageLayout corresponding to tr_texture_usage_storage, so
        // just ignore transitions into or out of tr_texture_usage_storage.
        if ((old_usage == tr_texture_usage_storage_image) ||
            (new_usage == tr_texture_usage_storage_image))
        {
            return;
        }
        tr_internal_vk_cmd_image_transition(p_cmd, p_texture, old_usage, new_usage);
    }
    else
        tr_internal_dx_cmd_image_transition(p_cmd, p_texture, old_usage, new_usage);
}

void tr_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target,
                                     tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_render_target);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
    {
        // Vulkan render passes take care of transitions, so just ignore this for now...
        // tr_internal_vk_cmd_render_target_transition(p_cmd, p_render_target, old_usage,
        // new_usage);
    }
    else
        tr_internal_dx_cmd_render_target_transition(p_cmd, p_render_target, old_usage, new_usage);
}

void tr_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target,
                                     tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_render_target);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
    {
        // Vulkan render passes take care of transitions, so just ignore this for now...
    }
    else
        tr_internal_dx_cmd_depth_stencil_transition(p_cmd, p_render_target, old_usage, new_usage);
}

void tr_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y,
                     uint32_t group_count_z)
{
    assert(NULL != p_cmd);
    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_dispatch(p_cmd, group_count_x, group_count_y, group_count_z);
    else
        tr_internal_dx_cmd_dispatch(p_cmd, group_count_x, group_count_y, group_count_z);
}

void tr_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height,
                                     uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level,
                                     tr_buffer* p_buffer, tr_texture* p_texture)
{
    assert(p_cmd != NULL);
    assert(p_buffer != NULL);
    assert(p_texture != NULL);

    if (p_cmd->cmd_pool->renderer->api == tr_api_vulkan)
        tr_internal_vk_cmd_copy_buffer_to_texture2d(p_cmd, width, height, row_pitch, buffer_offset,
                                                    mip_level, p_buffer, p_texture);
    else
        tr_internal_dx_cmd_copy_buffer_to_texture2d(p_cmd, width, height, row_pitch, buffer_offset,
                                                    mip_level, p_buffer, p_texture);
}

void tr_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore,
                           tr_fence* p_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    if (p_renderer->api == tr_api_vulkan)
        tr_internal_vk_acquire_next_image(p_renderer, p_signal_semaphore, p_fence);
    else
        tr_internal_dx_acquire_next_image(p_renderer, p_signal_semaphore, p_fence);
}

void tr_queue_submit(tr_queue* p_queue, uint32_t cmd_count, tr_cmd** pp_cmds,
                     uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores,
                     uint32_t signal_semaphore_count, tr_semaphore** pp_signal_semaphores)
{
    assert(NULL != p_queue);
    assert(cmd_count > 0);
    assert(NULL != pp_cmds);
    if (wait_semaphore_count > 0)
    {
        assert(NULL != pp_wait_semaphores);
    }
    if (signal_semaphore_count > 0)
    {
        assert(NULL != pp_signal_semaphores);
    }

    if (p_queue->renderer->api == tr_api_vulkan)
        tr_internal_vk_queue_submit(p_queue, cmd_count, pp_cmds, wait_semaphore_count,
                                    pp_wait_semaphores, signal_semaphore_count,
                                    pp_signal_semaphores);
    else
        tr_internal_dx_queue_submit(p_queue, cmd_count, pp_cmds, wait_semaphore_count,
                                    pp_wait_semaphores, signal_semaphore_count,
                                    pp_signal_semaphores);
}

void tr_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count,
                      tr_semaphore** pp_wait_semaphores)
{
    assert(NULL != p_queue);
    if (wait_semaphore_count > 0)
    {
        assert(NULL != pp_wait_semaphores);
    }

    if (p_queue->renderer->api == tr_api_vulkan)
        tr_internal_vk_queue_present(p_queue, wait_semaphore_count, pp_wait_semaphores);
    else
        tr_internal_dx_queue_present(p_queue, wait_semaphore_count, pp_wait_semaphores);
}

void tr_queue_wait_idle(tr_queue* p_queue)
{
    assert(NULL != p_queue);

    if (p_queue->renderer->api == tr_api_vulkan)
        tr_internal_vk_queue_wait_idle(p_queue);
    else
        tr_internal_dx_queue_wait_idle(p_queue);
}

// -------------------------------------------------------------------------------------------------
// Utility functions
// -------------------------------------------------------------------------------------------------
uint64_t tr_util_calc_storage_counter_offset(uint64_t buffer_size)
{
    uint64_t alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
    uint64_t result = (buffer_size + (alignment - 1)) & ~(alignment - 1);

    if (s_tr_internal->renderer->api == tr_api_vulkan)
        assert(0);

    return result;
}

void tr_util_set_storage_buffer_count(tr_queue* p_queue, uint64_t count_offset, uint32_t count,
                                      tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->dx_resource || NULL != p_buffer->vk_buffer);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, 256, true, &buffer);
    uint32_t* mapped_ptr = (uint32_t*)buffer->cpu_mapped_address;
    *(mapped_ptr) = count;

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    if (p_queue->renderer->api == tr_api_vulkan)
    {
        tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_storage_uav,
                                             tr_buffer_usage_transfer_dst);
        VkBufferCopy region = {0, 0, 4};
        vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_buffer->vk_buffer, 1, &region);

        tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst,
                                             tr_buffer_usage_storage_uav);
    }
    else
    {
        tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_storage_uav,
                                             tr_buffer_usage_transfer_dst);
        p_cmd->dx_cmd_list->CopyBufferRegion(p_buffer->dx_resource, count_offset,
                                             buffer->dx_resource, 0, 4);
        tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst,
                                             tr_buffer_usage_storage_uav);
    }
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_buffer->renderer, buffer);
}

void tr_util_clear_buffer(tr_queue* p_queue, tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->dx_resource || NULL != p_buffer->vk_buffer);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, p_buffer->size, true,
                     &buffer);
    memset(buffer->cpu_mapped_address, 0, buffer->size);

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    if (p_queue->renderer->api == tr_api_vulkan)
    {
        tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage,
                                             tr_buffer_usage_transfer_dst);
        VkBufferCopy region = {0, 0, p_buffer->size};
        vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_buffer->vk_buffer, 1, &region);
        tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst,
                                             p_buffer->usage);
    }
    else
    {
        tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage,
                                             tr_buffer_usage_transfer_dst);
        p_cmd->dx_cmd_list->CopyBufferRegion(p_buffer->dx_resource, 0, buffer->dx_resource, 0,
                                             p_buffer->size);
        tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst,
                                             p_buffer->usage);
    }
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_buffer->renderer, buffer);
}

void tr_util_update_buffer(tr_queue* p_queue, uint64_t size, const void* p_src_data,
                           tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_src_data);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->dx_resource || NULL != p_buffer->vk_buffer);
    assert(p_buffer->size >= size);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, size, true, &buffer);
    memcpy(buffer->cpu_mapped_address, p_src_data, size);

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    if (p_queue->renderer->api == tr_api_vulkan)
    {
        tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage,
                                             tr_buffer_usage_transfer_dst);
        VkBufferCopy region = {0, 0, size};
        vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_buffer->vk_buffer, 1, &region);
        tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst,
                                             p_buffer->usage);
    }
    else
    {
        tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage,
                                             tr_buffer_usage_transfer_dst);
        p_cmd->dx_cmd_list->CopyBufferRegion(p_buffer->dx_resource, 0, buffer->dx_resource, 0,
                                             size);
        tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst,
                                             p_buffer->usage);
    }
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_buffer->renderer, buffer);
}

void tr_util_update_texture_uint8(tr_queue* p_queue, uint32_t src_width, uint32_t src_height,
                                  uint32_t src_row_stride, const uint8_t* p_src_data,
                                  uint32_t src_channel_count, tr_texture* p_texture,
                                  tr_image_resize_uint8_fn resize_fn, void* p_user_data)
{
    assert(NULL != p_queue);
    assert(NULL != p_src_data);
    assert(NULL != p_texture);
    assert(NULL != p_texture->dx_resource || NULL != p_texture->vk_image);
    assert((src_width > 0) && (src_height > 0) && (src_row_stride > 0));
    assert(tr_sample_count_1 == p_texture->sample_count);

    vector<uint8_t> p_expanded_src_data;
    const uint32_t dst_channel_count = tr_util_format_channel_count(p_texture->format);
    assert(src_channel_count <= dst_channel_count);

    if (src_channel_count < dst_channel_count)
    {
        uint32_t expanded_row_stride = src_width * dst_channel_count;
        uint32_t expanded_size = expanded_row_stride * src_height;
        p_expanded_src_data.resize(expanded_size);

        const uint32_t src_pixel_stride = src_channel_count;
        const uint32_t expanded_pixel_stride = dst_channel_count;
        const uint8_t* src_row = p_src_data;
        uint8_t* expanded_row = p_expanded_src_data.data();
        for (uint32_t y = 0; y < src_height; ++y)
        {
            const uint8_t* src_pixel = src_row;
            uint8_t* expanded_pixel = expanded_row;
            for (uint32_t x = 0; x < src_width; ++x)
            {
                uint32_t c = 0;
                for (; c < src_channel_count; ++c)
                {
                    *(expanded_pixel + c) = *(src_pixel + c);
                }
                for (; c < dst_channel_count; ++c)
                {
                    *(expanded_pixel + c) = 0xFF;
                }
                src_pixel += src_pixel_stride;
                expanded_pixel += expanded_pixel_stride;
            }
            src_row += src_row_stride;
            expanded_row += expanded_row_stride;
        }
        src_row_stride = expanded_row_stride;
        src_channel_count = dst_channel_count;
        p_src_data = p_expanded_src_data.data();
    }

    tr_buffer* buffer = NULL;
    vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> subres_layouts;

    if (p_queue->renderer->api == tr_api_vulkan)
    {
        // Get memory requirements that covers all mip levels
        VkMemoryRequirements mem_reqs = {};
        vkGetImageMemoryRequirements(p_texture->renderer->vk_device, p_texture->vk_image,
                                     &mem_reqs);
        // Create temporary buffer big enough to fit all mip levels
        tr_create_buffer(p_texture->renderer, tr_buffer_usage_transfer_src, mem_reqs.size, true,
                         &buffer);
    }
    else
    {
        // Get resource layout and memory requirements for all mip levels
        D3D12_RESOURCE_DESC tex_resource_desc = {};
        tex_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        tex_resource_desc.Alignment = 0;
        tex_resource_desc.Width = (UINT)p_texture->width;
        tex_resource_desc.Height = (UINT)p_texture->height;
        tex_resource_desc.DepthOrArraySize = (UINT16)p_texture->depth;
        tex_resource_desc.MipLevels = (UINT16)p_texture->mip_levels;
        tex_resource_desc.Format = tr_util_to_dx_format(p_texture->format);
        tex_resource_desc.SampleDesc.Count = (UINT)p_texture->sample_count;
        tex_resource_desc.SampleDesc.Quality = (UINT)p_texture->sample_quality;
        tex_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        tex_resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        vector<UINT> subres_rowcounts;
        vector<UINT64> subres_row_strides;
        subres_layouts.resize(p_texture->mip_levels);
        subres_rowcounts.resize(p_texture->mip_levels);
        subres_row_strides.resize(p_texture->mip_levels);
        UINT64 buffer_size = 0;
        p_queue->renderer->dx_device->GetCopyableFootprints(
            &tex_resource_desc, 0, p_texture->mip_levels, 0, subres_layouts.data(),
            subres_rowcounts.data(), subres_row_strides.data(), &buffer_size);
        // Create temporary buffer big enough to fit all mip levels
        tr_create_buffer(p_texture->renderer, tr_buffer_usage_transfer_src, buffer_size, true,
                         &buffer);
    }
    // Use default simple resize if a resize function was not supplied
    if (NULL == resize_fn)
    {
        resize_fn = &tr_image_resize_uint8_t;
    }
    // Resize image into appropriate mip level
    uint32_t dst_width = p_texture->width;
    uint32_t dst_height = p_texture->height;
    VkDeviceSize buffer_offset = 0;
    for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level)
    {
        if (p_queue->renderer->api == tr_api_vulkan)
        {
            uint32_t dst_row_stride = src_row_stride >> mip_level;
            uint8_t* p_dst_data = (uint8_t*)buffer->cpu_mapped_address + buffer_offset;
            resize_fn(src_width, src_height, src_row_stride, p_src_data, dst_width, dst_height,
                      dst_row_stride, p_dst_data, dst_channel_count, p_user_data);
            buffer_offset += dst_row_stride * dst_height;
            //
            // If you're coming from D3D12, you might want to do something like:
            //
            //   // Get resource layout for all mip levels
            //   VkSubresourceLayout* subres_layouts =
            //   (VkSubresourceLayout*)xalloc(p_texture->mip_levels, sizeof(*subres_layouts));
            //   assert(NULL != subres_layouts); VkImageAspectFlags aspect_mask =
            //   tr_util_vk_determine_aspect_mask(tr_util_to_vk_format(p_texture->format)); for
            //   (uint32_t i = 0; i < p_texture->mip_levels; ++i) {
            //       VkImageSubresource img_sub_res = {};
            //       img_sub_res.aspectMask = aspect_mask;
            //       img_sub_res.mipLevel   = i;
            //       img_sub_res.arrayLayer = 0;
            //       vkGetImageSubresourceLayout(p_texture->renderer->vk_device,
            //       p_texture->vk_image, &img_sub_res, &(subres_layouts[i]));
            //   }
            //
            // ...unfortunately, in Vulkan this approach may be slightly
            // problematic, because you can only call vkGetImageSubresourceLayout on
            // an image that was created with VK_IMAGE_TILING_LINEAR. The validation
            // layers will issue an error if vkGetImageSubresourceLayout get called
            // on an image created with VK_IMAGE_TILING_OPTIMAL. For now, since the
            // total size of the memory is correct, we'll just calculate the row pitch
            // and offset for each mip level manually.
            //
        }
        else
        {
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT* subres_layout = &subres_layouts[mip_level];
            uint32_t dst_row_stride = subres_layout->Footprint.RowPitch;
            uint8_t* p_dst_data = (uint8_t*)buffer->cpu_mapped_address + subres_layout->Offset;
            resize_fn(src_width, src_height, src_row_stride, p_src_data, dst_width, dst_height,
                      dst_row_stride, p_dst_data, dst_channel_count, p_user_data);
        }
        dst_width >>= 1;
        dst_height >>= 1;
    }

    // Copy buffer to texture
    {
        tr_cmd_pool* p_cmd_pool = NULL;
        tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

        tr_cmd* p_cmd = NULL;
        tr_create_cmd(p_cmd_pool, false, &p_cmd);

        tr_begin_cmd(p_cmd);

        if (p_queue->renderer->api == tr_api_vulkan)
        {
            buffer_offset = 0;
            VkFormat format = tr_util_to_vk_format(p_texture->format);
            VkImageAspectFlags aspect_mask = tr_util_vk_determine_aspect_mask(format);
            const uint32_t region_count = p_texture->mip_levels;
            vector<VkBufferImageCopy> regions(region_count);

            dst_width = p_texture->width;
            dst_height = p_texture->height;
            for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level)
            {
                regions[mip_level].bufferOffset = buffer_offset;
                regions[mip_level].bufferRowLength = dst_width;
                regions[mip_level].bufferImageHeight = dst_height;
                regions[mip_level].imageSubresource.aspectMask = aspect_mask;
                regions[mip_level].imageSubresource.mipLevel = mip_level;
                regions[mip_level].imageSubresource.baseArrayLayer = 0;
                regions[mip_level].imageSubresource.layerCount = 1;
                regions[mip_level].imageOffset.x = 0;
                regions[mip_level].imageOffset.y = 0;
                regions[mip_level].imageOffset.z = 0;
                regions[mip_level].imageExtent.width = dst_width;
                regions[mip_level].imageExtent.height = dst_height;
                regions[mip_level].imageExtent.depth = 1;
                buffer_offset += (src_row_stride >> mip_level) * dst_height;
                dst_width >>= 1;
                dst_height >>= 1;
            }
            // Vulkan textures are created with VK_IMAGE_LAYOUT_UNDEFFINED
            // (tr_texture_usage_undefined)

            tr_internal_vk_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_undefined,
                                                tr_texture_usage_transfer_dst);
            vkCmdCopyBufferToImage(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_texture->vk_image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_count,
                                   regions.data());
            tr_internal_vk_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_transfer_dst,
                                                tr_texture_usage_sampled_image);
        }
        else
        {
            //
            // D3D12 textures are created with the following resources states
            // (tr_texture_usage_sampled_image):
            //     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            //     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            //
            tr_internal_dx_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_sampled_image,
                                                tr_texture_usage_transfer_dst);
            for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level)
            {
                const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = subres_layouts[mip_level];
                D3D12_TEXTURE_COPY_LOCATION src = {};
                src.pResource = buffer->dx_resource;
                src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                src.PlacedFootprint = layout;
                D3D12_TEXTURE_COPY_LOCATION dst = {};
                dst.pResource = p_texture->dx_resource;
                dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dst.SubresourceIndex = mip_level;

                p_cmd->dx_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
            }

            tr_internal_dx_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_transfer_dst,
                                                tr_texture_usage_sampled_image);
        }
        tr_end_cmd(p_cmd);

        tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
        tr_queue_wait_idle(p_queue);

        tr_destroy_cmd(p_cmd_pool, p_cmd);
        tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

        tr_destroy_buffer(p_texture->renderer, buffer);
    }
}

bool tr_vertex_layout_support_format(tr_format format)
{
    bool result = false;
    switch (format)
    {
        // 1 channel
    case tr_format_r8_unorm:
        result = true;
        break;
    case tr_format_r16_unorm:
        result = true;
        break;
    case tr_format_r16_float:
        result = true;
        break;
    case tr_format_r32_uint:
        result = true;
        break;
    case tr_format_r32_float:
        result = true;
        break;
        // 2 channel
    case tr_format_r8g8_unorm:
        result = true;
        break;
    case tr_format_r16g16_unorm:
        result = true;
        break;
    case tr_format_r16g16_float:
        result = true;
        break;
    case tr_format_r32g32_uint:
        result = true;
        break;
    case tr_format_r32g32_float:
        result = true;
        break;
        // 3 channel
    case tr_format_r8g8b8_unorm:
        result = true;
        break;
    case tr_format_r16g16b16_unorm:
        result = true;
        break;
    case tr_format_r16g16b16_float:
        result = true;
        break;
    case tr_format_r32g32b32_uint:
        result = true;
        break;
    case tr_format_r32g32b32_float:
        result = true;
        break;
        // 4 channel
    case tr_format_r8g8b8a8_unorm:
        result = true;
        break;
    case tr_format_r16g16b16a16_unorm:
        result = true;
        break;
    case tr_format_r16g16b16a16_float:
        result = true;
        break;
    case tr_format_r32g32b32a32_uint:
        result = true;
        break;
    case tr_format_r32g32b32a32_float:
        result = true;
        break;
    }
    return result;
}

uint32_t tr_vertex_layout_stride(const tr_vertex_layout* p_vertex_layout)
{
    assert(NULL != p_vertex_layout);

    uint32_t result = 0;
    for (uint32_t i = 0; i < p_vertex_layout->attrib_count; ++i)
    {
        result += tr_util_format_stride(p_vertex_layout->attribs[i].format);
    }
    return result;
}

void tr_render_target_set_color_clear_value(tr_render_target* p_render_target,
                                            uint32_t attachment_index, float r, float g, float b,
                                            float a)
{
    assert(NULL != p_render_target);
    assert(attachment_index < p_render_target->color_attachment_count);

    p_render_target->color_attachments[attachment_index]->clear_value.r = r;
    p_render_target->color_attachments[attachment_index]->clear_value.g = g;
    p_render_target->color_attachments[attachment_index]->clear_value.b = b;
    p_render_target->color_attachments[attachment_index]->clear_value.a = a;
}

void tr_render_target_set_depth_stencil_clear_value(tr_render_target* p_render_target, float depth,
                                                    uint8_t stencil)
{
    assert(NULL != p_render_target);

    p_render_target->depth_stencil_attachment->clear_value.depth = depth;
    p_render_target->depth_stencil_attachment->clear_value.stencil = stencil;
}

uint32_t tr_util_format_stride(tr_format format)
{
    uint32_t result = 0;
    switch (format)
    {
        // 1 channel
    case tr_format_r8_unorm:
        result = 1;
        break;
    case tr_format_r16_unorm:
        result = 2;
        break;
    case tr_format_r16_float:
        result = 2;
        break;
    case tr_format_r32_uint:
        result = 4;
        break;
    case tr_format_r32_float:
        result = 4;
        break;
        // 2 channel
    case tr_format_r8g8_unorm:
        result = 2;
        break;
    case tr_format_r16g16_unorm:
        result = 4;
        break;
    case tr_format_r16g16_float:
        result = 4;
        break;
    case tr_format_r32g32_uint:
        result = 8;
        break;
    case tr_format_r32g32_float:
        result = 8;
        break;
        // 3 channel
    case tr_format_r8g8b8_unorm:
        result = 3;
        break;
    case tr_format_r16g16b16_unorm:
        result = 6;
        break;
    case tr_format_r16g16b16_float:
        result = 6;
        break;
    case tr_format_r32g32b32_uint:
        result = 12;
        break;
    case tr_format_r32g32b32_float:
        result = 12;
        break;
        // 4 channel
    case tr_format_b8g8r8a8_unorm:
        result = 4;
        break;
    case tr_format_r8g8b8a8_unorm:
        result = 4;
        break;
    case tr_format_r16g16b16a16_unorm:
        result = 8;
        break;
    case tr_format_r16g16b16a16_float:
        result = 8;
        break;
    case tr_format_r32g32b32a32_uint:
        result = 16;
        break;
    case tr_format_r32g32b32a32_float:
        result = 16;
        break;
        // Depth/stencil
    case tr_format_d16_unorm:
        result = 0;
        break;
    case tr_format_x8_d24_unorm_pack32:
        result = 0;
        break;
    case tr_format_d32_float:
        result = 0;
        break;
    case tr_format_s8_uint:
        result = 0;
        break;
    case tr_format_d16_unorm_s8_uint:
        result = 0;
        break;
    case tr_format_d24_unorm_s8_uint:
        result = 0;
        break;
    case tr_format_d32_float_s8_uint:
        result = 0;
        break;
    }
    return result;
}

uint32_t tr_util_format_channel_count(tr_format format)
{
    uint32_t result = 0;
    switch (format)
    {
        // 1 channel
    case tr_format_r8_unorm:
        result = 1;
        break;
    case tr_format_r16_unorm:
        result = 1;
        break;
    case tr_format_r16_float:
        result = 1;
        break;
    case tr_format_r32_uint:
        result = 1;
        break;
    case tr_format_r32_float:
        result = 1;
        break;
        // 2 channel
    case tr_format_r8g8_unorm:
        result = 2;
        break;
    case tr_format_r16g16_unorm:
        result = 2;
        break;
    case tr_format_r16g16_float:
        result = 2;
        break;
    case tr_format_r32g32_uint:
        result = 2;
        break;
    case tr_format_r32g32_float:
        result = 2;
        break;
        // 3 channel
    case tr_format_r8g8b8_unorm:
        result = 3;
        break;
    case tr_format_r16g16b16_unorm:
        result = 3;
        break;
    case tr_format_r16g16b16_float:
        result = 3;
        break;
    case tr_format_r32g32b32_uint:
        result = 3;
        break;
    case tr_format_r32g32b32_float:
        result = 3;
        break;
        // 4 channel
    case tr_format_b8g8r8a8_unorm:
        result = 4;
        break;
    case tr_format_r8g8b8a8_unorm:
        result = 4;
        break;
    case tr_format_r16g16b16a16_unorm:
        result = 4;
        break;
    case tr_format_r16g16b16a16_float:
        result = 4;
        break;
    case tr_format_r32g32b32a32_uint:
        result = 4;
        break;
    case tr_format_r32g32b32a32_float:
        result = 4;
        break;
        // Depth/stencil
    case tr_format_d16_unorm:
        result = 0;
        break;
    case tr_format_x8_d24_unorm_pack32:
        result = 0;
        break;
    case tr_format_d32_float:
        result = 0;
        break;
    case tr_format_s8_uint:
        result = 0;
        break;
    case tr_format_d16_unorm_s8_uint:
        result = 0;
        break;
    case tr_format_d24_unorm_s8_uint:
        result = 0;
        break;
    case tr_format_d32_float_s8_uint:
        result = 0;
        break;
    }
    return result;
}

void tr_util_transition_buffer(tr_queue* p_queue, tr_buffer* p_buffer, tr_buffer_usage old_usage,
                               tr_buffer_usage new_usage)
{
    assert(NULL != p_queue);
    assert(NULL != p_buffer);

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_cmd_buffer_transition(p_cmd, p_buffer, old_usage, new_usage);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);
}

void tr_util_transition_image(tr_queue* p_queue, tr_texture* p_texture, tr_texture_usage old_usage,
                              tr_texture_usage new_usage)
{
    assert(NULL != p_queue);
    assert(NULL != p_texture);

    //
    // D3D12 textures are created with the following resources states
    // (tr_texture_usage_sampled_image):
    //     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
    //     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    //
    // So if the old_usage is tr_texture_usage_undefined and new_usage is
    // tr_texture_usage_sampled_image just skip it!
    //
    if ((old_usage == tr_texture_usage_undefined) && (new_usage == tr_texture_usage_sampled_image))
    {
        return;
    }

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_cmd_image_transition(p_cmd, p_texture, old_usage, new_usage);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);
}

void tr_util_update_texture_float(tr_queue* p_queue, uint32_t src_width, uint32_t src_height,
                                  uint32_t src_row_stride, const float* p_src_data,
                                  uint32_t channels, tr_texture* p_texture,
                                  tr_image_resize_float_fn resize_fn, void* p_user_data)
{
    assert(0);
}

uint32_t tr_util_calc_mip_levels(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return 0;

    uint32_t result = 1;
    while (width > 1 || height > 1)
    {
        width >>= 1;
        height >>= 1;
        result++;
    }
    return result;
}

void tr_create_renderer(const char* app_name, const tr_renderer_settings* settings,
                        tr_renderer** pp_renderer)
{
    if (NULL == s_tr_internal)
    {
        s_tr_internal = new tr_internal_data();
        assert(NULL != s_tr_internal);

        s_tr_internal->renderer = new tr_renderer();
        assert(NULL != s_tr_internal->renderer);

        // Shorter way to get to the object
        tr_renderer* p_renderer = s_tr_internal->renderer;
        p_renderer->api = settings->api;

        // Copy settings
        memcpy(&(p_renderer->settings), settings, sizeof(*settings));

        // Allocate storage for queues
        p_renderer->graphics_queue = new tr_queue();
        assert(NULL != p_renderer->graphics_queue);

        if (settings->api == tr_api_vulkan)
        {
            p_renderer->present_queue = new tr_queue();
            assert(NULL != p_renderer->present_queue);
        }
        else
        {
            // Writes to swapchain back buffers need to happen on the same queue as the one that
            // a swapchain uses to present. So just point the present queue at the graphics queue.
            p_renderer->present_queue = p_renderer->graphics_queue;
        }

        p_renderer->graphics_queue->renderer = p_renderer;
        p_renderer->present_queue->renderer = p_renderer;

        if (settings->api == tr_api_vulkan)
        {
            // Initialize the Vulkan bits
            tr_internal_vk_create_instance(app_name, p_renderer);
            tr_internal_vk_create_surface(p_renderer);
            tr_internal_vk_create_device(p_renderer);
            tr_internal_vk_create_swapchain(p_renderer);
        }
        else
        {
            // Initialize the D3D12 bits
            tr_internal_dx_create_device(p_renderer);
            tr_internal_dx_create_swapchain(p_renderer);
        }

        // Allocate and configure render target objects
        tr_internal_create_swapchain_renderpass(p_renderer);

        if (settings->api == tr_api_vulkan)
        {
            // Initialize the Vulkan bits of the render targets
            tr_internal_vk_create_swapchain_renderpass(p_renderer);
        }
        else
        {
            // Initialize the D3D12 bits of the render targets
            tr_internal_dx_create_swapchain_renderpass(p_renderer);
        }

        // Allocate storage for image acquired fences
        p_renderer->image_acquired_fences.resize(p_renderer->settings.swapchain.image_count);

        // Allocate storage for image acquire semaphores
        p_renderer->image_acquired_semaphores.resize(p_renderer->settings.swapchain.image_count);

        // Allocate storage for render complete semaphores
        p_renderer->render_complete_semaphores.resize(p_renderer->settings.swapchain.image_count);

        // Initialize fences and semaphores
        for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
        {
            tr_create_fence(p_renderer, &(p_renderer->image_acquired_fences[i]));
            tr_create_semaphore(p_renderer, &(p_renderer->image_acquired_semaphores[i]));
            tr_create_semaphore(p_renderer, &(p_renderer->render_complete_semaphores[i]));
        }

        // Renderer is good! Assign it to result!
        *(pp_renderer) = p_renderer;
    }
}

tr_internal_data* s_tr_internal = NULL;

// Proxy log callback
void tr_internal_log(tr_log_type type, const char* msg, const char* component)
{
    if (s_tr_internal->renderer->settings.log_fn)
    {
        s_tr_internal->renderer->settings.log_fn(type, msg, component);
    }
}

bool tr_image_resize_uint8_t(uint32_t src_width, uint32_t src_height, uint32_t src_row_stride,
                             const uint8_t* src_data, uint32_t dst_width, uint32_t dst_height,
                             uint32_t dst_row_stride, uint8_t* dst_data, uint32_t channel_cout,
                             void* user_data)
{
    float dx = (float)src_width / (float)dst_width;
    float dy = (float)src_height / (float)dst_height;

    const uint8_t src_pixel_stride = channel_cout;
    const uint8_t dst_pixel_stride = channel_cout;

    uint8_t* dst_row = dst_data;
    for (uint32_t y = 0; y < dst_height; ++y)
    {
        float src_x = 0;
        float src_y = (float)y * dy;
        uint8_t* dst_pixel = dst_row;
        for (uint32_t x = 0; x < dst_width; ++x)
        {
            const uint32_t src_offset =
                ((uint32_t)src_y * src_row_stride) + ((uint32_t)src_x * src_pixel_stride);
            const uint8_t* src_pixel = src_data + src_offset;
            for (uint32_t c = 0; c < channel_cout; ++c)
            {
                *(dst_pixel + c) = *(src_pixel + c);
            }
            src_x += dx;
            dst_pixel += dst_pixel_stride;
        }
        dst_row += dst_row_stride;
    }

    return true;
}

void tr_internal_create_swapchain_renderpass(tr_renderer* p_renderer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    p_renderer->swapchain_render_targets.resize(p_renderer->settings.swapchain.image_count);

    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        p_renderer->swapchain_render_targets[i] = new tr_render_target();
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        render_target->renderer = p_renderer;
        render_target->width = p_renderer->settings.width;
        render_target->height = p_renderer->settings.height;
        render_target->sample_count = (tr_sample_count)p_renderer->settings.swapchain.sample_count;
        render_target->color_format = p_renderer->settings.swapchain.color_format;
        render_target->color_attachment_count = 1;
        render_target->depth_stencil_format = p_renderer->settings.swapchain.depth_stencil_format;

        render_target->color_attachments[0] = new tr_texture();
        assert(NULL != render_target->color_attachments[0]);

        if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1)
        {
            render_target->color_attachments_multisample[0] = new tr_texture();
            assert(NULL != render_target->color_attachments_multisample[0]);
        }

        if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format)
        {
            render_target->depth_stencil_attachment = new tr_texture();
            assert(NULL != render_target->depth_stencil_attachment);

            if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1)
            {
                render_target->depth_stencil_attachment_multisample = new tr_texture();
                assert(NULL != render_target->depth_stencil_attachment_multisample);
            }
        }
    }

    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        render_target->color_attachments[0]->type = tr_texture_type_2d;
        render_target->color_attachments[0]->usage =
            (tr_texture_usage)(tr_texture_usage_color_attachment | tr_texture_usage_present);
        render_target->color_attachments[0]->width = p_renderer->settings.width;
        render_target->color_attachments[0]->height = p_renderer->settings.height;
        render_target->color_attachments[0]->depth = 1;
        render_target->color_attachments[0]->format = p_renderer->settings.swapchain.color_format;
        render_target->color_attachments[0]->mip_levels = 1;
        render_target->color_attachments[0]->clear_value.r =
            p_renderer->settings.swapchain.color_clear_value.r;
        render_target->color_attachments[0]->clear_value.g =
            p_renderer->settings.swapchain.color_clear_value.g;
        render_target->color_attachments[0]->clear_value.b =
            p_renderer->settings.swapchain.color_clear_value.b;
        render_target->color_attachments[0]->clear_value.a =
            p_renderer->settings.swapchain.color_clear_value.a;
        render_target->color_attachments[0]->sample_count = tr_sample_count_1;

        if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1)
        {
            render_target->color_attachments_multisample[0]->type = tr_texture_type_2d;
            render_target->color_attachments_multisample[0]->usage =
                tr_texture_usage_color_attachment;
            if (p_renderer->api == tr_api_vulkan)
                render_target->color_attachments_multisample[0]->usage |=
                    tr_texture_usage_sampled_image;
            render_target->color_attachments_multisample[0]->width = p_renderer->settings.width;
            render_target->color_attachments_multisample[0]->height = p_renderer->settings.height;
            render_target->color_attachments_multisample[0]->depth = 1;
            render_target->color_attachments_multisample[0]->format =
                p_renderer->settings.swapchain.color_format;
            render_target->color_attachments_multisample[0]->mip_levels = 1;
            render_target->color_attachments_multisample[0]->clear_value.r =
                p_renderer->settings.swapchain.color_clear_value.r;
            render_target->color_attachments_multisample[0]->clear_value.g =
                p_renderer->settings.swapchain.color_clear_value.g;
            render_target->color_attachments_multisample[0]->clear_value.b =
                p_renderer->settings.swapchain.color_clear_value.b;
            render_target->color_attachments_multisample[0]->clear_value.a =
                p_renderer->settings.swapchain.color_clear_value.a;
            render_target->color_attachments_multisample[0]->sample_count =
                render_target->sample_count;
        }

        if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format)
        {
            render_target->depth_stencil_attachment->type = tr_texture_type_2d;
            render_target->depth_stencil_attachment->usage =
                tr_texture_usage_depth_stencil_attachment;
            if (p_renderer->api == tr_api_vulkan)
                render_target->depth_stencil_attachment->usage |= tr_texture_usage_sampled_image;
            render_target->depth_stencil_attachment->width = p_renderer->settings.width;
            render_target->depth_stencil_attachment->height = p_renderer->settings.height;
            render_target->depth_stencil_attachment->depth = 1;
            render_target->depth_stencil_attachment->format =
                p_renderer->settings.swapchain.depth_stencil_format;
            render_target->depth_stencil_attachment->mip_levels = 1;
            render_target->depth_stencil_attachment->clear_value.depth =
                p_renderer->settings.swapchain.depth_stencil_clear_value.depth;
            render_target->depth_stencil_attachment->clear_value.stencil =
                p_renderer->settings.swapchain.depth_stencil_clear_value.stencil;
            render_target->depth_stencil_attachment->sample_count = tr_sample_count_1;

            if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1)
            {
                render_target->depth_stencil_attachment_multisample->type = tr_texture_type_2d;
                render_target->depth_stencil_attachment_multisample->usage =
                    tr_texture_usage_depth_stencil_attachment;
                if (p_renderer->api == tr_api_vulkan)
                    render_target->depth_stencil_attachment_multisample->usage |=
                        tr_texture_usage_sampled_image;
                render_target->depth_stencil_attachment_multisample->width =
                    p_renderer->settings.width;
                render_target->depth_stencil_attachment_multisample->height =
                    p_renderer->settings.height;
                render_target->depth_stencil_attachment_multisample->depth = 1;
                render_target->depth_stencil_attachment_multisample->format =
                    p_renderer->settings.swapchain.depth_stencil_format;
                render_target->depth_stencil_attachment_multisample->mip_levels = 1;
                render_target->depth_stencil_attachment_multisample->clear_value.depth =
                    p_renderer->settings.swapchain.depth_stencil_clear_value.depth;
                render_target->depth_stencil_attachment_multisample->clear_value.stencil =
                    p_renderer->settings.swapchain.depth_stencil_clear_value.stencil;
                render_target->depth_stencil_attachment_multisample->sample_count =
                    render_target->sample_count;
            }
        }
    }
}
