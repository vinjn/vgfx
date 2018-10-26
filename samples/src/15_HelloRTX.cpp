#include "GLFW/glfw3.h"
#if defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3native.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "glm/glm.hpp"
#include "vgfx.h"

using glm::mat4;
using glm::vec3;
using std::string;
using std::vector;

const char* k_app_name = "15_HelloRTX";
const uint32_t k_image_count = 3;
const std::string k_asset_dir = "../samples/assets/";

tr_renderer* m_renderer = nullptr;
tr_cmd_pool* m_cmd_pool = nullptr;
tr_cmd** m_cmds = nullptr;
tr_shader_program* m_shader = nullptr;
tr_buffer* m_tri_vertex_buffer = nullptr;
tr_buffer* m_tri_index_buffer = nullptr;
tr_pipeline* m_pipeline = nullptr;

uint32_t s_window_width;
uint32_t s_window_height;
uint64_t s_frame_count = 0;

#if defined(TINY_RENDERER_DX)

struct AccelerationStructureBuffers
{
    ID3D12ResourcePtr pScratch;
    ID3D12ResourcePtr pResult;
    ID3D12ResourcePtr pInstanceDesc; // Used only for top-level AS
};

static const D3D12_HEAP_PROPERTIES kUploadHeapProps = {
    D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0,
};

static const D3D12_HEAP_PROPERTIES kDefaultHeapProps = {
    D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0};

ID3D12ResourcePtr createBuffer(ID3D12DevicePtr pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags,
                               D3D12_RESOURCE_STATES initState,
                               const D3D12_HEAP_PROPERTIES& heapProps)
{
    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Alignment = 0;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Flags = flags;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.Height = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufDesc.MipLevels = 1;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.SampleDesc.Quality = 0;
    bufDesc.Width = size;

    ID3D12ResourcePtr pBuffer;
    pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr,
                                     IID_PPV_ARGS(&pBuffer));
    return pBuffer;
}

AccelerationStructureBuffers createBottomLevelAS(ID3D12DevicePtr pDevice,
                                                 ID3D12GraphicsCommandListPtr pCmdList,
                                                 ID3D12ResourcePtr pVB)
{
    D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
    geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geomDesc.Triangles.VertexBuffer.StartAddress = pVB->GetGPUVirtualAddress();
    geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(vec3);
    geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geomDesc.Triangles.VertexCount = 3;
    geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get the size requirements for the scratch and AS buffers
    D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC prebuildDesc = {};
    prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    prebuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    prebuildDesc.NumDescs = 1;
    prebuildDesc.pGeometryDescs = &geomDesc;
    prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    ID3D12DeviceRaytracingPrototypePtr pRtDevice = pDevice;
    pRtDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

    // Create the buffers. They need to support UAV, and since we are going to immediately use them,
    // we create them with an unordered-access state
    AccelerationStructureBuffers buffers;
    buffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
    buffers.pResult = createBuffer(
        pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);

    // Create the bottom-level AS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
    asDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    asDesc.pGeometryDescs = &geomDesc;
    asDesc.DestAccelerationStructureData.StartAddress = buffers.pResult->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData.SizeInBytes = info.ResultDataMaxSizeInBytes;

    asDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    asDesc.NumDescs = 1;
    asDesc.ScratchAccelerationStructureData.StartAddress = buffers.pScratch->GetGPUVirtualAddress();
    asDesc.ScratchAccelerationStructureData.SizeInBytes = info.ScratchDataSizeInBytes;

    asDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    ID3D12CommandListRaytracingPrototypePtr pRtCmdList = pCmdList;
    pRtCmdList->BuildRaytracingAccelerationStructure(&asDesc);

    // We need to insert a UAV barrier before using the acceleration structures in a raytracing
    // operation
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = buffers.pResult;
    pCmdList->ResourceBarrier(1, &uavBarrier);

    return buffers;
}

