#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <imgui.h>

struct Comparison {
    std::string lhs;
    std::string op;
    std::string rhs;
};

// OR of AND groups, empty means always true
struct Condition {
    std::vector<std::vector<Comparison>> any_of;
};

struct Block {
    std::string name;
    ImVec2 pos = ImVec2(0, 0);
    ImVec2 anchor = ImVec2(0, 0);
    Condition when;
};

struct BashSource {
    std::string text;

    bool is_bash = false;
    std::string bash_command;
    float refresh_rate_seconds = 1.0f;

    std::future<std::string> future_result;
    std::chrono::steady_clock::time_point last_refresh;
    bool is_running = false;
    bool needs_refresh = false;
};

struct TextBlock : Block, BashSource {
    ImVec4 color = ImVec4(1, 1, 1, 1);
    ImVec4 hover_color = ImVec4(1, 1, 1, 1);

    ImVec4 bg_color = ImVec4(0, 0, 0, 0);
    ImVec4 hover_bg_color = ImVec4(0, 0, 0, 0);

    float border_size = 0.0f;
    ImVec4 border_color = ImVec4(1, 1, 1, 1);

    bool   can_be_hover_tx = false;
    bool   can_be_hover_bg = false;

    std::string font_name = "Default";
    float font_size = 13.0f;
    ImFont* font_ptr = nullptr;
};

struct ButtonBlock : Block {
    std::string label = "Button";
    ImVec2 size = ImVec2(0, 0);
    std::string target_var;
    std::string action = "increment";
    std::string exec;
    float value_modifier = 1.0f;
    bool  has_border = false;
    float border = 0.0f;

    ImVec4 color = ImVec4(1, 1, 1, 1);
    ImVec4 bg_color = ImVec4(0.2f, 0.2f, 0.2f, 1);
    ImVec4 hover_color = ImVec4(1, 1, 1, 1);
    ImVec4 hover_bg_color = ImVec4(0.4f, 0.4f, 0.4f, 1);
    bool   can_be_hover_tx = false;

    float border_size = 0.0f;
    ImVec4 border_color = ImVec4(1, 1, 1, 1);
    ImVec4 hover_border_color = ImVec4(1, 1, 1, 1);
    bool   can_be_hover_border = false;

    std::string font_name = "Default";
    float font_size = 13.0f;
    ImFont* font_ptr = nullptr;
};

struct BarBlock : Block, BashSource {
    ImVec2 size = ImVec2(200, 12);
    std::string source_var;
    float value = 0.0f;
    float min = 0.0f;
    float max = 100.0f;

    ImVec4 color = ImVec4(1, 1, 1, 1);
    ImVec4 bg_color = ImVec4(0.2f, 0.2f, 0.2f, 1);

    float border_size = 0.0f;
    ImVec4 border_color = ImVec4(1, 1, 1, 1);
};

struct ImageBlock : Block {
    std::string path;
    ImVec2 size = ImVec2(0, 0);
    ImVec4 tint = ImVec4(1, 1, 1, 1);
};

class ConfigManager {
public:
    static constexpr int MONITOR_ALL = -1;
    static constexpr int MONITOR_PRIMARY = -2;

    ConfigManager();

    const std::string& getIniPath() const { return m_ini_path; }

    std::vector<TextBlock>& getIDTextBlocks() { return m_text_blocks; }
    std::vector<ButtonBlock>& getButtonBlocks() { return m_button_blocks; }
    std::vector<BarBlock>& getBarBlocks() { return m_bar_blocks; }
    std::vector<ImageBlock>& getImageBlocks() { return m_image_blocks; }
    std::map<std::string, float>& getVariables() { return m_variables; }
    const std::vector<std::string>& getErrors() const { return m_errors; }
    int getMonitor() const { return m_monitor; }

    void setScreenCount(int count) { m_screen_count = count; }

    void loadAndPrepareConfig(ImGuiIO& io);
    void updateBashBlocks();
    float secondsUntilNextRefresh(float max_wait) const;

    bool isVisible(const Block& block) const { return evaluate(block.when); }
    float barFraction(const BarBlock& bar) const;
    void pressButton(const ButtonBlock& btn);
    bool consumeReloadRequest();

private:
    enum class Apply { Applied, Invalid, Unknown };

    struct Entry {
        std::string key;
        std::string value;
        int line;
    };

    struct Section {
        std::string name;
        int line;
        std::vector<Entry> entries;
    };

    std::string m_ini_path;
    std::map<std::string, ImFont*> m_loaded_fonts;

    std::vector<TextBlock> m_text_blocks;
    std::vector<ButtonBlock> m_button_blocks;
    std::vector<BarBlock> m_bar_blocks;
    std::vector<ImageBlock> m_image_blocks;
    std::map<std::string, float> m_variables;
    std::map<std::string, std::string> m_theme;
    std::vector<std::string> m_errors;

    int m_monitor = MONITOR_ALL;
    int m_screen_count = 1;
    bool m_theme_uses_variables = false;
    bool m_reload_requested = false;

    std::shared_ptr<std::atomic<bool>> m_exec_finished;
    std::chrono::steady_clock::time_point m_last_exec;

    void parseConfig();
    std::vector<Section> readSections();
    void applyThemes(const std::vector<Section>& sections);
    void applySettings(const Section& section);
    template <typename T, typename Fn>
    void buildBlock(const Section& section, std::vector<T>& blocks, Fn apply);

    Apply applyBlock(Block& block, const std::string& key, const std::string& value);
    Apply applyBash(BashSource& src, const std::string& key, const std::string& value);
    Apply applyText(TextBlock& block, const std::string& key, const std::string& value);
    Apply applyButton(ButtonBlock& btn, const std::string& key, const std::string& value);
    Apply applyBar(BarBlock& bar, const std::string& key, const std::string& value);
    Apply applyImage(ImageBlock& img, const std::string& key, const std::string& value);

    void error(int line, const std::string& message);

    bool evaluate(const Condition& condition) const;
    bool evaluate(const Comparison& comparison) const;
    std::string resolveOperand(const std::string& operand) const;

    void updateBashSource(const Block& block, BashSource& src, std::chrono::steady_clock::time_point now);
    void onVariableChanged(const std::string& name);

    void initFonts(ImGuiIO& io);
    ImFont* loadFont(ImGuiIO& io, const std::string& name, float size);
    void createDefaultIniFile() const;
    std::string resolveVariables(const std::string& cmd) const;
    static std::string execBash(const std::string& cmd);
};
