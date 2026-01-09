#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "gltf_utils.h" 

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texCoord0;
};

class Mesh
{
public:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
   
    Mesh() = default;
    ~Mesh() = default;

    UINT vertexCount = 0;
    UINT indexCount = 0;
    UINT materialIndex = 0;
    bool hasIndices = false;
    DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT;

    void Load(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

    void BindBuffers(ID3D12GraphicsCommandList* commandList) const
    {
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        if (hasIndices)
        {
            commandList->IASetIndexBuffer(&indexBufferView);
        }
    }

    void Draw(ID3D12GraphicsCommandList* commandList) const
    {
        if (hasIndices)
        {
            commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
        }
        else
        {
            commandList->DrawInstanced(vertexCount, 1, 0, 0);
        }
    }
};
