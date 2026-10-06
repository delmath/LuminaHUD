#include "config_manager.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cctype>
#include <array>
#include <algorithm>
#include <memory>
#include <thread>

namespace fs = std::filesystem;

namespace {

void trim(std::string& s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
}

bool startsWith(const std::string& s, const char* prefix) {
    return s.rfind(prefix, 0) == 0;
}

bool isIdentChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::vector<std::string> split(const std::string& s, const std::string& sep) {
    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t pos = s.find(sep); pos != std::string::npos; pos = s.find(sep, start)) {
        parts.push_back(s.substr(start, pos - start));
        start = pos + sep.length();
    }
    parts.push_back(s.substr(start));
    return parts;
}

bool parseFloat(const std::string& value, float& out) {
    if (value.empty()) return false;
    char* end = nullptr;
    float parsed = std::strtof(value.c_str(), &end);
    if (end == value.c_str() || *end != '\0') return false;
    out = parsed;
    return true;
}

bool parseVec2(const std::string& value, ImVec2& out) {
    char c;
    ImVec2 parsed;
    std::stringstream ss(value);
    if (!(ss >> parsed.x >> c >> parsed.y)) return false;
    out = parsed;
    return true;
}

// RRGGBBAA or RRGGBB, with an optional leading '#'
bool parseColor(const std::string& value, ImVec4& out) {
    std::string hex = value;
    if (!hex.empty() && hex.front() == '#') hex.erase(0, 1);
    if (hex.length() == 6) hex += "FF";
    if (hex.length() != 8) return false;
    for (char c : hex)
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;

    unsigned int rgba = static_cast<unsigned int>(std::stoul(hex, nullptr, 16));
    out = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
    return true;
}

bool parseAnchor(const std::string& value, ImVec2& out) {
    static const std::map<std::string, ImVec2> anchors = {
        { "top-left",    ImVec2(0.0f, 0.0f) }, { "top",    ImVec2(0.5f, 0.0f) }, { "top-right",    ImVec2(1.0f, 0.0f) },
        { "left",        ImVec2(0.0f, 0.5f) }, { "center", ImVec2(0.5f, 0.5f) }, { "right",        ImVec2(1.0f, 0.5f) },
        { "bottom-left", ImVec2(0.0f, 1.0f) }, { "bottom", ImVec2(0.5f, 1.0f) }, { "bottom-right", ImVec2(1.0f, 1.0f) }
    };
    auto it = anchors.find(value);
    if (it == anchors.end()) return false;
    out = it->second;
    return true;
}

bool parseCondition(const std::string& text, Condition& out) {
    static const std::array<const char*, 6> operators = { "==", "!=", "<", ">", "<=", ">=" };
    Condition parsed;

    for (const std::string& group : split(text, "||")) {
        std::vector<Comparison> all_of;
        for (const std::string& part : split(group, "&&")) {
            Comparison comparison;
            std::stringstream ss(part);
            if (!(ss >> comparison.lhs)) return false;
            if (ss >> comparison.op) {
                std::getline(ss, comparison.rhs);
                trim(comparison.rhs);
                bool known_op = std::find(operators.begin(), operators.end(), comparison.op) != operators.end();
                if (!known_op || comparison.rhs.empty()) return false;
            }
            all_of.push_back(std::move(comparison));
        }
        parsed.any_of.push_back(std::move(all_of));
    }
    out = std::move(parsed);
    return true;
}

bool usesVariable(const std::string& cmd, const std::string& name) {
    std::string placeholder = "$" + name;
    for (size_t pos = cmd.find(placeholder); pos != std::string::npos; pos = cmd.find(placeholder, pos + 1)) {
        size_t end = pos + placeholder.length();
        if (end >= cmd.length() || !isIdentChar(cmd[end])) return true;
    }
    return false;
}

// Keys holding free text, where a leading '@' is not a theme reference
bool isRawKey(const std::string& key) {
    return key == "TEXT" || key == "BASH" || key == "LABEL" || key == "Exec" || key == "Path";
}

}

