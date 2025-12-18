#pragma once

#include "Globals.h"
#include "Module.h"
#include "ModuleResource.h"
#include "ModuleShaderDescriptors.h"


#include <array>
#include <vector>
#include <chrono>

class Module;
class ModuleD3D12;
class ModuleImGui;
class ModuleShaderDescriptors;
class ModuleCamera;
class ModuleInput;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
	void         update();
	bool         cleanUp();
    
    ModuleD3D12* getD3D12() const;
    ModuleResource* getResources() const;
    ModuleImGui* getImGui() const { return m_imgui; }
    ModuleShaderDescriptors* getShaderDescriptors() const { return shaderDescriptors; }
    ModuleD3D12* getD3D12() { return d3d12; }
    ModuleInput* getInput() { return input; }
    ModuleCamera* getCamera() { return camera; }

    float    getDeltaTime()      const { return float(elapsedMilis) * 0.001f; }
    float                       getFPS() const { return 1000.0f * float(MAX_FPS_TICKS) / tickSum; }
    float                       getAvgElapsedMs() const { return tickSum / float(MAX_FPS_TICKS); }
    uint64_t                    getElapsedMilis() const { return elapsedMilis; }

    bool                        isPaused() const { return paused; }
    bool                        setPaused(bool p) { paused = p; return paused; }

    void* getWindow() const { return window; }
    void requestQuit() { exit = true; } 

private:
    enum { MAX_FPS_TICKS = 30 };
    typedef std::array<uint64_t, MAX_FPS_TICKS> TickList;

    std::vector<Module*> modules;

    uint64_t  lastMilis = 0;
    TickList  tickList;
    uint64_t  tickIndex;
    uint64_t  tickSum = 0;
    uint64_t  elapsedMilis = 0;
    bool      paused = false;
    bool exit = false;

    void* window = nullptr;

    ModuleResource* m_resources = nullptr;
    ModuleImGui* m_imgui = nullptr;
    ModuleShaderDescriptors* shaderDescriptors = nullptr;
    ModuleCamera* camera = nullptr;
    ModuleD3D12* d3d12 = nullptr;
    ModuleInput* input = nullptr;
};

extern Application* app;
