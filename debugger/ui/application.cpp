#define NFD_IMPLEMENTATION
#include <nfd.h>
#include "application.h"
#include <imgui.h>
#include <fmt/format.h>
#include <plog/Log.h>

constexpr int FPS_FRAMECOUNT = 10;

debugger::ui::DebuggerApplication::DebuggerApplication(Configuration &config, uint16_t processFrequency) : config(config), processFrequency(processFrequency)
{

	pVectrex = std::make_shared<DebugVectrex>();
	pVectrex->Reset();

	pVectrex->SetPlayerOne(0x80, 0x80, 1, 1, 1, 1);
	pVectrex->SetPlayerTwo(0x80, 0x80, 1, 1, 1, 1);

	// create instances of widgets
	pwVectrex = std::make_unique<widget::VectrexWidget>(pVectrex, config);
	pwVMemory = std::make_unique<widget::VectrexMemoryWidget>(pVectrex, config);
	pwVDisassembly = std::make_unique<widget::VectrexDisassemblyWidget>(pVectrex, config);
	pwM6809 = std::make_unique<widget::M6809Widget>(pVectrex, config);
	pwVIA6522 = std::make_unique<widget::VIA6522Widget>(pVectrex, config);
	pwVControl = std::make_unique<widget::VectrexControlWidget>(config);
	pwVControllers = std::make_unique<widget::ControllerInputWidget>(pVectrex, config);

	pwVControl->setPauseCallback([this]() {
		pwVectrex->pause();
		pwVDisassembly->show();
		pwM6809->show();
		pwVDisassembly->jumpToAddress(pVectrex->GetM6809().getRegisters().PC);
		pwVDisassembly->addToAnalyseQueue(pVectrex->GetM6809().getRegisters().PC);
	});
	pwVControl->setResumeCallback([this]() {
		pwVectrex->resume();
		pwVDisassembly->hide();
		pwM6809->hide();
	});
	pwVControl->setStepCallback([this]() {
		pwVectrex->step();
		pwVDisassembly->jumpToAddress(pVectrex->GetM6809().getRegisters().PC);
	});
	pwVControl->setResetCallback([this]() {
		pwVectrex->reset();
		pwVDisassembly->jumpToAddress(pVectrex->GetM6809().getRegisters().PC);
	});
	pwVectrex->setPausedCallback([this](bool paused) {
		pwVControl->setPaused(paused);
	});
	pwVectrex->resume();
	lastRenderTime = std::chrono::high_resolution_clock::now();

}

void debugger::ui::DebuggerApplication::render()
{
	auto now = std::chrono::high_resolution_clock::now();
	fpsCounter.tick();
	float frametime = std::chrono::duration<float>(now - lastRenderTime).count();
	lastRenderTime = now;

	renderMenu();

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
	ImGui::Begin("BackgroundWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
	// Determine display format based on fps value

	// Calculate FPS, handling possible zero frametime
	std::string fpsText;
	float averageFrametime = fpsCounter.getAverageFrametime();
	float averageFPS = fpsCounter.getAverageFPS();
	if (averageFPS >= 1000.0f) {
		fpsText = "FPS: 999.9";
	}
	else if (averageFPS >= 100.0f) {
		fpsText = fmt::format("FPS: {:.01f} ({:03.2f}ms)", averageFPS, averageFrametime * 1000);
	}
	else {
		fpsText = fmt::format("FPS: {:05.01f} ({:03.1f}ms)", averageFPS, averageFrametime * 1000);
	}

	// Calculate the size of the text, which is fixed
	ImVec2 textSize = ImGui::CalcTextSize(fpsText.c_str());

	// Position in the top-right corner based on text width
	auto region = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPos(ImVec2(region.x - textSize.x - 10, 20));
	ImGui::TextUnformatted(fpsText.c_str());

	ImGui::End();

	if (pwVectrex) {
		pwVectrex->render(frametime * 1000);
		pwVectrex->process(frametime * 1000);
	}

	if (pwVMemory) {
		pwVMemory->render(frametime * 1000);
		pwVMemory->process(frametime * 1000);
	}

	if (pwVDisassembly) {
		pwVDisassembly->render(frametime * 1000);
		pwVDisassembly->process(frametime * 1000);
	}
	
	if (pwVControl) {
		pwVControl->render(frametime * 1000);
		pwVControl->process(frametime * 1000);
	}
	
	if (pwM6809) {
		pwM6809->render(frametime * 1000);
		pwM6809->process(frametime * 1000);
	}
	
	if (pwVIA6522) {
		pwVIA6522->render(frametime * 1000);
		pwVIA6522->process(frametime * 1000);
	}
	
	if (pwVControllers) {
		pwVControllers->render(frametime * 1000);
		pwVControllers->process(frametime * 1000);
	}
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
			// TODO: error dialogs
			if (ImGui::MenuItem("Load ROM...")) {
				nfdchar_t* outPath = nullptr;
				nfdu8filteritem_t filters[2] = { { "Vectrex ROMs", "vec" }};
				nfdopendialogu8args_t args = { 0 };
				args.filterList = filters;
				args.filterCount = 2;
				nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

				if (result == NFD_OKAY) {
					loadedRomPath = outPath;
					LOGI << fmt::format("ROM loaded from path: {}", loadedRomPath);
					auto romBuffer = readRomFile(loadedRomPath);
					pVectrex->LoadCartridge(romBuffer.data(), romBuffer.size());
					free(outPath);
				}
				else if (result == NFD_CANCEL) {
					LOGD << "Cancelled ROM Load";
				}
				else {
					LOGE << NFD_GetError();
				}
			}
			if (ImGui::MenuItem("Exit")) { 
				stop();
			}
			ImGui::EndMenu();
		}

		// Memory
		// - Viewer
		if (ImGui::BeginMenu("Memory")) {
			if (ImGui::MenuItem("Viewer")) {
				pwVMemory->show();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}
