#include <chrono>
#include <string>
#include <thread>
#include <filesystem>
#include <X11/Xlib.h>

#include "imgui.h"
#include "window/window_manager.hpp"
#include "config/config_manager.hpp"
#include "imgui/imgui_manager.hpp"
#include "drawSystem/draw_system.hpp"

namespace fs = std::filesystem;

int main() {
    ConfigManager configManager;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    configManager.loadAndPrepareConfig(io);

    WindowManager windowManager("GhostDash Desktop Widget");
    if (!windowManager.init()) {
        return -1;
    }

    GLFWwindow* window = windowManager.getWindow();

    ImGuiManager imGuiManager(window, "/tmp/imgui_trash.ini");
    io = imGuiManager.getIO();

    std::string ini_path = configManager.getIniPath();
    fs::file_time_type last_write_time;

    if (fs::exists(ini_path)) {
        last_write_time = fs::last_write_time(ini_path);
    }

    auto last_file_check = std::chrono::steady_clock::now();

    while (!windowManager.shouldClose()) {
        windowManager.pollEvents();

        auto now = std::chrono::steady_clock::now();
        if (now - last_file_check >= std::chrono::seconds(1)) {
            last_file_check = now;
            if (fs::exists(ini_path)) {
                auto current_write_time = fs::last_write_time(ini_path);
                if (current_write_time != last_write_time) {
                    last_write_time = current_write_time;
                    configManager.loadAndPrepareConfig(io);
                    imGuiManager.rebuildFontTexture();
                }
            }
        }

        imGuiManager.newFrame();
        configManager.updateBashBlocks();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("ProcessWindowOverlay", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        for (const auto& block : configManager.getIDTextBlocks())
            drawTextBlock(block);

        for (auto& btn : configManager.getButtonBlocks())
            drawButtonBlock(btn, configManager.getVariables());

        ImGui::End();

        imGuiManager.render();
        windowManager.swapBuffers();

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    return 0;
}
