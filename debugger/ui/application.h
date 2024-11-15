#pragma once
#include <memory>
#include <functional>
#include <chrono>
#include <vector>
#include "widgets/widget.h"
#include "widgets/vectrex.h"
#include "widgets/memview.h"
#include "widgets/disasm.h"
#include "widgets/control.h"
#include "widgets/controller.h"
#include "widgets/m6809.h"
#include "widgets/via6522.h"
#include "../configuration.h"
#include "fpscounter.h"

namespace debugger::ui {
	class DebuggerApplication
	{
	public:
		DebuggerApplication(Configuration& config, uint16_t processFrequency = 50);
		~DebuggerApplication() = default;
		void render();
		void process();
		// callback setters
		const bool isRunning() const { return running; }
		void stop() { running = false; }
	private:
		Configuration& config;
		uint16_t processFrequency;
		std::string loadedRomPath = "";
		std::chrono::time_point<std::chrono::high_resolution_clock> lastProcessTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> lastRenderTime;
		bool firstProcess = true;
		FPSCounter fpsCounter;
		std::atomic<bool> running = true;


		// widgets
		std::unique_ptr<widget::VectrexWidget> pwVectrex;
		std::unique_ptr<widget::VectrexMemoryWidget> pwVMemory;
		std::unique_ptr<widget::VectrexDisassemblyWidget> pwVDisassembly;
		std::unique_ptr<widget::VectrexControlWidget> pwVControl;
		std::unique_ptr<widget::M6809Widget> pwM6809;
		std::unique_ptr<widget::VIA6522Widget> pwVIA6522;
		std::unique_ptr<widget::ControllerInputWidget> pwVControllers;

		// emulator class
		std::shared_ptr<DebugVectrex> pVectrex;

		std::vector<uint8_t> readRomFile(const std::string& loadedRomPath) {
			std::vector<uint8_t> buffer;
			std::ifstream file(loadedRomPath, std::ios::binary | std::ios::ate);

			if (!file.is_open()) {
				LOGE << "Could not open file " << loadedRomPath;
				return buffer;  // Return empty buffer
			}

			std::streamsize size = file.tellg();
			if (size <= 0) {
				LOGE << "File is empty or error in size calculation " << loadedRomPath;
				return buffer;  // Return empty buffer
			}

			buffer.resize(size);
			file.seekg(0, std::ios::beg);
			if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
				LOGE << "Failed to read file " << loadedRomPath;
				buffer.clear(); 
			}

			file.close();

			return buffer;
		}

		void renderMenu();
	};
}