#include "application.h"
#include <imgui.h>
#include <fmt/format.h>
#include <plog/Log.h>

debugger::ui::DebuggerApplication::DebuggerApplication(Configuration &config, uint16_t processFrequency) : config(config), processFrequency(processFrequency)
{
	// create instances of widgets
	pwVectrex = std::make_unique<widget::VectrexWidget>(config);
	pwVectrex->show();
}

void debugger::ui::DebuggerApplication::render()
{
	auto now = std::chrono::high_resolution_clock::now();
	if (firstFrame) {
		lastRenderTime = now;
		firstFrame = false;
	}
	// Calculate frametime in seconds
	float frametime = std::chrono::duration<float>(now - lastRenderTime).count();
	lastRenderTime = now;

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
	ImGui::Begin("BackgroundWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

	renderMenu();
	if (pwVectrex) {
		pwVectrex->render(frametime * 1000);
	}



	// Calculate FPS, handling possible zero frametime
	float fps = (frametime > 0.0f) ? (1.0f / frametime) : 0.0f;

	// Determine display format based on fps value
	std::string fpsText;
	if (fps >= 1000.0f) {
		fpsText = "FPS: 999.9";
	}
	else if (fps >= 100.0f) {
		fpsText = fmt::format("FPS: {:.01f} ({:03.2f}ms)", fps, frametime * 1000);
	}
	else {
		fpsText = fmt::format("FPS: {:05.01f} ({:03.1f}ms)", fps, frametime * 1000);
	}

	// Calculate the size of the text, which is fixed
    ImVec2 textSize = ImGui::CalcTextSize(fpsText.c_str());

	// Position in the top-right corner based on text width
	auto region = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPos(ImVec2(region.x - textSize.x - 10, 20));
	ImGui::TextUnformatted(fpsText.c_str());

	ImGui::End();
}


void debugger::ui::DebuggerApplication::process()
{
	auto now = std::chrono::high_resolution_clock::now();
	if (firstProcess) {
		lastProcessTime = now;
		firstProcess = false;
	}
	long delta = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastProcessTime).count());
	if (pwVectrex) {
		pwVectrex->process(delta);
	}
	lastProcessTime = now;
}

void debugger::ui::DebuggerApplication::renderMenu()
{
	if (ImGui::BeginMainMenuBar()) {
		// File
		// - Load ROM...
		// - Exit
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) { 
				stop();
			}
			ImGui::EndMenu();
		}

		// Memory
		// - Viewer
		if (ImGui::BeginMenu("Memory")) {
			ImGui::EndMenu();
		}

		// Disassembly
		// - Viewer
		if (ImGui::BeginMenu("Disassembly")) {
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}
