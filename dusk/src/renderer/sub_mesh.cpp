#include "sub_mesh.h"

#include "engine.h"
#include "render_api.h"

namespace dusk
{
Error SubMesh::init(DynamicArray<Vertex>& vertices, DynamicArray<uint32_t>& indices)
{
    Error err = initGfxVertexBuffer(vertices);
    if (err != Error::Ok)
    {
        return err;
    }

    err = initGfxIndexBuffer(indices);
    if (err != Error::Ok)
    {
        return err;
    }

    return Error();
}

Error SubMesh::initGfxVertexBuffer(DynamicArray<Vertex>& vertices)
{
    if (Engine::getRenderAPI() == RenderAPI::API::VULKAN)
    {
        return Error::Ok;
    }

    return Error::NotSupported;
}

Error SubMesh::initGfxIndexBuffer(DynamicArray<uint32_t>& indices)
{
    if (Engine::getRenderAPI() == RenderAPI::API::VULKAN)
    {
        return Error::Ok;
    }

    return Error::NotSupported;
}

} // namespace dusk