ConfigManager::ConfigManager()
    : m_exec_finished(std::make_shared<std::atomic<bool>>(false)) {
    if (const char* home_dir = std::getenv("HOME")) {
        fs::path config_dir = fs::path(home_dir) / ".config" / "LuminaHUD";
        fs::create_directories(config_dir);
        m_ini_path = (config_dir / "config").string();
    } else {
        m_ini_path = "config";
    }

    if (!fs::exists(m_ini_path)) {
        createDefaultIniFile();
    }
}

std::string ConfigManager::resolveVariables(const std::string& cmd) const {
    if (cmd.find('$') == std::string::npos) return cmd;

    std::string final_cmd;
    final_cmd.reserve(cmd.length());

    for (size_t i = 0; i < cmd.length();) {
        if (cmd[i] == '$') {
            size_t end = i + 1;
            while (end < cmd.length() && isIdentChar(cmd[end])) ++end;

            auto it = m_variables.find(cmd.substr(i + 1, end - i - 1));
            if (end > i + 1 && it != m_variables.end()) {
                final_cmd += std::to_string(static_cast<int>(it->second));
                i = end;
                continue;
            }
        }
        final_cmd += cmd[i++];
    }
    return final_cmd;
}

std::string ConfigManager::execBash(const std::string& cmd) {
    std::array<char, 256> buffer;
    std::string result;
    result.reserve(512);

    FILE* pipe_ptr = popen(cmd.c_str(), "r");
    if (!pipe_ptr) return "Error exec";

    std::unique_ptr<FILE, int(*)(FILE*)> pipe(pipe_ptr, pclose);
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result.append(buffer.data());
    }

    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

void ConfigManager::loadAndPrepareConfig(ImGuiIO& io) {
    parseConfig();
    initFonts(io);
}

void ConfigManager::error(int line, const std::string& message) {
    std::string full = "config:" + std::to_string(line) + ": " + message;
    std::cerr << "LuminaHUD: " << full << std::endl;
    m_errors.push_back(std::move(full));
}

void ConfigManager::parseConfig() {
    // Keep the last output of bash blocks so a reload does not blank them
    std::map<std::string, std::string> previous_text;
    for (auto& block : m_text_blocks)
        if (block.is_bash) previous_text[block.name + '\n' + block.bash_command] = std::move(block.text);
    for (auto& bar : m_bar_blocks)
        if (bar.is_bash) previous_text[bar.name + '\n' + bar.bash_command] = std::move(bar.text);

    m_text_blocks.clear();
    m_button_blocks.clear();
    m_bar_blocks.clear();
    m_image_blocks.clear();
    m_theme.clear();
    m_errors.clear();
    m_monitor = MONITOR_ALL;
    m_theme_uses_variables = false;

    std::vector<Section> sections = readSections();
    applyThemes(sections);

    for (const auto& section : sections) {
        if (startsWith(section.name, "Theme_")) continue;
        else if (section.name == "Settings") applySettings(section);
        else if (startsWith(section.name, "TextBlock_")) buildBlock(section, m_text_blocks, &ConfigManager::applyText);
        else if (startsWith(section.name, "ButtonBlock_")) buildBlock(section, m_button_blocks, &ConfigManager::applyButton);
        else if (startsWith(section.name, "BarBlock_")) buildBlock(section, m_bar_blocks, &ConfigManager::applyBar);
        else if (startsWith(section.name, "ImageBlock_")) buildBlock(section, m_image_blocks, &ConfigManager::applyImage);
        else error(section.line, "unknown section [" + section.name + "]");
    }

    for (auto& block : m_text_blocks) {
        auto it = previous_text.find(block.name + '\n' + block.bash_command);
        if (block.is_bash && it != previous_text.end()) block.text = it->second;
    }
    for (auto& bar : m_bar_blocks) {
        auto it = previous_text.find(bar.name + '\n' + bar.bash_command);
        if (bar.is_bash && it != previous_text.end()) bar.text = it->second;
    }
}

