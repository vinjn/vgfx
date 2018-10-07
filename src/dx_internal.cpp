#include "dx_internal.h"
#include <d3dcompiler.h>
#include <stdio.h>
#include <combaseapi.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace std;

// Functions points for functions that need to be loaded
PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER fnD3D12CreateRootSignatureDeserializer = NULL;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE fnD3D12SerializeVersionedRootSignature = NULL;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER
fnD3D12CreateVersionedRootSignatureDeserializer = NULL;

// -------------------------------------------------------------------------------------------------
// Internal utility functions
// -------------------------------------------------------------------------------------------------
D3D12_RESOURCE_STATES tr_util_to_dx_resource_state_buffer(tr_buffer_usage usage)
{
    D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;
    if (tr_buffer_usage_transfer_src == (usage & tr_buffer_usage_transfer_src))
    {
        result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (tr_buffer_usage_transfer_dst == (usage & tr_buffer_usage_transfer_dst))
    {
        result |= D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (tr_buffer_usage_storage_srv == (usage & tr_buffer_usage_storage_srv))
    {
        result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    if (tr_buffer_usage_storage_uav == (usage & tr_buffer_usage_storage_uav))
    {
        result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    return result;
}

D3D12_RESOURCE_STATES tr_util_to_dx_resource_state_texture(tr_texture_usage_flags usage)
{
    D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;
    if (tr_texture_usage_transfer_src == (usage & tr_texture_usage_transfer_src))
    {
        result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (tr_texture_usage_transfer_dst == (usage & tr_texture_usage_transfer_dst))
    {
        result |= D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (tr_texture_usage_sampled_image == (usage & tr_texture_usage_sampled_image))
    {
        result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    if (tr_texture_usage_storage_image == (usage & tr_texture_usage_storage_image))
    {
        result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if (tr_texture_usage_color_attachment == (usage & tr_texture_usage_color_attachment))
    {
        result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (tr_texture_usage_depth_stencil_attachment ==
        (usage & tr_texture_usage_depth_stencil_attachment))
    {
        result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    if (tr_texture_usage_resolve_src == (usage & tr_texture_usage_resolve_src))
    {
        result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    }
    if (tr_texture_usage_resolve_dst == (usage & tr_texture_usage_resolve_dst))
    {
        result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
    }
    return result;
}

// -------------------------------------------------------------------------------------------------
// Internal init functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_create_device(tr_renderer* p_renderer)
{
#if defined(_DEBUG)
    if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(p_renderer->dx_debug_ctrl),
                                         (void**)&(p_renderer->dx_debug_ctrl))))
    {
        p_renderer->dx_debug_ctrl->EnableDebugLayer();
    }
#endif

    UINT flags = 0;
#if defined(_DEBUG)
    flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    HRESULT hres = CreateDXGIFactory2(flags, __uuidof(p_renderer->dx_factory),
                                      (void**)&(p_renderer->dx_factory));
    assert(SUCCEEDED(hres));

    D3D_FEATURE_LEVEL gpu_feature_levels[tr_max_gpus];
    for (uint32_t i = 0; i < tr_max_gpus; ++i)
    {
        gpu_feature_levels[i] = (D3D_FEATURE_LEVEL)0;
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != p_renderer->dx_factory->EnumAdapters1(i, &adapter);
         ++i)
    {
        DXGI_ADAPTER_DESC1 desc = {};
        adapter->GetDesc1(&desc);
        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }
        // Make sure the adapter can support a D3D12 device
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1,
                                        __uuidof(p_renderer->dx_device), NULL)))
        {
            hres = adapter->QueryInterface(
                __uuidof(IDXGIAdapter3), (void**)&(p_renderer->dx_gpus[p_renderer->dx_gpu_count]));
            if (SUCCEEDED(hres))
            {
                gpu_feature_levels[p_renderer->dx_gpu_count] = D3D_FEATURE_LEVEL_12_1;
                ++p_renderer->dx_gpu_count;
            }
        }
        else if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
                                             __uuidof(p_renderer->dx_device), NULL)))
        {
            hres = adapter->QueryInterface(
                __uuidof(IDXGIAdapter3), (void**)&(p_renderer->dx_gpus[p_renderer->dx_gpu_count]));
            if (SUCCEEDED(hres))
            {
                gpu_feature_levels[p_renderer->dx_gpu_count] = D3D_FEATURE_LEVEL_12_0;
                ++p_renderer->dx_gpu_count;
            }
        }
        else if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_1,
                                             __uuidof(p_renderer->dx_device), NULL)))
        {
            hres = adapter->QueryInterface(
                __uuidof(IDXGIAdapter3), (void**)&(p_renderer->dx_gpus[p_renderer->dx_gpu_count]));
            if (SUCCEEDED(hres))
            {
                gpu_feature_levels[p_renderer->dx_gpu_count] = D3D_FEATURE_LEVEL_11_1;
                ++p_renderer->dx_gpu_count;
            }
        }
        else if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                             __uuidof(p_renderer->dx_device), NULL)))
        {
            hres = adapter->QueryInterface(
                __uuidof(IDXGIAdapter3), (void**)&(p_renderer->dx_gpus[p_renderer->dx_gpu_count]));
            if (SUCCEEDED(hres))
            {
                gpu_feature_levels[p_renderer->dx_gpu_count] = D3D_FEATURE_LEVEL_11_0;
                ++p_renderer->dx_gpu_count;
            }
        }
    }
    assert(p_renderer->dx_gpu_count > 0);

    D3D_FEATURE_LEVEL target_feature_level = D3D_FEATURE_LEVEL_12_1;
    for (uint32_t i = 0; i < p_renderer->dx_gpu_count; ++i)
    {
        if (gpu_feature_levels[i] == D3D_FEATURE_LEVEL_12_1)
        {
            p_renderer->dx_active_gpu = p_renderer->dx_gpus[i].Get();
            break;
        }
    }

    if (p_renderer->dx_active_gpu == NULL)
    {
        for (uint32_t i = 0; i < p_renderer->dx_gpu_count; ++i)
        {
            if (gpu_feature_levels[i] == D3D_FEATURE_LEVEL_12_0)
            {
                p_renderer->dx_active_gpu = p_renderer->dx_gpus[i].Get();
                target_feature_level = D3D_FEATURE_LEVEL_12_0;
                break;
            }
        }
    }

    assert(p_renderer->dx_active_gpu != NULL);

    // Load functions
    {
        HMODULE module = ::GetModuleHandle(TEXT("d3d12.dll"));
        fnD3D12CreateRootSignatureDeserializer =
            (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(
                module, "D3D12SerializeVersionedRootSignature");

        fnD3D12SerializeVersionedRootSignature =
            (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(
                module, "D3D12SerializeVersionedRootSignature");

        fnD3D12CreateVersionedRootSignatureDeserializer =
            (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(
                module, "D3D12CreateVersionedRootSignatureDeserializer");
    }

    if ((fnD3D12CreateRootSignatureDeserializer == NULL) ||
        (fnD3D12SerializeVersionedRootSignature == NULL) ||
        (fnD3D12CreateVersionedRootSignatureDeserializer == NULL))
    {
        target_feature_level = D3D_FEATURE_LEVEL_12_0;
    }

    hres = D3D12CreateDevice(p_renderer->dx_active_gpu.Get(), target_feature_level,
                             __uuidof(p_renderer->dx_device), (void**)(&p_renderer->dx_device));
    assert(SUCCEEDED(hres));

    p_renderer->settings.dx_feature_level = target_feature_level;

    // Queues
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        hres = p_renderer->dx_device->CreateCommandQueue(
            &desc, IID_PPV_ARGS(&p_renderer->graphics_queue->dx_queue));
        assert(SUCCEEDED(hres));
    }

    // Create fence
    {
        hres = p_renderer->dx_device->CreateFence(
            0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&p_renderer->graphics_queue->dx_wait_idle_fence));
        assert(SUCCEEDED(hres));
        p_renderer->graphics_queue->dx_wait_idle_fence_value = 1;

        p_renderer->graphics_queue->dx_wait_idle_fence_event =
            CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(NULL != p_renderer->graphics_queue->dx_wait_idle_fence_event);
    }
}

void tr_internal_dx_create_swapchain(tr_renderer* p_renderer)
{
    assert(NULL != p_renderer->dx_device);

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = p_renderer->settings.width;
    desc.Height = p_renderer->settings.height;
    desc.Format = tr_util_to_dx_format(p_renderer->settings.swapchain.color_format);
    desc.Stereo = false;
    desc.SampleDesc.Count = 1; // If multisampling is needed, we'll resolve it later
    desc.SampleDesc.Quality = p_renderer->settings.swapchain.sample_quality;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = p_renderer->settings.swapchain.image_count;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = 0;

    if ((desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL) ||
        (desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD))
    {
        if (desc.BufferCount < 2)
        {
            desc.BufferCount = 2;
        }
    }

    if (desc.BufferCount > DXGI_MAX_SWAP_CHAIN_BUFFERS)
    {
        desc.BufferCount = DXGI_MAX_SWAP_CHAIN_BUFFERS;
    }

    p_renderer->settings.swapchain.image_count = desc.BufferCount;

    ComPtr<IDXGISwapChain1> swapchain;
    HRESULT hres = p_renderer->dx_factory->CreateSwapChainForHwnd(
        p_renderer->present_queue->dx_queue.Get(), p_renderer->settings.handle.hwnd, &desc, NULL,
        NULL, &swapchain);
    assert(SUCCEEDED(hres));

    hres = p_renderer->dx_factory->MakeWindowAssociation(p_renderer->settings.handle.hwnd,
                                                         DXGI_MWA_NO_ALT_ENTER);
    assert(SUCCEEDED(hres));

    hres = swapchain->QueryInterface(__uuidof(p_renderer->dx_swapchain),
                                     (void**)&(p_renderer->dx_swapchain));
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_create_swapchain_renderpass(tr_renderer* p_renderer)
{
    assert(NULL != p_renderer->dx_device);
    assert(NULL != p_renderer->dx_swapchain);

    vector<ID3D12Resource*> swapchain_images(p_renderer->settings.swapchain.image_count);
    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        HRESULT hres = p_renderer->dx_swapchain->GetBuffer(i, __uuidof(*swapchain_images.data()),
                                                           (void**)&(swapchain_images[i]));
        assert(SUCCEEDED(hres));
    }

    // Populate the vk_image field and create the Vulkan texture objects
    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        render_target->color_attachments[0]->dx_resource = swapchain_images[i];
        tr_internal_dx_create_texture(p_renderer, render_target->color_attachments[0]);

        if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1)
        {
            tr_internal_dx_create_texture(p_renderer,
                                          render_target->color_attachments_multisample[0]);
        }

        if (NULL != render_target->depth_stencil_attachment)
        {
            tr_internal_dx_create_texture(p_renderer, render_target->depth_stencil_attachment);

            if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1)
            {
                tr_internal_dx_create_texture(p_renderer,
                                              render_target->depth_stencil_attachment_multisample);
            }
        }
    }

    // Initialize Vulkan render target objects
    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i)
    {
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        tr_internal_dx_create_render_target(p_renderer, false, render_target);
    }
}

