#pragma once
#include <chrono>
#include <imgui.h>
#include <vectrexia.h>
#include <glad/glad.h>
#include <fmt/format.h>
#include <plog/Log.h>

#include "widget.h"
#include "../../util.h"
#include "../../vectrexdebug.h"

namespace debugger::ui::widget {
	class VectrexWidget : public Widget {
	public:
		static const int FRAME_WIDTH = 330;
		static const int FRAME_HEIGHT = 410;


		VectrexWidget(std::shared_ptr<DebugVectrex> pVectrex, Configuration& config) : Widget(config), pVectrex(pVectrex), onPaused(nullptr) {
			showWindow = true;
			initTexture();
		}

		void setPausedCallback(const std::function<void(bool)>& callback) {
			onPaused = callback;
		}

		// delta is in ms
		void process(long delta) override {
			if (!paused) {
				auto deltams = std::chrono::milliseconds(delta);
				auto deltans = std::chrono::duration_cast<std::chrono::nanoseconds>(deltams);
				auto cycles = TimerUtil::nanos_to_cycles(deltans.count());
				pVectrex->Run(std::min<uint64_t>(cycles, 30000));
			}
		}

		void render(long delta) override {
			// render the frame buffer from the vectrex in to this window
			// the window should be a fix ratio FRAME_WIDTH:FRAME_HEIGHT, with a minium size of FRAME_WIDTHxFRAME_HEIGHT
			ImGui::SetNextWindowSize(ImVec2(FRAME_WIDTH, FRAME_HEIGHT));
			ImGui::Begin("Vectrex", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNavInputs);
			auto fb = pVectrex->getFramebuffer();
			ImGuiIO& io = ImGui::GetIO();
			uint8_t p1_x, p1_y;

			if (io.KeysDown[ImGuiKey_UpArrow] && io.KeysDown[ImGuiKey_DownArrow]) {
				p1_y = 0x80;
			}
			else if (io.KeysDown[ImGuiKey_UpArrow]) {
				p1_y = 0xff;
			}
			else if (io.KeysDown[ImGuiKey_DownArrow]) {
				p1_y = 0x00;
			}
			else {
				p1_y = 0x80;
			}

			if (io.KeysDown[ImGuiKey_LeftArrow] && io.KeysDown[ImGuiKey_RightArrow]) {
				p1_x = 0x80;
			}
			else if (io.KeysDown[ImGuiKey_LeftArrow]) {
				p1_x = 0x00;
			}
			else if (io.KeysDown[ImGuiKey_RightArrow]) {
				p1_x = 0xff;
			}
			else {
				p1_x = 0x80;
			}
			uint8_t p1_b1 = io.KeysDown[ImGuiKey_A] ? 1 : 0;
			uint8_t p1_b2 = io.KeysDown[ImGuiKey_S] ? 1 : 0;
			uint8_t p1_b3 = io.KeysDown[ImGuiKey_D] ? 1 : 0;
			uint8_t p1_b4 = io.KeysDown[ImGuiKey_F] ? 1 : 0;

			pVectrex->SetPlayerOne(p1_x, p1_y, p1_b1, p1_b2, p1_b3, p1_b4);
			pVectrex->SetPlayerTwo(0x80, 0x80, 0, 0, 0, 0);
			// Define the pf_mono_t => pf_argb_t transform
			auto mono_to_argb = [](const vxgfx::pf_mono_t& p) {
				return vxgfx::pf_argb_t(
					0xff,
					vxgfx::pf_argb_t::to_c8(p.value),
					vxgfx::pf_argb_t::to_c8(p.value),
					vxgfx::pf_argb_t::to_c8(p.value));
				};

			// fb => out_buffer transform
			std::transform(fb->begin(), fb->end(), out_buffer.begin(), mono_to_argb);

			updateTexture(out_buffer.data());

			// Render the texture in ImGui window
			ImGui::SetCursorPos(ImVec2(0, 0));
			ImGui::Image((void*)(intptr_t)texture, ImVec2(textureWidth, textureHeight));

			ImGui::End();
		}

		void pause() {
			paused = true;
			LOGD << "Pausing Vectrex";
			if (onPaused) {
				onPaused(paused);
			}
		}

		void resume() {
			paused = false;
			LOGD << "Resuming Vectrex";
			if (onPaused) {
				onPaused(paused);
			}
		}

		void reset() {
			pVectrex->Reset();
		}

		void step() const {
			// run the minimum number of cycles, which will be 1 instruction
			pVectrex->Run(1);
		}

	private:
		std::shared_ptr<Vectrex> pVectrex;
		std::atomic<bool> paused = false;
		std::function<void(bool)> onPaused;

		vxgfx::framebuffer<FRAME_WIDTH, FRAME_HEIGHT, vxgfx::pf_argb_t> out_buffer{};
		GLuint texture = 0;
		const int textureWidth = FRAME_WIDTH;
		const int textureHeight = FRAME_HEIGHT;

		void initTexture() {
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureWidth, textureHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void updateTexture(const vxgfx::pf_argb_t* framebuffer) {
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_BGRA, GL_UNSIGNED_BYTE, framebuffer);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	};
}