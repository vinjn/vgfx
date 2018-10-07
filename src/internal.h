#pragma once

#include "vgfx.h"

#define TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer)                                               \
    assert(NULL != s_tr_internal);                                                                 \
    assert(NULL != s_tr_internal->renderer);                                                       \
    assert(NULL != p_renderer);                                                                    \
    assert(s_tr_internal->renderer == p_renderer);

#define TINY_RENDERER_SAFE_RELEASE(p_var)                                                          \
    if (NULL != p_var)                                                                             \
    {                                                                                              \
        p_var->Release();                                                                          \
        p_var = NULL;                                                                              \
    }

static inline uint32_t tr_max(uint32_t a, uint32_t b) { return a > b ? a : b; }

static inline uint32_t tr_min(uint32_t a, uint32_t b) { return a < b ? a : b; }

static inline uint32_t tr_round_up(uint32_t value, uint32_t multiple)
{
    assert(multiple);
    return ((value + multiple - 1) / multiple) * multiple;
}

// Internal singleton
struct tr_internal_data
{
    tr_renderer* renderer;
    tr_render_target* bound_render_target;
};

extern tr_internal_data* s_tr_internal;
void tr_internal_log(tr_log_type type, const char* msg, const char* component);

bool tr_image_resize_uint8_t(uint32_t src_width, uint32_t src_height, uint32_t src_row_stride,
    const uint8_t* src_data, uint32_t dst_width, uint32_t dst_height,
    uint32_t dst_row_stride, uint8_t* dst_data, uint32_t channel_cout,
    void* user_data);