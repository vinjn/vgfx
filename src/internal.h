#pragma once

#include <assert.h>
#include <stdint.h>

#define TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer)                                               \
    assert(NULL != s_tr_internal);                                                                 \
    assert(NULL != s_tr_internal->renderer);                                                       \
    assert(NULL != p_renderer);                                                                    \
    assert(s_tr_internal->renderer == p_renderer);

#define TINY_RENDERER_SAFE_FREE(p_var)                                                             \
    if (NULL != p_var)                                                                             \
    {                                                                                              \
        free((void*)p_var);                                                                        \
        p_var = NULL;                                                                              \
    }

static inline uint32_t tr_max(uint32_t a, uint32_t b) { return a > b ? a : b; }

static inline uint32_t tr_min(uint32_t a, uint32_t b) { return a < b ? a : b; }

static inline uint32_t tr_round_up(uint32_t value, uint32_t multiple)
{
    assert(multiple);
    return ((value + multiple - 1) / multiple) * multiple;
}
