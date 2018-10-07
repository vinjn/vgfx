#include "vgfx.h"
#include "ptr_vector.h"
#include "vk_internal.h"
#include "internal.h"

#pragma comment(lib, "vulkan-1.lib")

void tr_create_renderer(const char *app_name, const tr_renderer_settings* settings, tr_renderer** pp_renderer)
{
    if (NULL == s_tr_internal) {
        s_tr_internal = (tr_internal_data*)calloc(1, sizeof(*s_tr_internal));
        assert(NULL != s_tr_internal);

        s_tr_internal->renderer = (tr_renderer*)calloc(1, sizeof(*(s_tr_internal->renderer)));
        assert(NULL != s_tr_internal->renderer);

        // Shorter way to get to the object
        tr_renderer* p_renderer = s_tr_internal->renderer;

        // Copy settings
        memcpy(&(p_renderer->settings), settings, sizeof(*settings));

        // Allocate storage for queues
        p_renderer->graphics_queue = (tr_queue*)calloc(1, sizeof(*p_renderer->graphics_queue));
        assert(NULL != p_renderer->graphics_queue);
        p_renderer->present_queue = (tr_queue*)calloc(1, sizeof(*p_renderer->present_queue));
        assert(NULL != p_renderer->present_queue);

        p_renderer->graphics_queue->renderer = p_renderer;
        p_renderer->present_queue->renderer = p_renderer;

        // Initialize the Vulkan bits
        {
            tr_internal_vk_create_instance(app_name, p_renderer);
            tr_internal_vk_create_surface(p_renderer);
            tr_internal_vk_create_device(p_renderer);
            tr_internal_vk_create_swapchain(p_renderer);
        }

        // Allocate and configure render target objects
        tr_internal_create_swapchain_renderpass(p_renderer);

        // Initialize the Vulkan bits of the render targets
        tr_internal_vk_create_swapchain_renderpass(p_renderer);

        // Allocate storage for image acquired fences
        p_renderer->image_acquired_fences = (tr_fence**)calloc(p_renderer->settings.swapchain.image_count,
            sizeof(*(p_renderer->image_acquired_fences)));
        assert(NULL != p_renderer->image_acquired_fences);

        // Allocate storage for image acquire semaphores
        p_renderer->image_acquired_semaphores = (tr_semaphore**)calloc(p_renderer->settings.swapchain.image_count,
            sizeof(*(p_renderer->image_acquired_semaphores)));
        assert(NULL != p_renderer->image_acquired_semaphores);

        // Allocate storage for render complete semaphores
        p_renderer->render_complete_semaphores = (tr_semaphore**)calloc(p_renderer->settings.swapchain.image_count,
            sizeof(*(p_renderer->render_complete_semaphores)));
        assert(NULL != p_renderer->render_complete_semaphores);

        // Initialize fences and semaphores
        for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_create_fence(p_renderer, &(p_renderer->image_acquired_fences[i]));
            tr_create_semaphore(p_renderer, &(p_renderer->image_acquired_semaphores[i]));
            tr_create_semaphore(p_renderer, &(p_renderer->render_complete_semaphores[i]));
        }

        // No need to do this since, the render pass will take care of them
        //
        //// Transition the swapchain render targets to first use
        //if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
        //    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        //        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        //        // Color single-sample images are swapchain images and are not transitioned
        //
        //        // Color multi-sample
        //        tr_util_transition_image(p_renderer->graphics_queue, 
        //                                 render_target->color_attachments_multisample[0], 
        //                                 tr_texture_usage_undefined, 
        //                                 tr_texture_usage_color_attachment );
        //        // Depth/stencil
        //        if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format) {
        //            tr_util_transition_image(p_renderer->graphics_queue, 
        //                                     render_target->depth_stencil_attachment, 
        //                                     tr_texture_usage_undefined, 
        //                                     tr_texture_usage_depth_stencil_attachment );
        //        }
        //    }
        //}
        //else {
        //    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        //        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        //        // Color images are swapchain images and are not transitioned
        //
        //        // Depth/stencil
        //        if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format) {
        //            tr_util_transition_image(p_renderer->graphics_queue, 
        //                                     render_target->depth_stencil_attachment, 
        //                                     tr_texture_usage_undefined, 
        //                                     tr_texture_usage_depth_stencil_attachment );
        //        }
        //            
        //    }
        //}

        // Renderer is good! Assign it to result!
        *(pp_renderer) = p_renderer;
    }
}

void tr_destroy_renderer(tr_renderer* p_renderer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != s_tr_internal);

    // Destroy the swapchain render targets
    if (NULL != p_renderer->swapchain_render_targets) {
        for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_destroy_render_target(p_renderer, p_renderer->swapchain_render_targets[i]);
        }

    }

    // Destroy render sync objects
    if (NULL != p_renderer->image_acquired_fences) {
        for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_destroy_fence(p_renderer, p_renderer->image_acquired_fences[i]);
        }
    }
    if (NULL != p_renderer->image_acquired_semaphores) {
        for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_destroy_semaphore(p_renderer, p_renderer->image_acquired_semaphores[i]);
        }
    }
    if (NULL != p_renderer->render_complete_semaphores) {
        for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_destroy_semaphore(p_renderer, p_renderer->render_complete_semaphores[i]);
        }
    }

    // Destroy the Vulkan bits
    tr_internal_vk_destroy_swapchain(p_renderer);
    tr_internal_vk_destroy_surface(p_renderer);
    tr_internal_vk_destroy_device(p_renderer);
    tr_internal_vk_destroy_instance(p_renderer);

    // Free all the renderer components!
    TINY_RENDERER_SAFE_FREE(p_renderer->swapchain_render_targets);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->image_acquired_fences);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->image_acquired_semaphores);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->render_complete_semaphores);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->present_queue);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->graphics_queue);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer);
    TINY_RENDERER_SAFE_FREE(s_tr_internal);
}

void tr_create_fence(tr_renderer *p_renderer, tr_fence** pp_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_fence* p_fence = (tr_fence*)calloc(1, sizeof(*p_fence));
    assert(NULL != p_fence);

    tr_internal_vk_create_fence(p_renderer, p_fence);

    *pp_fence = p_fence;
}

void tr_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_fence);

    tr_internal_vk_destroy_fence(p_renderer, p_fence);

    TINY_RENDERER_SAFE_FREE(p_fence);
}

void tr_create_semaphore(tr_renderer *p_renderer, tr_semaphore** pp_semaphore)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_semaphore* p_semaphore = (tr_semaphore*)calloc(1, sizeof(*p_semaphore));
    assert(NULL != p_semaphore);

    tr_internal_vk_create_semaphore(p_renderer, p_semaphore);

    *pp_semaphore = p_semaphore;
}

void tr_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_semaphore);

    tr_internal_vk_destroy_semaphore(p_renderer, p_semaphore);

    TINY_RENDERER_SAFE_FREE(p_semaphore);
}