void tr_internal_dx_destroy_device(tr_renderer* p_renderer) {}

void tr_internal_dx_destroy_swapchain(tr_renderer* p_renderer) {}

// -------------------------------------------------------------------------------------------------
// Internal create functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_create_fence(tr_renderer* p_renderer, tr_fence* p_fence) {}

void tr_internal_dx_destroy_fence(tr_renderer* p_renderer, tr_fence* p_fence) {}

void tr_internal_dx_create_semaphore(tr_renderer* p_renderer, tr_semaphore* p_semaphore) {}

void tr_internal_dx_destroy_semaphore(tr_renderer* p_renderer, tr_semaphore* p_semaphore) {}

void tr_internal_dx_create_descriptor_set(tr_renderer* p_renderer,
                                          tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_renderer->dx_device);

    uint32_t cbvsrvuav_count = 0;
    uint32_t sampler_count = 0;

    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i)
    {
        uint32_t count = p_descriptor_set->descriptors[i].count;
        switch (p_descriptor_set->descriptors[i].type)
        {
        case tr_descriptor_type_sampler:
            sampler_count += count;
            break;
        case tr_descriptor_type_uniform_buffer_cbv:
            cbvsrvuav_count += count;
            break;
        case tr_descriptor_type_storage_buffer_srv:
            cbvsrvuav_count += count;
            break;
        case tr_descriptor_type_storage_buffer_uav:
            cbvsrvuav_count += count;
            break;
        case tr_descriptor_type_texture_srv:
            cbvsrvuav_count += count;
            break;
        case tr_descriptor_type_texture_uav:
            cbvsrvuav_count += count;
            break;
        case tr_descriptor_type_uniform_texel_buffer_srv:
            cbvsrvuav_count += count;
            break;
        case tr_descriptor_type_storage_texel_buffer_uav:
            cbvsrvuav_count += count;
            break;
        }
    }

    if (cbvsrvuav_count > 0)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = cbvsrvuav_count;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hres = p_renderer->dx_device->CreateDescriptorHeap(
            &desc, IID_PPV_ARGS(&p_descriptor_set->dx_cbvsrvuav_heap));
        assert(SUCCEEDED(hres));
    }

    if (sampler_count > 0)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.NumDescriptors = sampler_count;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hres = p_renderer->dx_device->CreateDescriptorHeap(
            &desc, IID_PPV_ARGS(&p_descriptor_set->dx_sampler_heap));
        assert(SUCCEEDED(hres));
    }

    // Assign heap offsets
    uint32_t cbvsrvuav_heap_offset = 0;
    uint32_t sampler_heap_offset = 0;
    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i)
    {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        switch (p_descriptor_set->descriptors[i].type)
        {
        case tr_descriptor_type_sampler:
        {
            descriptor->dx_heap_offset = sampler_heap_offset;
            sampler_heap_offset += descriptor->count;
        }
        break;

        case tr_descriptor_type_uniform_buffer_cbv:
        case tr_descriptor_type_storage_buffer_srv:
        case tr_descriptor_type_storage_buffer_uav:
        case tr_descriptor_type_uniform_texel_buffer_srv:
        case tr_descriptor_type_storage_texel_buffer_uav:
        case tr_descriptor_type_texture_srv:
        case tr_descriptor_type_texture_uav:
        {
            descriptor->dx_heap_offset = cbvsrvuav_heap_offset;
            cbvsrvuav_heap_offset += descriptor->count;
        }
        break;
        }
    }
}

void tr_internal_dx_destroy_descriptor_set(tr_renderer* p_renderer,
                                           tr_descriptor_set* p_descriptor_set)
{

}

void tr_internal_dx_create_cmd_pool(tr_renderer* p_renderer, tr_queue* p_queue, bool transient,
                                    tr_cmd_pool* p_cmd_pool)
{
    assert(NULL != p_renderer->dx_device);

    HRESULT hres = p_renderer->dx_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&p_cmd_pool->dx_cmd_alloc));
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_destroy_cmd_pool(tr_renderer* p_renderer, tr_cmd_pool* p_cmd_pool)
{
}

void tr_internal_dx_create_cmd(tr_cmd_pool* p_cmd_pool, bool secondary, tr_cmd* p_cmd)
{
    assert(NULL != p_cmd_pool->dx_cmd_alloc);
    assert(NULL != p_cmd_pool->renderer);

    ID3D12PipelineState* initialState = NULL;
    HRESULT hres = p_cmd_pool->renderer->dx_device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, p_cmd_pool->dx_cmd_alloc.Get(), initialState, IID_PPV_ARGS(&p_cmd->dx_cmd_list));
    assert(SUCCEEDED(hres));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    p_cmd->dx_cmd_list->Close();
}

void tr_internal_dx_destroy_cmd(tr_cmd_pool* p_cmd_pool, tr_cmd* p_cmd)
{

}

void tr_internal_dx_create_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    assert(NULL != p_renderer->dx_device);

    // Align the buffer size to multiples of 256
    if (p_buffer->usage & tr_buffer_usage_uniform_cbv)
    {
        p_buffer->size = tr_round_up((uint32_t)(p_buffer->size), 256);
    }

    D3D12_RESOURCE_DIMENSION res_dim = D3D12_RESOURCE_DIMENSION_BUFFER;

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_props.CreationNodeMask = 1;
    heap_props.VisibleNodeMask = 1;

    D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = res_dim;
    desc.Alignment = 0;
    desc.Width = p_buffer->size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if ((p_buffer->usage & tr_buffer_usage_storage_uav) ||
        (p_buffer->usage & tr_buffer_usage_counter_uav))
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // Adjust for padding
    UINT64 padded_size = 0;
    p_renderer->dx_device->GetCopyableFootprints(&desc, 0, 1, 0, NULL, NULL, NULL, &padded_size);
    p_buffer->size = (uint64_t)padded_size;
    desc.Width = padded_size;

    D3D12_RESOURCE_STATES res_states = D3D12_RESOURCE_STATE_COPY_DEST;
    switch (p_buffer->usage)
    {
    case tr_buffer_usage_uniform_cbv:
    {
        res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    break;

    case tr_buffer_usage_index:
    {
        res_states = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }
    break;

    case tr_buffer_usage_vertex:
    {
        res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    break;

    case tr_buffer_usage_storage_uav:
    {
        res_states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    break;

    case tr_buffer_usage_counter_uav:
    {
        res_states = D3D12_RESOURCE_STATE_COMMON;
    }
    break;
    }

    if (p_buffer->host_visible)
    {
        // D3D12_HEAP_TYPE_UPLOAD requires D3D12_RESOURCE_STATE_GENERIC_READ
        heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
        res_states = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    HRESULT hres = p_renderer->dx_device->CreateCommittedResource(
            &heap_props, heap_flags, &desc, res_states, NULL, IID_PPV_ARGS(&p_buffer->dx_resource));
    assert(SUCCEEDED(hres));

    if (p_buffer->host_visible)
    {
        D3D12_RANGE read_range = {0, 0};
        hres = p_buffer->dx_resource->Map(0, &read_range, (void**)&(p_buffer->cpu_mapped_address));
        assert(SUCCEEDED(hres));
    }

    switch (p_buffer->usage)
    {
    case tr_buffer_usage_index:
    {
        p_buffer->dx_index_buffer_view.BufferLocation =
            p_buffer->dx_resource->GetGPUVirtualAddress();
        p_buffer->dx_index_buffer_view.SizeInBytes = (UINT)p_buffer->size;
        // Format is filled out by tr_create_index_buffer
    }
    break;

    case tr_buffer_usage_vertex:
    {
        p_buffer->dx_vertex_buffer_view.BufferLocation =
            p_buffer->dx_resource->GetGPUVirtualAddress();
        p_buffer->dx_vertex_buffer_view.SizeInBytes = (UINT)p_buffer->size;
        // StrideInBytes is filled out by tr_create_vertex_buffer
    }
    break;

    case tr_buffer_usage_uniform_cbv:
    {
        p_buffer->dx_cbv_view_desc.BufferLocation = p_buffer->dx_resource->GetGPUVirtualAddress();
        p_buffer->dx_cbv_view_desc.SizeInBytes = (UINT)p_buffer->size;
    }
    break;

    case tr_buffer_usage_storage_srv:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC* desc = &(p_buffer->dx_srv_view_desc);
        desc->Format = DXGI_FORMAT_UNKNOWN;
        desc->ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc->Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc->Buffer.FirstElement = p_buffer->first_element;
        desc->Buffer.NumElements = (UINT)(p_buffer->element_count);
        desc->Buffer.StructureByteStride = (UINT)(p_buffer->struct_stride);
        desc->Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        if (p_buffer->raw)
        {
            desc->Format = DXGI_FORMAT_R32_TYPELESS;
            desc->Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
        }
    }
    break;

    case tr_buffer_usage_storage_uav:
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = &(p_buffer->dx_uav_view_desc);
        desc->Format = DXGI_FORMAT_UNKNOWN;
        desc->ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc->Buffer.FirstElement = p_buffer->first_element;
        desc->Buffer.NumElements = (UINT)(p_buffer->element_count);
        desc->Buffer.StructureByteStride = (UINT)(p_buffer->struct_stride);
        desc->Buffer.CounterOffsetInBytes = 0;
        desc->Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        if (p_buffer->raw)
        {
            desc->Format = DXGI_FORMAT_R32_TYPELESS;
            desc->Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;
        }
    }
    break;

    case tr_buffer_usage_uniform_texel_srv:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC* desc = &(p_buffer->dx_srv_view_desc);
        desc->Format = tr_util_to_dx_format(p_buffer->format);
        desc->ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc->Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc->Buffer.FirstElement = p_buffer->first_element;
        desc->Buffer.NumElements = (UINT)(p_buffer->element_count);
        desc->Buffer.StructureByteStride = (UINT)(p_buffer->struct_stride);
        desc->Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    }
    break;

    case tr_buffer_usage_storage_texel_uav:
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = &(p_buffer->dx_uav_view_desc);
        desc->Format = DXGI_FORMAT_UNKNOWN;
        desc->ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc->Buffer.FirstElement = p_buffer->first_element;
        desc->Buffer.NumElements = (UINT)(p_buffer->element_count);
        desc->Buffer.StructureByteStride = (UINT)(p_buffer->struct_stride);
        desc->Buffer.CounterOffsetInBytes = 0;
        desc->Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    }
    break;

    case tr_buffer_usage_counter_uav:
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = &(p_buffer->dx_uav_view_desc);
        desc->Format = DXGI_FORMAT_R32_TYPELESS;
        desc->ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc->Buffer.FirstElement = p_buffer->first_element;
        desc->Buffer.NumElements = (UINT)(p_buffer->element_count);
        desc->Buffer.StructureByteStride = (UINT)(p_buffer->struct_stride);
        desc->Buffer.CounterOffsetInBytes = 0;
        desc->Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    }
    break;
    }
}

