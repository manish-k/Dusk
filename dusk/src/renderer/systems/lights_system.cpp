#include "lights_system.h"

#include "engine.h"

#include "scene/scene.h"
#include "scene/components/lights.h"
#include "scene/components/transform.h"

#include "renderer/frame_data.h"
#include "renderer/texture_db.h"

#include "backend/vulkan/vk_device.h"
#include "backend/vulkan/vk_descriptors.h"
#include "backend/vulkan/vk_renderer.h"
#include "backend/vulkan/vk_swapchain.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace dusk
{

LightsSystem* LightsSystem::s_instance = nullptr;

LightsSystem::LightsSystem()
{
    DASSERT(!s_instance, "LightsSystem instance already exists");
    s_instance = this;

    setupDescriptors();
}

LightsSystem::~LightsSystem()
{
    m_lightsDescriptorPool->resetPool();
    m_lightsDescriptorSetLayout = nullptr;
    m_lightsDescriptorPool      = nullptr;

    m_ambientLightBuffer.free();
    m_directionalLightsBuffer.free();
    m_pointLightsBuffer.free();
    m_spotLightsBuffer.free();
}

void LightsSystem::registerAllLights(Scene& scene)
{
    auto ambDescInfo = m_ambientLightBuffer.getDescriptorInfoAtIndex(0);

    m_lightsDescriptorSet->configureBuffer(
        AMBIENT_BIND_INDEX,
        0,
        1,
        &ambDescInfo);

    auto directionalLightList = scene.GetGameObjectsWith<DirectionalLightComponent>();
    for (auto& entity : directionalLightList)
    {
        auto& light = directionalLightList.get<DirectionalLightComponent>(entity);

        if (light.id != -1) continue;

        registerDirectionalLight(light);
    }

    auto pointLightsList = scene.GetGameObjectsWith<PointLightComponent>();
    for (auto& entity : pointLightsList)
    {
        auto& light = pointLightsList.get<PointLightComponent>(entity);

        if (light.id != -1) continue;

        registerPointLight(light);
    }

    auto spotLightsList = scene.GetGameObjectsWith<SpotLightComponent>();
    for (auto& entity : spotLightsList)
    {
        auto& light = spotLightsList.get<SpotLightComponent>(entity);

        if (light.id != -1) continue;

        registerSpotLight(light);
    }

    m_lightsDescriptorSet->applyConfiguration();
}

void LightsSystem::registerAmbientLight(AmbientLightComponent& light)
{
    auto descInfo = m_ambientLightBuffer.getDescriptorInfoAtIndex(0);

    m_lightsDescriptorSet->configureBuffer(
        AMBIENT_BIND_INDEX,
        0,
        1,
        &descInfo);

    m_lightsDescriptorSet->applyConfiguration();
}

void LightsSystem::registerDirectionalLight(DirectionalLightComponent& light)
{
    DASSERT(light.id == -1, "Id already registerd");
    DASSERT(m_directionalLightsCount < MAX_LIGHTS_PER_TYPE);

    light.id = m_directionalLightsCount;
    ++m_directionalLightsCount;

    auto descInfo = m_directionalLightsBuffer.getDescriptorInfoAtIndex(light.id);

    m_lightsDescriptorSet->configureBuffer(
        DIRECTIONAL_BIND_INDEX,
        light.id,
        1,
        &descInfo);

    m_lightsDescriptorSet->applyConfiguration();
}

void LightsSystem::registerPointLight(PointLightComponent& light)
{
    DASSERT(light.id == -1, "Id already registerd");
    DASSERT(m_pointLightsCount < MAX_LIGHTS_PER_TYPE);

    light.id = m_pointLightsCount;
    ++m_pointLightsCount;

    auto descInfo = m_pointLightsBuffer.getDescriptorInfoAtIndex(light.id);

    m_lightsDescriptorSet->configureBuffer(
        POINT_BIND_INDEX,
        light.id,
        1,
        &descInfo);

    m_lightsDescriptorSet->applyConfiguration();
}

void LightsSystem::registerSpotLight(SpotLightComponent& light)
{
    DASSERT(light.id == -1, "Id already registerd");
    DASSERT(m_spotLightsCount < MAX_LIGHTS_PER_TYPE);

    light.id = m_spotLightsCount;
    ++m_spotLightsCount;

    auto descInfo = m_spotLightsBuffer.getDescriptorInfoAtIndex(light.id);

    m_lightsDescriptorSet->configureBuffer(
        SPOT_BIND_INDEX,
        light.id,
        1,
        &descInfo);

    m_lightsDescriptorSet->applyConfiguration();
}

void LightsSystem::updateLights(Scene& scene, GlobalUbo& ubo)
{
    auto ambientLightList = scene.GetGameObjectsWith<AmbientLightComponent>();
    for (auto& entity : ambientLightList)
    {
        auto& light = ambientLightList.get<AmbientLightComponent>(entity);

        m_ambientLightBuffer.writeAndFlush(0, &light, sizeof(AmbientLightComponent));
    }

    auto     directionalLightList = scene.GetGameObjectsWith<DirectionalLightComponent>();
    uint32_t counter              = 0u;
    for (auto& entity : directionalLightList)
    {
        auto& light = directionalLightList.get<DirectionalLightComponent>(entity);

        if (light.id == -1) continue;

        // ortho projection for dir lights
        glm::mat4 projection = glm::orthoRH_ZO(-10.0f, 10.0f, 10.0f, -10.0f, 0.1f, 1000.0f);
        projection[1][1] *= -1; // flip Y for Vulkan

        glm::vec3 lightPos   = glm::vec3(0.f) - light.direction * 10.0f;
        glm::mat4 view       = glm::lookAtRH(
            lightPos,
            glm::vec3(0.f),
            glm::vec3(0.0f, 1.0f, 0.0f));

        light.projView = projection * view;

        m_directionalLightsBuffer.writeAndFlushAtIndex(light.id, &light, sizeof(DirectionalLightComponent));

        ubo.directionalLightIndices[counter / 4][counter % 4] = light.id;
        ++counter;
    }
    ubo.directionalLightsCount = m_directionalLightsCount;

    // TODO: maybe not the efficient data oriented desgin
    auto pointLightList = scene.GetGameObjectsWith<PointLightComponent, TransformComponent>();
    counter             = 0u;
    for (auto& entity : pointLightList)
    {
        auto& light     = pointLightList.get<PointLightComponent>(entity);
        auto& transform = pointLightList.get<TransformComponent>(entity);

        light.position  = transform.translation;

        if (light.id == -1) continue;

        m_pointLightsBuffer.writeAndFlushAtIndex(light.id, &light, sizeof(PointLightComponent));

        ubo.pointLightIndices[counter / 4][counter % 4] = light.id;
        ++counter;
    }
    ubo.pointLightsCount = m_pointLightsCount;

    auto spotLightList   = scene.GetGameObjectsWith<SpotLightComponent, TransformComponent>();
    counter              = 0u;
    for (auto& entity : spotLightList)
    {
        auto& light     = spotLightList.get<SpotLightComponent>(entity);
        auto& transform = pointLightList.get<TransformComponent>(entity);

        light.position  = transform.translation;

        if (light.id == -1) continue;

        m_spotLightsBuffer.writeAndFlushAtIndex(light.id, &light, sizeof(SpotLightComponent));

        ubo.spotLightIndices[counter / 4][counter % 4] = light.id;
        ++counter;
    }
    ubo.spotLightsCount = m_spotLightsCount;
}

void LightsSystem::setupDescriptors()
{
    VulkanContext ctx      = VkGfxDevice::getSharedVulkanContext();

    m_lightsDescriptorPool = VkGfxDescriptorPool::Builder(ctx)
                                 .addPoolSize(
                                     VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                     MAX_LIGHTS_PER_TYPE)
                                 .setDebugName("lights_desc_pool")
                                 .build(
                                     1,
                                     VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);

    m_lightsDescriptorSetLayout = VkGfxDescriptorSetLayout::Builder(ctx)
                                      .addBinding(
                                          AMBIENT_BIND_INDEX,
                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                          1) // Ambient light
                                      .addBinding(
                                          DIRECTIONAL_BIND_INDEX,
                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                          MAX_LIGHTS_PER_TYPE,
                                          true) // Directional Light
                                      .addBinding(
                                          POINT_BIND_INDEX,
                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                          MAX_LIGHTS_PER_TYPE,
                                          true) // Point Light
                                      .addBinding(
                                          SPOT_BIND_INDEX,
                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                          MAX_LIGHTS_PER_TYPE,
                                          true) // Spot Light
                                      .setDebugName("lights_desc_set_layout")
                                      .build();

    m_lightsDescriptorSet = m_lightsDescriptorPool->allocateDescriptorSet(*m_lightsDescriptorSetLayout, "lights_desc_set");

    // create ambient light buffer
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(AmbientLightComponent),
        1,
        "ambient_light_buffer",
        &m_ambientLightBuffer);

    // create directional light buffer
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(DirectionalLightComponent),
        MAX_LIGHTS_PER_TYPE,
        "directional_lights_buffer",
        &m_directionalLightsBuffer);

    // create point light buffer
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(PointLightComponent),
        MAX_LIGHTS_PER_TYPE,
        "point_lights_buffer",
        &m_pointLightsBuffer);

    // create spot light buffer
    GfxBuffer::createHostWriteBuffer(
        GfxBufferUsageFlags::StorageBuffer,
        sizeof(SpotLightComponent),
        MAX_LIGHTS_PER_TYPE,
        "spot_lights_buffer",
        &m_spotLightsBuffer);
}

} // namespace dusk