std::vector<ConfigManager::Section> ConfigManager::readSections() {
    std::vector<Section> sections;

    std::ifstream file(m_ini_path);
    if (!file) return sections;

    std::string line;
    int line_number = 0;

    while (std::getline(file, line)) {
        ++line_number;
        trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            sections.push_back({ line.substr(1, line.length() - 2), line_number, {} });
            continue;
        }

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos || sections.empty()) {
            error(line_number, eq_pos == std::string::npos ? "expected key=value" : "key outside of a [section]");
            continue;
        }

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);
        trim(key); trim(value);
        sections.back().entries.push_back({ std::move(key), std::move(value), line_number });
    }
    return sections;
}

// Themes apply in file order, a later matching theme overrides the values of an earlier one
void ConfigManager::applyThemes(const std::vector<Section>& sections) {
    for (const auto& section : sections) {
        if (!startsWith(section.name, "Theme_")) continue;

        bool active = true;
        for (const auto& entry : section.entries) {
            if (entry.key != "When") continue;

            Condition condition;
            if (!parseCondition(entry.value, condition)) {
                error(entry.line, "invalid condition '" + entry.value + "'");
                active = false;
                continue;
            }
            for (const auto& all_of : condition.any_of)
                for (const auto& comparison : all_of)
                    if (startsWith(comparison.lhs, "var.") || startsWith(comparison.rhs, "var."))
                        m_theme_uses_variables = true;

            if (!evaluate(condition)) active = false;
        }
        if (!active) continue;

        for (const auto& entry : section.entries)
            if (entry.key != "When") m_theme[entry.key] = entry.value;
    }
}

void ConfigManager::applySettings(const Section& section) {
    for (const auto& entry : section.entries) {
        if (entry.key != "Monitor") {
            error(entry.line, "unknown key '" + entry.key + "'");
            continue;
        }

        float index = 0.0f;
        if (entry.value == "all") m_monitor = MONITOR_ALL;
        else if (entry.value == "primary") m_monitor = MONITOR_PRIMARY;
        else if (parseFloat(entry.value, index) && index >= 0.0f) m_monitor = static_cast<int>(index);
        else error(entry.line, "invalid value '" + entry.value + "' for Monitor");
    }
}

template <typename T, typename Fn>
void ConfigManager::buildBlock(const Section& section, std::vector<T>& blocks, Fn apply) {
    T block;
    block.name = section.name;

    for (const auto& entry : section.entries) {
        std::string value = entry.value;
        if (value.length() > 1 && value.front() == '@' && !isRawKey(entry.key)) {
            auto it = m_theme.find(value.substr(1));
            if (it == m_theme.end()) {
                error(entry.line, "unknown theme value '" + value + "'");
                continue;
            }
            value = it->second;
        }

        Apply result = applyBlock(block, entry.key, value);
        if (result == Apply::Unknown) result = (this->*apply)(block, entry.key, value);

        if (result == Apply::Unknown) error(entry.line, "unknown key '" + entry.key + "'");
        else if (result == Apply::Invalid) error(entry.line, "invalid value '" + value + "' for " + entry.key);
    }
    blocks.push_back(std::move(block));
}

ConfigManager::Apply ConfigManager::applyBlock(Block& block, const std::string& key, const std::string& value) {
    bool ok = true;
    if (key == "Pos") ok = parseVec2(value, block.pos);
    else if (key == "Anchor") ok = parseAnchor(value, block.anchor);
    else if (key == "When") ok = parseCondition(value, block.when);
    else return Apply::Unknown;
    return ok ? Apply::Applied : Apply::Invalid;
}

ConfigManager::Apply ConfigManager::applyBash(BashSource& src, const std::string& key, const std::string& value) {
    bool ok = true;
    if (key == "BASH") { src.bash_command = value; src.is_bash = true; }
    else if (key == "Refresh") ok = parseFloat(value, src.refresh_rate_seconds);
    else return Apply::Unknown;
    return ok ? Apply::Applied : Apply::Invalid;
}