void tr_internal_dx_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
}

void tr_internal_dx_create_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
    assert(NULL != p_renderer->dx_device);
    assert(DXGI_FORMAT_UNKNOWN != tr_util_to_dx_format(p_texture->format));

    p_texture->renderer = p_renderer;

    if (NULL == p_texture->dx_resource.Get())
    {
        D3D12_RESOURCE_DIMENSION res_dim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
        switch (p_texture->type)
        {
        case tr_texture_type_1d:
            res_dim = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;
        case tr_texture_type_2d:
            res_dim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        case tr_texture_type_3d:
            res_dim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;
        case tr_texture_type_cube:
            res_dim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        }
        assert(D3D12_RESOURCE_DIMENSION_UNKNOWN != res_dim);

        D3D12_HEAP_PROPERTIES heap_props = {};
        heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_props.CreationNodeMask = 1;
        heap_props.VisibleNodeMask = 1;

        D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_NONE;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = res_dim;
        desc.Alignment = 0;
        desc.Width = p_texture->width;
        desc.Height = p_texture->height;
        desc.DepthOrArraySize = p_texture->depth;
        desc.MipLevels = (UINT16)p_texture->mip_levels;
        desc.Format = tr_util_to_dx_format(p_texture->format);
        desc.SampleDesc.Count = (UINT)p_texture->sample_count;
        desc.SampleDesc.Quality = (UINT)p_texture->sample_quality;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        if (p_texture->usage & tr_texture_usage_color_attachment)
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (p_texture->usage & tr_texture_usage_depth_stencil_attachment)
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        if (p_texture->usage & tr_texture_usage_storage_image)
        {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_RESOURCE_STATES res_states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (p_texture->usage & tr_texture_usage_color_attachment)
        {
            res_states = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        D3D12_CLEAR_VALUE clear_value = {};
        clear_value.Format = tr_util_to_dx_format(p_texture->format);
        if (tr_texture_usage_depth_stencil_attachment ==
            (p_texture->usage & tr_texture_usage_depth_stencil_attachment))
        {
            clear_value.DepthStencil.Depth = p_texture->clear_value.depth;
            clear_value.DepthStencil.Stencil = p_texture->clear_value.stencil;
        }
        else
        {
            clear_value.Color[0] = p_texture->clear_value.r;
            clear_value.Color[1] = p_texture->clear_value.g;
            clear_value.Color[2] = p_texture->clear_value.b;
            clear_value.Color[3] = p_texture->clear_value.a;
        }

        D3D12_CLEAR_VALUE* p_clear_value = NULL;
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
            (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
        {
            p_clear_value = &clear_value;
        }

        HRESULT hres = p_renderer->dx_device->CreateCommittedResource(
            &heap_props, heap_flags, &desc, res_states, p_clear_value,
            IID_PPV_ARGS(&p_texture->dx_resource));
        assert(SUCCEEDED(hres));

        p_texture->owns_image = true;
    }

    if (p_texture->usage & tr_texture_usage_sampled_image)
    {
        D3D12_SRV_DIMENSION view_dim = D3D12_SRV_DIMENSION_UNKNOWN;
        switch (p_texture->type)
        {
        case tr_texture_type_1d:
            view_dim = D3D12_SRV_DIMENSION_TEXTURE1D;
            break;
        case tr_texture_type_2d:
            view_dim = D3D12_SRV_DIMENSION_TEXTURE2D;
            break;
        case tr_texture_type_3d:
            view_dim = D3D12_SRV_DIMENSION_TEXTURE3D;
            break;
        case tr_texture_type_cube:
            view_dim = D3D12_SRV_DIMENSION_TEXTURE2D;
            break;
        }
        assert(D3D12_SRV_DIMENSION_UNKNOWN != view_dim);

        p_texture->dx_srv_view_desc.Shader4ComponentMapping =
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        p_texture->dx_srv_view_desc.Format = tr_util_to_dx_format(p_texture->format);
        p_texture->dx_srv_view_desc.ViewDimension = view_dim;
        p_texture->dx_srv_view_desc.Texture2D.MipLevels = (UINT)p_texture->mip_levels;
    }

    if (p_texture->usage & tr_texture_usage_storage_image)
    {
        p_texture->dx_uav_view_desc.Format = tr_util_to_dx_format(p_texture->format);
        p_texture->dx_uav_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        p_texture->dx_uav_view_desc.Texture2D.MipSlice = 0;
        p_texture->dx_uav_view_desc.Texture2D.PlaneSlice = 0;
    }
}

void tr_internal_dx_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{

}

void tr_internal_dx_create_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    assert(NULL != p_renderer->dx_device);

    p_sampler->dx_sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    p_sampler->dx_sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    p_sampler->dx_sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    p_sampler->dx_sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    p_sampler->dx_sampler_desc.MipLODBias = 0;
    p_sampler->dx_sampler_desc.MaxAnisotropy = 0;
    p_sampler->dx_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    p_sampler->dx_sampler_desc.BorderColor[0] = 0.0f;
    p_sampler->dx_sampler_desc.BorderColor[1] = 0.0f;
    p_sampler->dx_sampler_desc.BorderColor[2] = 0.0f;
    p_sampler->dx_sampler_desc.BorderColor[3] = 0.0f;
    p_sampler->dx_sampler_desc.MinLOD = 0.0f;
    p_sampler->dx_sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
}

void tr_internal_dx_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    assert(NULL != p_renderer->dx_device);

    // NO-OP for now
}

void tr_internal_dx_create_shader_program(
    tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt,
    uint32_t hull_size, const void* hull_code, const char* hull_enpt, uint32_t domn_size,
    const void* domn_code, const char* domn_enpt, uint32_t geom_size, const void* geom_code,
    const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt,
    uint32_t comp_size, const void* comp_code, const char* comp_enpt,
    tr_shader_program* p_shader_program)
{
    assert(NULL != p_renderer->dx_device);

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compile_flags = 0;
#endif

    int major = 5;
    int minor = 0;
    switch (p_renderer->settings.dx_shader_target)
    {
    case tr_dx_shader_target_5_1:
    {
        major = 5;
        minor = 1;
    }
    break;
    case tr_dx_shader_target_6_0:
    {
        major = 6;
        minor = 0;
    }
    break;
    }

    char vs_target[16] = {};
    char hs_target[16] = {};
    char ds_target[16] = {};
    char gs_target[16] = {};
    char ps_target[16] = {};
    char cs_target[16] = {};

    sprintf_s(vs_target, "vs_%d_%d", major, minor);
    sprintf_s(hs_target, "hs_%d_%d", major, minor);
    sprintf_s(ds_target, "ds_%d_%d", major, minor);
    sprintf_s(gs_target, "gs_%d_%d", major, minor);
    sprintf_s(ps_target, "ps_%d_%d", major, minor);
    sprintf_s(cs_target, "cs_%d_%d", major, minor);

    const char* vs_name = "VERTEX";
    const char* hs_name = "HULL";
    const char* ds_name = "DOMAIN";
    const char* gs_name = "GEOMETRY";
    const char* ps_name = "PIXEL";
    const char* cs_name = "COMPUTE";

    for (uint32_t i = 0; i < tr_shader_stage_count; ++i)
    {
        tr_shader_stage stage_mask = (tr_shader_stage)(1 << i);
        if (stage_mask == (p_shader_program->shader_stages & stage_mask))
        {
            const void* source = NULL;
            SIZE_T source_len = 0;
            const char* source_name = NULL;
            const char* target = NULL;
            const char* entry_point = NULL;
            ID3DBlob** compiled_code = NULL;
            switch (stage_mask)
            {
            case tr_shader_stage_vert:
            {
                source = vert_code;
                source_len = vert_size;
                source_name = vs_name;
                target = vs_target;
                entry_point = vert_enpt;
                compiled_code = &(p_shader_program->dx_vert);
            }
            break;
            case tr_shader_stage_hull:
            {
                source = hull_code;
                source_len = hull_size;
                source_name = hs_name;
                target = hs_target;
                entry_point = hull_enpt;
                compiled_code = &(p_shader_program->dx_hull);
            }
            break;
            case tr_shader_stage_domn:
            {
                source = domn_code;
                source_len = domn_size;
                source_name = ds_name;
                target = ds_target;
                entry_point = domn_enpt;
                compiled_code = &(p_shader_program->dx_domn);
            }
            break;
            case tr_shader_stage_geom:
            {
                source = geom_code;
                source_len = geom_size;
                source_name = gs_name;
                target = gs_target;
                entry_point = geom_enpt;
                compiled_code = &(p_shader_program->dx_geom);
            }
            break;
            case tr_shader_stage_frag:
            {
                source = frag_code;
                source_len = frag_size;
                source_name = ps_name;
                target = ps_target;
                entry_point = frag_enpt;
                compiled_code = &(p_shader_program->dx_frag);
            }
            break;
            case tr_shader_stage_comp:
            {
                source = comp_code;
                source_len = comp_size;
                source_name = cs_name;
                target = cs_target;
                entry_point = comp_enpt;
                compiled_code = &(p_shader_program->dx_comp);
            }
            break;
            }

            D3D_SHADER_MACRO macros[] = {"D3D12", "1", NULL, NULL};
            ComPtr<ID3DBlob> error_msgs;
            HRESULT hres =
                D3DCompile2(source, source_len, source_name, macros, NULL, entry_point, target,
                            compile_flags, 0, 0, NULL, 0, compiled_code, &error_msgs);
            if (FAILED(hres))
            {
                vector<char> msg(error_msgs->GetBufferSize() + 1);
                memcpy(msg.data(), error_msgs->GetBufferPointer(), error_msgs->GetBufferSize());
                tr_internal_log(tr_log_type_error, msg.data(),
                                "tr_internal_dx_create_shader_program");
            }
            assert(SUCCEEDED(hres));
        }
    }
}