AccelerationStructureBuffers createTopLevelAS(ID3D12DevicePtr pDevice,
                                              ID3D12GraphicsCommandListPtr pCmdList,
                                              ID3D12ResourcePtr pBottomLevelAS, uint64_t& tlasSize)
{
    // First, get the size of the TLAS buffers and create them
    D3D12_GET_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO_DESC prebuildDesc = {};
    prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    prebuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    prebuildDesc.NumDescs = 1;
    prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    ID3D12DeviceRaytracingPrototypePtr pRtDevice = pDevice;
    pRtDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

    // Create the buffers
    AccelerationStructureBuffers buffers;
    buffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes,
                                    D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
    buffers.pResult = createBuffer(
        pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
    tlasSize = info.ResultDataMaxSizeInBytes;

    // The instance desc should be inside a buffer, create and map the buffer
    buffers.pInstanceDesc =
        createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_RESOURCE_FLAG_NONE,
                     D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
    D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
    buffers.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);

    // Initialize the instance desc. We only have a single instance
    pInstanceDesc->InstanceID = 0; // This value will be exposed to the shader via InstanceID()
    pInstanceDesc->InstanceContributionToHitGroupIndex =
        0; // This is the offset inside the shader-table. We only have a single geometry, so the
           // offset 0
    pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    mat4 m; // Identity matrix
    memcpy(pInstanceDesc->Transform, &m, sizeof(pInstanceDesc->Transform));
    pInstanceDesc->AccelerationStructure = pBottomLevelAS->GetGPUVirtualAddress();
    pInstanceDesc->InstanceMask = 0xFF;

    // Unmap
    buffers.pInstanceDesc->Unmap(0, nullptr);

    // Create the TLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
    asDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    asDesc.DestAccelerationStructureData.StartAddress = buffers.pResult->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData.SizeInBytes = info.ResultDataMaxSizeInBytes;

    asDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    asDesc.InstanceDescs = buffers.pInstanceDesc->GetGPUVirtualAddress();
    asDesc.NumDescs = 1;
    asDesc.ScratchAccelerationStructureData.StartAddress = buffers.pScratch->GetGPUVirtualAddress();
    asDesc.ScratchAccelerationStructureData.SizeInBytes = info.ScratchDataSizeInBytes;

    asDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    ID3D12CommandListRaytracingPrototypePtr pRtList = pCmdList;
    pRtList->BuildRaytracingAccelerationStructure(&asDesc);

    // We need to insert a UAV barrier before using the acceleration structures in a raytracing
    // operation
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = buffers.pResult;
    pCmdList->ResourceBarrier(1, &uavBarrier);

    return buffers;
}

ID3D12ResourcePtr mpVertexBuffer;
ID3D12ResourcePtr mpTopLevelAS;
ID3D12ResourcePtr mpBottomLevelAS;
uint64_t mTlasSize = 0;

void createAccelerationStructures()
{
#if 0
    //mpVertexBuffer = createTriangleVB(mpDevice);
    AccelerationStructureBuffers bottomLevelBuffers = createBottomLevelAS(mpDevice, mpCmdList, mpVertexBuffer);
    AccelerationStructureBuffers topLevelBuffers = createTopLevelAS(mpDevice, mpCmdList, bottomLevelBuffers.pResult, mTlasSize);

    // Store the AS buffers. The rest of the buffers will be released once we exit the function
    mpTopLevelAS = topLevelBuffers.pResult;
    mpBottomLevelAS = bottomLevelBuffers.pResult;
#endif
}

#else

struct AccelerationStructureBuffers
{
    VkBuffer scratchBuffer = VK_NULL_HANDLE;
    VkDeviceMemory scratchMem = VK_NULL_HANDLE;
    VkBuffer resultBuffer = VK_NULL_HANDLE;
    VkDeviceMemory resultMem = VK_NULL_HANDLE;
    VkBuffer instancesBuffer = VK_NULL_HANDLE;
    VkDeviceMemory instancesMem = VK_NULL_HANDLE;
    VkAccelerationStructureNVX structure = VK_NULL_HANDLE;
};

VkDeviceSize GetScratchBufferSize(VkAccelerationStructureNVX handle)
{
    VkMemoryRequirements2 scratchMemReq;
    {
        VkAccelerationStructureMemoryRequirementsInfoNVX memoryRequirementsInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NVX };
        memoryRequirementsInfo.accelerationStructure = handle;
        tr_get_renderer().vkGetAccelerationStructureScratchMemoryRequirementsNVX(tr_get_renderer().vk_device, &memoryRequirementsInfo, &scratchMemReq);
    }
    return scratchMemReq.memoryRequirements.size;
};

