#include "config_manager.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <array>

namespace fs = std::filesystem;

ConfigManager::ConfigManager() {
    if (const char* home_dir = std::getenv("HOME")) {
        fs::path config_dir = fs::path(home_dir) / ".config" / "process_window";
        fs::create_directories(config_dir);
        m_ini_path = (config_dir / "process_window_layout.ini").string();
    } else {
        m_ini_path = "process_window_layout.ini";
    }

    if (!fs::exists(m_ini_path)) {
        createDefaultIniFile();
    }
}

std::string ConfigManager::execBash(const std::string& cmd) {
    std::string final_cmd = cmd;
    for (const auto& [var_name, var_val] : m_variables) {
        std::string placeholder = "$" + var_name;
        size_t pos = final_cmd.find(placeholder);
        while (pos != std::string::npos) {
            std::string val_str = std::to_string((int)var_val);
            final_cmd.replace(pos, placeholder.length(), val_str);
            pos = final_cmd.find(placeholder, pos + val_str.length());
        }
    }

    std::array<char, 128> buffer;
    std::string result;

    FILE* pipe_ptr = popen(final_cmd.c_str(), "r");
    if (!pipe_ptr) return "Error exec";

    std::unique_ptr<FILE, int(*)(FILE*)> pipe(pipe_ptr, pclose);
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

void ConfigManager::loadAndPrepareConfig(ImGuiIO& io) {
    parseConfig();
    initFonts(io, m_text_blocks);
}

void ConfigManager::parseConfig() {
    m_text_blocks.clear();
    m_button_blocks.clear();

    std::ifstream file(m_ini_path);
    if (!file) return;

    std::string line;
    TextBlock current_text;
    ButtonBlock current_button;
    bool is_text = false, is_btn = false;

    auto trim = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
    };

    while (std::getline(file, line)) {
        trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            if (is_text) m_text_blocks.push_back(std::move(current_text));
            if (is_btn) m_button_blocks.push_back(current_button);
            is_text = false; is_btn = false;

            std::string section = line.substr(1, line.length() - 2);
            if (section.rfind("TextBlock_", 0) == 0) {
                current_text = TextBlock();
                is_text = true;
            } else if (section.rfind("ButtonBlock_", 0) == 0) {
                current_button = { "Button", ImVec2(0,0), ImVec2(0,0), "", "increment", 1.0f, false, 0.0f, ImVec4(1,1,1,1), ImVec4(0.2f,0.2f,0.2f,1), ImVec4(1,1,1,1), ImVec4(0.4f,0.4f,0.4f,1), 0.0f, ImVec4(1,1,1,1) };
                is_btn = true;
            }
        }
        else if (is_text || is_btn) {
            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) continue;
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            trim(key); trim(value);

            if (is_text) {
                if (key == "TEXT") current_text.text = value;
                else if (key == "BASH") { current_text.bash_command = value; current_text.is_bash = true; }
                else if (key == "Refresh") current_text.refresh_rate_seconds = std::stof(value);
                else if (key == "Pos") { char c; std::stringstream ss(value); ss >> current_text.pos.x >> c >> current_text.pos.y; }
                else if (key == "Color") {
                    unsigned int rgba = 0xFFFFFFFF; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_text.color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "BgColor") {
                    unsigned int rgba = 0x00000000; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_text.bg_color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "HoverColor") {
                    unsigned int rgba = 0xFFFFFFFF; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_text.hover_color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "HoverBgColor") {
                    unsigned int rgba = 0x00000000; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_text.hover_bg_color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "BorderSize") current_text.border_size = std::stof(value);
                else if (key == "BorderColor") {
                    unsigned int rgba = 0xFFFFFFFF; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_text.border_color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "Font") current_text.font_name = value;
                else if (key == "Size") current_text.font_size = std::stof(value);
            }
            else if (is_btn) {
                if (key == "LABEL") current_button.label = value;
                else if (key == "TargetVar") {
                    current_button.target_var = value;
                    if (m_variables.find(value) == m_variables.end()) m_variables[value] = 0.0f;
                }
                else if (key == "Action") current_button.action = value;
                else if (key == "Border") { current_button.border = std::stof(value); current_button.has_border = true;}
                else if (key == "Modifier") current_button.value_modifier = std::stof(value);
                else if (key == "Pos") { char c; std::stringstream ss(value); ss >> current_button.pos.x >> c >> current_button.pos.y; }
                else if (key == "Size") { char c; std::stringstream ss(value); ss >> current_button.size.x >> c >> current_button.size.y; }
                else if (key == "Color") {
                    unsigned int rgba = 0xFFFFFFFF; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_button.color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "BgColor") {
                    unsigned int rgba = 0x333333FF; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_button.bg_color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "HoverColor") {
                    unsigned int rgba = 0xFFFFFFFF; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_button.hover_color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "HoverBgColor") {
                    unsigned int rgba = 0x555555FF; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_button.hover_bg_color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
                else if (key == "BorderSize") current_button.border_size = std::stof(value);
                else if (key == "BorderColor") {
                    unsigned int rgba = 0xFFFFFFFF; std::stringstream ss; ss << std::hex << value; ss >> rgba;
                    current_button.border_color = ImVec4(((rgba >> 24) & 0xFF)/255.0f, ((rgba >> 16) & 0xFF)/255.0f, ((rgba >> 8) & 0xFF)/255.0f, (rgba & 0xFF)/255.0f);
                }
            }
        }
    }
    if (is_text) m_text_blocks.push_back(std::move(current_text));
    if (is_btn) m_button_blocks.push_back(current_button);
}

