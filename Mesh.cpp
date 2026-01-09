#include "Globals.h"
#include "Mesh.h"
#include "Application.h"
#include "ModuleResource.h"
#include "gltf_utils.h"


void Mesh::Load(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
    ModuleResource* resources = app->getResources();

    const auto& posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end())
        return; 

    // Get vertex count from position accessor
    vertexCount = UINT(model.accessors[posIt->second].count);

    // Allocate temporary vertex array
    std::unique_ptr<Vertex[]> vertices = std::make_unique<Vertex[]>(vertexCount);
    uint8_t* vertexData = reinterpret_cast<uint8_t*>(vertices.get());

    // Load positions
    if (!loadAccessorData(vertexData + offsetof(Vertex, position),
        sizeof(DirectX::XMFLOAT3),
        sizeof(Vertex),
        vertexCount,
        model,
        posIt->second))
    {
        LOG("Failed to load vertex positions");
        return;
    }

    // Load normals
    loadAccessorData(vertexData + offsetof(Vertex, normal),
        sizeof(DirectX::XMFLOAT3),
        sizeof(Vertex),
        vertexCount,
        model,
        primitive.attributes,
        "NORMAL");

    // Load texture coordinates
    loadAccessorData(vertexData + offsetof(Vertex, texCoord0),
        sizeof(DirectX::XMFLOAT2),
        sizeof(Vertex),
        vertexCount,
        model,
        primitive.attributes,
        "TEXCOORD_0");

    vertexBuffer = resources->createDefaultBuffer(
        vertices.get(),
        vertexCount * sizeof(Vertex),
        "MeshVB");

    if (!vertexBuffer)
    {
        LOG("Failed to create vertex buffer");
        return;
    }

    //SETUP VERTEX BUFFER VIEW
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = vertexCount * sizeof(Vertex);

    //LOAD INDEX DATA
    if (primitive.indices >= 0)
    {
        // Determine index type
        const ::tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
        indexCount = UINT(indexAccessor.count);

        switch (indexAccessor.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            indexFormat = DXGI_FORMAT_R8_UINT;
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            indexFormat = DXGI_FORMAT_R16_UINT;
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            indexFormat = DXGI_FORMAT_R32_UINT;
            break;
        default:
            LOG("Unsupported index format");
            return;
        }

        // Calculate element size
        size_t elementSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);

        // Allocate temporary index buffer
        std::unique_ptr<uint8_t[]> indices = std::make_unique<uint8_t[]>(indexCount * elementSize);

        // Load index data using your utility function
        if (!loadAccessorData(indices.get(),
            elementSize,
            elementSize,
            indexCount,
            model,
            primitive.indices))
        {
            LOG("Failed to load index data");
            return;
        }

        //CREATE INDEX BUFFER
        indexBuffer = resources->createDefaultBuffer(
            indices.get(),
            indexCount * elementSize,
            "MeshIB");

        if (!indexBuffer)
        {
            LOG("Failed to create index buffer");
            return;
        }

        //SETUP INDEX BUFFER VIEW
        indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        indexBufferView.Format = indexFormat;
        indexBufferView.SizeInBytes = indexCount * elementSize;

        hasIndices = true;
    }
    else
    {
        hasIndices = false;
    }

    //STORE MATERIAL INDEX
    materialIndex = (primitive.material >= 0) ? UINT(primitive.material) : 0;
}