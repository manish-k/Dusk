#include "vk_debug.h"

namespace dusk
{
namespace vkdebug
{

void cmdBeginLabel(VkCommandBuffer cb, const char* labelName, glm::vec4 color)
{
#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    label.pLabelName           = labelName;
    memcpy(label.color, &color, sizeof(float) * 4);
    vkCmdBeginDebugUtilsLabelEXT(cb, &label);
#endif
}

void cmdInsertLabel(VkCommandBuffer cb, const char* labelName, glm::vec4 color)
{
#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    label.pLabelName           = labelName;
    memcpy(label.color, &color, sizeof(float) * 4);
    vkCmdInsertDebugUtilsLabelEXT(cb, &label);
#endif
}

void cmdEndLabel(VkCommandBuffer cb)
{
#ifdef VK_RENDERER_DEBUG
    vkCmdEndDebugUtilsLabelEXT(cb);
#endif
}

void queueBeginLabel(VkQueue queue, const char* labelName, glm::vec4 color)
{
#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    label.pLabelName           = labelName;
    memcpy(label.color, &color, sizeof(float) * 4);
    vkQueueBeginDebugUtilsLabelEXT(queue, &label);
#endif
}

void queueInsertLabel(VkQueue queue, const char* labelName, glm::vec4 color)
{
#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
    label.pLabelName           = labelName;
    memcpy(label.color, &color, sizeof(float) * 4);
    vkQueueInsertDebugUtilsLabelEXT(queue, &label);
#endif
}

void queueEndLabel(VkQueue queue)
{
#ifdef VK_RENDERER_DEBUG
    vkQueueEndDebugUtilsLabelEXT(queue);
#endif
}

void setObjectName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* objectName)
{
#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType                    = objectType;
    nameInfo.objectHandle                  = objectHandle;
    nameInfo.pObjectName                   = objectName;
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
#endif
}

void setObjectTag(VkDevice device, VkObjectType objectType, uint64_t objectHandle, uint64_t tagId, void* tag, size_t tagSize)
{
#ifdef VK_RENDERER_DEBUG
    VkDebugUtilsObjectTagInfoEXT tagInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT };
    tagInfo.objectType                   = objectType;
    tagInfo.objectHandle                 = objectHandle;
    tagInfo.tagName                      = tagId;
    tagInfo.tagSize                      = tagSize;
    tagInfo.pTag                         = tag;
    vkSetDebugUtilsObjectTagEXT(device, &tagInfo);
#endif
}

} // namespace vkdebug
} // namespace dusk