ConfigManager::Apply ConfigManager::applyText(TextBlock& block, const std::string& key, const std::string& value) {
    Apply bash_result = applyBash(block, key, value);
    if (bash_result != Apply::Unknown) return bash_result;

    bool ok = true;
    if (key == "TEXT") block.text = value;
    else if (key == "Color") ok = parseColor(value, block.color);
    else if (key == "BgColor") ok = parseColor(value, block.bg_color);
    else if (key == "HoverColor") { ok = parseColor(value, block.hover_color); block.can_be_hover_tx = ok; }
    else if (key == "HoverBgColor") { ok = parseColor(value, block.hover_bg_color); block.can_be_hover_bg = ok; }
    else if (key == "BorderSize") ok = parseFloat(value, block.border_size);
    else if (key == "BorderColor") ok = parseColor(value, block.border_color);
    else if (key == "Font") block.font_name = value;
    else if (key == "Size") ok = parseFloat(value, block.font_size);
    else return Apply::Unknown;
    return ok ? Apply::Applied : Apply::Invalid;
}

ConfigManager::Apply ConfigManager::applyButton(ButtonBlock& btn, const std::string& key, const std::string& value) {
    bool ok = true;
    if (key == "LABEL") btn.label = value;
    else if (key == "TargetVar") {
        btn.target_var = value;
        m_variables.emplace(value, 0.0f);
    }
    else if (key == "Action") { ok = value == "increment" || value == "decrement" || value == "set"; if (ok) btn.action = value; }
    else if (key == "Exec") btn.exec = value;
    else if (key == "Border") { ok = parseFloat(value, btn.border); btn.has_border = ok; }
    else if (key == "Modifier") ok = parseFloat(value, btn.value_modifier);
    else if (key == "Size") ok = parseVec2(value, btn.size);
    else if (key == "Color") ok = parseColor(value, btn.color);
    else if (key == "BgColor") ok = parseColor(value, btn.bg_color);
    else if (key == "HoverColor") { ok = parseColor(value, btn.hover_color); btn.can_be_hover_tx = ok; }
    else if (key == "HoverBgColor") ok = parseColor(value, btn.hover_bg_color);
    else if (key == "BorderSize") ok = parseFloat(value, btn.border_size);
    else if (key == "BorderColor") ok = parseColor(value, btn.border_color);
    else if (key == "HoverBorderColor") { ok = parseColor(value, btn.hover_border_color); btn.can_be_hover_border = ok; }
    else if (key == "Font") btn.font_name = value;
    else if (key == "FontSize") ok = parseFloat(value, btn.font_size);
    else return Apply::Unknown;
    return ok ? Apply::Applied : Apply::Invalid;
}

ConfigManager::Apply ConfigManager::applyBar(BarBlock& bar, const std::string& key, const std::string& value) {
    Apply bash_result = applyBash(bar, key, value);
    if (bash_result != Apply::Unknown) return bash_result;

    bool ok = true;
    if (key == "Var") {
        bar.source_var = value;
        m_variables.emplace(value, 0.0f);
    }
    else if (key == "Value") ok = parseFloat(value, bar.value);
    else if (key == "Min") ok = parseFloat(value, bar.min);
    else if (key == "Max") ok = parseFloat(value, bar.max);
    else if (key == "Size") ok = parseVec2(value, bar.size);
    else if (key == "Color") ok = parseColor(value, bar.color);
    else if (key == "BgColor") ok = parseColor(value, bar.bg_color);
    else if (key == "BorderSize") ok = parseFloat(value, bar.border_size);
    else if (key == "BorderColor") ok = parseColor(value, bar.border_color);
    else return Apply::Unknown;
    return ok ? Apply::Applied : Apply::Invalid;
}

ConfigManager::Apply ConfigManager::applyImage(ImageBlock& img, const std::string& key, const std::string& value) {
    bool ok = true;
    if (key == "Path") {
        const char* home_dir = std::getenv("HOME");
        img.path = (startsWith(value, "~/") && home_dir) ? std::string(home_dir) + value.substr(1) : value;
    }
    else if (key == "Size") ok = parseVec2(value, img.size);
    else if (key == "Tint") ok = parseColor(value, img.tint);
    else return Apply::Unknown;
    return ok ? Apply::Applied : Apply::Invalid;
}

