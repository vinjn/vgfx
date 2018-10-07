#include "ptr_vector.h"
#include <memory.h>
#include <stdlib.h>

bool ptr_vector_create(ptr_vector** pp_vector, pfn_ptr_vector_destroy_elem pfn_destroy_elem)
{
    if (NULL == pfn_destroy_elem) {
        return false;
    }

    ptr_vector* p_vector = (ptr_vector*)calloc(1, sizeof(*p_vector));
    if (NULL == p_vector) {
        return false;
    }

    p_vector->_size = 0;
    p_vector->_capacity = 0;
    p_vector->_destroy_elem_fn = pfn_destroy_elem;
    *pp_vector = p_vector;

    return true;
}

bool ptr_vector_resize(ptr_vector* p_vector, size_t n)
{
    if (NULL == p_vector) {
        return false;
    }

    size_t new_capacity = ((7 * n) / 4) + (n > 0 ? 1 : 0);
    void** new_data = NULL;
    if ((new_capacity != p_vector->_capacity) && (new_capacity > 0)) {
        new_data = (void**)calloc(new_capacity, sizeof(*new_data));
        if (NULL == new_data) {
            return false;
        }
    }

    if (NULL != new_data) {
        if (p_vector->_size > 0) {
            if (NULL == p_vector->_data) {
                return false;
            }

            void* ret = memcpy(new_data,
                p_vector->_data, p_vector->_size * sizeof(*(p_vector->_data)));
            if (ret != new_data) {
                return false;
            }
        }

        if (n < p_vector->_size) {
            pfn_ptr_vector_destroy_elem destroy_elem_fn = p_vector->_destroy_elem_fn;
            for (size_t i = n; i < p_vector->_size; ++i) {
                if (NULL != destroy_elem_fn) {
                    destroy_elem_fn(p_vector->_data[i]);
                }
                p_vector->_data[i] = NULL;
            }
        }

        if (NULL != p_vector->_data) {
            free(p_vector->_data);
        }

        p_vector->_size = n;
        p_vector->_capacity = new_capacity;
        p_vector->_data = new_data;
    }

    return true;
}

bool ptr_vector_push_back(ptr_vector* p_vector, void* p)
{
    if ((NULL == p_vector) || (NULL == p)) {
        return false;
    }

    size_t new_size = p_vector->_size + 1;
    if (new_size >= p_vector->_capacity) {
        bool ret = ptr_vector_resize(p_vector, new_size);
        if (!ret) {
            return false;
        }
    }

    p_vector->_size = new_size;
    p_vector->_data[new_size - 1] = p;

    return true;
}

bool ptr_vector_erase(ptr_vector* p_vector, void* p)
{
    if ((NULL == p_vector) || (NULL == p)) {
        return false;
    }

    size_t n = (size_t)-1;
    for (size_t i = 0; i < p_vector->_size; ++i) {
        if (p == p_vector->_data[i]) {
            n = i;
            break;
        }
    }

    if ((((size_t)-1) != n) && (NULL != p_vector->_data)) {
        if (n > 0) {
            for (size_t i = 0; i < (n - 1); ++i) {
                p_vector->_data[i] = p_vector->_data[i + 1];
            }
        }

        if (n < (p_vector->_size - 1)) {
            for (size_t i = n; i < p_vector->_size; ++i) {
                p_vector->_data[i] = p_vector->_data[i + 1];
            }
        }

        for (size_t i = (p_vector->_size - 1); i < p_vector->_capacity; ++i) {
            p_vector->_data[i] = NULL;
        }

        pfn_ptr_vector_destroy_elem destroy_elem_fn = p_vector->_destroy_elem_fn;
        if (NULL != destroy_elem_fn) {
            destroy_elem_fn(p);
        }

        p_vector->_size -= 1;

        if ((p_vector->_size < (p_vector->_capacity)) && (p_vector->_capacity > 10)) {
            bool ret = ptr_vector_resize(p_vector, p_vector->_size);
            if (!ret) {
                return false;
            }
        }
    }

    return true;
}

bool ptr_vector_remove(ptr_vector* p_vector, void* p)
{
    if ((NULL == p_vector) || (NULL == p)) {
        return false;
    }

    size_t n = (size_t)-1;
    for (size_t i = 0; i < p_vector->_size; ++i) {
        if (p == p_vector->_data[i]) {
            n = i;
            break;
        }
    }

    if ((((size_t)-1) != n) && (NULL != p_vector->_data)) {
        if (n > 0) {
            for (size_t i = 0; i < (n - 1); ++i) {
                p_vector->_data[i] = p_vector->_data[i + 1];
            }
        }

        if (n < (p_vector->_size - 1)) {
            for (size_t i = n; i < p_vector->_size; ++i) {
                p_vector->_data[i] = p_vector->_data[i + 1];
            }
        }

        for (size_t i = (p_vector->_size - 1); i < p_vector->_capacity; ++i) {
            p_vector->_data[i] = NULL;
        }

        p_vector->_size -= 1;

        if ((p_vector->_size < (p_vector->_capacity)) && (p_vector->_capacity > 10)) {
            bool ret = ptr_vector_resize(p_vector, p_vector->_size);
            if (!ret) {
                return false;
            }
        }
    }

    return true;
}


bool ptr_vector_destroy(ptr_vector* p_vector)
{
    if (NULL == p_vector) {
        return false;
    }

    if ((p_vector->_capacity > 0) && (NULL != p_vector->_data)) {
        if (p_vector->_size > 0) {
            pfn_ptr_vector_destroy_elem destroy_elem_fn = p_vector->_destroy_elem_fn;
            for (size_t i = 0; i < p_vector->_size; ++i) {
                if (NULL != destroy_elem_fn) {
                    destroy_elem_fn(p_vector->_data[i]);
                }
                p_vector->_data[i] = NULL;
            }
            p_vector->_size = 0;
            p_vector->_destroy_elem_fn = NULL;
        }
        p_vector->_capacity = 0;
        free(p_vector->_data);
        p_vector->_data = NULL;
    }
    free(p_vector);

    return true;
}