void tr_internal_dx_destroy_shader_program(tr_renderer* p_renderer,
                                           tr_shader_program* p_shader_program)
{

}

void tr_internal_dx_create_root_signature(tr_renderer* p_renderer,
                                          tr_descriptor_set* p_descriptor_set,
                                          tr_pipeline* p_pipeline)
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    HRESULT hres = p_renderer->dx_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
                                                              &feature_data, sizeof(feature_data));
    if (FAILED(hres))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    uint32_t range_count = 0;
    vector<D3D12_DESCRIPTOR_RANGE1> ranges_11;
    vector<D3D12_DESCRIPTOR_RANGE> ranges_10;

    uint32_t parameter_count = 0;
    vector<D3D12_ROOT_PARAMETER1> parameters_11;
    vector<D3D12_ROOT_PARAMETER> parameters_10;

    if (NULL != p_descriptor_set)
    {
        const uint32_t descriptor_count = p_descriptor_set->descriptor_count;

        uint32_t cbvsrvuav_count = 0;
        uint32_t sampler_count = 0;
        for (uint32_t i = 0; i < descriptor_count; ++i)
        {
            uint32_t count = p_descriptor_set->descriptors[i].count;
            switch (p_descriptor_set->descriptors[i].type)
            {
            case tr_descriptor_type_sampler:
                sampler_count += count;
                break;
            case tr_descriptor_type_uniform_buffer_cbv:
                cbvsrvuav_count += count;
                break;
            case tr_descriptor_type_storage_buffer_srv:
                cbvsrvuav_count += count;
                break;
            case tr_descriptor_type_storage_buffer_uav:
                cbvsrvuav_count += count;
                break;
            case tr_descriptor_type_uniform_texel_buffer_srv:
                cbvsrvuav_count += count;
                break;
            case tr_descriptor_type_storage_texel_buffer_uav:
                cbvsrvuav_count += count;
                break;
            case tr_descriptor_type_texture_srv:
                cbvsrvuav_count += count;
                break;
            case tr_descriptor_type_texture_uav:
                cbvsrvuav_count += count;
                break;
            }
        }

        // Allocate everything with an upper bound of descriptor counts
        ranges_11.resize(descriptor_count);
        ranges_10.resize(descriptor_count);

        parameters_11.resize(descriptor_count);
        parameters_10.resize(descriptor_count);

        // Build ranges
        for (uint32_t descriptor_index = 0; descriptor_index < descriptor_count; ++descriptor_index)
        {
            tr_descriptor* descriptor = &(p_descriptor_set->descriptors[descriptor_index]);
            D3D12_DESCRIPTOR_RANGE1* range_11 = &ranges_11[range_count];
            D3D12_DESCRIPTOR_RANGE* range_10 = &ranges_10[range_count];
            D3D12_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
            D3D12_ROOT_PARAMETER* param_10 = &parameters_10[parameter_count];
            param_11->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param_10->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

            // Start out with visibility on all shader stages
            param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            uint32_t shader_stage_count = 0;
            // Select one if there is only one
            if (descriptor->shader_stages & tr_shader_stage_vert)
            {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_hull)
            {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_domn)
            {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_geom)
            {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_frag)
            {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_comp)
            {
                // Keep D3D12_SHADER_VISIBILITY_ALL for compute shaders
                ++shader_stage_count;
            }
            // Go back to all shader stages if there's more than one stage
            if (shader_stage_count > 1)
            {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            }

            bool assign_range = false;
            switch (descriptor->type)
            {
            case tr_descriptor_type_sampler:
            {
                range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                assign_range = true;
            }
            break;
            case tr_descriptor_type_storage_buffer_srv:
            case tr_descriptor_type_uniform_texel_buffer_srv:
            case tr_descriptor_type_texture_srv:
            {
                range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                assign_range = true;
            }
            break;
            case tr_descriptor_type_uniform_buffer_cbv:
            {
                range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                assign_range = true;
            }
            break;
            case tr_descriptor_type_storage_buffer_uav:
            case tr_descriptor_type_storage_texel_buffer_uav:
            case tr_descriptor_type_texture_uav:
            {
                range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                assign_range = true;
            }
            break;
            }

            if (assign_range)
            {
                range_11->NumDescriptors = descriptor->count;
                range_11->BaseShaderRegister = descriptor->binding;
                range_11->RegisterSpace = 0;
                range_11->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
                range_11->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                range_10->NumDescriptors = descriptor->count;
                range_10->BaseShaderRegister = descriptor->binding;
                range_10->RegisterSpace = 0;
                range_10->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                param_11->DescriptorTable.pDescriptorRanges = range_11;
                param_11->DescriptorTable.NumDescriptorRanges = 1;

                param_10->DescriptorTable.pDescriptorRanges = range_10;
                param_10->DescriptorTable.NumDescriptorRanges = 1;

                descriptor->dx_root_parameter_index = parameter_count;

                ++range_count;
                ++parameter_count;
            }
        }
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
    if (D3D_ROOT_SIGNATURE_VERSION_1_1 == feature_data.HighestVersion)
    {
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        desc.Desc_1_1.NumParameters = parameter_count;
        desc.Desc_1_1.pParameters = parameters_11.data();
        desc.Desc_1_1.NumStaticSamplers = 0;
        desc.Desc_1_1.pStaticSamplers = NULL;
        desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }
    else if (D3D_ROOT_SIGNATURE_VERSION_1_0 == feature_data.HighestVersion)
    {
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
        desc.Desc_1_0.NumParameters = parameter_count;
        desc.Desc_1_0.pParameters = parameters_10.data();
        desc.Desc_1_0.NumStaticSamplers = 0;
        desc.Desc_1_0.pStaticSamplers = NULL;
        desc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }

    ComPtr<ID3DBlob> sig_blob;
    ComPtr<ID3DBlob> error_msgs;
    if (D3D_ROOT_SIGNATURE_VERSION_1_1 == feature_data.HighestVersion)
    {
        hres = fnD3D12SerializeVersionedRootSignature(&desc, &sig_blob, &error_msgs);
    }
    else
    {
        hres = D3D12SerializeRootSignature(&(desc.Desc_1_0), D3D_ROOT_SIGNATURE_VERSION_1_0,
                                           &sig_blob, &error_msgs);
    }
    assert(SUCCEEDED(hres));

    hres = p_renderer->dx_device->CreateRootSignature(
        0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(),
        IID_PPV_ARGS(&p_pipeline->dx_root_signature));
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_create_pipeline_state(tr_renderer* p_renderer,
                                          tr_shader_program* p_shader_program,
                                          const tr_vertex_layout* p_vertex_layout,
                                          tr_render_target* p_render_target,
                                          const tr_pipeline_settings* p_pipeline_settings,
                                          tr_pipeline* p_pipeline)
{
    D3D12_SHADER_BYTECODE VS = {};
    D3D12_SHADER_BYTECODE PS = {};
    D3D12_SHADER_BYTECODE DS = {};
    D3D12_SHADER_BYTECODE HS = {};
    D3D12_SHADER_BYTECODE GS = {};
    if (NULL != p_shader_program->dx_vert)
    {
        VS.BytecodeLength = p_shader_program->dx_vert->GetBufferSize();
        VS.pShaderBytecode = p_shader_program->dx_vert->GetBufferPointer();
    }
    if (NULL != p_shader_program->dx_frag)
    {
        PS.BytecodeLength = p_shader_program->dx_frag->GetBufferSize();
        PS.pShaderBytecode = p_shader_program->dx_frag->GetBufferPointer();
    }
    if (NULL != p_shader_program->dx_domn)
    {
        DS.BytecodeLength = p_shader_program->dx_domn->GetBufferSize();
        DS.pShaderBytecode = p_shader_program->dx_domn->GetBufferPointer();
    }
    if (NULL != p_shader_program->dx_hull)
    {
        HS.BytecodeLength = p_shader_program->dx_hull->GetBufferSize();
        HS.pShaderBytecode = p_shader_program->dx_hull->GetBufferPointer();
    }
    if (NULL != p_shader_program->dx_geom)
    {
        GS.BytecodeLength = p_shader_program->dx_geom->GetBufferSize();
        GS.pShaderBytecode = p_shader_program->dx_geom->GetBufferPointer();
    }

    D3D12_STREAM_OUTPUT_DESC stream_output_desc = {};
    stream_output_desc.pSODeclaration = NULL;
    stream_output_desc.NumEntries = 0;
    stream_output_desc.pBufferStrides = NULL;
    stream_output_desc.NumStrides = 0;
    stream_output_desc.RasterizedStream = 0;

    D3D12_BLEND_DESC blend_desc = {};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;
    for (UINT attrib_index = 0; attrib_index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
         ++attrib_index)
    {
        blend_desc.RenderTarget[attrib_index].BlendEnable = FALSE;
        blend_desc.RenderTarget[attrib_index].LogicOpEnable = FALSE;
        blend_desc.RenderTarget[attrib_index].SrcBlend = D3D12_BLEND_ONE;
        blend_desc.RenderTarget[attrib_index].DestBlend = D3D12_BLEND_ZERO;
        blend_desc.RenderTarget[attrib_index].BlendOp = D3D12_BLEND_OP_ADD;
        blend_desc.RenderTarget[attrib_index].SrcBlendAlpha = D3D12_BLEND_ONE;
        blend_desc.RenderTarget[attrib_index].DestBlendAlpha = D3D12_BLEND_ZERO;
        blend_desc.RenderTarget[attrib_index].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blend_desc.RenderTarget[attrib_index].LogicOp = D3D12_LOGIC_OP_NOOP;
        blend_desc.RenderTarget[attrib_index].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    D3D12_CULL_MODE cull_mode = D3D12_CULL_MODE_NONE;
    switch (p_pipeline_settings->cull_mode)
    {
    case tr_cull_mode_back:
        cull_mode = D3D12_CULL_MODE_BACK;
        break;
    case tr_cull_mode_front:
        cull_mode = D3D12_CULL_MODE_FRONT;
        break;
    }
    BOOL front_face_ccw = (tr_front_face_ccw == p_pipeline_settings->front_face) ? TRUE : FALSE;
    D3D12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = cull_mode;
    rasterizer_desc.FrontCounterClockwise = front_face_ccw;
    rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.MultisampleEnable = FALSE;
    rasterizer_desc.AntialiasedLineEnable = FALSE;
    rasterizer_desc.ForcedSampleCount = 0;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_DEPTH_STENCILOP_DESC depth_stencilop_desc = {};
    depth_stencilop_desc.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencilop_desc.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencilop_desc.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depth_stencilop_desc.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    D3D12_DEPTH_STENCIL_DESC depth_stencil_desc = {};
    depth_stencil_desc.DepthEnable = p_pipeline_settings->depth ? TRUE : FALSE;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depth_stencil_desc.StencilEnable = FALSE;
    depth_stencil_desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth_stencil_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth_stencil_desc.FrontFace = depth_stencilop_desc;
    depth_stencil_desc.BackFace = depth_stencilop_desc;

    uint32_t input_element_count = 0;
    D3D12_INPUT_ELEMENT_DESC input_elements[tr_max_vertex_attribs] = {};
    char semantic_names[tr_max_vertex_attribs][tr_max_semantic_name_length] = {};

    uint32_t attrib_count = tr_min(p_vertex_layout->attrib_count, tr_max_vertex_attribs);
    for (uint32_t attrib_index = 0; attrib_index < p_vertex_layout->attrib_count; ++attrib_index)
    {
        const tr_vertex_attrib* attrib = &(p_vertex_layout->attribs[attrib_index]);
        assert(tr_semantic_undefined != attrib->semantic);

        if (attrib->semantic_name_length > 0)
        {
            uint32_t name_length =
                tr_min(tr_max_semantic_name_length, attrib->semantic_name_length);
            strncpy_s(semantic_names[attrib_index], attrib->semantic_name, name_length);
        }
        else
        {
            char name[tr_max_semantic_name_length] = {};
            switch (attrib->semantic)
            {
            case tr_semantic_position:
                sprintf_s(name, "POSITION");
                break;
            case tr_semantic_normal:
                sprintf_s(name, "NORMAL");
                break;
            case tr_semantic_color:
                sprintf_s(name, "COLOR");
                break;
            case tr_semantic_tangent:
                sprintf_s(name, "TANGENT");
                break;
            case tr_semantic_bitangent:
                sprintf_s(name, "BITANGENT");
                break;
            case tr_semantic_texcoord0:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord1:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord2:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord3:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord4:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord5:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord6:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord7:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord8:
                sprintf_s(name, "TEXCOORD");
                break;
            case tr_semantic_texcoord9:
                sprintf_s(name, "TEXCOORD");
                break;
            default:
                break;
            }
            assert(0 != strlen(name));
            strncpy_s(semantic_names[attrib_index], name, strlen(name));
        }

        UINT semantic_index = 0;
        switch (attrib->semantic)
        {
        case tr_semantic_texcoord0:
            semantic_index = 0;
            break;
        case tr_semantic_texcoord1:
            semantic_index = 1;
            break;
        case tr_semantic_texcoord2:
            semantic_index = 2;
            break;
        case tr_semantic_texcoord3:
            semantic_index = 3;
            break;
        case tr_semantic_texcoord4:
            semantic_index = 4;
            break;
        case tr_semantic_texcoord5:
            semantic_index = 5;
            break;
        case tr_semantic_texcoord6:
            semantic_index = 6;
            break;
        case tr_semantic_texcoord7:
            semantic_index = 7;
            break;
        case tr_semantic_texcoord8:
            semantic_index = 8;
            break;
        case tr_semantic_texcoord9:
            semantic_index = 9;
            break;
        default:
            break;
        }

        D3D12_INPUT_ELEMENT_DESC input_element_desc = {};
        input_elements[input_element_count].SemanticName = semantic_names[attrib_index];
        input_elements[input_element_count].SemanticIndex = semantic_index;
        input_elements[input_element_count].Format = tr_util_to_dx_format(attrib->format);
        input_elements[input_element_count].InputSlot = 0;
        input_elements[input_element_count].AlignedByteOffset = attrib->offset;
        input_elements[input_element_count].InputSlotClass =
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_elements[input_element_count].InstanceDataStepRate = 0;
        ++input_element_count;
    }

    D3D12_INPUT_LAYOUT_DESC input_layout_desc = {};
    input_layout_desc.pInputElementDescs = input_elements;
    input_layout_desc.NumElements = input_element_count;

    uint32_t render_target_count =
        tr_min(p_render_target->color_attachment_count, tr_max_render_target_attachments);
    render_target_count = tr_min(render_target_count, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

    DXGI_SAMPLE_DESC sample_desc = {};
    sample_desc.Count = (UINT)p_render_target->sample_count;
    sample_desc.Quality = 0;

    D3D12_CACHED_PIPELINE_STATE cached_pso_desc = {};
    cached_pso_desc.pCachedBlob = NULL;
    cached_pso_desc.CachedBlobSizeInBytes = 0;

    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    switch (p_pipeline->settings.primitive_topo)
    {
    case tr_primitive_topo_point_list:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        break;
    case tr_primitive_topo_line_list:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case tr_primitive_topo_line_strip:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case tr_primitive_topo_tri_list:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case tr_primitive_topo_tri_strip:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case tr_primitive_topo_1_point_patch:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        break;
    case tr_primitive_topo_2_point_patch:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        break;
    case tr_primitive_topo_3_point_patch:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        break;
    case tr_primitive_topo_4_point_patch:
        topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        break;
    default:
        break;
    }
    assert(topology != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc = {};
    pipeline_state_desc.pRootSignature = p_pipeline->dx_root_signature.Get();
    pipeline_state_desc.VS = VS;
    pipeline_state_desc.PS = PS;
    pipeline_state_desc.DS = DS;
    pipeline_state_desc.HS = HS;
    pipeline_state_desc.GS = GS;
    pipeline_state_desc.StreamOutput = stream_output_desc;
    pipeline_state_desc.BlendState = blend_desc;
    pipeline_state_desc.SampleMask = UINT_MAX;
    pipeline_state_desc.RasterizerState = rasterizer_desc;
    pipeline_state_desc.DepthStencilState = depth_stencil_desc;
    pipeline_state_desc.InputLayout = input_layout_desc;
    pipeline_state_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    pipeline_state_desc.PrimitiveTopologyType = topology;
    pipeline_state_desc.NumRenderTargets = render_target_count;
    pipeline_state_desc.DSVFormat =
        (p_render_target->depth_stencil_attachment != NULL)
            ? tr_util_to_dx_format(p_render_target->depth_stencil_attachment->format)
            : DXGI_FORMAT_UNKNOWN;
    pipeline_state_desc.SampleDesc = sample_desc;
    pipeline_state_desc.NodeMask = 0;
    pipeline_state_desc.CachedPSO = cached_pso_desc;
    pipeline_state_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    for (uint32_t attrib_index = 0; attrib_index < render_target_count; ++attrib_index)
    {
        pipeline_state_desc.RTVFormats[attrib_index] =
            tr_util_to_dx_format(p_render_target->color_attachments[attrib_index]->format);
    }

    HRESULT hres = p_renderer->dx_device->CreateGraphicsPipelineState(
        &pipeline_state_desc, IID_PPV_ARGS(&p_pipeline->dx_pipeline_state));
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program,
                                    const tr_vertex_layout* p_vertex_layout,
                                    tr_descriptor_set* p_descriptor_set,
                                    tr_render_target* p_render_target,
                                    const tr_pipeline_settings* p_pipeline_settings,
                                    tr_pipeline* p_pipeline)
{
    assert(NULL != p_renderer->dx_device);
    assert((NULL != p_shader_program->dx_vert) || (NULL != p_shader_program->dx_hull) ||
           (NULL != p_shader_program->dx_domn) || (NULL != p_shader_program->dx_geom) ||
           (NULL != p_shader_program->dx_frag));
    assert((NULL != p_render_target->dx_rtv_heap) || (NULL != p_render_target->dx_dsv_heap));

    tr_internal_dx_create_root_signature(p_renderer, p_descriptor_set, p_pipeline);
    tr_internal_dx_create_pipeline_state(p_renderer, p_shader_program, p_vertex_layout,
                                         p_render_target, p_pipeline_settings, p_pipeline);
}

void tr_internal_dx_create_compute_pipeline_state(tr_renderer* p_renderer,
                                                  tr_shader_program* p_shader_program,
                                                  const tr_pipeline_settings* p_pipeline_settings,
                                                  tr_pipeline* p_pipeline)
{
    D3D12_SHADER_BYTECODE CS = {};
    if (NULL != p_shader_program->dx_comp)
    {
        CS.BytecodeLength = p_shader_program->dx_comp->GetBufferSize();
        CS.pShaderBytecode = p_shader_program->dx_comp->GetBufferPointer();
    }

    D3D12_CACHED_PIPELINE_STATE cached_pso_desc = {};
    cached_pso_desc.pCachedBlob = NULL;
    cached_pso_desc.CachedBlobSizeInBytes = 0;

    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_state_desc = {};
    pipeline_state_desc.pRootSignature = p_pipeline->dx_root_signature.Get();
    pipeline_state_desc.CS = CS;
    pipeline_state_desc.NodeMask = 0;
    pipeline_state_desc.CachedPSO = cached_pso_desc;
    pipeline_state_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    HRESULT hres = p_renderer->dx_device->CreateComputePipelineState(
        &pipeline_state_desc, IID_PPV_ARGS(&p_pipeline->dx_pipeline_state));
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_create_compute_pipeline(tr_renderer* p_renderer,
                                            tr_shader_program* p_shader_program,
                                            tr_descriptor_set* p_descriptor_set,
                                            const tr_pipeline_settings* p_pipeline_settings,
                                            tr_pipeline* p_pipeline)
{
    assert(NULL != p_renderer->dx_device);
    assert(NULL != p_shader_program->dx_comp);

    tr_internal_dx_create_root_signature(p_renderer, p_descriptor_set, p_pipeline);
    tr_internal_dx_create_compute_pipeline_state(p_renderer, p_shader_program, p_pipeline_settings,
                                                 p_pipeline);
}

void tr_internal_dx_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline)
{
}

void tr_internal_dx_create_render_target(tr_renderer* p_renderer, bool is_swapchain,
                                         tr_render_target* p_render_target)
{
    assert(NULL != p_renderer->dx_device);

    if (p_render_target->color_attachment_count > 0)
    {
        if (p_render_target->sample_count > tr_sample_count_1)
        {
            assert(NULL != p_render_target->color_attachments_multisample);
        }
        else
        {
            assert(NULL != p_render_target->color_attachments);
        }

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = (UINT)p_render_target->color_attachment_count;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT hres = p_renderer->dx_device->CreateDescriptorHeap(
            &desc, IID_PPV_ARGS(&p_render_target->dx_rtv_heap));
        assert(SUCCEEDED(hres));

        D3D12_CPU_DESCRIPTOR_HANDLE handle =
            p_render_target->dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        const UINT inc_size =
            p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i)
        {
            if (p_render_target->sample_count > tr_sample_count_1)
            {
                assert(NULL != p_render_target->color_attachments_multisample[i]);
                assert(NULL !=
                       p_render_target->color_attachments_multisample[i]->dx_resource.Get());

                p_renderer->dx_device->CreateRenderTargetView(
                    p_render_target->color_attachments_multisample[i]->dx_resource.Get(), NULL, handle);
            }
            else
            {
                assert(NULL != p_render_target->color_attachments[i]);
                assert(NULL != p_render_target->color_attachments[i]->dx_resource.Get());

                p_renderer->dx_device->CreateRenderTargetView(
                    p_render_target->color_attachments[i]->dx_resource.Get(), NULL, handle);
            }
            handle.ptr += inc_size;
        }
    }

    if (tr_format_undefined != p_render_target->depth_stencil_format)
    {

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = 1;
        desc.NodeMask = 0;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT hres = p_renderer->dx_device->CreateDescriptorHeap(
            &desc, IID_PPV_ARGS(&p_render_target->dx_dsv_heap));
        assert(SUCCEEDED(hres));

        D3D12_CPU_DESCRIPTOR_HANDLE handle =
            p_render_target->dx_dsv_heap->GetCPUDescriptorHandleForHeapStart();
        const UINT inc_size =
            p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        if (p_render_target->sample_count > tr_sample_count_1)
        {
            assert(NULL != p_render_target->depth_stencil_attachment_multisample);
            assert(NULL !=
                   p_render_target->depth_stencil_attachment_multisample->dx_resource.Get());

            p_renderer->dx_device->CreateDepthStencilView(
                p_render_target->depth_stencil_attachment_multisample->dx_resource.Get(), NULL, handle);
        }
        else
        {
            assert(NULL != p_render_target->depth_stencil_attachment);
            assert(NULL != p_render_target->depth_stencil_attachment->dx_resource.Get());

            p_renderer->dx_device->CreateDepthStencilView(
                p_render_target->depth_stencil_attachment->dx_resource.Get(), NULL, handle);
        }
    }
}

void tr_internal_dx_destroy_render_target(tr_renderer* p_renderer,
                                          tr_render_target* p_render_target)
{
}

// -------------------------------------------------------------------------------------------------
// Internal descriptor set functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_update_descriptor_set(tr_renderer* p_renderer,
                                          tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_renderer->dx_device);
    assert((NULL != p_descriptor_set->dx_cbvsrvuav_heap) ||
           (NULL != p_descriptor_set->dx_sampler_heap));

    // Not really efficient, just write less frequently ;)
    uint32_t write_count = 0;
    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i)
    {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        if ((NULL != descriptor->samplers) || (NULL != descriptor->textures) ||
            (NULL != descriptor->uniform_buffers))
        {
            ++write_count;
        }
    }
    // Bail if there's nothing to write
    if (0 == write_count)
    {
        return;
    }

    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i)
    {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        if ((NULL == descriptor->samplers) && (NULL == descriptor->textures) &&
            (NULL == descriptor->uniform_buffers))
        {
            continue;
        }

        switch (descriptor->type)
        {
        case tr_descriptor_type_sampler:
        {
            assert(NULL != descriptor->samplers);

            D3D12_CPU_DESCRIPTOR_HANDLE handle =
                p_descriptor_set->dx_sampler_heap->GetCPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            for (uint32_t i = 0; i < descriptor->count; ++i)
            {
                assert(NULL != descriptor->samplers[i]);

                D3D12_SAMPLER_DESC* sampler_desc = &(descriptor->samplers[i]->dx_sampler_desc);
                p_renderer->dx_device->CreateSampler(sampler_desc, handle);
                handle.ptr += handle_inc_size;
            }
        }
        break;

        case tr_descriptor_type_uniform_buffer_cbv:
        {
            assert(NULL != descriptor->uniform_buffers);

            D3D12_CPU_DESCRIPTOR_HANDLE handle =
                p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            for (uint32_t i = 0; i < descriptor->count; ++i)
            {
                assert(NULL != descriptor->uniform_buffers[i]);

                ID3D12Resource* resource = descriptor->uniform_buffers[i]->dx_resource.Get();
                D3D12_CONSTANT_BUFFER_VIEW_DESC* view_desc =
                    &(descriptor->uniform_buffers[i]->dx_cbv_view_desc);
                p_renderer->dx_device->CreateConstantBufferView(view_desc, handle);
                handle.ptr += handle_inc_size;
            }
        }
        break;

        case tr_descriptor_type_storage_buffer_srv:
        case tr_descriptor_type_uniform_texel_buffer_srv:
        {
            assert(NULL != descriptor->buffers);

            D3D12_CPU_DESCRIPTOR_HANDLE handle =
                p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            for (uint32_t i = 0; i < descriptor->count; ++i)
            {
                assert(NULL != descriptor->buffers[i]);

                ID3D12Resource* resource = descriptor->buffers[i]->dx_resource.Get();
                D3D12_SHADER_RESOURCE_VIEW_DESC* view_desc =
                    &(descriptor->buffers[i]->dx_srv_view_desc);
                p_renderer->dx_device->CreateShaderResourceView(resource, view_desc, handle);
                handle.ptr += handle_inc_size;
            }
        }
        break;

        case tr_descriptor_type_storage_buffer_uav:
        case tr_descriptor_type_storage_texel_buffer_uav:
        {
            assert(NULL != descriptor->buffers);

            D3D12_CPU_DESCRIPTOR_HANDLE handle =
                p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            for (uint32_t i = 0; i < descriptor->count; ++i)
            {
                assert(NULL != descriptor->buffers[i]);

                ID3D12Resource* resource = descriptor->buffers[i]->dx_resource.Get();
                D3D12_UNORDERED_ACCESS_VIEW_DESC* view_desc =
                    &(descriptor->buffers[i]->dx_uav_view_desc);
                if (descriptor->buffers[i]->counter_buffer != NULL)
                {
                    ID3D12Resource* counter_resource =
                        descriptor->buffers[i]->counter_buffer->dx_resource.Get();
                    p_renderer->dx_device->CreateUnorderedAccessView(resource, counter_resource,
                                                                     view_desc, handle);
                }
                else
                {
                    p_renderer->dx_device->CreateUnorderedAccessView(resource, NULL, view_desc,
                                                                     handle);
                }
                handle.ptr += handle_inc_size;
            }
        }
        break;

        case tr_descriptor_type_texture_srv:
        {
            assert(NULL != descriptor->textures);

            D3D12_CPU_DESCRIPTOR_HANDLE handle =
                p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            for (uint32_t i = 0; i < descriptor->count; ++i)
            {
                assert(NULL != descriptor->textures[i]);

                ID3D12Resource* resource = descriptor->textures[i]->dx_resource.Get();
                D3D12_SHADER_RESOURCE_VIEW_DESC* view_desc =
                    &(descriptor->textures[i]->dx_srv_view_desc);
                p_renderer->dx_device->CreateShaderResourceView(resource, view_desc, handle);
                handle.ptr += handle_inc_size;
            }
        }
        break;

        case tr_descriptor_type_texture_uav:
        {
            assert(NULL != descriptor->textures);

            D3D12_CPU_DESCRIPTOR_HANDLE handle =
                p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
            UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            for (uint32_t i = 0; i < descriptor->count; ++i)
            {
                assert(NULL != descriptor->textures[i]);

                ID3D12Resource* resource = descriptor->textures[i]->dx_resource.Get();
                D3D12_UNORDERED_ACCESS_VIEW_DESC* view_desc =
                    &(descriptor->textures[i]->dx_uav_view_desc);
                p_renderer->dx_device->CreateUnorderedAccessView(resource, NULL, view_desc, handle);
                handle.ptr += handle_inc_size;
            }
        }
        break;
        }
    }
}

