#include "Globals.h"
#include "ModuleResource.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "d3dx12.h" 
#include "ReadData.h"

#include "DirectXTex.h"
#include <algorithm> 
#include <string>

using namespace DirectX;

// --- Constructor/Destructor ---
ModuleResource::ModuleResource()
{
}

ModuleResource::~ModuleResource()
{
}

bool ModuleResource::init()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device4* device = d3d12->getDevice();

    // Creem Command Allocator i Command List per a ús exclusiu dels recursos
    bool ok = SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ok = ok && SUCCEEDED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList)));

    // El Command List s'inicialitza tancat. Se reseteja immediatament.
    ok = ok && SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

    return ok;
}

bool ModuleResource::cleanUp()
{
    // Els ComPtr s'alliberen automàticament.
    return true;
}

void ModuleResource::preRender()
{
    // Implementació del Garbage Collector: alliberem recursos antics
    UINT completedFrame = app->getD3D12()->getLastCompletedFrame();

    for (int i = 0; i < deferredFrees.size();)
    {
        if (completedFrame >= deferredFrees[i].frame)
        {
            // Substituïm el recurs alliberat per l'últim de la llista (per evitar forats)
            deferredFrees[i] = deferredFrees.back();
            deferredFrees.pop_back();
        }
        else
        {
            ++i; // Només avancem si el recurs encara no es pot alliberar
        }
    }
}

void ModuleResource::deferRelease(ComPtr<ID3D12Resource> resource)
{
    if (resource)
    {
        UINT currentFrame = app->getD3D12()->getCurrentFrame();

        // Comprovem si el recurs ja està a la llista de deferRelease
        auto it = std::find_if(deferredFrees.begin(), deferredFrees.end(), [resource](const DeferredFree& item) -> bool
            { return item.resource.Get() == resource.Get(); });

        if (it != deferredFrees.end())
        {
            // Si ja hi és, només actualitzem el frame
            it->frame = currentFrame;
        }
        else
        {
            // Si no hi és, l'afegim a la llista
            deferredFrees.push_back({ currentFrame, resource });
        }
    }
}

ComPtr<ID3D12Resource> ModuleResource::createDefaultBuffer(
    const void* initData,
    UINT64 byteSize,
    const char* debugName)
{
    ID3D12Device* device = app->getD3D12()->getDevice();
    ID3D12GraphicsCommandList* commandList = app->getD3D12()->getCommandList();

    ComPtr<ID3D12Resource> defaultBuffer;
    ComPtr<ID3D12Resource> uploadBuffer;

    // Este recurso es el destino final de los datos del vértice.
    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC defaultResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    if (FAILED(device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &defaultResourceDesc,
        D3D12_RESOURCE_STATE_COMMON, // Estado inicial
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf()))))
    {
        return nullptr;
    }if (debugName)
    {
        std::wstring wDebugName(debugName, debugName + strlen(debugName));
        defaultBuffer->SetName(wDebugName.c_str());
    }

    //Crear RECURSO UPLOAD

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf()))))
    {
        return nullptr;
    }


    void* pMappedData = nullptr;
    CD3DX12_RANGE readRange(0, 0); // No necesitamos leer nada de la GPU

    if (FAILED(uploadBuffer->Map(0, &readRange, &pMappedData)))
    {
        return nullptr;
    }
    memcpy(pMappedData, initData, byteSize);
    uploadBuffer->Unmap(0, nullptr);

    CD3DX12_RESOURCE_BARRIER transitionWrite = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &transitionWrite);

    commandList->CopyResource(defaultBuffer.Get(), uploadBuffer.Get());

    CD3DX12_RESOURCE_BARRIER transitionRead = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    commandList->ResourceBarrier(1, &transitionRead);
   
    return defaultBuffer; // Devolver el recurso final en la GPU
}