void tr_create_descriptor_set(tr_renderer* p_renderer, uint32_t descriptor_count, const tr_descriptor* p_descriptors, tr_descriptor_set** pp_descriptor_set)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_descriptor_set* p_descriptor_set = (tr_descriptor_set*)calloc(1, sizeof(*p_descriptor_set));
    assert(NULL != p_descriptor_set);

    p_descriptor_set->descriptors = (tr_descriptor*)calloc(descriptor_count, sizeof(*(p_descriptor_set->descriptors)));
    assert(NULL != p_descriptor_set->descriptors);

    p_descriptor_set->descriptor_count = descriptor_count;
    memcpy(p_descriptor_set->descriptors, p_descriptors, descriptor_count * sizeof(*(p_descriptor_set->descriptors)));

    tr_internal_vk_create_descriptor_set(p_renderer, p_descriptor_set);

    *pp_descriptor_set = p_descriptor_set;
}

void tr_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_descriptor_set);

    TINY_RENDERER_SAFE_FREE(p_descriptor_set->descriptors);

    tr_internal_vk_destroy_descriptor_set(p_renderer, p_descriptor_set);

    TINY_RENDERER_SAFE_FREE(p_descriptor_set);
}

void tr_create_cmd_pool(tr_renderer *p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool** pp_cmd_pool)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_cmd_pool* p_cmd_pool = (tr_cmd_pool*)calloc(1, sizeof(*p_cmd_pool));
    assert(NULL != p_cmd_pool);

    p_cmd_pool->renderer = p_renderer;

    tr_internal_vk_create_cmd_pool(p_renderer, p_queue, transient, p_cmd_pool);

    *pp_cmd_pool = p_cmd_pool;
}

void tr_destroy_cmd_pool(tr_renderer *p_renderer, tr_cmd_pool* p_cmd_pool)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_cmd_pool);

    tr_internal_vk_destroy_cmd_pool(p_renderer, p_cmd_pool);

    TINY_RENDERER_SAFE_FREE(p_cmd_pool);
}

void tr_create_cmd(tr_cmd_pool* p_cmd_pool, bool secondary, tr_cmd** pp_cmd)
{
    assert(NULL != p_cmd_pool);

    tr_cmd* p_cmd = (tr_cmd*)calloc(1, sizeof(*p_cmd));
    assert(NULL != p_cmd);

    p_cmd->cmd_pool = p_cmd_pool;

    tr_internal_vk_create_cmd(p_cmd_pool, secondary, p_cmd);

    *pp_cmd = p_cmd;
}

void tr_destroy_cmd(tr_cmd_pool* p_cmd_pool, tr_cmd* p_cmd)
{
    assert(NULL != p_cmd_pool);
    assert(NULL != p_cmd);

    tr_internal_vk_destroy_cmd(p_cmd_pool, p_cmd);

    TINY_RENDERER_SAFE_FREE(p_cmd);
}

void tr_create_cmd_n(tr_cmd_pool *p_cmd_pool, bool secondary, uint32_t cmd_count, tr_cmd*** ppp_cmd)
{
    assert(NULL != ppp_cmd);

    tr_cmd** pp_cmd = (tr_cmd**)calloc(cmd_count, sizeof(*pp_cmd));
    assert(NULL != pp_cmd);

    for (uint32_t i = 0; i < cmd_count; ++i) {
        tr_create_cmd(p_cmd_pool, secondary, &(pp_cmd[i]));
    }

    *ppp_cmd = pp_cmd;
}

void tr_destroy_cmd_n(tr_cmd_pool *p_cmd_pool, uint32_t cmd_count, tr_cmd** pp_cmd)
{
    assert(NULL != pp_cmd);

    for (uint32_t i = 0; i < cmd_count; ++i) {
        tr_destroy_cmd(p_cmd_pool, pp_cmd[i]);
    }


    TINY_RENDERER_SAFE_FREE(pp_cmd);
}

void tr_create_buffer(tr_renderer* p_renderer, tr_buffer_usage usage, uint64_t size, bool host_visible, tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0);

    tr_buffer* p_buffer = (tr_buffer*)calloc(1, sizeof(*p_buffer));
    assert(NULL != p_buffer);

    p_buffer->renderer = p_renderer;
    p_buffer->usage = usage;
    p_buffer->size = size;
    p_buffer->host_visible = host_visible;

    tr_internal_vk_create_buffer(p_renderer, p_buffer);

    *pp_buffer = p_buffer;
}

void tr_create_index_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_index_type index_type, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_index, size, host_visible, pp_buffer);
    (*pp_buffer)->index_type = index_type;
}

void tr_create_uniform_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_uniform_cbv, size, host_visible, pp_buffer);
}

void tr_create_vertex_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, uint32_t vertex_stride, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_vertex, size, host_visible, pp_buffer);
    (*pp_buffer)->vertex_stride = vertex_stride;
}

void tr_create_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element, uint64_t element_count, uint64_t struct_stride, bool raw, tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0);

    tr_buffer* p_buffer = (tr_buffer*)calloc(1, sizeof(*p_buffer));
    assert(NULL != p_buffer);

    p_buffer->renderer = p_renderer;
    p_buffer->usage = tr_buffer_usage_storage_srv;
    p_buffer->size = size;
    p_buffer->host_visible = false;
    p_buffer->format = tr_format_undefined;
    p_buffer->first_element = first_element;
    p_buffer->element_count = element_count;
    p_buffer->struct_stride = struct_stride;

    tr_internal_vk_create_buffer(p_renderer, p_buffer);

    *pp_buffer = p_buffer;
}

void tr_create_rw_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element, uint64_t element_count, uint64_t struct_stride, bool raw, tr_buffer** pp_counter_buffer, tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0);

    // Create counter buffer
    if (pp_counter_buffer != NULL) {
        tr_buffer* p_counter_buffer = (tr_buffer*)calloc(1, sizeof(*p_counter_buffer));
        assert(NULL != p_counter_buffer);

        p_counter_buffer->renderer = p_renderer;
        p_counter_buffer->usage = tr_buffer_usage_storage_uav;
        p_counter_buffer->size = 4;
        p_counter_buffer->host_visible = false;
        p_counter_buffer->format = tr_format_undefined;
        p_counter_buffer->first_element = 0;
        p_counter_buffer->element_count = 1;
        p_counter_buffer->struct_stride = 4;

        tr_internal_vk_create_buffer(p_renderer, p_counter_buffer);

        *pp_counter_buffer = p_counter_buffer;
    }

    // Create data buffer
    {
        tr_buffer* p_buffer = (tr_buffer*)calloc(1, sizeof(*p_buffer));
        assert(NULL != p_buffer);

        p_buffer->renderer = p_renderer;
        p_buffer->usage = tr_buffer_usage_storage_uav;
        p_buffer->size = size;
        p_buffer->host_visible = false;
        p_buffer->format = tr_format_undefined;
        p_buffer->first_element = first_element;
        p_buffer->element_count = element_count;
        p_buffer->struct_stride = struct_stride;

        if (pp_counter_buffer != NULL) {
            p_buffer->counter_buffer = *pp_counter_buffer;
        }

        tr_internal_vk_create_buffer(p_renderer, p_buffer);

        *pp_buffer = p_buffer;
    }
}