// -------------------------------------------------------------------------------------------------
// Internal command buffer functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_begin_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != p_cmd->cmd_pool->dx_cmd_alloc);

    HRESULT hres = p_cmd->cmd_pool->dx_cmd_alloc->Reset();
    assert(SUCCEEDED(hres));

    hres = p_cmd->dx_cmd_list->Reset(p_cmd->cmd_pool->dx_cmd_alloc.Get(), NULL);
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_end_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd->dx_cmd_list);

    HRESULT hres = p_cmd->dx_cmd_list->Close();
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(s_tr_internal->bound_render_target == p_render_target);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE* p_rtv_handle = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle = NULL;

    if (p_render_target->color_attachment_count > 0)
    {
        rtv_handle = p_render_target->dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        p_rtv_handle = &rtv_handle;
    }

    if (tr_format_undefined != p_render_target->depth_stencil_format)
    {
        dsv_handle = p_render_target->dx_dsv_heap->GetCPUDescriptorHandleForHeapStart();
        p_dsv_handle = &dsv_handle;
    }

    p_cmd->dx_cmd_list->OMSetRenderTargets(p_render_target->color_attachment_count, p_rtv_handle,
                                           TRUE, p_dsv_handle);
}

void tr_internal_dx_cmd_end_render(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd->dx_cmd_list);

    if ((NULL != s_tr_internal->bound_render_target) &&
        (s_tr_internal->bound_render_target->sample_count > tr_sample_count_1))
    {
        tr_render_target* render_target = s_tr_internal->bound_render_target;
        tr_texture** ss_attachments = render_target->color_attachments;
        tr_texture** ms_attachments = render_target->color_attachments_multisample;
        uint32_t color_attachment_count = render_target->color_attachment_count;
        bool is_present =
            (tr_texture_usage_present == (ss_attachments[0]->usage & tr_texture_usage_present));

        // This means we're dealing with a multisample swapchain
        if ((1 == color_attachment_count) && is_present)
        {
            tr_texture* ss_attachment = ss_attachments[0];
            tr_texture* ms_attachment = ms_attachments[0];
            if (tr_texture_usage_present == (ss_attachment->usage & tr_texture_usage_present))
            {
                // If the render targets have transitioned correctly, we can expect them to be in
                // the require states
                tr_internal_dx_cmd_image_transition(p_cmd, ss_attachment,
                                                    tr_texture_usage_color_attachment,
                                                    tr_texture_usage_resolve_dst);
                tr_internal_dx_cmd_image_transition(p_cmd, ms_attachment,
                                                    tr_texture_usage_color_attachment,
                                                    tr_texture_usage_resolve_src);
                // Resolve from multisample to single sample
                p_cmd->dx_cmd_list->ResolveSubresource(
                    ss_attachment->dx_resource.Get(), 0, ms_attachment->dx_resource.Get(), 0,
                    tr_util_to_dx_format(render_target->color_format));
                // Put it back the way we found it
                tr_internal_dx_cmd_image_transition(p_cmd, ss_attachment,
                                                    tr_texture_usage_resolve_dst,
                                                    tr_texture_usage_color_attachment);
                tr_internal_dx_cmd_image_transition(p_cmd, ms_attachment,
                                                    tr_texture_usage_resolve_src,
                                                    tr_texture_usage_color_attachment);
            }
        }
    }
}

