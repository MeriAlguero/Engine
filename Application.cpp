#include "Globals.h"
#include "Application.h"
#include "ModuleInput.h"
#include "ModuleD3D12.h"
#include "ModuleResource.h"
#include "ModuleImGui.h"
#include "ModuleCamera.h"
#include "ModuleRenderer.h"


#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    this->window = hWnd;

    d3d12 = new ModuleD3D12((HWND)hWnd);
    shaderDescriptors = new ModuleShaderDescriptors();
    camera = new ModuleCamera();
    input = new ModuleInput((HWND)hWnd);
    m_imgui = new ModuleImGui();
    m_resources = new ModuleResource();
    renderer = new ModuleRenderer();

    modules.push_back(d3d12);
    modules.push_back(shaderDescriptors);
    modules.push_back(input);
    modules.push_back(camera);
    modules.push_back(m_resources);
    modules.push_back(renderer);
    modules.push_back(m_imgui);
   
}

Application::~Application()
{
    cleanUp();

	for(auto it = modules.rbegin(); it != modules.rend(); ++it)
    {
        delete *it;
    }
}
 
bool Application::init()
{
	bool ret = true;

	for(auto it = modules.begin(); it != modules.end() && ret; ++it)
		ret = (*it)->init();

    lastMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	return ret;
}
/*bool Application::postInit() {

}*/

void Application::update()
{
    using namespace std::chrono_literals;

    // Update milis
    uint64_t currentMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    elapsedMilis = currentMilis - lastMilis;
    lastMilis = currentMilis;
    tickSum -= tickList[tickIndex];
    tickSum += elapsedMilis;
    tickList[tickIndex] = elapsedMilis;
    tickIndex = (tickIndex + 1) % MAX_FPS_TICKS;

    if (!app->paused)
    {
        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->update();

        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->preRender();

        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->render();

        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->postRender();
    }
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
ModuleD3D12* Application::getD3D12() const
{
    
    for (Module* module : modules) {
       
        ModuleD3D12* d3d12 = dynamic_cast<ModuleD3D12*>(module);
        if (d3d12 != nullptr) {
            return d3d12; // Encontramos el módulo D3D12, lo devolvemos
        }
    }
    return nullptr;
}

ModuleResource* Application::getResources() const
{
    return m_resources;
}