void tr_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_buffer);

    tr_internal_vk_destroy_buffer(p_renderer, p_buffer);

    TINY_RENDERER_SAFE_FREE(p_buffer);
}

void tr_create_texture(
    tr_renderer*             p_renderer,
    tr_texture_type          type,
    uint32_t                 width,
    uint32_t                 height,
    uint32_t                 depth,
    tr_sample_count          sample_count,
    tr_format                format,
    uint32_t                 mip_levels,
    const tr_clear_value*    p_clear_value,
    bool                     host_visible,
    tr_texture_usage_flags   usage,
    tr_texture**             pp_texture
)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert((width > 0) && (height > 0) && (depth > 0));

    tr_texture* p_texture = (tr_texture*)calloc(1, sizeof(*p_texture));
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
    p_texture->host_visible = host_visible;
    p_texture->cpu_mapped_address = NULL;
    p_texture->owns_image = false;
    if (NULL != p_clear_value) {
        if (tr_texture_usage_depth_stencil_attachment == (usage & tr_texture_usage_depth_stencil_attachment)) {
            p_texture->clear_value.depth = p_clear_value->depth;
            p_texture->clear_value.stencil = p_clear_value->stencil;
        }
        else {
            p_texture->clear_value.r = p_clear_value->r;
            p_texture->clear_value.g = p_clear_value->g;
            p_texture->clear_value.b = p_clear_value->b;
            p_texture->clear_value.a = p_clear_value->a;
        }
    }

    tr_internal_vk_create_texture(p_renderer, p_texture);

    *pp_texture = p_texture;
}

void tr_create_texture_1d(
    tr_renderer*            p_renderer,
    uint32_t                width,
    tr_sample_count         sample_count,
    tr_format               format,
    bool                    host_visible,
    tr_texture_usage_flags  usage,
    tr_texture**            pp_texture
)
{
    tr_create_texture(p_renderer, tr_texture_type_1d, width, 1, 1, sample_count, format, 1, NULL, host_visible, usage, pp_texture);
}

void tr_create_texture_2d(
    tr_renderer*              p_renderer,
    uint32_t                  width,
    uint32_t                  height,
    tr_sample_count           sample_count,
    tr_format                 format,
    uint32_t                  mip_levels,
    const tr_clear_value*     clear_value,
    bool                      host_visible,
    tr_texture_usage_flags    usage,
    tr_texture**              pp_texture
)
{
    if (tr_max_mip_levels == mip_levels) {
        mip_levels = tr_util_calc_mip_levels(width, height);
    }

    tr_create_texture(p_renderer, tr_texture_type_2d, width, height, 1, sample_count, format, mip_levels, clear_value, host_visible, usage, pp_texture);
}

void tr_create_texture_3d(
    tr_renderer*            p_renderer,
    uint32_t                width,
    uint32_t                height,
    uint32_t                depth,
    tr_sample_count         sample_count,
    tr_format               format,
    bool                    host_visible,
    tr_texture_usage_flags  usage,
    tr_texture**            pp_texture
)
{
    tr_create_texture(p_renderer, tr_texture_type_3d, width, height, depth, sample_count, format, 1, NULL, host_visible, usage, pp_texture);
}

void tr_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_texture);

    tr_internal_vk_destroy_texture(p_renderer, p_texture);

    TINY_RENDERER_SAFE_FREE(p_texture);
}

void tr_create_sampler(tr_renderer* p_renderer, tr_sampler** pp_sampler)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_sampler* p_sampler = (tr_sampler*)calloc(1, sizeof(*p_sampler));
    assert(NULL != p_sampler);

    p_sampler->renderer = p_renderer;

    tr_internal_vk_create_sampler(p_renderer, p_sampler);

    *pp_sampler = p_sampler;
}

void tr_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_sampler);

    tr_internal_vk_destroy_sampler(p_renderer, p_sampler);

    TINY_RENDERER_SAFE_FREE(p_sampler);
}

void tr_create_shader_program_n(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t tesc_size, const void* tesc_code, const char* tesc_enpt, uint32_t tese_size, const void* tese_code, const char* tese_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    if (vert_size > 0) {
        assert(NULL != vert_code);
    }
    if (tesc_size > 0) {
        assert(NULL != tesc_code);
    }
    if (tese_size > 0) {
        assert(NULL != tese_code);
    }
    if (geom_size > 0) {
        assert(NULL != geom_code);
    }
    if (frag_size > 0) {
        assert(NULL != frag_code);
    }
    if (comp_size > 0) {
        assert(NULL != comp_code);
    }

    tr_shader_program* p_shader_program = (tr_shader_program*)calloc(1, sizeof(*p_shader_program));
    assert(NULL != p_shader_program);

    p_shader_program->renderer = p_renderer;
    p_shader_program->shader_stages |= (vert_size > 0) ? tr_shader_stage_vert : 0;
    p_shader_program->shader_stages |= (tesc_size > 0) ? tr_shader_stage_tesc : 0;
    p_shader_program->shader_stages |= (tese_size > 0) ? tr_shader_stage_tese : 0;
    p_shader_program->shader_stages |= (geom_size > 0) ? tr_shader_stage_geom : 0;
    p_shader_program->shader_stages |= (frag_size > 0) ? tr_shader_stage_frag : 0;
    p_shader_program->shader_stages |= (comp_size > 0) ? tr_shader_stage_comp : 0;

    tr_internal_vk_create_shader_program(p_renderer, vert_size, vert_code, vert_enpt, tesc_size, tesc_code, tesc_enpt, tese_size, tese_code, tese_enpt, geom_size, geom_code, geom_enpt, frag_size, frag_code, frag_enpt, comp_size, comp_code, comp_enpt, p_shader_program);

    if ((vert_enpt != NULL) && (strlen(vert_enpt) > 0)) {
        p_shader_program->vert_entry_point = (const char*)calloc(strlen(vert_enpt) + 1, sizeof(char));
        assert(p_shader_program->vert_entry_point != NULL);
        strncpy((char*)p_shader_program->vert_entry_point, vert_enpt, strlen(vert_enpt));
    }

    if ((tesc_enpt != NULL) && (strlen(tesc_enpt) > 0)) {
        p_shader_program->tesc_entry_point = (const char*)calloc(strlen(tesc_enpt) + 1, sizeof(char));
        assert(p_shader_program->tesc_entry_point != NULL);
        strncpy((char*)p_shader_program->tesc_entry_point, tesc_enpt, strlen(tesc_enpt));
    }

    if ((tese_enpt != NULL) && (strlen(tese_enpt) > 0)) {
        p_shader_program->tese_entry_point = (const char*)calloc(strlen(tese_enpt) + 1, sizeof(char));
        assert(p_shader_program->tese_entry_point != NULL);
        strncpy((char*)p_shader_program->tese_entry_point, tese_enpt, strlen(tese_enpt));
    }

    if ((geom_enpt != NULL) && (strlen(geom_enpt) > 0)) {
        p_shader_program->geom_entry_point = (const char*)calloc(strlen(geom_enpt) + 1, sizeof(char));
        assert(p_shader_program->geom_entry_point != NULL);
        strncpy((char*)p_shader_program->geom_entry_point, geom_enpt, strlen(geom_enpt));
    }

    if ((frag_enpt != NULL) && (strlen(frag_enpt) > 0)) {
        p_shader_program->frag_entry_point = (const char*)calloc(strlen(frag_enpt) + 1, sizeof(char));
        assert(p_shader_program->frag_entry_point != NULL);
        strncpy((char*)p_shader_program->frag_entry_point, frag_enpt, strlen(frag_enpt));
    }

    if ((comp_enpt != NULL) && (strlen(comp_enpt) > 0)) {
        p_shader_program->comp_entry_point = (const char*)calloc(strlen(comp_enpt) + 1, sizeof(char));
        assert(p_shader_program->comp_entry_point != NULL);
        strncpy((char*)p_shader_program->comp_entry_point, comp_enpt, strlen(comp_enpt));
    }

    *pp_shader_program = p_shader_program;
}

