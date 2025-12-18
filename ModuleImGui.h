#pragma once
#include "Module.h"
#include <d3d12.h>
#include "Module.h"
#include "ImGuiPass.h" 
#include <memory>

struct ImGuiContext;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;

class ModuleImGui : public Module
{
public:
    ModuleImGui();
    ~ModuleImGui() override;


    bool init() override;
    void preRender() override;
    void render() override;
    void postRender() override;
    bool showConfigWindow = true;
    bool showAboutWindow = true;

private:
    std::unique_ptr<ImGuiPass> imguiPass;

    ComPtr<ID3D12DescriptorHeap> srvDescHeap;
    float fpsLog[100] = { 0.0f };

    void drawMainMenuBar();
    // void drawDemoWindow();
    void drawConfigurationWindow();
};