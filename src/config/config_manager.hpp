#pragma once

#include <future>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <imgui.h>

struct TextBlock {
    std::string text;
    ImVec2 pos;

    ImVec4 color = ImVec4(1, 1, 1, 1);
    ImVec4 hover_color = ImVec4(1, 1, 1, 1);

    ImVec4 bg_color = ImVec4(0, 0, 0, 0);
    ImVec4 hover_bg_color = ImVec4(0, 0, 0, 0);

    float border_size = 0.0f;
    ImVec4 border_color = ImVec4(1, 1, 1, 1);

    bool   can_be_hover_tx = false;
    bool   can_be_hover_bg = false;

    std::string font_name;
    float font_size;
    ImFont* font_ptr = nullptr;

    bool is_bash = false;
    std::string bash_command;
    float refresh_rate_seconds = 1.0f;

    std::future<std::string> future_result;
    std::chrono::steady_clock::time_point last_refresh;
    bool is_running = false;
};

struct ButtonBlock {
    std::string label;
    ImVec2 pos;
    ImVec2 size;
    std::string target_var;
    std::string action;
    float value_modifier = 1.0f;
    bool  has_border = false;
    float border = 0.0f;

    ImVec4 color = ImVec4(1, 1, 1, 1);
    ImVec4 bg_color = ImVec4(0.2f, 0.2f, 0.2f, 1);
    ImVec4 hover_color = ImVec4(1, 1, 1, 1);
    ImVec4 hover_bg_color = ImVec4(0.4f, 0.4f, 0.4f, 1);
    bool   can_be_hover_tx = false;
    bool   can_be_hover_bg = false;

    float border_size = 0.0f;
    ImVec4 border_color = ImVec4(1, 1, 1, 1);
    ImVec4 hover_border_color = ImVec4(1, 1, 1, 1);
};

class ConfigManager {
public:
    ConfigManager();

    const std::string& getIniPath() const { return m_ini_path; }

    std::vector<TextBlock>& getIDTextBlocks() { return m_text_blocks; }
    std::vector<ButtonBlock>& getButtonBlocks() { return m_button_blocks; }
    std::map<std::string, float>& getVariables() { return m_variables; }

    void loadAndPrepareConfig(ImGuiIO& io);
    void updateBashBlocks();

private:
    std::string m_ini_path;
    std::map<std::string, ImFont*> m_loaded_fonts;

    std::vector<TextBlock> m_text_blocks;
    std::vector<ButtonBlock> m_button_blocks;
    std::map<std::string, float> m_variables;

    void parseConfig();
    void initFonts(ImGuiIO& io, std::vector<TextBlock>& blocks);
    void createDefaultIniFile() const;
    std::string execBash(const std::string& cmd);
};
