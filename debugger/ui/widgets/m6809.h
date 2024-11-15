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
    class M6809Widget : public Widget {
    public:
        M6809Widget(std::shared_ptr<DebugVectrex> pVectrex, Configuration& config)
            : Widget(config), m6809(pVectrex->GetM6809()) {
        }

        void render(long delta) override {
            ImGui::Begin("M6809", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

            auto registers = m6809.getRegisters();

            if (ImGui::BeginTable("Registers", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Register");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("D");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%04X", registers.D);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("A");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", registers.A);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("B");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", registers.B);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("X");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%04X", registers.X);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Y");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%04X", registers.Y);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("PC");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%04X", registers.PC);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("USP");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%04X", registers.USP);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("SP");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%04X", registers.SP);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("DP");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", registers.DP);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("CC");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", registers.CC);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Flags");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("C:%d V:%d Z:%d N:%d I:%d H:%d F:%d E:%d",
                    registers.flags.C, registers.flags.V, registers.flags.Z, registers.flags.N,
                    registers.flags.I, registers.flags.H, registers.flags.F, registers.flags.E);

                ImGui::EndTable();
            }

            ImGui::End();
        }

    private:
        M6809& m6809;
    };
}
