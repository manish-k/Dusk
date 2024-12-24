#pragma once

#include "vk.h"

#include <volk.h>
#include <glm/glm.hpp>

namespace dusk
{
namespace vkdebug
{
/**
 * @brief Begin labeling the commands in command buffer.
 * @param cb command buffer
 * @param labelName label name
 * @param color label color
 */
void cmdBeginLabel(VkCommandBuffer cb, const char* labelName, glm::vec4 color);

/**
 * @brief Insert label for the next command in command buffer.
 * @param cb command buffer
 * @param labelName label name
 * @param color label color
 */
void cmdInsertLabel(VkCommandBuffer cb, const char* labelName, glm::vec4 color);

/**
 * @brief
 * @param cb end labeling of the commands in command buffer. Must be called after cmdBeginLabel.
 */
void cmdEndLabel(VkCommandBuffer cb);

/**
 * @brief Begin labeling the commands in the queue.
 * @param queue queue handle
 * @param labelName label name
 * @param color label color
 */
void queueBeginLabel(VkQueue queue, const char* labelName, glm::vec4 color);

/**
 * @brief Insert label for the next command in the queue.
 * @param queue queue handle
 * @param labelName label name
 * @param color label color
 */
void queueInsertLabel(VkQueue queue, const char* labelName, glm::vec4 color);

/**
 * @brief end labeling of the commands in the queue. Must be called after queueBeginLabel.
 */
void queueEndLabel(VkQueue queue);

/**
 * @brief Add name to the vulkan object via it's handle.
 * @param device logical device
 * @param objectType type of the object
 * @param objectHandle handle of the object
 * @param objectName name for the object
 */
void setObjectName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* objectName);

/**
 * @brief Add arbitrary data to the vulkan object via it's handle.
 * @param device logical device
 * @param objectType type of the object
 * @param objectHandle handle of the object
 * @param tagId id/name for the tag
 * @param tag struct of the data to add
 * @param tagSize size of the struct
 */
void setObjectTag(VkDevice device, VkObjectType objectType, uint64_t objectHandle, uint64_t tagId, void* tag, size_t tagSize);
} // namespace vkdebug
} // namespace dusk