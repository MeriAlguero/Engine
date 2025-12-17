#include "Globals.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleImGui.h"
#include "ReadData.h"

//#include "ModuleSamplers.h"
//#include "ModuleCamera.h"
//#include "ModuleShaderDescriptors.h"
#include <d3d12.h>
#include "d3dx12.h"

ModuleD3D12::ModuleD3D12(HWND wnd) : hWnd(wnd)
{

}

ModuleD3D12::~ModuleD3D12()
{
   // flush();
}

bool ModuleD3D12::init()
{
#if defined(_DEBUG)
    enableDebugLayer();
#endif 


    bool ok = createFactory();
    ok = ok && createDevice(false);
    if (ok)
    {
        //CREAR COMMAND QUEUE (Cola de Comandos)
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //Para comandos de gráficos
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ok = SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

        //CREAR COMMAND ALLOCATOR (Asignador)
        if (ok)
        {
            ok = SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
        }

        //CREAR COMMAND LIST (Lista de Comandos)
        if (ok)
        {
            ok = SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

            // La Command List debe estar cerrada al inicio para que el render() pueda resetearla
            if (ok) commandList->Close();
        }
    }

    //CREAR RTV DESCRIPTOR HEAP (Montón de descriptores para Render Target) ---
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

    rtvHeapDesc.NumDescriptors = BACK_BUFFER_COUNT;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //Tipo: Render Target View
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //No es visible para shaders

    if (ok)
    {
        ok = SUCCEEDED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
    }

    //Guardar el tamaño del descriptor (útil para movernos en el heap)
    if (ok)
    {
        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
    
    RECT clientRect = {};
    GetClientRect(hWnd, &clientRect);
    uint32_t windowWidth = clientRect.right - clientRect.left;
    uint32_t windowHeight = clientRect.bottom - clientRect.top;

    //CREAR SWAP CHAIN
    if (ok)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = windowWidth;
        swapChainDesc.Height = windowHeight;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = BACK_BUFFER_COUNT; // Asumimos 2
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = 0;

        ComPtr<IDXGISwapChain1> swapChain1;

        ok = SUCCEEDED(factory->CreateSwapChainForHwnd(
            commandQueue.Get(), //La Command Queue que usará para Presentar
            hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        ));

        // Obtenemos la versión 4 del Swap Chain
        if (ok) ok = SUCCEEDED(swapChain1.As(&swapChain));

        // Guardar el índice del buffer actual
        if (ok) currentFrame = swapChain->GetCurrentBackBufferIndex();
    }

    //CREAR RTVs (Vistas a los Render Targets)
    if (ok)
    {
        // Obtenemos el punto de partida en el Heap de descriptores
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

        for (uint32_t i = 0; i < BACK_BUFFER_COUNT; ++i)
        {
            // A. Obtener el recurso de la Swap Chain (el Back Buffer)
            ok = SUCCEEDED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            if (!ok) break;

            // B. Crear la Render Target View (RTV) y colocarla en el Heap
            device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);

            // C. Movernos al siguiente espacio en el Heap
            rtvHandle.Offset(rtvDescriptorSize);
        }
    }

    if (ok)
    {
        ok = SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    }

    if (ok)
    {
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr)
        {
            ok = false;
        }
    }

    //CREACIO TRIANGLE
    bool ok_triangle = createVertexBuffer();
    ok_triangle = ok_triangle && createRootSignature();
    ok_triangle = ok_triangle && createPSO();

  
    return ok && ok_triangle;


}

bool ModuleD3D12::flush()
{
    fenceValue++;
    // La Command Queue pone la señal
    if (FAILED(commandQueue->Signal(fence.Get(), fenceValue)))
        return false;

    // Si la GPU no ha completado el trabajo
    if (fence->GetCompletedValue() < fenceValue)
    {
        if (FAILED(fence->SetEventOnCompletion(fenceValue, fenceEvent)))
            return false;

        // Espera de la CPU
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // Al final del frame, actualizamos el índice del buffer actual
    currentFrame = swapChain->GetCurrentBackBufferIndex();

    return true;
}

void ModuleD3D12::enableDebugLayer()
{
    ComPtr<ID3D12Debug> debugInterface;

    D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));

    debugInterface->EnableDebugLayer();
}

bool ModuleD3D12::createFactory()
{
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    return SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));
}

