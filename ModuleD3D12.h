#pragma once

#include "Module.h"
#include <dxgi1_6.h>
#include <cstdint>
#include <chrono>

class ModuleD3D12 : public Module
{
private:
    /*WINDOW*/
    HWND hWnd = NULL;
    /*Core DX12*/
    ComPtr<IDXGIFactory6> factory;
    ComPtr<IDXGIAdapter4> adapter;
    ComPtr<ID3D12Device5> device;

    // --- RECURSOS NECESARIOS PARA EL RENDERIZADO ---
    ComPtr<ID3D12CommandQueue> commandQueue; //La Cola de Comandos (donde se envía el trabajo a la GPU)
    ComPtr<ID3D12GraphicsCommandList> commandList; //El Command List (donde se graban los comandos)
    ComPtr<ID3D12CommandAllocator> commandAllocator; //El Command Allocator (gestiona la memoria para el Command List)
    
    //RECURSOS SWAP CHAIN Y RTV HEAP
    static constexpr uint32_t BACK_BUFFER_COUNT = 2; //Numero de Buffers
    ComPtr<IDXGISwapChain4> swapChain; // El objeto que gestiona la presentación
    uint32_t currentFrame = 0; // Índice del buffer actual
    ComPtr<ID3D12DescriptorHeap> rtvHeap; // <-- Define el Heap de descriptores
    uint32_t rtvDescriptorSize = 0; // <-- Define el tamaño del descriptor

    ComPtr<ID3D12Resource> renderTargets[BACK_BUFFER_COUNT]; // Array para guardar los Back Buffers
    
    ComPtr<ID3D12Fence> fence;              // <-- Resuelve 'fence' no definido
    uint64_t fenceValue = 0;
    HANDLE fenceEvent;
    
    
    /*
    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    ComPtr<ID3D12Resource> bufferUploadHeap;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;*/

  
public:

    ModuleD3D12(HWND hWnd);
    ~ModuleD3D12();

    bool init() override;
    void preRender() override;
    void render() override;
    void postRender() override;
    
    bool flush();

    ID3D12GraphicsCommandList* getCommandList() const; //Obtiene la lista de comandos para grabar
    ID3D12CommandAllocator* getCommandAllocator() const; //Para rastrear la lista de comandos
    ID3D12Resource* getBackBuffer() const; //Obtiene el Back buffer, recurso GPU, que vamos a dibujar
    D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetDescriptor() const; //Obtiene descriptor para acceder al render target (back buffer)
    ID3D12CommandQueue* getDrawCommandQueue() const; // Obtiene la cola donde se enviará la lista de comandos

    
private:

    void enableDebugLayer();
    bool createFactory();
    bool createDevice(bool useWarp);
    
};
