#include <Windows.h>
#include <d3d12.h>
#include "d3dx12.h" 
#include "Globals.h"
#include "ModuleResource.h"

#include "Application.h"
#include "ModuleD3D12.h"

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


ComPtr<ID3D12Resource> ModuleResource::createRawTexture2D(const void* data, size_t rowSize, size_t width, size_t height, DXGI_FORMAT format)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

    D3D12_RESOURCE_DESC desc = {};
    desc.Width = UINT(width);
    desc.Height = UINT(height);
    desc.MipLevels = 1;
    desc.DepthOrArraySize = 1;
    desc.Format = format;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.SampleDesc.Count = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ComPtr<ID3D12Resource> texture = nullptr;

    // 1. Crear Texture DEFAULT
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture)));
    if (!ok) return nullptr;

    // 2. Calcular mida requerida i obtenir Heap UPLOAD
    UINT64 requiredSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
    device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, nullptr, &requiredSize);
    ComPtr<ID3D12Resource> upload = getUploadHeap(requiredSize);
    if (!upload) return nullptr;

    if (ok)
    {
        // 3. Map i Copia Manual de dades (una fila a la vegada)
        BYTE* uploadData = nullptr;
        CD3DX12_RANGE readRange(0, 0);
        upload->Map(0, &readRange, reinterpret_cast<void**>(&uploadData));
        for (uint32_t i = 0; i < height; ++i)
        {
            // Còpia manual: rowSize és el tamany de la fila a la CPU. Footprint.RowPitch és el tamany de la fila a la GPU (pot ser més gran).
            memcpy(uploadData + layout.Offset + layout.Footprint.RowPitch * i, (BYTE*)data + rowSize * i, rowSize);
        }
        upload->Unmap(0, nullptr);

        // 4. Gravar Comandament de Còpia
        CD3DX12_TEXTURE_COPY_LOCATION Dst(texture.Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION Src(upload.Get(), layout);
        commandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

        // 5. Barrera: COPY_DEST -> PIXEL_SHADER_RESOURCE
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);

        commandList->Close();

        // 6. Executar i Flush
        ID3D12CommandList* commandLists[] = { commandList.Get() };
        queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
        d3d12->flush();

        // 7. Reset Command List
        commandAllocator->Reset();
        ok = SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));
    }

    return texture;
}

ComPtr<ID3D12Resource> ModuleResource::createTextureFromMemory(const void* data, size_t size, const char* name)
{
    ScratchImage image;
    bool ok = SUCCEEDED(LoadFromDDSMemory(data, size, DDS_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromHDRMemory(data, size, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromTGAMemory(data, size, TGA_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromWICMemory(data, size, WIC_FLAGS_NONE, nullptr, image));

    if (ok)
    {
        return createTextureFromImage(image, name);
    }

    return nullptr;
}

ComPtr<ID3D12Resource> ModuleResource::createTextureFromFile(const std::filesystem::path& path, bool defaultSRGB)
{
    const wchar_t* fileName = path.c_str();
    ScratchImage image;
    bool ok = SUCCEEDED(LoadFromDDSFile(fileName, DDS_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromHDRFile(fileName, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromTGAFile(fileName, defaultSRGB ? TGA_FLAGS_DEFAULT_SRGB : TGA_FLAGS_NONE, nullptr, image));
    ok = ok || SUCCEEDED(LoadFromWICFile(fileName, defaultSRGB ? WIC_FLAGS_DEFAULT_SRGB : WIC_FLAGS_NONE, nullptr, image));

    if (ok)
    {
        return createTextureFromImage(image, path.string().c_str());
    }

    return nullptr;
}

ComPtr<ID3D12Resource> ModuleResource::createTextureFromImage(const ScratchImage& image, const char* name)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

    ComPtr<ID3D12Resource> texture;
    const TexMetadata& metaData = image.GetMetadata();


    if (metaData.dimension == TEX_DIMENSION_TEXTURE2D)
    {

        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(metaData.format, UINT64(metaData.width), UINT(metaData.height),
            UINT16(metaData.arraySize), UINT16(metaData.mipLevels));

        CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture)));
        if (!ok) return nullptr;


        ComPtr<ID3D12Resource> upload;
        UINT64 requiredIntermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(image.GetImageCount()));
        upload = getUploadHeap(requiredIntermediateSize);
        ok = upload != nullptr;
        if (!ok) return nullptr;


        if (ok)
        {
     
            std::vector<D3D12_SUBRESOURCE_DATA> subData;
            subData.reserve(image.GetImageCount());

            for (size_t item = 0; item < metaData.arraySize; ++item)
            {
                for (size_t level = 0; level < metaData.mipLevels; ++level)
                {
                    const DirectX::Image* subImg = image.GetImage(level, item, 0);

                    D3D12_SUBRESOURCE_DATA data = { subImg->pixels, (LONG_PTR)subImg->rowPitch, (LONG_PTR)subImg->slicePitch };

                    subData.push_back(data);
                }
            }

 
            ok = UpdateSubresources(commandList.Get(), texture.Get(), upload.Get(), 0, 0, UINT(image.GetImageCount()), subData.data()) != 0;
            if (!ok) return nullptr;
        }

        if (ok)
        {

            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            commandList->ResourceBarrier(1, &barrier);

            commandList->Close();

            // 6. Executar i Flush
            ID3D12CommandList* commandLists[] = { commandList.Get() };
            queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

            d3d12->flush();


            commandAllocator->Reset();
            ok = SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

            // 8. Assignar Nom
            texture->SetName(std::wstring(name, name + strlen(name)).c_str());

            return texture;
        }
    }

    return ComPtr<ID3D12Resource>();
}

// --- Render Target / Depth Stencil Creation ---

ComPtr<ID3D12Resource> ModuleResource::createRenderTarget(DXGI_FORMAT format, size_t width, size_t height, size_t arraySize, size_t mipLevels, UINT sampleCount, const Vector4& clearColour, const char* name)
{
    ComPtr<ID3D12Resource> texture;
    ModuleD3D12* d3d12 = app->getD3D12();

    const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    const D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, (UINT64)(width), (UINT)(height),
        UINT16(arraySize), UINT16(mipLevels), sampleCount, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE clearValue = { format , { clearColour.x, clearColour.y, clearColour.z, clearColour.w } };

    if (FAILED(d3d12->getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &desc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&texture))))
    {
        return nullptr;
    }

    texture->SetName(std::wstring(name, name + strlen(name)).c_str());

    return texture;
}

ComPtr<ID3D12Resource> ModuleResource::createDepthStencil(DXGI_FORMAT format, size_t width, size_t height, UINT sampleCount, float clearDepth, uint8_t clearStencil, const char* name)
{
    ComPtr<ID3D12Resource> texture;
    ModuleD3D12* d3d12 = app->getD3D12();

    const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    const D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format, (UINT64)(width), (UINT)(height),
        1, 1, sampleCount, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE clear;
    clear.Format = format;
    clear.DepthStencil.Depth = clearDepth;
    clear.DepthStencil.Stencil = clearStencil;

    if (FAILED(d3d12->getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear, IID_PPV_ARGS(&texture))))
    {
        return nullptr;
    }

    texture->SetName(std::wstring(name, name + strlen(name)).c_str());

    return texture;
}
Microsoft::WRL::ComPtr<ID3D12Resource> ModuleResource::getUploadHeap(uint64_t size) {
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;

    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

    app->getD3D12()->getDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadHeap)
    );

    return uploadHeap;
}

