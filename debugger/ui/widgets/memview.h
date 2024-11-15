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

    constexpr int bytesPerLine = 16;

    class VectrexMemoryWidget : public Widget {
    public:
        VectrexMemoryWidget(std::shared_ptr<DebugVectrex> pVectrex, Configuration& config) : Widget(config), pVectrex(pVectrex) {
        }

        void render(long delta) override {
            bool bShowWindow = showWindow.load();
            if (bShowWindow) {
                ImGui::Begin("Memory View", &bShowWindow);

                auto memory = pVectrex->getMemory();
                size_t totalLines = memory.size() / bytesPerLine;

                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(totalLines));

                while (clipper.Step()) {
                    for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; ++line) {
                        size_t i = line * bytesPerLine;

                        // Print the memory address
                        char addressStr[16];
                        snprintf(addressStr, sizeof(addressStr), "%04X: ", static_cast<uint16_t>(i));
                        ImGui::TextUnformatted(addressStr);
                        ImGui::SameLine();

                        // Print the hex representation of each byte, grouped in 8 + 8
                        for (int j = 0; j < bytesPerLine; ++j) {
                            if (i + j < memory.size()) {
                                char hexStr[8];
                                snprintf(hexStr, sizeof(hexStr), "%02X", memory[i + j]);
                                ImGui::TextUnformatted(hexStr);
                                ImGui::SameLine();

                                // Add extra spacing after the first group of 8 bytes
                                if ((j + 1) % 8 == 0 && (j + 1) != bytesPerLine) {
                                    ImGui::Dummy(ImVec2(10.0f, 0.0f));
                                    ImGui::SameLine();
                                }
                            }
                        }

                        // Add some spacing between hex and ASCII representation
                        ImGui::SameLine();
                        ImGui::Dummy(ImVec2(20.0f, 0.0f));
                        ImGui::SameLine();

                        // Print the ASCII representation of each byte, grouped in 8 + 8
                        for (int j = 0; j < bytesPerLine; ++j) {
                            if (i + j < memory.size()) {
                                unsigned char c = memory[i + j];
                                char asciiStr[2] = { (c >= 32 && c <= 126) ? c : '.', '\0' };
                                ImGui::TextUnformatted(asciiStr);
                                ImGui::SameLine();

                                if ((j + 1) % 8 == 0 && (j + 1) != bytesPerLine) {
                                    ImGui::Dummy(ImVec2(10.0f, 0.0f));
                                    ImGui::SameLine();
                                }
                            }
                        }

                        // End the current line
                        ImGui::NewLine();
                    }
                }

                clipper.End();

                ImGui::End();
                showWindow = bShowWindow;
            }
        }

    private:
        std::shared_ptr<DebugVectrex> pVectrex;
    };
}
