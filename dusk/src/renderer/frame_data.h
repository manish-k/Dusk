#pragma once

#include "scene/scene.h"
#include "backend/vulkan/vk.h"

namespace dusk
{
// already aligned
struct GlobalUbo
{
    glm::mat4 prjoection { 1.f };
    glm::mat4 view { 1.f };
    glm::mat4 inverseView { 1.f };
    glm::vec4 lightDirection { 0.f };                    // w will always be zero
    glm::vec4 ambientLightColor { 1.f, 1.f, 1.f, 0.2f }; // w is intensity

    // padding for 64b alignment
    glm::vec4 unused0 {};
    glm::vec4 unused1 {};
};

struct FrameData
{
    uint32_t         frameIndex;
    TimeStep         frameTime;
    VkCommandBuffer  commandBuffer;
    Scene&           scene;
    VkDescriptorSet& globalDescriptorSet;
    VkDescriptorSet& materialDescriptorSet;
};
} // namespace dusk