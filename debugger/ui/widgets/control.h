#pragma once 

#include <chrono>
#include <functional>
#include <imgui.h>
#include <vectrexia.h>
#include <glad/glad.h>
#include <fmt/format.h>
#include <plog/Log.h>

#include "widget.h"
#include "../../util.h"
#include "vectrex.h"

namespace debugger::ui::widget {
    class VectrexControlWidget : public Widget {
    public:
        VectrexControlWidget(Configuration& config)
            : Widget(config), onPause(nullptr), onResume(nullptr), onStep(nullptr) {
            showWindow = true;
            paused = false;
        }

        void setPauseCallback(const std::function<void()>& callback) {
            onPause = callback;
        }

        void setResumeCallback(const std::function<void()>& callback) {
            onResume = callback;
        }

        void setStepCallback(const std::function<void()>& callback) {
            onStep = callback;
        }

        void setResetCallback(const std::function<void()>& callback) {
            onReset = callback;
        }

        void render(long delta) override {
            ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

            if (ImGui::Button(paused.load() ? "Resume" : "Pause")) {
                if (paused.load()) {
                    if (onResume) {
                        onResume();
                    }
                }
                else {
                    if (onPause) {
                        onPause();
                    }
                }
            }
            ImGui::SameLine();

            if (paused.load()) {
                if (ImGui::Button("Step")) {
                    if (onStep) {
                        onStep();
                    }
                }
            }
            else {
                ImGui::BeginDisabled();
                ImGui::Button("Step");
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                if (onReset) {
                    onReset();
                }
            }

            ImGui::End();
        }

		void setPaused(bool paused) {
            this->paused = paused;
		}

    private:
        std::function<void()> onPause;
        std::function<void()> onResume;
        std::function<void()> onStep;
        std::function<void()> onReset;
        std::atomic<bool> paused;
    };
}