std::string ConfigManager::resolveOperand(const std::string& operand) const {
    if (operand == "screens") return std::to_string(m_screen_count);
    if (startsWith(operand, "env.")) {
        const char* env_value = std::getenv(operand.c_str() + 4);
        return env_value ? env_value : "";
    }
    if (startsWith(operand, "var.")) {
        auto it = m_variables.find(operand.substr(4));
        return std::to_string(static_cast<int>(it != m_variables.end() ? it->second : 0.0f));
    }
    return operand;
}

bool ConfigManager::evaluate(const Comparison& comparison) const {
    std::string lhs = resolveOperand(comparison.lhs);
    if (comparison.op.empty()) return !lhs.empty() && lhs != "0";

    std::string rhs = resolveOperand(comparison.rhs);
    float lhs_number, rhs_number;
    int order;
    if (parseFloat(lhs, lhs_number) && parseFloat(rhs, rhs_number))
        order = (lhs_number > rhs_number) - (lhs_number < rhs_number);
    else
        order = lhs.compare(rhs);

    if (comparison.op == "==") return order == 0;
    if (comparison.op == "!=") return order != 0;
    if (comparison.op == "<") return order < 0;
    if (comparison.op == ">") return order > 0;
    if (comparison.op == "<=") return order <= 0;
    return order >= 0;
}

bool ConfigManager::evaluate(const Condition& condition) const {
    if (condition.any_of.empty()) return true;

    for (const auto& all_of : condition.any_of) {
        bool matches = true;
        for (const auto& comparison : all_of) {
            if (!evaluate(comparison)) {
                matches = false;
                break;
            }
        }
        if (matches) return true;
    }
    return false;
}

float ConfigManager::barFraction(const BarBlock& bar) const {
    float value = bar.value;
    if (bar.is_bash) {
        value = std::strtof(bar.text.c_str(), nullptr);
    } else if (!bar.source_var.empty()) {
        auto it = m_variables.find(bar.source_var);
        if (it != m_variables.end()) value = it->second;
    }

    if (bar.max <= bar.min) return 0.0f;
    return std::clamp((value - bar.min) / (bar.max - bar.min), 0.0f, 1.0f);
}

void ConfigManager::pressButton(const ButtonBlock& btn) {
    if (!btn.target_var.empty()) {
        float& target = m_variables[btn.target_var];
        float previous = target;

        if (btn.action == "increment") {
            if (!btn.has_border) target += btn.value_modifier;
            else if (target < btn.border) target = std::min(target + btn.value_modifier, btn.border);
        } else if (btn.action == "decrement") {
            if (!btn.has_border) target -= btn.value_modifier;
            else if (target > btn.border) target = std::max(target - btn.value_modifier, btn.border);
        } else if (btn.action == "set") {
            target = btn.value_modifier;
        }

        if (target != previous) onVariableChanged(btn.target_var);
    }

    if (!btn.exec.empty()) {
        m_last_exec = std::chrono::steady_clock::now();
        std::thread([cmd = resolveVariables(btn.exec), finished = m_exec_finished]() {
            int status = std::system(cmd.c_str());
            (void)status;
            finished->store(true);
        }).detach();
    }
}

void ConfigManager::onVariableChanged(const std::string& name) {
    for (auto& block : m_text_blocks)
        if (block.is_bash && usesVariable(block.bash_command, name)) block.needs_refresh = true;
    for (auto& bar : m_bar_blocks)
        if (bar.is_bash && usesVariable(bar.bash_command, name)) bar.needs_refresh = true;

    if (m_theme_uses_variables) m_reload_requested = true;
}

bool ConfigManager::consumeReloadRequest() {
    bool requested = m_reload_requested;
    m_reload_requested = false;
    return requested;
}