void tr_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, tr_shader_program** pp_shader_program)
{
    tr_create_shader_program_n(p_renderer, vert_size, vert_code, vert_enpt, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, frag_size, frag_code, frag_enpt, 0, NULL, NULL, pp_shader_program);
}

void tr_create_shader_program_compute(tr_renderer* p_renderer, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program)
{
    tr_create_shader_program_n(p_renderer, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, comp_size, comp_code, comp_enpt, pp_shader_program);
}

void tr_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_internal_vk_destroy_shader_program(p_renderer, p_shader_program);
}

void tr_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_render_target);
    assert(NULL != p_pipeline_settings);

    tr_pipeline* p_pipeline = (tr_pipeline*)calloc(1, sizeof(*p_pipeline));
    assert(NULL != p_pipeline);

    memcpy(&(p_pipeline->settings), p_pipeline_settings, sizeof(*p_pipeline_settings));

    tr_internal_vk_create_pipeline(p_renderer, p_shader_program, p_vertex_layout, p_descriptor_set, p_render_target, p_pipeline_settings, p_pipeline);
    p_pipeline->type = tr_pipeline_type_graphics;

    *pp_pipeline = p_pipeline;
}

void tr_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_shader_program);
    assert(NULL != p_pipeline_settings);

    tr_pipeline* p_pipeline = (tr_pipeline*)calloc(1, sizeof(*p_pipeline));
    assert(NULL != p_pipeline);

    memcpy(&(p_pipeline->settings), p_pipeline_settings, sizeof(*p_pipeline_settings));

    tr_internal_vk_create_compute_pipeline(p_renderer, p_shader_program, p_descriptor_set, p_pipeline_settings, p_pipeline);
    p_pipeline->type = tr_pipeline_type_compute;

    *pp_pipeline = p_pipeline;
}

void tr_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_pipeline);

    tr_internal_vk_destroy_pipeline(p_renderer, p_pipeline);

    TINY_RENDERER_SAFE_FREE(p_pipeline);
}

void tr_create_render_target(
    tr_renderer*            p_renderer,
    uint32_t                width,
    uint32_t                height,
    tr_sample_count         sample_count,
    tr_format               color_format,
    uint32_t                color_attachment_count,
    const tr_clear_value*   p_color_clear_values,
    tr_format               depth_stencil_format,
    const tr_clear_value*   p_depth_stencil_clear_value,
    tr_render_target**      pp_render_target
)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_render_target* p_render_target = (tr_render_target*)calloc(1, sizeof(*p_render_target));
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
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
            const tr_clear_value* clear_value = (NULL != p_color_clear_values) ? &p_color_clear_values[i] : NULL;

            tr_create_texture_2d(p_renderer,
                p_render_target->width,
                p_render_target->height,
                tr_sample_count_1,
                p_render_target->color_format,
                1,
                clear_value,
                false,
                (tr_texture_usage)(tr_texture_usage_color_attachment | tr_texture_usage_sampled_image),
                &(p_render_target->color_attachments[i]));

            if (p_render_target->sample_count > tr_sample_count_1) {
                tr_create_texture_2d(p_renderer,
                    p_render_target->width,
                    p_render_target->height,
                    p_render_target->sample_count,
                    p_render_target->color_format,
                    1,
                    clear_value,
                    false,
                    (tr_texture_usage)(tr_texture_usage_color_attachment | tr_texture_usage_sampled_image),
                    &(p_render_target->color_attachments_multisample[i]));
            }
        }

        // Depth/stencil
        if (tr_format_undefined != p_render_target->depth_stencil_format) {
            tr_create_texture_2d(p_renderer,
                p_render_target->width,
                p_render_target->height,
                p_render_target->sample_count,
                p_render_target->color_format,
                1,
                p_depth_stencil_clear_value,
                false,
                (tr_texture_usage)(tr_texture_usage_depth_stencil_attachment | tr_texture_usage_sampled_image),
                &(p_render_target->depth_stencil_attachment));
        }
    }

    // Create Vulkan specific objects for the render target
    tr_internal_vk_create_render_target(p_renderer, false, p_render_target);

    *pp_render_target = p_render_target;
}

void tr_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_render_target);

    if ((s_tr_internal->renderer == p_renderer) && (NULL != p_render_target)) {
        // Destroy color attachments
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
            tr_destroy_texture(p_renderer, p_render_target->color_attachments[i]);
            if (NULL != p_render_target->color_attachments_multisample[i]) {
                tr_destroy_texture(p_renderer, p_render_target->color_attachments_multisample[i]);
            }
        }

        // Destroy depth attachment
        if (NULL != p_render_target->depth_stencil_attachment) {
            tr_destroy_texture(p_renderer, p_render_target->depth_stencil_attachment);
        }

        // Destroy VkRenderPass object
        if (VK_NULL_HANDLE != p_render_target->vk_render_pass) {
            vkDestroyRenderPass(p_renderer->vk_device, p_render_target->vk_render_pass, NULL);
        }

        // Destroy VkFramebuffer object
        if (VK_NULL_HANDLE != p_render_target->vk_framebuffer) {
            vkDestroyFramebuffer(p_renderer->vk_device, p_render_target->vk_framebuffer, NULL);
        }

    }

    TINY_RENDERER_SAFE_FREE(p_render_target);
}

// -------------------------------------------------------------------------------------------------
// Descriptor set functions
// -------------------------------------------------------------------------------------------------
void tr_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_renderer);
    assert(NULL != p_descriptor_set);

    tr_internal_vk_update_descriptor_set(p_renderer, p_descriptor_set);
}