AccelerationStructureBuffers CreateAccelerationStructure(VkAccelerationStructureTypeNVX type, vector<VkGeometryNVX> geometries, uint32_t instanceCount)
{
    AccelerationStructureBuffers newAS;
    VkResult code = VK_SUCCESS;

    // structure
    VkAccelerationStructureCreateInfoNVX structureInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NVX };
    {
        structureInfo.type = type;
        structureInfo.flags = 0;
        structureInfo.compactedSize = 0;
        structureInfo.instanceCount = instanceCount;
        structureInfo.geometryCount = geometries.size();
        structureInfo.pGeometries = geometries.data();

        code = tr_get_renderer().vkCreateAccelerationStructureNVX(tr_get_renderer().vk_device,
            &structureInfo, nullptr, &newAS.structure);
        assert(code == VK_SUCCESS);
    }

    // resultMem
    VkMemoryAllocateInfo resultMemInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    {
        VkMemoryRequirements2 ASMemReq;
        VkAccelerationStructureMemoryRequirementsInfoNVX memoryRequirementsInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NVX };
        memoryRequirementsInfo.accelerationStructure = newAS.structure;
        tr_get_renderer().vkGetAccelerationStructureMemoryRequirementsNVX(tr_get_renderer().vk_device,
            &memoryRequirementsInfo, &ASMemReq);

        resultMemInfo.allocationSize = ASMemReq.memoryRequirements.size;
        tr_util_vk_get_memory_type(ASMemReq.memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &resultMemInfo.memoryTypeIndex);
        code = vkAllocateMemory(tr_get_renderer().vk_device, &resultMemInfo, nullptr, &newAS.resultMem);
        assert(code == VK_SUCCESS);
    }

    // bind structure to resultMem
    VkBindAccelerationStructureMemoryInfoNVX bindInfo = { VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NVX };
    {
        bindInfo.accelerationStructure = newAS.structure;
        bindInfo.memory = newAS.resultMem;
        bindInfo.memoryOffset = 0;
        bindInfo.deviceIndexCount = 0;
        bindInfo.pDeviceIndices = nullptr;

        code = tr_get_renderer().vkBindAccelerationStructureMemoryNVX(tr_get_renderer().vk_device, 1, &bindInfo);
        assert(code == VK_SUCCESS);
    }

    // scratch

    // build AS
    //vkCmdBuildAccelerationStructureNV(cmdBuf, VK_TOP_LEVEL_ACCELERATION_STRUCTURE_NV, tlas, instances, scratchBuf)

    //    vkCmdPipelineBarrier()

    return newAS;
};

#endif

