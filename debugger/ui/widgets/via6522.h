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
    class VIA6522Widget : public Widget {
    public:
        VIA6522Widget(std::shared_ptr<DebugVectrex> pVectrex, Configuration& config)
            : Widget(config), via6522(pVectrex->GetVIA6522()) {
        }

        void render(long delta) override {
            ImGui::Begin("VIA6522", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

            if (ImGui::BeginTable("Registers", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Register");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();


                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_ORB");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_ORB));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_ORA");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_ORA));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_DDRB");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_DDRB));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_DDRA");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_DDRA));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_T1CL");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_T1CL));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_T1CH");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_T1CH));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_T1LL");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_T1LL));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_T1LH");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_T1LH));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_T2CL");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_T2CL));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_T2CH");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_T2CH));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_SR");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_SR));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_ACR");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_ACR));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_PCR");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_PCR));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_IFR");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_IFR));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_IER");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_IER));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("REG_ORA_NO_HANDSHAKE");
                ImGui::TableSetColumnIndex(1); ImGui::Text("0x%02X", via6522.Peek(REG_ORA_NO_HANDSHAKE));
                
                ImGui::EndTable();
            }

            ImGui::End();
        }

    private:
        VIA6522& via6522;
    };
}