// -------------------------------------------------------------------------------------------------
// Command buffer functions
// -------------------------------------------------------------------------------------------------
void tr_begin_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_vk_begin_cmd(p_cmd);
}

void tr_end_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_vk_end_cmd(p_cmd);
}

void tr_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
    assert(NULL != p_cmd);
    assert(NULL != p_render_target);

    s_tr_internal->bound_render_target = p_render_target;

    tr_internal_vk_cmd_begin_render(p_cmd, p_render_target);
}

void tr_cmd_end_render(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_end_render(p_cmd);

    s_tr_internal->bound_render_target = NULL;
}

void tr_cmd_set_viewport(tr_cmd* p_cmd, float x, float y, float width, float height, float min_depth, float max_depth)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_set_viewport(p_cmd, x, y, width, height, min_depth, max_depth);
}

void tr_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_set_scissor(p_cmd, x, y, width, height);
}

void tr_cmd_set_line_width(tr_cmd* p_cmd, float line_width)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_set_line_width(p_cmd, line_width);
}

void tr_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd);

    tr_cmd_internal_vk_cmd_clear_color_attachment(p_cmd, attachment_index, clear_value);
}

void tr_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd);

    tr_cmd_internal_vk_cmd_clear_depth_stencil_attachment(p_cmd, clear_value);
}

void tr_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline)
{
    assert(NULL != p_cmd);
    assert(NULL != p_pipeline);

    tr_internal_vk_cmd_bind_pipeline(p_cmd, p_pipeline);
}

void tr_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_cmd);
    assert(NULL != p_pipeline);
    assert(NULL != p_descriptor_set);

    tr_internal_vk_cmd_bind_descriptor_sets(p_cmd, p_pipeline, p_descriptor_set);
}

void tr_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer)
{
    assert(NULL != p_cmd);
    assert(NULL != p_buffer);

    tr_internal_vk_cmd_bind_index_buffer(p_cmd, p_buffer);
}

void tr_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers)
{
    assert(NULL != p_cmd);
    assert(0 != buffer_count);
    assert(NULL != pp_buffers);

    tr_internal_vk_cmd_bind_vertex_buffers(p_cmd, buffer_count, pp_buffers);
}

void tr_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_draw(p_cmd, vertex_count, first_vertex);
}

void tr_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_draw_indexed(p_cmd, index_count, first_index);
}

void tr_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_buffer);

    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, old_usage, new_usage);
}

void tr_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_texture);

    // Vulkan doesn't have an VkImageLayout corresponding to tr_texture_usage_storage, so
    // just ignore transitions into or out of tr_texture_usage_storage.
    if ((old_usage == tr_texture_usage_storage_image) || (new_usage == tr_texture_usage_storage_image)) {
        return;
    }

    tr_internal_vk_cmd_image_transition(p_cmd, p_texture, old_usage, new_usage);
}

void tr_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    // Vulkan render passes take care of transitions, so just ignore this for now...
}

void tr_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    // Vulkan render passes take care of transitions, so just ignore this for now...
}

void tr_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    assert(NULL != p_cmd);
    tr_internal_vk_cmd_dispatch(p_cmd, group_count_x, group_count_y, group_count_z);
}

void tr_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture)
{
    assert(p_cmd != NULL);
    assert(p_buffer != NULL);
    assert(p_texture != NULL);

    tr_internal_vk_cmd_copy_buffer_to_texture2d(p_cmd, width, height, row_pitch, buffer_offset, mip_level, p_buffer, p_texture);
}

void tr_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_internal_vk_acquire_next_image(p_renderer, p_signal_semaphore, p_fence);
}

void tr_queue_submit(
    tr_queue*      p_queue,
    uint32_t       cmd_count,
    tr_cmd**       pp_cmds,
    uint32_t       wait_semaphore_count,
    tr_semaphore** pp_wait_semaphores,
    uint32_t       signal_semaphore_count,
    tr_semaphore** pp_signal_semaphores
)
{
    assert(NULL != p_queue);
    assert(cmd_count > 0);
    assert(NULL != pp_cmds);
    if (wait_semaphore_count > 0) {
        assert(NULL != pp_wait_semaphores);
    }
    if (signal_semaphore_count > 0) {
        assert(NULL != pp_signal_semaphores);
    }

    tr_internal_vk_queue_submit(p_queue,
        cmd_count,
        pp_cmds,
        wait_semaphore_count,
        pp_wait_semaphores,
        signal_semaphore_count,
        pp_signal_semaphores);
}

void tr_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores)
{
    assert(NULL != p_queue);
    if (wait_semaphore_count > 0) {
        assert(NULL != pp_wait_semaphores);
    }

    tr_internal_vk_queue_present(p_queue, wait_semaphore_count, pp_wait_semaphores);
}

void tr_queue_wait_idle(tr_queue* p_queue)
{
    assert(NULL != p_queue);

    tr_internal_vk_queue_wait_idle(p_queue);
}

void tr_render_target_set_color_clear_value(tr_render_target* p_render_target, uint32_t attachment_index, float r, float g, float b, float a)
{
    assert(NULL != p_render_target);
    assert(attachment_index < p_render_target->color_attachment_count);

    p_render_target->color_attachments[attachment_index]->clear_value.r = r;
    p_render_target->color_attachments[attachment_index]->clear_value.g = g;
    p_render_target->color_attachments[attachment_index]->clear_value.b = b;
    p_render_target->color_attachments[attachment_index]->clear_value.a = a;
}

void tr_render_target_set_depth_stencil_clear_value(tr_render_target* p_render_target, float depth, uint8_t stencil)
{
    assert(NULL != p_render_target);

    p_render_target->depth_stencil_attachment->clear_value.depth = depth;
    p_render_target->depth_stencil_attachment->clear_value.stencil = stencil;
}

bool tr_vertex_layout_support_format(tr_format format)
{
    bool result = false;
    switch (format) {
        // 1 channel
    case tr_format_r8_unorm: result = true; break;
    case tr_format_r16_unorm: result = true; break;
    case tr_format_r16_float: result = true; break;
    case tr_format_r32_uint: result = true; break;
    case tr_format_r32_float: result = true; break;
        // 2 channel
    case tr_format_r8g8_unorm: result = true; break;
    case tr_format_r16g16_unorm: result = true; break;
    case tr_format_r16g16_float: result = true; break;
    case tr_format_r32g32_uint: result = true; break;
    case tr_format_r32g32_float: result = true; break;
        // 3 channel
    case tr_format_r8g8b8_unorm: result = true; break;
    case tr_format_r16g16b16_unorm: result = true; break;
    case tr_format_r16g16b16_float: result = true; break;
    case tr_format_r32g32b32_uint: result = true; break;
    case tr_format_r32g32b32_float: result = true; break;
        // 4 channel
    case tr_format_r8g8b8a8_unorm: result = true; break;
    case tr_format_r16g16b16a16_unorm: result = true; break;
    case tr_format_r16g16b16a16_float: result = true; break;
    case tr_format_r32g32b32a32_uint: result = true; break;
    case tr_format_r32g32b32a32_float: result = true; break;
    }
    return result;
}

