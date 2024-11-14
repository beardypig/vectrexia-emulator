#pragma once
#include <memory>
#include <functional>
#include <chrono>
#include "widgets/vectrex.h"
#include "../configuration.h"

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
		std::chrono::time_point<std::chrono::high_resolution_clock> lastProcessTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> lastRenderTime;
		bool firstProcess = true;
		bool firstFrame = true;
		std::atomic<bool> running = true;


		// widgets
		std::unique_ptr<widget::VectrexWidget> pwVectrex;

		void renderMenu();


	};
}