void tr_internal_dx_cmd_set_viewport(tr_cmd* p_cmd, float x, float y, float width, float height,
                                     float min_depth, float max_depth)
{
    assert(NULL != p_cmd->dx_cmd_list);

    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = min_depth;
    viewport.MaxDepth = max_depth;

    p_cmd->dx_cmd_list->RSSetViewports(1, &viewport);
}

void tr_internal_dx_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width,
                                    uint32_t height)
{
    assert(NULL != p_cmd->dx_cmd_list);

    D3D12_RECT scissor = {};
    scissor.left = x;
    scissor.top = y;
    scissor.right = x + width;
    scissor.bottom = y + height;

    p_cmd->dx_cmd_list->RSSetScissorRects(1, &scissor);
}

void tr_cmd_internal_dx_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index,
                                                   const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != s_tr_internal->bound_render_target);

    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        s_tr_internal->bound_render_target->dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();
    UINT inc_size = p_cmd->cmd_pool->renderer->dx_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    handle.ptr += attachment_index * inc_size;

    FLOAT color_rgba[4] = {};
    color_rgba[0] = clear_value->r;
    color_rgba[1] = clear_value->g;
    color_rgba[2] = clear_value->b;
    color_rgba[3] = clear_value->a;

    p_cmd->dx_cmd_list->ClearRenderTargetView(handle, color_rgba, 0, NULL);
}

