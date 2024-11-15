#pragma once 

#include <imgui.h>
#include <vectrexia.h>
#include <fmt/format.h>
#include <plog/Log.h>

#include "widget.h"
#include "../../util.h"
#include "vectrex.h"

namespace debugger::ui::widget {
    class ControllerInputWidget : public Widget {
    public:
        ControllerInputWidget(std::shared_ptr<DebugVectrex> pVectrex, Configuration& config)
            : Widget(config), pVectrex(pVectrex) {
        }

        void render(long delta) override {
            ImGui::Begin("Controller Input", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavInputs);

            if (ImGui::BeginTable("Controllers", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("");
                ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("1");
                ImGui::TableSetupColumn("2");
                ImGui::TableSetupColumn("3");
                ImGui::TableSetupColumn("4");
                ImGui::TableHeadersRow();

				VectrexController player1 = pVectrex->getPlayer1();
				VectrexController player2 = pVectrex->getPlayer2();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Player 1");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%d", static_cast<int>(player1.pot_x) - 0x80);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%d", static_cast<int>(player1.pot_y) - 0x80);
                ImGui::TableSetColumnIndex(3); ImGui::Text("%d", player1.btn_1 ^ 1);
                ImGui::TableSetColumnIndex(4); ImGui::Text("%d", player1.btn_2 ^ 1);
                ImGui::TableSetColumnIndex(5); ImGui::Text("%d", player1.btn_3 ^ 1);
                ImGui::TableSetColumnIndex(6); ImGui::Text("%d", player1.btn_4 ^ 1);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Player 2");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%d", static_cast<int>(player2.pot_x) - 0x80);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%d", static_cast<int>(player2.pot_y) - 0x80);
                ImGui::TableSetColumnIndex(3); ImGui::Text("%d", player2.btn_1 ^ 1);
                ImGui::TableSetColumnIndex(4); ImGui::Text("%d", player2.btn_2 ^ 1);
                ImGui::TableSetColumnIndex(5); ImGui::Text("%d", player2.btn_3 ^ 1);
                ImGui::TableSetColumnIndex(6); ImGui::Text("%d", player2.btn_4 ^ 1);

                ImGui::EndTable();
            }

            ImGui::End();
        }

    private:
        std::shared_ptr<DebugVectrex> pVectrex;
    };
}
