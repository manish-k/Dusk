#pragma once

#include "dusk.h"
#include "vk.h"
#include "vk_types.h"

#include <volk/volk.h>
#include <string>

namespace dusk
{

namespace vulkan
{
/**
 * @brief Creates and initializes a VulkanGPUAllocator using VMA. The allocator is responsible for managing GPU memory allocations for buffers and images.
 * @param VulkanContext
 * @param Pointer to the VulkanGPUAllocator to be created and initialized.
 * @return Result of the allocator creation.
 */
VulkanResult createGPUAllocator(
    const VulkanContext& context,
    VulkanGPUAllocator*  pOutGpuAllocator);

/**
 * @brief Destroys the VulkanGPUAllocator and releases all associated resources.
 * @param Pointer to the VulkanGPUAllocator to be destroyed .
 */
void destroyGPUAllocator(VulkanGPUAllocator* pOutGpuAllocator);

/**
 * @brief Allocates a GPU buffer using the provided VulkanGPUAllocator, buffer creation info, memory usage, and allocation flags. The allocated buffer is returned through the pBufferResult parameter.
 * @param VulkanGPUAllocator pointer used for allocating the buffer
 * @param Buffer creation info structure containing parameters for buffer creation
 * @param Usage pattern for the buffer memory (e.g., VMA_MEMORY_USAGE_AUTO, VMA_MEMORY_USAGE_GPU_ONLY, etc.)
 * @param VMA Flags that specify additional allocation behavior (e.g., VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_ALLOCATION_CREATE_MAPPED_BIT, etc.)
 * @param VulkanGfxBuffer pointer where the allocated buffer information will be stored, including the Vulkan buffer handle, allocation handle, mapped memory pointer (if applicable), size in bytes, and memory property flags
 * @return Result of the buffer allocation.
 */
VulkanResult allocateGPUBuffer(
    VulkanGPUAllocator*       pGpuAllocator,
    const VkBufferCreateInfo& bufferCreateInfo,
    VmaMemoryUsage            usage,
    VmaAllocationCreateFlags  flags,
    VulkanGfxBuffer*          pBufferResult);

/**
 * @brief Frees a previously allocated GPU buffer, releasing the associated Vulkan buffer and VMA allocation.
 * @param Pointer to the VulkanGPUAllocator used for freeing the buffer.
 * @param Pointerto the VulkanGfxBuffer containing the buffer and allocation information to be freed.
 */
void freeGPUBuffer(
    VulkanGPUAllocator* pGpuAllocator,
    VulkanGfxBuffer*    pGfxBuffer);

/**
 * @brief Maps the memory of a GPU allocation to a host-accessible pointer, allowing the CPU to read from or write to the allocated GPU memory. The mapped pointer is returned through the ppMappedBlock parameter.
 * @param Pointerto the VulkanGPUAllocator used for mapping the memory.
 * @param Allocationhandle representing the GPU memory allocation to be mapped.
 * @param Pointer to the host memory block address.
 * @return
 */
VulkanResult mapGPUMemory(
    VulkanGPUAllocator* pGpuAllocator,
    VmaAllocation       allocation,
    void**              ppMappedBlock);

/**
 * @brief Unmaps a previously mapped GPU memory allocation.
 * @param Pointerto the VulkanGPUAllocator used for unmapping the memory.
 * @param Allocationhandle representing the GPU memory allocation to be unmapped.
 */
void unmapGPUMemory(
    VulkanGPUAllocator* pGpuAllocator,
    VmaAllocation       allocation);

/**
 * @brief Allocates a GPU image using the provided VulkanGPUAllocator, image creation info, memory usage, and allocation flags. The allocated image is returned through the pImageResult parameter.
 * @param Pointerto the VulkanGPUAllocator used for allocating the image.
 * @param Image creation info structure containing parameters for image creation.
 * @param Usage pattern for the image memory (e.g., VMA_MEMORY_USAGE_AUTO, VMA_MEMORY_USAGE_GPU_ONLY, etc.)
 * @param Flags that specify additional allocation behavior (e.g., VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_ALLOCATION_CREATE_MAPPED_BIT, etc.)
 * @param Pointer to the VulkanGfxImage where the allocated image information will be stored.
 * @return Result of the image allocation.
 */
VulkanResult allocateGPUImage(
    VulkanGPUAllocator*      pGpuAllocator,
    const VkImageCreateInfo& imageCreateInfo,
    VmaMemoryUsage           usage,
    VmaAllocationCreateFlags flags,
    VulkanGfxImage*          pImageResult);

/**
 * @brief Frees a previously allocated GPU image, releasing the associated Vulkan image and VMA allocation.
 * @param Pointer to the VulkanGPUAllocator used for freeing the image.
 * @param Pointer to the VulkanGfxImage containing the image and allocation information to be freed.
 */
void freeGPUImage(
    VulkanGPUAllocator* pGpuAllocator,
    VulkanGfxImage*     pGfxImage);

/**
 * @brief Flushes the CPU memory of one or more GPU allocations, ensuring that any changes made to the mapped memory are visible to the GPU. This is necessary when the CPU has written to a mapped memory block and needs to ensure that the changes are propagated to the GPU before it accesses the memory.
 * @param Pointerto the VulkanGPUAllocator used for flushing the memory.
 * @param Allocations representing the GPU memory allocations to be flushed.
 * @param Offsets representing the byte offsets within each allocation where the flush should begin.
 * @param Sizes representing the size in bytes of the memory range to be flushed for each allocation.
 * @return Result of the flush operation.
 */
VulkanResult flushCPUMemory(
    VulkanGPUAllocator*         pGpuAllocator,
    DynamicArray<VmaAllocation> allocations,
    DynamicArray<VkDeviceSize>  offsets,
    DynamicArray<VkDeviceSize>  sizes);

/**
 * @brief Invalidates the CPU memory of one or more GPU allocations, ensuring that any changes made to the memory by the GPU are visible to the CPU. This is necessary when the GPU has written to a mapped memory block and the CPU needs to ensure that it sees the latest data before reading from the memory.
 * @param Pointerto the VulkanGPUAllocator used for invalidating the memory.
 * @param Allocations representing the GPU memory allocations to be invalidated.
 * @param Offsets representing the byte offsets within each allocation where the invalidation should begin.
 * @param Sizes representing the size in bytes of the memory range to be invalidated for each allocation.
 * @return Result of the invalidate operation.
 */
VulkanResult invalidateCPUMemory(
    VulkanGPUAllocator*         pGpuAllocator,
    DynamicArray<VmaAllocation> allocations,
    DynamicArray<VkDeviceSize>  offsets,
    DynamicArray<VkDeviceSize>  sizes);

/**
 * @brief Writes data from a host memory block to a GPU allocation, allowing the CPU to update the contents of the allocated GPU memory. This function handles the necessary mapping, copying, and flushing operations to ensure that the data is correctly transferred to the GPU.
 * @param Pointerto the VulkanGPUAllocator used for writing to the allocation.
 * @param Pointer to the source host memory block containing the data to be written to the GPU allocation.
 * @param Destination allocation handle representing the GPU memory allocation to which the data will be written.
 * @param Destination offset in bytes within the GPU allocation where the data should be written.
 * @param Sizein bytes of the data to be written from the host memory block to the GPU allocation.
 * @return Result of the write operation.
 */
VulkanResult writeToAllocation(
    VulkanGPUAllocator* pGpuAllocator,
    const void*         pSrcHostBlock,
    VmaAllocation       dstAllocation,
    VkDeviceSize        dstOffset,
    VkDeviceSize        size);

/**
 * @brief Sets a debug name for a GPU allocation, which can be useful for debugging and profiling purposes.
 * @param Pointer to the VulkanGPUAllocator used for setting the allocation name.
 * @param Destination allocation handle representing the GPU memory allocation for which the name will be set.
 * @param Namestring that will be associated with the GPU allocation for debugging purposes.
 */
void setAllocationName(
    VulkanGPUAllocator* pGpuAllocator,
    VmaAllocation       dstAllocation,
    const std::string&  name);

} // namespace vulkan
} // namespace dusk