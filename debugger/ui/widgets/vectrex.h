#pragma once
#include <chrono>
#include <imgui.h>
#include <vectrexia.h>
#include <glad/glad.h>
#include <fmt/format.h>
#include <plog/Log.h>

#include "widget.h"
#include "../../util.h"

namespace debugger::ui::widget {
	class VectrexWidget : public Widget {
	public:
		static const int FRAME_WIDTH = 330;
		static const int FRAME_HEIGHT = 410;

		VectrexWidget(Configuration& config) : Widget(config) {
			showWindow = true;
			vectrex = std::make_unique<Vectrex>();
			vectrex->Reset();

			vectrex->SetPlayerOne(0x80, 0x80, 1, 1, 1, 1);
			vectrex->SetPlayerTwo(0x80, 0x80, 1, 1, 1, 1);

			initTexture();
		}

		// delta is in ms
		void process(long delta) override {
			if (!paused) {
				auto deltams = std::chrono::milliseconds(delta);
				auto deltans = std::chrono::duration_cast<std::chrono::nanoseconds>(deltams);
				auto cycles = TimerUtil::nanos_to_cycles(deltans.count());
				vectrex->Run(std::min<uint64_t>(cycles, 30000));
			}
		}

		void render(long delta) override {
			// render the frame buffer from the vectrex in to this window
			// the window should be a fix ratio FRAME_WIDTH:FRAME_HEIGHT, with a minium size of FRAME_WIDTHxFRAME_HEIGHT
			ImGui::SetNextWindowSize(ImVec2(FRAME_WIDTH, FRAME_HEIGHT));
			ImGui::Begin("Vectrex", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
			auto fb = vectrex->getFramebuffer();

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
		}

		void resume() {
			paused = false;
		}

	private:
		std::atomic<bool> paused = false;
		std::unique_ptr<Vectrex> vectrex;

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