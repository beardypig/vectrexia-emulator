#pragma once
#define _PEPARSE_WINDOWS_CONFLICTS
#include <memory>
#include <fstream>
#include <vector>
#include <optional>
#include <fmt/format.h>
#include "widget.h"
#include "../color.h"
#include <vectrexia.h>
#include <map>
#include <set>
#include <future>
#include <stack>

template <>
struct fmt::formatter<M6809Register> {
    // Parse format specifications (e.g., alignment, width, etc.)
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    // Format the M6809Register enum
    template <typename FormatContext>
    auto format(const M6809Register& reg, FormatContext& ctx) const -> decltype(ctx.out()) {
        // Map the enum value to its string representation
        const char* name = "UNKNOWN";
        switch (reg) {
        case M6809Register::A:       name = "A"; break;
        case M6809Register::B:       name = "B"; break;
        case M6809Register::D:       name = "D"; break;
        case M6809Register::X:       name = "X"; break;
        case M6809Register::Y:       name = "Y"; break;
        case M6809Register::U:       name = "U"; break;
        case M6809Register::S:       name = "S"; break;
        case M6809Register::PC:      name = "PC"; break;
        case M6809Register::CC:      name = "CC"; break;
        case M6809Register::DP:      name = "DP"; break;
        case M6809Register::INVALID: name = "INVALID"; break;
        }
        return fmt::format_to(ctx.out(), "{}", name);
    }
};

namespace debugger::ui::color::syntax {
    const auto address = color::gray;
    const auto mnemonic = color::lightBlue;
    const auto data = color::teal;
    const auto op_register = color::yellow;
    const auto op_memory = color::green;
    const auto op_immediate = color::orange;
    const auto op_relative = color::orange;
    const auto op_default = color::white;
    const auto xRefDefault = color::white;
    const auto xRefHover = color::lightBlue;
}

namespace debugger::ui::widget
{
    constexpr uint16_t MAX_LABEL_SIZE = 16;

    class VectrexDisassemblyWidget : public Widget {
    public:
        VectrexDisassemblyWidget(std::shared_ptr<DebugVectrex> pVectrex, Configuration& config) : Widget(config), pVectrex(pVectrex), disassembler(pVectrex->GetM6809().getDisassembler())
        {
            // dynamic labels
            setTempLabel(disassembler.Read<uint16_t>(M6809::RESET_VECTOR), "RESET");
            setTempLabel(disassembler.Read<uint16_t>(M6809::SWI1_VECTOR), "SW1");
            setTempLabel(disassembler.Read<uint16_t>(M6809::SWI2_VECTOR), "SW2");
            setTempLabel(disassembler.Read<uint16_t>(M6809::SWI3_VECTOR), "SW3");
            setTempLabel(disassembler.Read<uint16_t>(M6809::NMI_VECTOR), "NMI");
            setTempLabel(disassembler.Read<uint16_t>(M6809::IRQ_VECTOR), "IRQ");
            setTempLabel(disassembler.Read<uint16_t>(M6809::FIRQ_VECTOR), "FIRQ");

            // add the labels from the config file to the disassembly
            config.applyLabels([this](uint16_t address, const std::string& label) {
                setLabel(address, label);
                });
            
            // start off with a set of valid addresses from the vector table
            addToAnalyseQueue(disassembler.Read<uint16_t>(M6809::RESET_VECTOR));
        }

		void addToAnalyseQueue(uint16_t address) {
			analyseQueue.push(address);
			LOGD << fmt::format("Adding address {:#06x} to the analysis queue (total={})", address, analyseQueue.size());
		}

        void setLabel(uint16_t address, const char* label) {
			setLabel(address, std::string(label));
        }
        
        void setLabel(uint16_t address, const std::string& label) {
            labels.insert_or_assign(address, label);
            config.setLabel(address, label);
			LOGD << fmt::format("Setting label {:#06x} to {}", address, label);
        }        
        
        void setTempLabel(uint16_t address, const char* label) {
            labels.emplace(address, label);
        }
        
        void setTempLabel(uint16_t address, const std::string& label) {
            labels.emplace(address, label);
        }