void ConfigManager::updateBashSource(const Block& block, BashSource& src, std::chrono::steady_clock::time_point now) {
    if (!src.is_bash) return;

    if (src.is_running) {
        if (src.future_result.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        src.text = src.future_result.get();
        src.is_running = false;
        src.last_refresh = now;
    }

    if (!evaluate(block.when)) return;

    std::chrono::duration<float> elapsed = now - src.last_refresh;
    if (src.needs_refresh || elapsed.count() >= src.refresh_rate_seconds) {
        src.needs_refresh = false;
        src.is_running = true;
        src.future_result = std::async(std::launch::async, [cmd = resolveVariables(src.bash_command)]() {
            return execBash(cmd);
        });
    }
}

void ConfigManager::updateBashBlocks() {
    auto now = std::chrono::steady_clock::now();

    // A button command just ended, what the blocks display may be out of date
    if (m_exec_finished->exchange(false)) {
        for (auto& block : m_text_blocks) block.needs_refresh = block.is_bash;
        for (auto& bar : m_bar_blocks) bar.needs_refresh = bar.is_bash;
    }

    for (auto& block : m_text_blocks) updateBashSource(block, block, now);
    for (auto& bar : m_bar_blocks) updateBashSource(bar, bar, now);
}

float ConfigManager::secondsUntilNextRefresh(float max_wait) const {
    auto now = std::chrono::steady_clock::now();
    float wait = max_wait;

    auto consider = [&](const Block& block, const BashSource& src) {
        if (!src.is_bash) return;

        if (src.is_running) {
            wait = std::min(wait, 0.05f);
            return;
        }
        if (!evaluate(block.when)) return;

        std::chrono::duration<float> elapsed = now - src.last_refresh;
        wait = std::min(wait, src.needs_refresh ? 0.0f : src.refresh_rate_seconds - elapsed.count());
    };

    for (const auto& block : m_text_blocks) consider(block, block);
    for (const auto& bar : m_bar_blocks) consider(bar, bar);

    if (now - m_last_exec < std::chrono::seconds(1)) wait = std::min(wait, 0.05f);

    return std::max(wait, 0.0f);
}

void ConfigManager::initFonts(ImGuiIO& io) {
    // The first font of the atlas is the ImGui default one, keep it small whatever the blocks ask for
    loadFont(io, "Default", 13.0f);

    for (auto& block : m_text_blocks) block.font_ptr = loadFont(io, block.font_name, block.font_size);
    for (auto& btn : m_button_blocks) btn.font_ptr = loadFont(io, btn.font_name, btn.font_size);
}

ImFont* ConfigManager::loadFont(ImGuiIO& io, const std::string& name, float size) {
    std::string font_key = name + "_" + std::to_string((int)size);
    auto it = m_loaded_fonts.find(font_key);
    if (it != m_loaded_fonts.end()) return it->second;

    ImFontConfig config;
    config.SizePixels = size;

    ImFont* new_font = nullptr;
    if (name != "Default") {
        std::vector<fs::path> search_paths = {
            "/usr/share/fonts", "/usr/local/share/fonts",
            "/host/usr/share/fonts", "/host/usr/local/share/fonts"
        };
        if (const char* home_dir = std::getenv("HOME")) {
            search_paths.push_back(fs::path(home_dir) / ".local" / "share" / "fonts");
            search_paths.push_back(fs::path(home_dir) / ".fonts");
        }

        std::string target_filename = name + ".ttf";
        fs::path found_path = "";
        for (const auto& base_path : search_paths) {
            if (!fs::exists(base_path)) continue;
            for (const auto& entry : fs::recursive_directory_iterator(base_path, fs::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file() && entry.path().filename() == target_filename) {
                    found_path = entry.path();
                    break;
                }
            }
            if (!found_path.empty()) break;
        }

        if (!found_path.empty()) new_font = io.Fonts->AddFontFromFileTTF(found_path.string().c_str(), size);
    }
    if (!new_font) new_font = io.Fonts->AddFontDefault(&config);

    if (new_font) m_loaded_fonts[font_key] = new_font;
    return new_font;
}

void ConfigManager::createDefaultIniFile() const {
    std::ofstream ofs(m_ini_path);
    if (!ofs) return;
    ofs << "[TextBlock_0]\nTEXT=Hello World!\nPos=10.0,10.0\nColor=FFFFFFFF\nFont=Default\nSize=24.0\n";
}
