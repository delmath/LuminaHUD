#include "config_manager.hpp"


void drawTextBlock(const TextBlock& block) {
    if (block.font_ptr) {
        ImGui::PushFont(block.font_ptr);
    }

    ImGui::SetCursorScreenPos(block.pos);
    ImGui::PushStyleColor(ImGuiCol_Text, block.color);

    ImGui::TextUnformatted(block.text.c_str());

    ImGui::PopStyleColor();

    if (block.font_ptr) {
        ImGui::PopFont();
    }
}

void drawButtonBlock(ButtonBlock& btn, std::map<std::string, float>& variables) {
    ImGui::SetCursorScreenPos(btn.pos);

    if (ImGui::Button(btn.label.c_str(), btn.size)) {
        if (btn.action == "increment") {
            variables[btn.target_var] += btn.value_modifier;
        } else if (btn.action == "decrement") {
            variables[btn.target_var] -= btn.value_modifier;
        }
    }
}