uint32_t tr_vertex_layout_stride(const tr_vertex_layout* p_vertex_layout)
{
    assert(NULL != p_vertex_layout);

    uint32_t result = 0;
    for (uint32_t i = 0; i < p_vertex_layout->attrib_count; ++i) {
        result += tr_util_format_stride(p_vertex_layout->attribs[i].format);
    }
    return result;
}

// -------------------------------------------------------------------------------------------------
// Utility functions
// -------------------------------------------------------------------------------------------------

// This function always returns zero for Vulkan for now because
// of how the counter are configured.
uint64_t tr_util_calc_storage_counter_offset(uint64_t buffer_size)
{
    uint64_t result = 0;
    return result;
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

VkFormat tr_util_to_vk_format(tr_format format)
{
    VkFormat result = VK_FORMAT_UNDEFINED;
    switch (format) {
        // 1 channel
    case tr_format_r8_unorm: result = VK_FORMAT_R8_UNORM; break;
    case tr_format_r16_unorm: result = VK_FORMAT_R16_UNORM; break;
    case tr_format_r16_float: result = VK_FORMAT_R16_SFLOAT; break;
    case tr_format_r32_uint: result = VK_FORMAT_R32_UINT; break;
    case tr_format_r32_float: result = VK_FORMAT_R32_SFLOAT; break;
        // 2 channel
    case tr_format_r8g8_unorm: result = VK_FORMAT_R8G8_UNORM; break;
    case tr_format_r16g16_unorm: result = VK_FORMAT_R16G16_UNORM; break;
    case tr_format_r16g16_float: result = VK_FORMAT_R16G16_SFLOAT; break;
    case tr_format_r32g32_uint: result = VK_FORMAT_R32G32_UINT; break;
    case tr_format_r32g32_float: result = VK_FORMAT_R32G32_SFLOAT; break;
        // 3 channel
    case tr_format_r8g8b8_unorm: result = VK_FORMAT_R8G8B8_UNORM; break;
    case tr_format_r16g16b16_unorm: result = VK_FORMAT_R16G16B16_UNORM; break;
    case tr_format_r16g16b16_float: result = VK_FORMAT_R16G16B16_SFLOAT; break;
    case tr_format_r32g32b32_uint: result = VK_FORMAT_R32G32B32_UINT; break;
    case tr_format_r32g32b32_float: result = VK_FORMAT_R32G32B32_SFLOAT; break;
        // 4 channel
    case tr_format_b8g8r8a8_unorm: result = VK_FORMAT_B8G8R8A8_UNORM; break;
    case tr_format_r8g8b8a8_unorm: result = VK_FORMAT_R8G8B8A8_UNORM; break;
    case tr_format_r16g16b16a16_unorm: result = VK_FORMAT_R16G16B16A16_UNORM; break;
    case tr_format_r16g16b16a16_float: result = VK_FORMAT_R16G16B16A16_SFLOAT; break;
    case tr_format_r32g32b32a32_uint: result = VK_FORMAT_R32G32B32A32_UINT; break;
    case tr_format_r32g32b32a32_float: result = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        // Depth/stencil
    case tr_format_d16_unorm: result = VK_FORMAT_D16_UNORM; break;
    case tr_format_x8_d24_unorm_pack32: result = VK_FORMAT_X8_D24_UNORM_PACK32; break;
    case tr_format_d32_float: result = VK_FORMAT_D32_SFLOAT; break;
    case tr_format_s8_uint: result = VK_FORMAT_S8_UINT; break;
    case tr_format_d16_unorm_s8_uint: result = VK_FORMAT_D16_UNORM_S8_UINT; break;
    case tr_format_d24_unorm_s8_uint: result = VK_FORMAT_D24_UNORM_S8_UINT; break;
    case tr_format_d32_float_s8_uint: result = VK_FORMAT_D32_SFLOAT_S8_UINT; break;
    }
    return result;
}

tr_format tr_util_from_vk_format(VkFormat format)
{
    tr_format result = tr_format_undefined;
    switch (format) {
        // 1 channel
    case VK_FORMAT_R8_UNORM: result = tr_format_r8_unorm; break;
    case VK_FORMAT_R16_UNORM: result = tr_format_r16_unorm; break;
    case VK_FORMAT_R16_SFLOAT: result = tr_format_r16_float; break;
    case VK_FORMAT_R32_UINT: result = tr_format_r32_uint; break;
    case VK_FORMAT_R32_SFLOAT: result = tr_format_r32_float; break;
        // 2 channel
    case VK_FORMAT_R8G8_UNORM: result = tr_format_r8g8_unorm; break;
    case VK_FORMAT_R16G16_UNORM: result = tr_format_r16g16_unorm; break;
    case VK_FORMAT_R16G16_SFLOAT: result = tr_format_r16g16_float; break;
    case VK_FORMAT_R32G32_UINT: result = tr_format_r32g32_uint; break;
    case VK_FORMAT_R32G32_SFLOAT: result = tr_format_r32g32_float; break;
        // 3 channel
    case VK_FORMAT_R8G8B8_UNORM: result = tr_format_r8g8b8_unorm; break;
    case VK_FORMAT_R16G16B16_UNORM: result = tr_format_r16g16b16_unorm; break;
    case VK_FORMAT_R16G16B16_SFLOAT: result = tr_format_r16g16b16_float; break;
    case VK_FORMAT_R32G32B32_UINT: result = tr_format_r32g32b32_uint; break;
    case VK_FORMAT_R32G32B32_SFLOAT: result = tr_format_r32g32b32_float; break;
        // 4 channel
    case VK_FORMAT_B8G8R8A8_UNORM: result = tr_format_b8g8r8a8_unorm; break;
    case VK_FORMAT_R8G8B8A8_UNORM: result = tr_format_r8g8b8a8_unorm; break;
    case VK_FORMAT_R16G16B16A16_UNORM: result = tr_format_r16g16b16a16_unorm; break;
    case VK_FORMAT_R16G16B16A16_SFLOAT: result = tr_format_r16g16b16a16_float; break;
    case VK_FORMAT_R32G32B32A32_UINT: result = tr_format_r32g32b32a32_uint; break;
    case VK_FORMAT_R32G32B32A32_SFLOAT: result = tr_format_r32g32b32a32_float; break;
        // Depth/stencil
    case VK_FORMAT_D16_UNORM: result = tr_format_d16_unorm; break;
    case VK_FORMAT_X8_D24_UNORM_PACK32: result = tr_format_x8_d24_unorm_pack32; break;
    case VK_FORMAT_D32_SFLOAT: result = tr_format_d32_float; break;
    case VK_FORMAT_S8_UINT: result = tr_format_s8_uint; break;
    case VK_FORMAT_D16_UNORM_S8_UINT: result = tr_format_d16_unorm_s8_uint; break;
    case VK_FORMAT_D24_UNORM_S8_UINT: result = tr_format_d24_unorm_s8_uint; break;
    case VK_FORMAT_D32_SFLOAT_S8_UINT: result = tr_format_d32_float_s8_uint; break;
    }
    return result;
}

VkShaderStageFlags tr_util_to_vk_shader_stages(tr_shader_stage shader_stages)
{
    VkShaderStageFlags result = 0;
    if (tr_shader_stage_all_graphics == (shader_stages & tr_shader_stage_all_graphics)) {
        result = VK_SHADER_STAGE_ALL_GRAPHICS;
    }
    else {
        if (tr_shader_stage_vert == (shader_stages & tr_shader_stage_vert)) {
            result |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (tr_shader_stage_tesc == (shader_stages & tr_shader_stage_tesc)) {
            result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }
        if (tr_shader_stage_tese == (shader_stages & tr_shader_stage_tese)) {
            result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        if (tr_shader_stage_geom == (shader_stages & tr_shader_stage_geom)) {
            result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }
        if (tr_shader_stage_frag == (shader_stages & tr_shader_stage_frag)) {
            result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if (tr_shader_stage_comp == (shader_stages & tr_shader_stage_comp)) {
            result |= VK_SHADER_STAGE_COMPUTE_BIT;
        }
    }
    return result;
}

uint32_t tr_util_format_stride(tr_format format)
{
    uint32_t result = 0;
    switch (format) {
        // 1 channel
    case tr_format_r8_unorm: result = 1; break;
    case tr_format_r16_unorm: result = 2; break;
    case tr_format_r16_float: result = 2; break;
    case tr_format_r32_uint: result = 4; break;
    case tr_format_r32_float: result = 4; break;
        // 2 channel
    case tr_format_r8g8_unorm: result = 2; break;
    case tr_format_r16g16_unorm: result = 4; break;
    case tr_format_r16g16_float: result = 4; break;
    case tr_format_r32g32_uint: result = 8; break;
    case tr_format_r32g32_float: result = 8; break;
        // 3 channel
    case tr_format_r8g8b8_unorm: result = 3; break;
    case tr_format_r16g16b16_unorm: result = 6; break;
    case tr_format_r16g16b16_float: result = 6; break;
    case tr_format_r32g32b32_uint: result = 12; break;
    case tr_format_r32g32b32_float: result = 12; break;
        // 4 channel
    case tr_format_b8g8r8a8_unorm: result = 4; break;
    case tr_format_r8g8b8a8_unorm: result = 4; break;
    case tr_format_r16g16b16a16_unorm: result = 8; break;
    case tr_format_r16g16b16a16_float: result = 8; break;
    case tr_format_r32g32b32a32_uint: result = 16; break;
    case tr_format_r32g32b32a32_float: result = 16; break;
        // Depth/stencil
    case tr_format_d16_unorm: result = 0; break;
    case tr_format_x8_d24_unorm_pack32: result = 0; break;
    case tr_format_d32_float: result = 0; break;
    case tr_format_s8_uint: result = 0; break;
    case tr_format_d16_unorm_s8_uint: result = 0; break;
    case tr_format_d24_unorm_s8_uint: result = 0; break;
    case tr_format_d32_float_s8_uint: result = 0; break;
    }
    return result;
}

uint32_t tr_util_format_channel_count(tr_format format)
{
    uint32_t result = 0;
    switch (format) {
        // 1 channel
    case tr_format_r8_unorm: result = 1; break;
    case tr_format_r16_unorm: result = 1; break;
    case tr_format_r16_float: result = 1; break;
    case tr_format_r32_uint: result = 1; break;
    case tr_format_r32_float: result = 1; break;
        // 2 channel
    case tr_format_r8g8_unorm: result = 2; break;
    case tr_format_r16g16_unorm: result = 2; break;
    case tr_format_r16g16_float: result = 2; break;
    case tr_format_r32g32_uint: result = 2; break;
    case tr_format_r32g32_float: result = 2; break;
        // 3 channel
    case tr_format_r8g8b8_unorm: result = 3; break;
    case tr_format_r16g16b16_unorm: result = 3; break;
    case tr_format_r16g16b16_float: result = 3; break;
    case tr_format_r32g32b32_uint: result = 3; break;
    case tr_format_r32g32b32_float: result = 3; break;
        // 4 channel
    case tr_format_b8g8r8a8_unorm: result = 4; break;
    case tr_format_r8g8b8a8_unorm: result = 4; break;
    case tr_format_r16g16b16a16_unorm: result = 4; break;
    case tr_format_r16g16b16a16_float: result = 4; break;
    case tr_format_r32g32b32a32_uint: result = 4; break;
    case tr_format_r32g32b32a32_float: result = 4; break;
        // Depth/stencil
    case tr_format_d16_unorm: result = 0; break;
    case tr_format_x8_d24_unorm_pack32: result = 0; break;
    case tr_format_d32_float: result = 0; break;
    case tr_format_s8_uint: result = 0; break;
    case tr_format_d16_unorm_s8_uint: result = 0; break;
    case tr_format_d24_unorm_s8_uint: result = 0; break;
    case tr_format_d32_float_s8_uint: result = 0; break;
    }
    return result;
}

void tr_util_transition_buffer(tr_queue* p_queue, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage)
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

void tr_util_transition_image(tr_queue* p_queue, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_queue);
    assert(NULL != p_texture);

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

bool tr_image_resize_uint8_t(
    uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* src_data,
    uint32_t dst_width, uint32_t dst_height, uint32_t dst_row_stride, uint8_t* dst_data,
    uint32_t channel_cout, void* user_data
)
{
    float dx = (float)src_width / (float)dst_width;
    float dy = (float)src_height / (float)dst_height;

    const uint8_t src_pixel_stride = channel_cout;
    const uint8_t dst_pixel_stride = channel_cout;

    uint8_t* dst_row = dst_data;
    for (uint32_t y = 0; y < dst_height; ++y) {
        float src_x = 0;
        float src_y = (float)y * dy;
        uint8_t* dst_pixel = dst_row;
        for (uint32_t x = 0; x < dst_width; ++x) {
            const uint32_t src_offset = ((uint32_t)src_y * src_row_stride) + ((uint32_t)src_x * src_pixel_stride);
            const uint8_t* src_pixel = src_data + src_offset;
            for (uint32_t c = 0; c < channel_cout; ++c) {
                *(dst_pixel + c) = *(src_pixel + c);
            }
            src_x += dx;
            dst_pixel += dst_pixel_stride;
        }
        dst_row += dst_row_stride;
    }

    return true;
}

void tr_util_set_storage_buffer_count(tr_queue* p_queue, uint64_t count_offset, uint32_t count, tr_buffer* p_counter_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_counter_buffer);
    assert(NULL != p_counter_buffer->vk_buffer);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_counter_buffer->renderer, tr_buffer_usage_transfer_src, p_counter_buffer->size, true, &buffer);
    uint32_t* mapped_ptr = (uint32_t*)buffer->cpu_mapped_address;
    *(mapped_ptr) = count;

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_counter_buffer, tr_buffer_usage_storage_uav, tr_buffer_usage_transfer_dst);
    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = (VkDeviceSize)4;
    vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_counter_buffer->vk_buffer, 1, &region);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_counter_buffer, tr_buffer_usage_transfer_dst, tr_buffer_usage_storage_uav);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_counter_buffer->renderer, buffer);
}