void tr_cmd_internal_dx_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd,
                                                           const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != s_tr_internal->bound_render_target);

    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        s_tr_internal->bound_render_target->dx_dsv_heap->GetCPUDescriptorHandleForHeapStart();

    p_cmd->dx_cmd_list->ClearDepthStencilView(
        handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clear_value->depth,
        (uint8_t)clear_value->stencil, 0, nullptr);
}

void tr_internal_dx_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != p_pipeline->dx_pipeline_state);
    assert(NULL != p_pipeline->dx_root_signature);

    p_cmd->dx_cmd_list->SetPipelineState(p_pipeline->dx_pipeline_state.Get());

    if (p_pipeline->type == tr_pipeline_type_graphics)
    {
        D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        switch (p_pipeline->settings.primitive_topo)
        {
        case tr_primitive_topo_point_list:
            topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
        case tr_primitive_topo_line_list:
            topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            break;
        case tr_primitive_topo_line_strip:
            topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;
        case tr_primitive_topo_tri_list:
            topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
        case tr_primitive_topo_tri_strip:
            topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;
        case tr_primitive_topo_1_point_patch:
            topology = D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
            break;
        case tr_primitive_topo_2_point_patch:
            topology = D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
            break;
        case tr_primitive_topo_3_point_patch:
            topology = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
            break;
        case tr_primitive_topo_4_point_patch:
            topology = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
            break;
        default:
            break;
        }
        assert(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED != topology);

        p_cmd->dx_cmd_list->SetGraphicsRootSignature(p_pipeline->dx_root_signature.Get());
        p_cmd->dx_cmd_list->IASetPrimitiveTopology(topology);
    }
    else if (p_pipeline->type == tr_pipeline_type_compute)
    {
        p_cmd->dx_cmd_list->SetComputeRootSignature(p_pipeline->dx_root_signature.Get());
    }
}

void tr_internal_dx_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline,
                                             tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_cmd->dx_cmd_list);

    uint32_t descriptor_heap_count = 0;
    ID3D12DescriptorHeap* descriptor_heaps[2];
    if (NULL != p_descriptor_set->dx_cbvsrvuav_heap)
    {
        descriptor_heaps[descriptor_heap_count] = p_descriptor_set->dx_cbvsrvuav_heap.Get();
        ++descriptor_heap_count;
    }
    if (NULL != p_descriptor_set->dx_sampler_heap)
    {
        descriptor_heaps[descriptor_heap_count] = p_descriptor_set->dx_sampler_heap.Get();
        ++descriptor_heap_count;
    }

    if (descriptor_heap_count > 0)
    {
        p_cmd->dx_cmd_list->SetDescriptorHeaps(descriptor_heap_count, descriptor_heaps);
    }

    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i)
    {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        if (UINT32_MAX == descriptor->dx_root_parameter_index)
        {
            continue;
        }

        switch (p_descriptor_set->descriptors[i].type)
        {
        case tr_descriptor_type_sampler:
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle =
                p_descriptor_set->dx_sampler_heap->GetGPUDescriptorHandleForHeapStart();
            UINT handle_inc_size =
                p_cmd->cmd_pool->renderer->dx_device->GetDescriptorHandleIncrementSize(
                    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            p_cmd->dx_cmd_list->SetGraphicsRootDescriptorTable(descriptor->dx_root_parameter_index,
                                                               handle);
        }
        break;

        case tr_descriptor_type_uniform_buffer_cbv:
        case tr_descriptor_type_storage_buffer_srv:
        case tr_descriptor_type_storage_buffer_uav:
        case tr_descriptor_type_texture_srv:
        case tr_descriptor_type_texture_uav:
        case tr_descriptor_type_uniform_texel_buffer_srv:
        case tr_descriptor_type_storage_texel_buffer_uav:
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle =
                p_descriptor_set->dx_cbvsrvuav_heap->GetGPUDescriptorHandleForHeapStart();
            UINT handle_inc_size =
                p_cmd->cmd_pool->renderer->dx_device->GetDescriptorHandleIncrementSize(
                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
            if (p_pipeline->type == tr_pipeline_type_graphics)
            {
                p_cmd->dx_cmd_list->SetGraphicsRootDescriptorTable(
                    descriptor->dx_root_parameter_index, handle);
            }
            else if (p_pipeline->type == tr_pipeline_type_compute)
            {
                p_cmd->dx_cmd_list->SetComputeRootDescriptorTable(
                    descriptor->dx_root_parameter_index, handle);
            }
        }
        break;
        }
    }
}

void tr_internal_dx_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer)
{
    assert(NULL != p_cmd->dx_cmd_list);

    assert(NULL != p_buffer->dx_index_buffer_view.BufferLocation);

    p_cmd->dx_cmd_list->IASetIndexBuffer(&(p_buffer->dx_index_buffer_view));
}

void tr_internal_dx_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count,
                                            tr_buffer** pp_buffers)
{
    assert(NULL != p_cmd->dx_cmd_list);

    D3D12_VERTEX_BUFFER_VIEW views[tr_max_vertex_attribs] = {};
    for (uint32_t i = 0; i < buffer_count; ++i)
    {
        assert(NULL != pp_buffers[i]->dx_vertex_buffer_view.BufferLocation);

        views[i] = pp_buffers[i]->dx_vertex_buffer_view;
    }

    p_cmd->dx_cmd_list->IASetVertexBuffers(0, buffer_count, views);
}

void tr_internal_dx_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex)
{
    assert(NULL != p_cmd->dx_cmd_list);

    p_cmd->dx_cmd_list->DrawInstanced((UINT)vertex_count, (UINT)1, (UINT)first_vertex, (UINT)0);
}

void tr_internal_dx_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index)
{
    assert(NULL != p_cmd->dx_cmd_list);

    p_cmd->dx_cmd_list->DrawIndexedInstanced((UINT)index_count, (UINT)1, (UINT)first_index, (UINT)0,
                                             (UINT)0);
}

void tr_internal_dx_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer,
                                          tr_buffer_usage old_usage, tr_buffer_usage new_usage)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != p_buffer->dx_resource.Get());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = p_buffer->dx_resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = tr_util_to_dx_resource_state_buffer(old_usage);
    barrier.Transition.StateAfter = tr_util_to_dx_resource_state_buffer(new_usage);

    p_cmd->dx_cmd_list->ResourceBarrier(1, &barrier);
}

void tr_internal_dx_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture,
                                         tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != p_texture->dx_resource.Get());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = p_texture->dx_resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = tr_util_to_dx_resource_state_texture(old_usage);
    barrier.Transition.StateAfter = tr_util_to_dx_resource_state_texture(new_usage);

    p_cmd->dx_cmd_list->ResourceBarrier(1, &barrier);
}

