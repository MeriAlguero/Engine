#pragma once
#include "Module.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class DescriptorTable {
public:
    DescriptorTable() = default;
    DescriptorTable(ID3D12Device* device, ID3D12DescriptorHeap* heap, UINT index, UINT size);

    void createTextureSRV(ID3D12Resource* texture);
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle() const;

private:
    ID3D12Device* device = nullptr;
    ID3D12DescriptorHeap* heap = nullptr;
    UINT index = 0;
    UINT descriptorSize = 0;
};

class ModuleShaderDescriptors : public Module {
public:
    bool init() override;
    DescriptorTable allocTable(UINT size = 1); // Reserva espacio para N texturas
    ID3D12DescriptorHeap* getHeap() const { return heap.Get(); }

private:
    ComPtr<ID3D12DescriptorHeap> heap;
    UINT currentDescriptorIndex = 0;
    UINT descriptorSize = 0;
    static const UINT MAX_DESCRIPTORS = 1024; // Espacio de sobra
};