        void process(long delta) override {
            if (progress.load() >= 100.0f && analyseTask.valid()) {
                // make sure we don't block in the case of a bug/race
                if (analyseTask.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) {
                    analyseTask.get();
                }
            }
            if (!analyseQueue.empty() && !analyseTask.valid()) {
				LOGI << "Starting disassembly analysis";
                analyseTask = std::async(std::launch::async, &VectrexDisassemblyWidget::analyse, this, std::ref(progress));
            }
        }

        void render(long delta) override {
            charWidth = ImGui::CalcTextSize(" ").x;
            bool bShowWindow = showWindow.load();

            static char addressInput[6] = "";
            static bool addressError = false;
            bool enterPressed = false;

            if (bShowWindow) {
                if (ImGui::Begin("Disassembly", &bShowWindow)) {
                    // Address input box and jump button
                    ImGui::Text("Jump to Address:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(80); // Set input box width
                    if (ImGui::InputText("##AddressInput", addressInput, sizeof(addressInput), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        // Clear the error flag when editing the input
                        addressError = false;
                        enterPressed = true;
                    }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();

                    if (ImGui::Button("Jump") || enterPressed) {
                        try {
                            std::string input(addressInput);

                            // Check if the input matches a label
                            auto it = std::find_if(labels.begin(), labels.end(),
                                [&input](const auto& pair) {
                                    return pair.second == input;
                                });

                            if (it != labels.end()) {
                                // Found a matching label, use the associated address
                                jumpToAddress(it->first);
                            }
                            else {
                                // Parse input as a hexadecimal address
                                uint32_t addr = static_cast<uint32_t>(std::stoul(input, nullptr, 16));

                                // Validate the address range (0x0000 to 0xFFFF)
                                if (addr >= 0x0000 && addr <= 0xFFFF) {
                                    jumpToAddress(addr);
                                }
                                else {
                                    addressError = true; // Invalid range
                                }
                            }
                        }
                        catch (const std::exception&) {
                            addressError = true; // Invalid input (e.g., non-hex or invalid label)
                        }
                    }

                    ImGui::SameLine();
                    if (analyseTask.valid()) {
                        ImGui::ProgressBar(progress.load() / 100.0f, ImVec2(0.0f, 0.0f));
                    }
                    else {
                        if (ImGui::Button("ReAnalyse")) {
                            // start off with a set of valid addresses from the vector table
                            addToAnalyseQueue(disassembler.Read<uint16_t>(M6809::RESET_VECTOR));
                            addToAnalyseQueue(pVectrex->GetM6809().getRegisters().PC);
                        }
                    }

                    if (addressError) {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid address. Please enter a value between 0000 and FFFF.");
                    }

                    // Set up the scrollable disassembly region
                    renderInstructions(instructions);
                }
                ImGui::End();
            }

            showWindow = bShowWindow;
        }


        void jumpToAddress(uint16_t addr) {
			LOGD << fmt::format("Jumping to address {:#06x}", addr);
            scrollToAddress = addr;        
            shouldScrollToAddress = true;   
        }

    private:
        std::shared_ptr<DebugVectrex> pVectrex;
        std::vector<std::string> messages;
        M6809Disassemble& disassembler;
        std::future<void> analyseTask;
        std::atomic<float> progress = 0.0f;
        // queue of addresses to analyse
        std::stack<uint16_t> analyseQueue;

        std::map<uint16_t, std::string> labels;
        std::set<uint16_t> breakpoints;
        std::map<uint16_t, std::set<uint16_t>> xRefs;

        uint16_t scrollToAddress = 0; 
        bool shouldScrollToAddress = false;  

        // rendering 
        std::array<std::optional<M6809Instruction>, 0x10000> instructions;
        float charWidth = 0.0f;

        void analyse(std::atomic<float>& progress)
        {
            // record the addresses we have processed to avoid infinite loops
            std::set<uint16_t> visitedAddresses;
            progress = 0.0f;

            while (!analyseQueue.empty()) {
                uint16_t runtime_address = analyseQueue.top(); analyseQueue.pop();
                // skip any previously processed addresses
                if (visitedAddresses.find(runtime_address) != visitedAddresses.end()) {
                    continue;
                }
                visitedAddresses.insert(runtime_address);
                while (true) {
                    try {
                        auto instruction = disassembler.disasm(runtime_address);
                        instructions[instruction.address] = instruction;
                        runtime_address += instruction.length;
                        // TODO: build XREFs
                        if (instruction.mnemonic == "jsr" || instruction.mnemonic == "bsr" ||
                            instruction.mnemonic == "jmp" ||
                            instruction.mnemonic == "bra" || instruction.mnemonic == "lbra" ||
                            instruction.mnemonic == "brn" || instruction.mnemonic == "lbrn" ||
                            instruction.mnemonic == "bcs" || instruction.mnemonic == "lbcs" ||
                            instruction.mnemonic == "bcc" || instruction.mnemonic == "lbcc" ||
                            instruction.mnemonic == "bhi" || instruction.mnemonic == "lbhi" ||
                            instruction.mnemonic == "bls" || instruction.mnemonic == "lbls" ||
                            instruction.mnemonic == "beq" || instruction.mnemonic == "lbeq" ||
                            instruction.mnemonic == "bne" || instruction.mnemonic == "lbne" ||
                            instruction.mnemonic == "bgt" || instruction.mnemonic == "lbgt" ||
                            instruction.mnemonic == "blt" || instruction.mnemonic == "lblt" ||
                            instruction.mnemonic == "bge" || instruction.mnemonic == "lbge" ||
                            instruction.mnemonic == "ble" || instruction.mnemonic == "lble" ||
                            instruction.mnemonic == "bpl" || instruction.mnemonic == "lbpl" ||
                            instruction.mnemonic == "bmi" || instruction.mnemonic == "lbmi" ||
                            instruction.mnemonic == "bvs" || instruction.mnemonic == "lbvs" ||
                            instruction.mnemonic == "bvc" || instruction.mnemonic == "lbvc") {
                            auto operand = instruction.operands.begin();
                            uint16_t target = 0;
                            if (instruction.operands.begin()->type == M6809OperandType::EXTENDED) {
                                target = operand->extendedAddress;
                            }
                            else if (operand->type == M6809OperandType::DIRECT) {
                                target = operand->directAddress;
                            }
                            else if (operand->type == M6809OperandType::REL) {
                                target = operand->relativeAddress.base + operand->relativeAddress.offset;
                            }

                            if (instruction.mnemonic == "jsr" || instruction.mnemonic == "bsr") {
                                xRefs[target].insert(instruction.address);
                                // automatically create a label is non exists already
								setTempLabel(target, fmt::format("FUNC_{:04x}", target));
                            }
                            analyseQueue.push(target);
                        }

                        // stop on return instructions
                        if (instruction.mnemonic == "jmp" || instruction.mnemonic == "bra" || instruction.mnemonic == "rti" || instruction.mnemonic == "ret") {
                            break;
                        }
                    }
                    catch (std::exception& e) {
                        // stop on invalid instructions
                        break;
                    }
                }
            }
            progress = 100.0f;

        }

        // Clear and set a single message
        void setMessage(const std::string& message) {
            messages.clear();
            messages.push_back(message);
        }

        void formatInstruction(const M6809Instruction& instruction) {
            if (!instruction.mnemonic.empty()) { // TODO: replace mnmemonic with an enum
                // Render the mnemonic
                ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::mnemonic.Value); // Color for mnemonics
                ImGui::TextUnformatted(instruction.mnemonic.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine(0, 0);
                ImGui::TextUnformatted(" ");
                ImGui::SameLine(0, 0);

                // Render the operands
                formatOperands(instruction);
            }

        }

        void renderInstructions(const std::array<std::optional<M6809Instruction>, 0x10000>& instructions) {
            // Static variables for storing the label being edited
            static uint16_t selectedAddress = 0;  
            static char labelInput[17] = "";      
            static bool openLabelPopup = false;   
            static bool confirmLabelChange = false;
            int seekRow = 0;
            static float currentScroll = 0.0f;
            static int32_t contextMenuOpen = -1;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Start the scrollable region
            if (ImGui::BeginChild("DisassemblyRegion", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
                ImGuiListClipper clipper;

                // Prepare the rows for rendering (flatten the instruction data into rows)
                std::vector<std::pair<uint16_t, std::optional<M6809Instruction>>> rows;
                for (uint32_t addr = 0, row = 0; addr < instructions.size(); ++addr, ++row) {
                    if (instructions[addr].has_value()) {
                        rows.push_back(std::make_pair(addr, instructions[addr]));
                        addr += instructions[addr].value().length - 1; // Skip ahead by the length of the current instruction
                    }
                    else {
                        rows.push_back(std::make_pair(addr, std::nullopt));
                    }
                }

                // Start clipping for better performance
                clipper.Begin(rows.size(), ImGui::GetTextLineHeight());
                while (clipper.Step()) {
                    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                        auto [addr, instructionOpt] = rows[row];
                        auto isBreakpointSet = breakpoints.contains(addr);

                        ImGui::PushID(addr);  

						ImGui::PushStyleColor(ImGuiCol_Header, ui::color::applyTransparency(ui::color::darkGray, 0.4f).Value);           // when selected
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ui::color::applyTransparency(ui::color::darkGray, 0.4f).Value);    // hover
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, ui::color::applyTransparency(ui::color::darkGray, 0.2f).Value);     // active (click)

                        renderInstructionRow(addr, instructionOpt, isBreakpointSet);

                        // Make the entire row selectable for interaction (right-click)
                        if (ImGui::Selectable("##hiddenSelectable", contextMenuOpen == addr, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, ImGui::GetTextLineHeight()))) {
                            // left-click handler noop
                        }

                        ImGui::PopStyleColor(3);

                        // Create right-click context menu for each row
                        if (ImGui::BeginPopupContextItem("InstructionContextMenu")) {
                            selectedAddress = addr;
							contextMenuOpen = addr;
                            auto itLabel = labels.find(addr);

							// label handling
                            if (ImGui::MenuItem("Edit Label")) {
                                // Set up the selected address and label input buffer
                                if (itLabel != labels.end()) {
                                    strncpy(labelInput, itLabel->second.c_str(), sizeof(labelInput) - 1);
                                    labelInput[sizeof(labelInput) - 1] = '\0';
                                }
                                else {
                                    labelInput[0] = '\0';
                                }
                                openLabelPopup = true;
                            }
                            if (itLabel != labels.end() && ImGui::MenuItem("Remove Label")) {
								labels.erase(itLabel);
								config.removeLabel(addr);
                            }

							// breakpoint handling
                            ImGui::Separator();
                            if (!isBreakpointSet && ImGui::MenuItem("Add Breakpoint")) {
                                breakpoints.insert(addr);
                            }
                            if (isBreakpointSet && ImGui::MenuItem("Remove Breakpoint")) {
                                breakpoints.erase(addr);
                            }
                            ImGui::EndPopup();
                        }
                        // clear the highlight when closing the context menu
                        else if (contextMenuOpen == addr) {
                            contextMenuOpen = -1;
                        }

                        ImGui::PopID();
                    }
                }
                clipper.End();
                currentScroll = ImGui::GetScrollY();
                if (shouldScrollToAddress && ImGui::GetScrollMaxY() > 0.0f) {
                    //  find the closest row for the given scroll target address
                    uint32_t minDifference = UINT32_MAX;

                    for (int row = 0; row < rows.size(); ++row) {
                        // Calculate the absolute difference between current address and scrollToAddress
                        uint32_t difference = std::abs(static_cast<int32_t>(scrollToAddress) - static_cast<int32_t>(rows[row].first));

                        // Update the closest row if this address is closer to scrollToAddress
                        if (difference < minDifference) {
                            minDifference = difference;
                            seekRow = row;
                        }
                    }

                    auto scrollTo = ImGui::GetTextLineHeight() * seekRow;
                    ImGui::SetScrollY(scrollTo);
                    shouldScrollToAddress = false;
                    LOGD << fmt::format("Scrolling to {}", scrollTo);
                }
            }
            ImGui::EndChild();  // End the scrollable region

