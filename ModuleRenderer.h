#pragma once
#include "Module.h"
#include "ModuleCamera.h"
#include <d3d12.h>
#include <wrl/client.h>
#include "DebugDrawPass.h"
#include "Model.h"

class ModuleD3D12;
class ModuleCamera;

class ModuleRenderer : public Module {
public:
    ModuleRenderer();
    ~ModuleRenderer();

    bool init() override;
    void render() override;
    void preRender() override;
    void postRender() override;

    void setGridVisible(bool visible) { showGrid = visible; }
    void setAxisVisible(bool visible) { showAxis = visible; }
    bool isGridVisible() const { return showGrid; }
    bool isAxisVisible() const { return showAxis; }

private:
    bool createVertexBuffer();
    bool createRootSignature();
    bool createPipelineState();
    void renderModel();
    void renderDebugDraw();

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

    uint32_t vertexCount = 0;
    Model model;
    bool showGrid = true;
    bool showAxis = true;
    bool modelLoaded = false;

    // Debug drawing
    DebugDrawPass* debugDraw = nullptr;
};