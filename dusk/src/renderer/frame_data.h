#pragma once

#include "dusk.h"
#include "renderer/systems/lights_system.h"

#include <volk.h>
#include <glm/glm.hpp>

namespace dusk
{
class Scene;

struct alignas(16) GlobalUbo
{
    glm::mat4 prjoection             = { 1.f };
    glm::mat4 view                   = { 1.f };
    glm::mat4 inverseView            = { 1.f };
    glm::mat4 inverseProjection      = { 1.f };

    glm::vec4 frustumPlanes[6]       = {};

    uint32_t  directionalLightsCount = 0u;
    uint32_t  pointLightsCount       = 0u;
    uint32_t  spotLightsCount        = 0u;
    uint32_t  padding                = 0u;

    alignas(16) Array<glm::uvec4, MAX_LIGHTS_PER_TYPE / 4> directionalLightIndices;
    alignas(16) Array<glm::uvec4, MAX_LIGHTS_PER_TYPE / 4> pointLightIndices;
    alignas(16) Array<glm::uvec4, MAX_LIGHTS_PER_TYPE / 4> spotLightIndices;
};

struct FrameData
{
    uint32_t         frameIndex;
    TimeStep         frameTime;
    VkCommandBuffer  commandBuffer;
    Scene*           scene;
    uint32_t         width;
    uint32_t         height;

    VkDescriptorSet& globalDescriptorSet;
    VkDescriptorSet& textureDescriptorSet;
    VkDescriptorSet& lightsDescriptorSet;
    VkDescriptorSet& materialDescriptorSet;
};
} // namespace dusk