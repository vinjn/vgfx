#pragma once

typedef void(*pfn_ptr_vector_destroy_elem)(void*);

typedef struct ptr_vector {
    size_t                      _size;
    size_t                      _capacity;
    void**                      _data;
    pfn_ptr_vector_destroy_elem _destroy_elem_fn;
} ptr_vector;

bool ptr_vector_create(ptr_vector** pp_vector, pfn_ptr_vector_destroy_elem pfn_destroy_elem);
bool ptr_vector_resize(ptr_vector* p_vector, size_t n);

bool ptr_vector_push_back(ptr_vector* p_vector, void* p);

bool ptr_vector_erase(ptr_vector* p_vector, void* p);

bool ptr_vector_remove(ptr_vector* p_vector, void* p);

bool ptr_vector_destroy(ptr_vector* p_vector);
