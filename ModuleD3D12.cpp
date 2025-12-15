#include "Globals.h"
#include "Application.h"
#include "ModuleD3D12.h"


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

   //getWindowSize(windowWidth, windowHeight);

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
    return ok;
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
void ModuleD3D12::preRender(){}
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
    
    }

void ModuleD3D12::postRender()
{

    swapChain->Present(1, 0);

    flush();
}

ID3D12GraphicsCommandList* ModuleD3D12::getCommandList() const { return commandList.Get(); }
ID3D12CommandAllocator* ModuleD3D12::getCommandAllocator() const { return commandAllocator.Get(); }
ID3D12CommandQueue* ModuleD3D12::getDrawCommandQueue() const { return commandQueue.Get(); }
ID3D12Resource* ModuleD3D12::getBackBuffer() const { return renderTargets[currentFrame].Get(); }
D3D12_CPU_DESCRIPTOR_HANDLE ModuleD3D12::getRenderTargetDescriptor() const {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(currentFrame * rtvDescriptorSize);
    return rtvHandle;
}