            // Check if we need to open the popup
            if (openLabelPopup) {
                ImGui::OpenPopup("Edit Label");
                openLabelPopup = false; 
            }
            if (ImGui::BeginPopupModal("Edit Label", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Enter label for address 0x%04x:", selectedAddress);
                ImGui::InputText("##LabelInput", labelInput, sizeof(labelInput) - 1);

                // Buttons to save or cancel
                if (ImGui::Button("OK", ImVec2(120, 0))) {
					// use flags to prevent this from being called multiple times
                    confirmLabelChange = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // Handle label assignment outside of the popup UI
            if (confirmLabelChange) {
				setLabel(selectedAddress, labelInput);
                confirmLabelChange = false;
                ImGui::CloseCurrentPopup();  
            }
        }

        // Render the instruction data
		void formatDataBuffer(const uint8_t* buffer, size_t count) {
            auto byteWidth = charWidth * 3;
            ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::data.Value);
            auto dataStart = ImGui::GetCursorPos();
            auto lineHeight = ImGui::GetTextLineHeight();
            for (uint8_t i = 0; i < count; ++i) {
                ImGui::SetCursorPos(ImVec2(dataStart.x + ((i % 4) * byteWidth), dataStart.y + (i / 4 * lineHeight)));
                ImGui::TextUnformatted(fmt::format("{:02x} ", buffer[i]).c_str());
            }
            ImGui::PopStyleColor();
            ImGui::SetCursorPos(ImVec2(dataStart.x + (4 * byteWidth), dataStart.y));  // move to the end of the possible data line
		}

        void renderInstructionRow(uint16_t addr, std::optional<M6809Instruction>& instructionOpt, bool breakpoint = false) {
            // offsets
            auto breakpointOffset = charWidth;
            auto labelOffset = charWidth * 2;
			auto instructionOffset = charWidth * (MAX_LABEL_SIZE + 1);  // address, data, instruction
			auto xRefOffset = charWidth * (MAX_LABEL_SIZE + 40);

            uint8_t buffer[4];
            uint16_t PC = pVectrex->GetM6809().getRegisters().PC;
            auto lineStart = ImGui::GetCursorPos();

            // Highlight the program counter
            if (addr == PC) {
                ImGui::TextColored(ui::color::orange.Value, ">");
			}
            if (breakpoint) {
				// draw a red dot for breakpoints
                ImGui::SetCursorPos(ImVec2(lineStart.x + breakpointOffset, lineStart.y));
				ImGui::TextColored(ui::color::red, "•");
            }


            // Render label if it exists
            if (labels.find(addr) != labels.end()) {
                ImGui::SameLine(0, 0);
                ImGui::SetCursorPos(ImVec2(lineStart.x + labelOffset, lineStart.y));
                ImGui::TextUnformatted(labels[addr].c_str());
            }

            ImGui::SetCursorPos(ImVec2(lineStart.x + instructionOffset, lineStart.y));
            // Render the instruction address
            ImGui::PushStyleColor(ImGuiCol_Text, ui::color::syntax::address.Value);
            ImGui::TextUnformatted(fmt::format("{:#06x}  ", addr).c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 0);

            if (!instructionOpt.has_value()) {
                buffer[0] = pVectrex->GetM6809().getDisassembler().Read<uint8_t>(addr);
                formatDataBuffer(buffer, 1);
            }
            else {
                const auto& instruction = instructionOpt.value();

                for (auto i = 0; i < instruction.length; i++) {
                    buffer[i] = pVectrex->GetM6809().getDisassembler().Read<uint8_t>(instruction.address + i);
                }

                formatDataBuffer(buffer, instruction.length);
                formatInstruction(instruction);
                addr += instruction.length - 1;
            }
            // Render cross-references
            ImGui::SetCursorPos(ImVec2(lineStart.x + xRefOffset, lineStart.y));
            if (xRefs.find(addr) != xRefs.end()) {
                renderXRefs(addr);
            }

        }

        void formatOperands(const M6809Instruction instruction)
        {
            for (const auto& operand : instruction.operands) {
                std::ostringstream ss;
				uint16_t relTarget;
				std::map<uint16_t, std::string>::iterator label;
                switch (operand.type) {
                case M6809OperandType::REG:
                    ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_register.Value);
                    ImGui::TextUnformatted(fmt::format("{}", operand.reg).c_str());
                    ImGui::PopStyleColor();
                    break;
                case M6809OperandType::IMMEDIATE:
                    ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_immediate.Value);
                    ImGui::TextUnformatted(fmt::format("#${:02x}", operand.immediate).c_str());
                    ImGui::PopStyleColor();
                    break;
                case M6809OperandType::REL:
                    ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_relative.Value);
                    relTarget = operand.relativeAddress.base + operand.relativeAddress.offset;
                    label = labels.find(relTarget);
                    if (label != labels.end()) {
                        ImGui::TextUnformatted(label->second.c_str());
                    }
                    else {
                        ImGui::TextUnformatted(fmt::format("${:04x}", relTarget).c_str());
                    }
                    ImGui::PopStyleColor();
                    break;
                case M6809OperandType::DIRECT:
                    ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_memory.Value);
                    label = labels.find(operand.directAddress);
                    if (label != labels.end()) {
                        ImGui::TextUnformatted(label->second.c_str());
                    }
                    else {
                        ImGui::TextUnformatted(fmt::format("<${:02x}", operand.directAddress).c_str());
                    }
                    ImGui::PopStyleColor();
                    break;
                case M6809OperandType::EXTENDED:
                    ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_memory.Value);
                    label = labels.find(operand.extendedAddress);
                    if (label != labels.end()) {
                        ImGui::TextUnformatted(label->second.c_str());
                    }
                    else {
                        ImGui::TextUnformatted(fmt::format("${:04x}", operand.extendedAddress).c_str());
                    }
                    ImGui::PopStyleColor();
                    break;
                case M6809OperandType::INDEXED:
                    ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_memory.Value);
                    if (operand.indexed.indirect) {
                        ImGui::TextUnformatted("[");
                        ImGui::SameLine(0, 0);
                    }

                    if (operand.indexed.indexReg == M6809Register::INVALID) {
                        ImGui::TextUnformatted(fmt::format("${:04x}", static_cast<uint16_t>(operand.indexed.offset)).c_str());
                        ImGui::SameLine(0, 0);
                    }

                    else if (operand.indexed.increment != 0) {
                        ImGui::TextUnformatted(",");
                        ImGui::SameLine(0, 0);

                        ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_register.Value);
                        ImGui::TextUnformatted(toString(operand.indexed.indexReg).c_str());
                        ImGui::SameLine(0, 0);

                        switch (operand.indexed.increment) {
                        case 1: ImGui::TextUnformatted("+"); break;
                        case 2: ImGui::TextUnformatted("++"); break;
                        case -1: ImGui::TextUnformatted("-"); break;
                        case -2: ImGui::TextUnformatted("--"); break;
                        default:
                            break;
                        }
                        ImGui::PopStyleColor();
                        ImGui::SameLine(0, 0);
                    }
                    else if (operand.indexed.offset == 0) {
                        ImGui::TextUnformatted(",");
                        ImGui::SameLine(0, 0);

                        ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_register.Value);
                        ImGui::TextUnformatted(toString(operand.indexed.indexReg).c_str());
                        ImGui::PopStyleColor();
                        ImGui::SameLine(0, 0);
                    }
                    else {
                        ImGui::TextUnformatted(fmt::format("${:02x},", operand.indexed.offset).c_str());
                        ImGui::SameLine(0, 0);

                        ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_register.Value);
                        ImGui::TextUnformatted(toString(operand.indexed.indexReg).c_str());
                        ImGui::PopStyleColor();
                        ImGui::SameLine(0, 0);
                    }
                    if (operand.indexed.indirect) {
                        ImGui::TextUnformatted("]");
                    }
                    ImGui::PopStyleColor();
                    break;
                case M6809OperandType::INHERENT:
                    break;
                default:
                    ImGui::PushStyleColor(ImGuiCol_Text, color::syntax::op_default.Value);
                    ImGui::TextUnformatted("???");
                    ImGui::PopStyleColor();
                    break;
                }
                ImGui::SameLine(0, 0);
                if (&operand != &instruction.operands.back()) {
                    ImGui::TextUnformatted(", ");
                    ImGui::SameLine(0, 0);
                }
            }
        }

        void renderXRefs(uint16_t address) {
            bool first = true;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Push a default text color
            ImGui::PushStyleColor(ImGuiCol_Text, ui::color::syntax::xRefDefault.Value);
            ImGui::PushStyleColor(ImGuiCol_TextLink, ui::color::syntax::xRefDefault.Value);
            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ui::color::syntax::xRefHover.Value);
            for (auto it = xRefs[address].begin(); it != xRefs[address].end(); ++it) {
                if (!first) {
                    ImGui::TextUnformatted(", ");
                }
                else {
					ImGui::TextUnformatted("# XREF: ");
                }
                ImGui::SameLine(0, 0);
                first = false;

                std::string xRefStr = fmt::format("{:#06x}", *it);

				if (ImGui::TextLink(xRefStr.c_str())) {
                    LOGD << "XREf Clicked: " << xRefStr;
					jumpToAddress(*it);
				}

                if (ImGui::IsItemHovered()) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }

                ImGui::SameLine(0, 0);
            }
            ImGui::PopStyleColor(3);
        }

    };
};