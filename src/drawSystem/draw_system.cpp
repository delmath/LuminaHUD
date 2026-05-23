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

    const float pad_x = 12.0f;
    const float pad_y = 8.0f;

    ImVec2 text_size = ImGui::CalcTextSize(block.text.c_str());
    ImVec2 rect_min = real_pos;
    ImVec2 rect_max = ImVec2(real_pos.x + text_size.x + (pad_x * 2), real_pos.y + text_size.y + (pad_y * 2));

    bool is_hovered = ImGui::IsMouseHoveringRect(rect_min, rect_max);

    const ImVec4& final_text_color = is_hovered && block.can_be_hover_tx ? block.hover_color : block.color;
    const ImVec4& final_bg_color   = is_hovered && block.can_be_hover_bg ? block.hover_bg_color : block.bg_color;

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

    ImVec2 text_size = ImGui::CalcTextSize(btn.label.c_str());

    float padding_x = btn.size.x;
    float padding_y = btn.size.y;

    ImVec2 real_size = ImVec2(
        text_size.x + (padding_x * 2.0f),
        text_size.y + (padding_y * 2.0f)
    );

    ImGui::SetCursorScreenPos(real_pos);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding_x, padding_y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, btn.border_size);

    ImGui::PushStyleColor(ImGuiCol_Text, btn.color);
    ImGui::PushStyleColor(ImGuiCol_Button, btn.bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn.hover_bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, btn.hover_bg_color);
    ImGui::PushStyleColor(ImGuiCol_Border, btn.border_color);

    if (ImGui::Button(btn.label.c_str(), real_size)) {
        float& target = variables[btn.target_var];

        if (btn.action == "increment" && btn.has_border && target < btn.border) {
            if (target + btn.value_modifier <= btn.border)
                target += btn.value_modifier;
            else
                target = btn.border;
        } else if (btn.action == "decrement" && btn.has_border && target > btn.border) {
            if (target - btn.value_modifier >= btn.border)
                target -= btn.value_modifier;
            else
                target = btn.border;
        } else if (btn.action == "set") {
            target = btn.value_modifier;
        }
    }

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);
}