void tr_internal_dx_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target,
                                                 tr_texture_usage old_usage,
                                                 tr_texture_usage new_usage)
{
    assert(NULL != p_cmd->dx_cmd_list);

    if (p_render_target->sample_count > tr_sample_count_1)
    {
        if (1 == p_render_target->color_attachment_count)
        {
            tr_texture* ss_attachment = p_render_target->color_attachments[0];
            tr_texture* ms_attachment = p_render_target->color_attachments_multisample[0];

            // This means we're dealing with a multisample swapchain
            if (tr_texture_usage_present == (ss_attachment->usage & tr_texture_usage_present))
            {
                if ((tr_texture_usage_present == old_usage) &&
                    (tr_texture_usage_color_attachment == new_usage))
                {
                    tr_internal_dx_cmd_image_transition(p_cmd, ss_attachment,
                                                        tr_texture_usage_present,
                                                        tr_texture_usage_color_attachment);
                }

                if ((tr_texture_usage_color_attachment == old_usage) &&
                    (tr_texture_usage_present == new_usage))
                {
                    tr_internal_dx_cmd_image_transition(p_cmd, ss_attachment,
                                                        tr_texture_usage_color_attachment,
                                                        tr_texture_usage_present);
                }
            }
        }
    }
    else
    {
        if (1 == p_render_target->color_attachment_count)
        {
            tr_texture* attachment = p_render_target->color_attachments[0];
            // This means we're dealing with a single sample swapchain
            if (tr_texture_usage_present == (attachment->usage & tr_texture_usage_present))
            {
                if ((tr_texture_usage_present == old_usage) &&
                    (tr_texture_usage_color_attachment == new_usage))
                {
                    tr_internal_dx_cmd_image_transition(p_cmd, attachment, tr_texture_usage_present,
                                                        tr_texture_usage_color_attachment);
                }

                if ((tr_texture_usage_color_attachment == old_usage) &&
                    (tr_texture_usage_present == new_usage))
                {
                    tr_internal_dx_cmd_image_transition(p_cmd, attachment,
                                                        tr_texture_usage_color_attachment,
                                                        tr_texture_usage_present);
                }
            }
        }
    }
}

void tr_internal_dx_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target,
                                                 tr_texture_usage old_usage,
                                                 tr_texture_usage new_usage)
{
    assert(NULL != p_cmd->dx_cmd_list);

    if (p_render_target->sample_count > tr_sample_count_1)
    {
        tr_internal_dx_cmd_image_transition(
            p_cmd, p_render_target->depth_stencil_attachment_multisample, old_usage, new_usage);
    }
    else
    {
        tr_internal_dx_cmd_image_transition(p_cmd, p_render_target->depth_stencil_attachment,
                                            old_usage, new_usage);
    }
}

void tr_internal_dx_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y,
                                 uint32_t group_count_z)
{
    assert(p_cmd->dx_cmd_list != NULL);

    p_cmd->dx_cmd_list->Dispatch(group_count_x, group_count_y, group_count_z);
}

void tr_internal_dx_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height,
                                                 uint32_t row_pitch, uint64_t buffer_offset,
                                                 uint32_t mip_level, tr_buffer* p_buffer,
                                                 tr_texture* p_texture)
{
    assert(p_cmd->dx_cmd_list != NULL);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
    layout.Offset = buffer_offset;
    layout.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    layout.Footprint.Width = width;
    layout.Footprint.Height = height;
    layout.Footprint.Depth = 1;
    layout.Footprint.RowPitch = row_pitch;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = p_buffer->dx_resource.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = layout;
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = p_texture->dx_resource.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = mip_level;

    p_cmd->dx_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
}

// -------------------------------------------------------------------------------------------------
// Internal queue functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore,
                                       tr_fence* p_fence)
{
    assert(NULL != p_renderer->dx_swapchain);

    p_renderer->swapchain_image_index = p_renderer->dx_swapchain->GetCurrentBackBufferIndex();
}

void tr_internal_dx_queue_submit(tr_queue* p_queue, uint32_t cmd_count, tr_cmd** pp_cmds,
                                 uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores,
                                 uint32_t signal_semaphore_count,
                                 tr_semaphore** pp_signal_semaphores)
{
    assert(NULL != p_queue->dx_queue);

    ID3D12CommandList* cmds[tr_max_submit_cmds];
    uint32_t count = cmd_count > tr_max_submit_cmds ? tr_max_submit_cmds : cmd_count;
    for (uint32_t i = 0; i < count; ++i)
    {
        cmds[i] = pp_cmds[i]->dx_cmd_list.Get();
    }

    p_queue->dx_queue->ExecuteCommandLists(count, cmds);
}

void tr_internal_dx_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count,
                                  tr_semaphore** pp_wait_semaphores)
{
    assert(NULL != p_queue->renderer->dx_swapchain);

    UINT sync_interval = 1;
    UINT flags = 0;
    p_queue->renderer->dx_swapchain->Present(sync_interval, flags);
}

void tr_internal_dx_queue_wait_idle(tr_queue* p_queue)
{
    assert(NULL != p_queue->dx_queue);
    assert(NULL != p_queue->dx_wait_idle_fence);
    assert(NULL != p_queue->dx_wait_idle_fence_event);

    // Signal and increment the fence value
    const UINT64 fence_value = p_queue->dx_wait_idle_fence_value;
    p_queue->dx_queue->Signal(p_queue->dx_wait_idle_fence.Get(), fence_value);
    ++p_queue->dx_wait_idle_fence_value;

    // Wait until the previous frame is finished.
    const UINT64 complted_value = p_queue->dx_wait_idle_fence->GetCompletedValue();
    if (complted_value < fence_value)
    {
        p_queue->dx_wait_idle_fence->SetEventOnCompletion(fence_value,
                                                          p_queue->dx_wait_idle_fence_event);
        WaitForSingleObject(p_queue->dx_wait_idle_fence_event, INFINITE);
    }
}

DXGI_FORMAT tr_util_to_dx_format(tr_format format)
{
    DXGI_FORMAT result = DXGI_FORMAT_UNKNOWN;
    switch (format)
    {
        // 1 channel
    case tr_format_r8_unorm:
        result = DXGI_FORMAT_R8_UNORM;
        break;
    case tr_format_r16_unorm:
        result = DXGI_FORMAT_R16_UNORM;
        break;
    case tr_format_r16_float:
        result = DXGI_FORMAT_R16_FLOAT;
        break;
    case tr_format_r32_uint:
        result = DXGI_FORMAT_R32_UINT;
        break;
    case tr_format_r32_float:
        result = DXGI_FORMAT_R32_FLOAT;
        break;
        // 2 channel
    case tr_format_r8g8_unorm:
        result = DXGI_FORMAT_R8G8_UNORM;
        break;
    case tr_format_r16g16_unorm:
        result = DXGI_FORMAT_R16G16_UNORM;
        break;
    case tr_format_r16g16_float:
        result = DXGI_FORMAT_R16G16_FLOAT;
        break;
    case tr_format_r32g32_uint:
        result = DXGI_FORMAT_R32G32_UINT;
        break;
    case tr_format_r32g32_float:
        result = DXGI_FORMAT_R32G32_FLOAT;
        break;
        // 3 channel
    case tr_format_r32g32b32_uint:
        result = DXGI_FORMAT_R32G32B32_UINT;
        break;
    case tr_format_r32g32b32_float:
        result = DXGI_FORMAT_R32G32B32_FLOAT;
        break;
        // 4 channel
    case tr_format_b8g8r8a8_unorm:
        result = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;
    case tr_format_r8g8b8a8_unorm:
        result = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case tr_format_r16g16b16a16_unorm:
        result = DXGI_FORMAT_R16G16B16A16_UNORM;
        break;
    case tr_format_r16g16b16a16_float:
        result = DXGI_FORMAT_R16G16B16A16_FLOAT;
        break;
    case tr_format_r32g32b32a32_uint:
        result = DXGI_FORMAT_R32G32B32A32_UINT;
        break;
    case tr_format_r32g32b32a32_float:
        result = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;
        // Depth/stencil
    case tr_format_d16_unorm:
        result = DXGI_FORMAT_D16_UNORM;
        break;
    case tr_format_x8_d24_unorm_pack32:
        result = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
        break;
    case tr_format_d32_float:
        result = DXGI_FORMAT_D32_FLOAT;
        break;
    case tr_format_d24_unorm_s8_uint:
        result = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;
    case tr_format_d32_float_s8_uint:
        result = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        break;
    }
    return result;
}

tr_format tr_util_from_dx_format(DXGI_FORMAT format)
{
    tr_format result = tr_format_undefined;
    switch (format)
    {
        // 1 channel
    case DXGI_FORMAT_R8_UNORM:
        result = tr_format_r8_unorm;
        break;
    case DXGI_FORMAT_R16_UNORM:
        result = tr_format_r16_unorm;
        break;
    case DXGI_FORMAT_R16_FLOAT:
        result = tr_format_r16_float;
        break;
    case DXGI_FORMAT_R32_UINT:
        result = tr_format_r32_uint;
        break;
    case DXGI_FORMAT_R32_FLOAT:
        result = tr_format_r32_float;
        break;
        // 2 channel
    case DXGI_FORMAT_R8G8_UNORM:
        result = tr_format_r8g8_unorm;
        break;
    case DXGI_FORMAT_R16G16_UNORM:
        result = tr_format_r16g16_unorm;
        break;
    case DXGI_FORMAT_R16G16_FLOAT:
        result = tr_format_r16g16_float;
        break;
    case DXGI_FORMAT_R32G32_UINT:
        result = tr_format_r32g32_uint;
        break;
    case DXGI_FORMAT_R32G32_FLOAT:
        result = tr_format_r32g32_float;
        break;
        // 3 channel
    case DXGI_FORMAT_R32G32B32_UINT:
        result = tr_format_r32g32b32_uint;
        break;
    case DXGI_FORMAT_R32G32B32_FLOAT:
        result = tr_format_r32g32b32_float;
        break;
        // 4 channel
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        result = tr_format_b8g8r8a8_unorm;
        break;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        result = tr_format_r8g8b8a8_unorm;
        break;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        result = tr_format_r16g16b16a16_unorm;
        break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        result = tr_format_r16g16b16a16_float;
        break;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        result = tr_format_r32g32b32a32_uint;
        break;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        result = tr_format_r32g32b32a32_float;
        break;
        // Depth/stencil
    case DXGI_FORMAT_D16_UNORM:
        result = tr_format_d16_unorm;
        break;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        result = tr_format_x8_d24_unorm_pack32;
        break;
    case DXGI_FORMAT_D32_FLOAT:
        result = tr_format_d32_float;
        break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        result = tr_format_d24_unorm_s8_uint;
        break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        result = tr_format_d32_float_s8_uint;
        break;
    }
    return result;
}
