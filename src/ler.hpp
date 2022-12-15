//
// Created by loulfy on 14/12/2022.
//

#ifndef MINIMALRT_LER_HPP
#define MINIMALRT_LER_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <map>
#include <list>
#include <mutex>
#include <memory>
#include <limits>
#include <iostream>
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

namespace ler
{
    struct Buffer
    {
        vk::Buffer handle;
        vk::BufferCreateInfo info;
        vma::Allocation allocation;

        [[nodiscard]] uint32_t length() const { return info.size; }
    };

    using BufferPtr = std::shared_ptr<Buffer>;

    struct Texture
    {
        vk::Image handle;
        vk::ImageCreateInfo info;
        vk::UniqueImageView view;
        vma::Allocation allocation;
    };

    using TexturePtr = std::shared_ptr<Texture>;

    struct SwapChain
    {
        vk::UniqueSwapchainKHR handle;
        vk::Format format = vk::Format::eB8G8R8A8Unorm;
        vk::Extent2D extent;
    };

    struct RenderPass
    {
        vk::UniqueRenderPass handle;
        std::vector<vk::AttachmentDescription2> attachments;
    };

    struct Instance
    {
        alignas(16) glm::mat4 model = glm::mat4(1.f);
        alignas(16) glm::vec3 bMin = glm::vec3(0.f);
        alignas(16) glm::vec3 bMax = glm::vec3(0.f);
        alignas(4) glm::uint matId = 0;
    };

    struct Material
    {
        alignas(4) glm::uint texId = 0;
        alignas(4) glm::uint norId = 0;
        alignas(16) glm::vec3 color = glm::vec3(1.f);
    };

    struct MeshIndirect
    {
        uint32_t countIndex = 0;
        uint32_t firstIndex = 0;
        uint32_t countVertex = 0;
        uint32_t firstVertex = 0;
        uint32_t materialId = 0;
        glm::vec3 bMin = glm::vec3(0.f);
        glm::vec3 bMax = glm::vec3(0.f);
    };

    struct Scene
    {
        Buffer staging;
        Buffer indexBuffer;
        Buffer vertexBuffer;
        Buffer normalBuffer;
        Buffer tangentBuffer;
        Buffer texcoordBuffer;
        Buffer indirectBuffer;
        Buffer instanceBuffer;
        Buffer materialBuffer;

        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t drawCount = 0;

        std::vector<Texture> textures;
        std::vector<Instance> instances;
        std::vector<Material> materials;
        std::vector<MeshIndirect> geometries;
        std::vector<vk::DrawIndexedIndirectCommand> commands;
    };

    struct LerSettings
    {
        vk::Instance instance;
        vk::PhysicalDevice physicalDevice;
        vk::Device device;
        uint32_t graphicsQueueFamily = UINT32_MAX;
    };

    class LerContext
    {
    public:

        ~LerContext();
        explicit LerContext(const LerSettings& settings);

        // Buffer
        Buffer createBuffer(uint32_t byteSize, vk::BufferUsageFlags usages = vk::BufferUsageFlagBits());
        void uploadBuffer(Buffer& staging, const void* src, uint32_t byteSize);
        void copyBuffer(Buffer& staging, Buffer& dst, uint64_t byteSize = VK_WHOLE_SIZE);
        void copyBufferToTexture(vk::CommandBuffer& cmd, const Buffer& buffer, const TexturePtr& texture);

        // Texture
        TexturePtr createTexture(vk::Format format, const vk::Extent2D& extent, vk::SampleCountFlagBits sampleCount, bool isRenderTarget = false);
        TexturePtr createTextureFromNative(vk::Image image, vk::Format format, const vk::Extent2D& extent);
        uint32_t loadTextureFromFile(const fs::path& path);
        uint32_t loadTextureFromMemory(const unsigned char* buffer, uint32_t size);

        SwapChain createSwapChain(vk::SurfaceKHR surface, uint32_t width, uint32_t height, bool vSync = true);

        // RenderPass
        RenderPass createDefaultRenderPass(vk::Format surfaceFormat);
        std::vector<vk::UniqueFramebuffer> createFrameBuffers(const RenderPass& renderPass, const SwapChain& swapChain);

        // Scene
        Scene fromFile(const fs::path& path);
        static void transformBoundingBox(const glm::mat4& t, glm::vec3& min, glm::vec3& max);

        // Execution
        vk::CommandBuffer getCommandBuffer();
        void submitAndWait(vk::CommandBuffer& cmd);

    private:

        void mergeSceneBuffer(Scene& scene, Buffer& dest, const aiScene* aiScene, const std::function<bool(aiMesh*)>& predicate, const std::function<void*(aiMesh*)>& provider);
        static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes, bool vSync);
        static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
        vk::Format chooseDepthFormat();
        uint32_t loadTexture(const aiScene* aiScene, const aiString& filename, const fs::path& path);

        std::mutex m_mutex;
        vk::PhysicalDevice m_physicalDevice;
        vk::Device m_device;
        vk::Queue m_queue;
        vma::Allocator m_allocator;
        vk::UniqueCommandPool m_commandPool;
        std::list<vk::CommandBuffer> m_commandBuffersPool;

        std::vector<TexturePtr> m_textures;
        std::map<std::string, uint32_t> m_cache;
    };
}

#endif //MINIMALRT_LER_HPP