bool ModuleD3D12::createDevice(bool useWarp)
{
    bool ok = true;

    if (useWarp)
    {
        ComPtr<IDXGIAdapter1> adapter;
        ok = ok && SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        ok = ok && SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
    }
    else
    {
        ComPtr<IDXGIAdapter4> adapter;
        SIZE_T maxDedicatedVideoMemory = 0;
        factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
        ok = SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
    }

    if (ok)
    {
        // Check for tearing to disable V-Sync needed for some monitors
        BOOL tearing = FALSE;
        factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing));

        //allowTearing = tearing == TRUE;

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5;
        HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        if (SUCCEEDED(hr) && features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
           // supportsRT = true;
        }
    }


    return ok;
}
void ModuleD3D12::preRender()
{
    //Esperar solo si no es el frame inicial
    if (fenceValue != 0)
    {
        HRESULT hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    commandAllocator->Reset();

}
/*
void ModuleD3D12::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(d3d12->getRenderTargetDescriptor(), clearColor, 0, nullptr);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();
    ID3D12CommandList* commandLists[] = { commandList };
    d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    
    }*/

void ModuleD3D12::render()
{


    ID3D12GraphicsCommandList* commandList = getCommandList();
    commandList->Reset(getCommandAllocator(), pso.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    LONG width = (LONG)getWindowWidth();
    LONG height = (LONG)getWindowHeight(); 

    D3D12_VIEWPORT viewport{ 0.0, 0.0, float(width),  float(height) , 0.0, 1.0 };
    D3D12_RECT scissor{ 0, 0, width, height };

    float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = getRenderTargetDescriptor();

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);


    commandList->OMSetRenderTargets(1, &rtv, false, nullptr);
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    commandList->DrawInstanced(vertexCount, 1, 0, 0);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if (SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }

}

void ModuleD3D12::postRender()
{

    swapChain->Present(1, 0);
    ++fenceValue;
    HRESULT hr = commandQueue->Signal(fence.Get(), fenceValue);
    currentFrame = swapChain->GetCurrentBackBufferIndex();
    //flush();
}

bool ModuleD3D12::createVertexBuffer()
{
    struct Vertex
    {
        float x, y, z;
    };

    static Vertex vertices[3] =
    {
        {-1.0f, -1.0f, 0.0f }, // 0
        { 0.0f, 1.0f, 0.0f  }, // 1
        { 1.0f, -1.0f, 0.0f }  // 2
    };

    // Assumeix que app->getResources() existeix i té la funció createDefaultBuffer
    vertexBuffer = app->getResources()->createDefaultBuffer(vertices, sizeof(vertices), "TriangleVertexBuffer");
    vertexCount = 3;

    if (!vertexBuffer) return false;
   // app->getResources()->flushResourceCommands();

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = sizeof(vertices);

    return true;
}
bool ModuleD3D12::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    // La signatura arrel és buida, només permet el Input Layout
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr)))
    {
        return false;
    }

    // Crida al teu getDevice()
    if (FAILED(getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
    {
        return false;
    }

    return true;
}
bool ModuleD3D12::createPSO()
{
    // Input Layout: Només té la posició (MY_POSITION) com indica el professor
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"MY_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    // Lectura dels Shaders
    auto dataVS = DX::ReadData(L"BasicTransformVS.cso");
    auto dataPS = DX::ReadData(L"SolidColorPS.cso");

    // Descripció de la Pipeline
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = { dataVS.data(), dataVS.size() };
    psoDesc.PS = { dataPS.data(), dataPS.size() };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc = { 1, 0 };
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.NumRenderTargets = 1;

    // Crear el PSO
    return SUCCEEDED(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

ID3D12Device5* ModuleD3D12::getDevice() const { return device.Get(); }
uint32_t ModuleD3D12::getCurrentFrame() const { return currentFrame; }
UINT ModuleD3D12::getLastCompletedFrame() const { return (UINT)fence->GetCompletedValue();}

ID3D12GraphicsCommandList* ModuleD3D12::getCommandList() const { return commandList.Get(); }
ID3D12CommandAllocator* ModuleD3D12::getCommandAllocator() const { return commandAllocator.Get(); }
ID3D12CommandQueue* ModuleD3D12::getDrawCommandQueue() const { return commandQueue.Get(); }
ID3D12Resource* ModuleD3D12::getBackBuffer() const { return renderTargets[currentFrame].Get(); }

D3D12_CPU_DESCRIPTOR_HANDLE ModuleD3D12::getRenderTargetDescriptor() const {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(currentFrame * rtvDescriptorSize);
    return rtvHandle;
}

