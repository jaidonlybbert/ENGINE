#include<glm/glm.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "logger/Logging.hpp"
#include "gui/Gui.hpp"

void Gui::registerDrawCall(std::function<void(void)> drawCall)
{
	drawCalls.emplace_back(drawCall);
}

void Gui::drawGui() 
{
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// call all registered draw calls
	for (auto& f : drawCalls)
	{
		f();
	}

	// Rendering
	ImGui::Render();
}