void ConfigManager::updateBashBlocks() {
    auto now = std::chrono::steady_clock::now();

    for (auto& block : m_text_blocks) {
        bool holds_variable = false;
        if (block.is_bash) {
            for (const auto& [var_name, var_val] : m_variables) {
                if (block.bash_command.find("$" + var_name) != std::string::npos) {
                    holds_variable = true;
                    break;
                }
            }
        }

        if (holds_variable) {
            std::chrono::duration<float> elapsed = now - block.last_refresh;
            if (elapsed.count() >= block.refresh_rate_seconds || block.text.empty()) {
                if (block.is_running) {
                    if (block.future_result.valid()) block.future_result.wait();
                    block.is_running = false;
                }
                block.text = this->execBash(block.bash_command);
                block.last_refresh = now;
            }
            continue;
        }

        if (block.is_running) {
            if (block.future_result.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                block.text = block.future_result.get();
                block.is_running = false;
                block.last_refresh = now;
            }
            continue;
        }

        std::chrono::duration<float> elapsed = now - block.last_refresh;
        if (elapsed.count() >= block.refresh_rate_seconds) {
            block.is_running = true;
            block.future_result = std::async(std::launch::async, [this, cmd = block.bash_command]() {
                return this->execBash(cmd);
            });
        }
    }
}

void ConfigManager::initFonts(ImGuiIO& io, std::vector<TextBlock>& blocks) {
    ImFontConfig config;
    std::vector<fs::path> search_paths = {
        "/usr/share/fonts", "/usr/local/share/fonts",
        "/host/usr/share/fonts", "/host/usr/local/share/fonts"
    };
    if (const char* home_dir = std::getenv("HOME")) {
        search_paths.push_back(fs::path(home_dir) / ".local" / "share" / "fonts");
        search_paths.push_back(fs::path(home_dir) / ".fonts");
    }

    for (auto& block : blocks) {
        std::string font_key = block.font_name + "_" + std::to_string((int)block.font_size);
        auto it = m_loaded_fonts.find(font_key);
        if (it != m_loaded_fonts.end()) {
            block.font_ptr = it->second;
            continue;
        }

        ImFont* new_font = nullptr;
        if (block.font_name == "Default") {
            config.SizePixels = block.font_size;
            new_font = io.Fonts->AddFontDefault(&config);
        } else {
            std::string target_filename = block.font_name + ".ttf";
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

            if (!found_path.empty()) {
                new_font = io.Fonts->AddFontFromFileTTF(found_path.string().c_str(), block.font_size);
            } else {
                config.SizePixels = block.font_size;
                new_font = io.Fonts->AddFontDefault(&config);
            }
        }
        if (new_font) {
            m_loaded_fonts[font_key] = new_font;
            block.font_ptr = new_font;
        }
    }
}

void ConfigManager::createDefaultIniFile() const {
    std::ofstream ofs(m_ini_path);
    if (!ofs) return;
    ofs << "[TextBlock_0]\nTEXT=Hello World!\nPos=10.0,10.0\nColor=FFFFFFFF\nFont=Default\nSize=24.0\n";
}