void init_tiny_renderer(GLFWwindow* window)
{
    vector<string> instance_layers = {
#if defined(_DEBUG)
        "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    vector<string> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                        VK_NVX_RAYTRACING_EXTENSION_NAME};

    int width = 0;
    int height = 0;
    glfwGetWindowSize(window, &width, &height);
    s_window_width = (uint32_t)width;
    s_window_height = (uint32_t)height;

    tr_renderer_settings settings = {0};
#if defined(TINY_RENDERER_DX)
    settings.api = tr_api_d3d12;
#endif

#if defined(__linux__)
    settings.handle.connection = XGetXCBConnection(glfwGetX11Display());
    settings.handle.window = glfwGetX11Window(window);
#elif defined(_WIN32)
    settings.handle.hinstance = ::GetModuleHandle(NULL);
    settings.handle.hwnd = glfwGetWin32Window(window);
#endif
    settings.width = s_window_width;
    settings.height = s_window_height;
    settings.swapchain.image_count = k_image_count;
    settings.swapchain.sample_count = tr_sample_count_8;
    settings.swapchain.color_format = tr_format_b8g8r8a8_unorm;
    settings.swapchain.depth_stencil_format = tr_format_undefined;
    settings.log_fn = renderer_log;
#if defined(TINY_RENDERER_VK)
    settings.vk_debug_fn = vulkan_debug;
    settings.instance_layers = instance_layers;
    settings.device_extensions = device_extensions;
#endif
    tr_create_renderer(k_app_name, &settings, &m_renderer);

    tr_create_cmd_pool(m_renderer, m_renderer->graphics_queue, false, &m_cmd_pool);
    tr_create_cmd_n(m_cmd_pool, false, k_image_count, &m_cmds);

#if defined(TINY_RENDERER_VK)

    // Uses HLSL source
    auto vert = load_file(k_asset_dir + "simple.vs.spv");
    auto frag = load_file(k_asset_dir + "simple.ps.spv");
#elif defined(TINY_RENDERER_DX)
    auto vert = load_file(k_asset_dir + "simple.hlsl");
    auto frag = vert;
#endif
    tr_create_shader_program(m_renderer, (uint32_t)vert.size(), (uint32_t*)(vert.data()), "VSMain",
                             (uint32_t)frag.size(), (uint32_t*)(frag.data()), "PSMain", &m_shader);

    tr_vertex_layout vertex_layout = {};
    vertex_layout.attrib_count = 1;
    vertex_layout.attribs[0].semantic = tr_semantic_position;
    vertex_layout.attribs[0].format = tr_format_r32g32b32a32_float;
    vertex_layout.attribs[0].binding = 0;
    vertex_layout.attribs[0].location = 0;
    vertex_layout.attribs[0].offset = 0;
    tr_pipeline_settings pipeline_settings = {tr_primitive_topo_tri_list};
    tr_create_pipeline(m_renderer, m_shader, &vertex_layout, nullptr,
                       m_renderer->swapchain_render_targets[0], &pipeline_settings, &m_pipeline);

    // tri
    {
        std::vector<float> vertexData = {
            0.00f, 0.25f, 0.0f, 1.0f, -0.25f, -0.25f, 0.0f, 1.0f, 0.25f, -0.25f, 0.0f, 1.0f,
        };

        vertexData[4 * 0 + 0] += -0.5f;
        vertexData[4 * 1 + 0] += -0.5f;
        vertexData[4 * 2 + 0] += -0.5f;

        uint64_t vertexDataSize = sizeof(vertexData[0]) * vertexData.size();
        uint32_t vertexStride = sizeof(float) * 4;
        tr_create_vertex_buffer(m_renderer, vertexDataSize, true, vertexStride,
                                &m_tri_vertex_buffer);
        memcpy(m_tri_vertex_buffer->cpu_mapped_address, vertexData.data(), vertexDataSize);

        std::vector<uint16_t> indexData = { 0, 1, 2 };

        uint64_t indexDataSize = sizeof(indexData[0]) * indexData.size();
        tr_create_index_buffer(m_renderer, indexDataSize, true, tr_index_type_uint16,
            &m_tri_index_buffer);
        memcpy(m_tri_index_buffer->cpu_mapped_address, indexData.data(), indexDataSize);
    }

#if defined(TINY_RENDERER_VK)
    std::vector<VkGeometryNVX> geometries;

#endif
}

void destroy_tiny_renderer() { tr_destroy_renderer(m_renderer); }

void draw_frame()
{
    uint32_t frameIdx = s_frame_count % m_renderer->settings.swapchain.image_count;

    tr_fence* image_acquired_fence = m_renderer->image_acquired_fences[frameIdx];
    tr_semaphore* image_acquired_semaphore = m_renderer->image_acquired_semaphores[frameIdx];
    tr_semaphore* render_complete_semaphores = m_renderer->render_complete_semaphores[frameIdx];

    tr_acquire_next_image(m_renderer, image_acquired_semaphore, image_acquired_fence);

    uint32_t swapchain_image_index = m_renderer->swapchain_image_index;
    tr_render_target* render_target = m_renderer->swapchain_render_targets[swapchain_image_index];

    tr_cmd* cmd = m_cmds[frameIdx];

    tr_begin_cmd(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_present,
                                    tr_texture_usage_color_attachment);
    tr_cmd_set_viewport(cmd, 0, 0, (float)s_window_width, (float)s_window_height, 0.0f, 1.0f);
    tr_cmd_set_scissor(cmd, 0, 0, s_window_width, s_window_height);
    tr_cmd_begin_render(cmd, render_target);
    tr_clear_value clear_value = {0.0f, 0.0f, 0.0f, 0.0f};
    tr_cmd_clear_color_attachment(cmd, 0, &clear_value);
    tr_cmd_bind_pipeline(cmd, m_pipeline);
    tr_cmd_bind_vertex_buffers(cmd, 1, &m_tri_vertex_buffer);
    tr_cmd_draw(cmd, 3, 0);
    tr_cmd_end_render(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_color_attachment,
                                    tr_texture_usage_present);
    tr_end_cmd(cmd);

    tr_queue_submit(m_renderer->graphics_queue, 1, &cmd, 1, &image_acquired_semaphore, 1,
                    &render_complete_semaphores);
    tr_queue_present(m_renderer->present_queue, 1, &render_complete_semaphores);

    tr_queue_wait_idle(m_renderer->graphics_queue);
}

int main(int argc, char** argv)
{
    glfwSetErrorCallback(app_glfw_error);
    if (!glfwInit())
    {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, k_app_name, NULL, NULL);
    init_tiny_renderer(window);

    while (!glfwWindowShouldClose(window))
    {
        draw_frame();
        glfwPollEvents();
    }

    destroy_tiny_renderer();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
