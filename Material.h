#pragma once
#include <filesystem>
#include <vector>
#include "Mesh.h"


class Material
{
public:
    DirectX::XMFLOAT4 baseColor;
    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    UINT srvIndex;

    void Load(const class tinygltf::Model& model, const class tinygltf::Material& material, const char* basePath);
};
