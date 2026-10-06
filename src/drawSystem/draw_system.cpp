#include "draw_system.hpp"
#include <imgui.h>
#include <GL/gl.h>
#include <algorithm>
#include <map>
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#include "stb_image.h"
#pragma GCC diagnostic pop

struct Texture {
    GLuint id = 0;
    int width = 0;
    int height = 0;
};

static std::map<std::string, Texture> s_textures;

// Pos is a percentage of the display, the anchor picks which point of the block sits on it
static ImVec2 blockOrigin(const Block& block, const ImVec2& size) {
    ImGuiIO& io = ImGui::GetIO();

    return ImVec2(
        (block.pos.x / 100.0f) * io.DisplaySize.x - size.x * block.anchor.x,
        (block.pos.y / 100.0f) * io.DisplaySize.y - size.y * block.anchor.y
    );
}

void drawTextBlock(const TextBlock& block) {
    if (block.font_ptr) ImGui::PushFont(block.font_ptr);

    const float pad_x = 12.0f;
    const float pad_y = 8.0f;

    ImVec2 text_size = ImGui::CalcTextSize(block.text.c_str());
    ImVec2 rect_size = ImVec2(text_size.x + (pad_x * 2), text_size.y + (pad_y * 2));
    ImVec2 rect_min = blockOrigin(block, rect_size);
    ImVec2 rect_max = ImVec2(rect_min.x + rect_size.x, rect_min.y + rect_size.y);

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

    ImGui::SetCursorScreenPos(ImVec2(rect_min.x + pad_x, rect_min.y + pad_y));
    ImGui::TextColored(final_text_color, "%s", block.text.c_str());

    if (block.font_ptr) ImGui::PopFont();
}

bool drawButtonBlock(const ButtonBlock& btn) {
    if (btn.font_ptr) ImGui::PushFont(btn.font_ptr);

    ImVec2 text_size = ImGui::CalcTextSize(btn.label.c_str(), nullptr, true);

    float padding_x = btn.size.x;
    float padding_y = btn.size.y;

    ImVec2 real_size = ImVec2(
        text_size.x + (padding_x * 2.0f),
        text_size.y + (padding_y * 2.0f)
    );
    ImVec2 real_pos = blockOrigin(btn, real_size);

    bool is_hovered = ImGui::IsMouseHoveringRect(real_pos, ImVec2(real_pos.x + real_size.x, real_pos.y + real_size.y));

    ImGui::SetCursorScreenPos(real_pos);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding_x, padding_y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, btn.border_size);

    ImGui::PushStyleColor(ImGuiCol_Text, is_hovered && btn.can_be_hover_tx ? btn.hover_color : btn.color);
    ImGui::PushStyleColor(ImGuiCol_Button, btn.bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn.hover_bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, btn.hover_bg_color);
    ImGui::PushStyleColor(ImGuiCol_Border, is_hovered && btn.can_be_hover_border ? btn.hover_border_color : btn.border_color);

    // Buttons sharing a label would otherwise share an ImGui ID
    ImGui::PushID(btn.name.c_str());
    bool clicked = ImGui::Button(btn.label.c_str(), real_size);
    ImGui::PopID();

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);

    if (btn.font_ptr) ImGui::PopFont();

    return clicked;
}

void drawBarBlock(const BarBlock& bar, float fraction) {
    ImVec2 rect_min = blockOrigin(bar, bar.size);
    ImVec2 rect_max = ImVec2(rect_min.x + bar.size.x, rect_min.y + bar.size.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (bar.bg_color.w > 0.0f) {
        draw_list->AddRectFilled(rect_min, rect_max, ImGui::ColorConvertFloat4ToU32(bar.bg_color));
    }

    if (fraction > 0.0f) {
        ImVec2 fill_max = ImVec2(rect_min.x + bar.size.x * fraction, rect_max.y);
        draw_list->AddRectFilled(rect_min, fill_max, ImGui::ColorConvertFloat4ToU32(bar.color));
    }

    if (bar.border_size > 0.0f) {
        draw_list->AddRect(
            rect_min, rect_max,
            ImGui::ColorConvertFloat4ToU32(bar.border_color),
            0.0f, 0, bar.border_size
        );
    }
}

static const Texture& loadTexture(const std::string& path) {
    auto it = s_textures.find(path);
    if (it != s_textures.end()) return it->second;

    // A failed load is cached too, so a missing file is not retried every frame
    Texture& texture = s_textures[path];

    int channels;
    unsigned char* pixels = stbi_load(path.c_str(), &texture.width, &texture.height, &channels, 4);
    if (!pixels) return texture;

    GLint previous_texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glBindTexture(GL_TEXTURE_2D, previous_texture);
    stbi_image_free(pixels);

    return texture;
}

void drawImageBlock(const ImageBlock& img) {
    const Texture& texture = loadTexture(img.path);
    if (texture.id == 0) return;

    ImVec2 size = img.size;
    if (size.x <= 0.0f || size.y <= 0.0f) size = ImVec2((float)texture.width, (float)texture.height);

    ImVec2 rect_min = blockOrigin(img, size);
    ImVec2 rect_max = ImVec2(rect_min.x + size.x, rect_min.y + size.y);

    ImGui::GetWindowDrawList()->AddImage(
        ImTextureRef((ImTextureID)texture.id),
        rect_min, rect_max,
        ImVec2(0, 0), ImVec2(1, 1),
        ImGui::ColorConvertFloat4ToU32(img.tint)
    );
}

void clearImageCache() {
    for (const auto& [path, texture] : s_textures)
        if (texture.id != 0) glDeleteTextures(1, &texture.id);
    s_textures.clear();
}

void drawErrors(const std::vector<std::string>& errors) {
    if (errors.empty()) return;

    const float pad = 8.0f;
    const float line_height = ImGui::GetTextLineHeightWithSpacing();
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();

    float width = 0.0f;
    for (const auto& message : errors)
        width = std::max(width, ImGui::CalcTextSize(message.c_str()).x);

    ImVec2 rect_min = ImVec2(10.0f, 10.0f);
    ImVec2 rect_max = ImVec2(rect_min.x + width + (pad * 2), rect_min.y + line_height * errors.size() + (pad * 2));
    draw_list->AddRectFilled(rect_min, rect_max, IM_COL32(0, 0, 0, 200));

    float y = rect_min.y + pad;
    for (const auto& message : errors) {
        draw_list->AddText(ImVec2(rect_min.x + pad, y), IM_COL32(255, 90, 90, 255), message.c_str());
        y += line_height;
    }
}