void tr_util_clear_buffer(tr_queue* p_queue, tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->vk_buffer);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, p_buffer->size, true, &buffer);
    memset(buffer->cpu_mapped_address, 0, buffer->size);

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage, tr_buffer_usage_transfer_dst);
    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = (VkDeviceSize)p_buffer->size;
    vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_buffer->vk_buffer, 1, &region);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst, p_buffer->usage);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_buffer->renderer, buffer);
}

void tr_util_update_buffer(tr_queue* p_queue, uint64_t size, const void* p_src_data, tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_src_data);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->vk_buffer);
    assert(p_buffer->size >= size);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, size, true, &buffer);
    memcpy(buffer->cpu_mapped_address, p_src_data, size);

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage, tr_buffer_usage_transfer_dst);
    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = (VkDeviceSize)size;
    vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_buffer->vk_buffer, 1, &region);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst, p_buffer->usage);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_buffer->renderer, buffer);
}

void tr_util_update_texture_uint8(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* p_src_data, uint32_t src_channel_count, tr_texture* p_texture, tr_image_resize_uint8_fn resize_fn, void* p_user_data)
{
    assert(NULL != p_queue);
    assert(NULL != p_src_data);
    assert(NULL != p_texture);
    assert(NULL != p_texture->vk_image);
    assert((src_width > 0) && (src_height > 0) && (src_row_stride > 0));
    assert(tr_sample_count_1 == p_texture->sample_count);

    uint8_t* p_expanded_src_data = NULL;
    const uint32_t dst_channel_count = tr_util_format_channel_count(p_texture->format);
    assert(src_channel_count <= dst_channel_count);

    if (src_channel_count < dst_channel_count) {
        uint32_t expanded_row_stride = src_width * dst_channel_count;
        uint32_t expanded_size = expanded_row_stride * src_height;
        p_expanded_src_data = (uint8_t*)calloc(1, expanded_size);
        assert(NULL != p_expanded_src_data);

        const uint32_t src_pixel_stride = src_channel_count;
        const uint32_t expanded_pixel_stride = dst_channel_count;
        const uint8_t* src_row = p_src_data;
        uint8_t* expanded_row = p_expanded_src_data;
        for (uint32_t y = 0; y < src_height; ++y) {
            const uint8_t* src_pixel = src_row;
            uint8_t* expanded_pixel = expanded_row;
            for (uint32_t x = 0; x < src_width; ++x) {
                uint32_t c = 0;
                for (; c < src_channel_count; ++c) {
                    *(expanded_pixel + c) = *(src_pixel + c);
                }
                for (; c < dst_channel_count; ++c) {
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
        p_src_data = p_expanded_src_data;
    }

    // Get memory requirements that covers all mip levels
    VkMemoryRequirements mem_reqs = {};
    vkGetImageMemoryRequirements(p_texture->renderer->vk_device, p_texture->vk_image, &mem_reqs);
    // Create temporary buffer big enough to fit all mip levels
    tr_buffer* buffer = NULL;
    tr_create_buffer(p_texture->renderer, tr_buffer_usage_transfer_src, mem_reqs.size, true, &buffer);
    //
    // If you're coming from D3D12, you might want to do something like:
    //
    //   // Get resource layout for all mip levels
    //   VkSubresourceLayout* subres_layouts = (VkSubresourceLayout*)calloc(p_texture->mip_levels, sizeof(*subres_layouts));
    //   assert(NULL != subres_layouts);
    //   VkImageAspectFlags aspect_mask = tr_util_vk_determine_aspect_mask(tr_util_to_vk_format(p_texture->format));
    //   for (uint32_t i = 0; i < p_texture->mip_levels; ++i) {
    //       VkImageSubresource img_sub_res = {};
    //       img_sub_res.aspectMask = aspect_mask;
    //       img_sub_res.mipLevel   = i;
    //       img_sub_res.arrayLayer = 0;
    //       vkGetImageSubresourceLayout(p_texture->renderer->vk_device, p_texture->vk_image, &img_sub_res, &(subres_layouts[i]));
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

    // Use default simple resize if a resize function was not supplied
    if (NULL == resize_fn) {
        resize_fn = &tr_image_resize_uint8_t;
    }
    // Resize image into appropriate mip level
    uint32_t dst_width = p_texture->width;
    uint32_t dst_height = p_texture->height;
    VkDeviceSize buffer_offset = 0;
    for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level) {
        uint32_t dst_row_stride = src_row_stride >> mip_level;
        uint8_t* p_dst_data = (uint8_t*)buffer->cpu_mapped_address + buffer_offset;
        resize_fn(src_width, src_height, src_row_stride, p_src_data, dst_width, dst_height, dst_row_stride, p_dst_data, dst_channel_count, p_user_data);
        buffer_offset += dst_row_stride * dst_height;
        dst_width >>= 1;
        dst_height >>= 1;
    }

    // Copy buffer to texture
    buffer_offset = 0;
    VkFormat format = tr_util_to_vk_format(p_texture->format);
    VkImageAspectFlags aspect_mask = tr_util_vk_determine_aspect_mask(format);
    {
        const uint32_t region_count = p_texture->mip_levels;
        VkBufferImageCopy* regions = (VkBufferImageCopy*)calloc(region_count, sizeof(*regions));
        assert(NULL != regions);

        dst_width = p_texture->width;
        dst_height = p_texture->height;
        for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level) {
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

        tr_cmd_pool* p_cmd_pool = NULL;
        tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

        tr_cmd* p_cmd = NULL;
        tr_create_cmd(p_cmd_pool, false, &p_cmd);

        tr_begin_cmd(p_cmd);
        //
        // Vulkan textures are created with VK_IMAGE_LAYOUT_UNDEFFINED (tr_texture_usage_undefined)
        //
        tr_internal_vk_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_undefined, tr_texture_usage_transfer_dst);
        vkCmdCopyBufferToImage(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_texture->vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_count, regions);
        tr_internal_vk_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_transfer_dst, tr_texture_usage_sampled_image);
        tr_end_cmd(p_cmd);

        tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
        tr_queue_wait_idle(p_queue);

        tr_destroy_cmd(p_cmd_pool, p_cmd);
        tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

        tr_destroy_buffer(p_texture->renderer, buffer);

        TINY_RENDERER_SAFE_FREE(regions);
    }

    TINY_RENDERER_SAFE_FREE(p_expanded_src_data);
}

void tr_util_update_texture_float(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* p_src_data, uint32_t channels, tr_texture* p_texture, tr_image_resize_float_fn resize_fn, void* p_user_data)
{
}
