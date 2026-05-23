#include "config/config_manager.hpp"
#include <imgui.h>
#include <map>
#include <string>

void drawTextBlock(const TextBlock& block) {
    ImGuiIO& io = ImGui::GetIO();

    ImVec2 real_pos = ImVec2(
        (block.pos.x / 100.0f) * io.DisplaySize.x,
        (block.pos.y / 100.0f) * io.DisplaySize.y
    );

    if (block.font_ptr) ImGui::PushFont(block.font_ptr);

    float pad_x = 12.0f;
    float pad_y = 8.0f;

    ImVec2 text_size = ImGui::CalcTextSize(block.text.c_str());
    ImVec2 rect_min = real_pos;
    ImVec2 rect_max = ImVec2(real_pos.x + text_size.x + (pad_x * 2), real_pos.y + text_size.y + (pad_y * 2));

    bool is_hovered = ImGui::IsMouseHoveringRect(rect_min, rect_max);

    ImVec4 final_text_color = is_hovered ? block.hover_color : block.color;
    ImVec4 final_bg_color   = is_hovered ? block.hover_bg_color : block.bg_color;

    if (final_bg_color.w > 0.0f) {
        ImGui::GetWindowDrawList()->AddRectFilled(rect_min, rect_max, ImGui::ColorConvertFloat4ToU32(final_bg_color));
    }

    if (block.border_size > 0.0f) {
        ImGui::GetWindowDrawList()->AddRect(
            rect_min, rect_max,
            ImGui::ColorConvertFloat4ToU32(block.border_color),
            0.0f, 0, block.border_size
        );
    }

    ImGui::SetCursorScreenPos(ImVec2(real_pos.x + pad_x, real_pos.y + pad_y));
    ImGui::TextColored(final_text_color, "%s", block.text.c_str());

    if (block.font_ptr) ImGui::PopFont();
}

void drawButtonBlock(ButtonBlock& btn, std::map<std::string, float>& variables) {
    ImGuiIO& io = ImGui::GetIO();

    ImVec2 real_pos = ImVec2(
        (btn.pos.x / 100.0f) * io.DisplaySize.x,
        (btn.pos.y / 100.0f) * io.DisplaySize.y
    );
    ImVec2 real_size = ImVec2(
        (btn.size.x / 100.0f) * io.DisplaySize.x,
        (btn.size.y / 100.0f) * io.DisplaySize.y
    );

    ImGui::SetCursorScreenPos(real_pos);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, btn.border_size);

    ImGui::PushStyleColor(ImGuiCol_Text, btn.color);
    ImGui::PushStyleColor(ImGuiCol_Button, btn.bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn.hover_bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, btn.hover_bg_color);
    ImGui::PushStyleColor(ImGuiCol_Border, btn.border_color);

    if (ImGui::Button(btn.label.c_str(), real_size)) {
        if (btn.action == "increment" && (btn.has_border && variables[btn.target_var] < btn.border)) {
        	if (variables[btn.target_var] + btn.value_modifier <= btn.border)
            	variables[btn.target_var] += btn.value_modifier;
        	else
        		variables[btn.target_var] = btn.border;
        } else if (btn.action == "decrement" && (btn.has_border && variables[btn.target_var] > btn.border)) {
       	if (variables[btn.target_var] - btn.value_modifier >= btn.border)
           	variables[btn.target_var] -= btn.value_modifier;
       	else
       		variables[btn.target_var] = btn.border;
        } else if (btn.action == "set") {
        	variables[btn.target_var] = btn.value_modifier;
        }
    }

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(1);
}
