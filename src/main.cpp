#include <chrono>
#include <string>
#include <filesystem>
#include <X11/Xlib.h>

#include "imgui.h"
#include "window_manager.hpp"
#include "config_manager.hpp"
#include "imgui_manager.hpp"
#include "draw_system.hpp"

namespace fs = std::filesystem;

int main() {
    ConfigManager configManager;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    WindowManager windowManager("GhostDash Desktop Widget");
    configManager.setScreenCount(windowManager.getMonitorCount());
    configManager.loadAndPrepareConfig(io);

    if (!windowManager.init(configManager.getMonitor())) {
        return -1;
    }

    GLFWwindow* window = windowManager.getWindow();

    ImGuiManager imGuiManager(window);

    std::string ini_path = configManager.getIniPath();
    fs::file_time_type last_write_time;

    if (fs::exists(ini_path)) {
        last_write_time = fs::last_write_time(ini_path);
    }

    auto last_file_check = std::chrono::steady_clock::now();
    int frames_to_draw = 0;

    while (!windowManager.shouldClose()) {
        // Sleep until an input event or the next refresh, then draw a few frames so ImGui can settle
        if (frames_to_draw == 0) {
            windowManager.waitEvents(configManager.secondsUntilNextRefresh(1.0f));
            frames_to_draw = 3;
        } else {
            windowManager.pollEvents();
        }
        --frames_to_draw;

        bool reload = configManager.consumeReloadRequest();

        if (windowManager.consumeMonitorChange()) {
            configManager.setScreenCount(windowManager.getMonitorCount());
            reload = true;
        }

        auto now = std::chrono::steady_clock::now();
        if (now - last_file_check >= std::chrono::seconds(1)) {
            last_file_check = now;
            if (fs::exists(ini_path)) {
                auto current_write_time = fs::last_write_time(ini_path);
                if (current_write_time != last_write_time) {
                    last_write_time = current_write_time;
                    clearImageCache();
                    reload = true;
                }
            }
        }

        if (reload) {
            configManager.loadAndPrepareConfig(io);
            imGuiManager.rebuildFontTexture();
            windowManager.applyMonitor(configManager.getMonitor());
        }

        imGuiManager.newFrame();
        configManager.updateBashBlocks();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("ProcessWindowOverlay", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        for (const auto& img : configManager.getImageBlocks())
            if (configManager.isVisible(img)) drawImageBlock(img);

        for (const auto& bar : configManager.getBarBlocks())
            if (configManager.isVisible(bar)) drawBarBlock(bar, configManager.barFraction(bar));

        for (const auto& block : configManager.getIDTextBlocks())
            if (configManager.isVisible(block)) drawTextBlock(block);

        for (const auto& btn : configManager.getButtonBlocks())
            if (configManager.isVisible(btn) && drawButtonBlock(btn)) configManager.pressButton(btn);

        drawErrors(configManager.getErrors());

        ImGui::End();

        imGuiManager.render();
        windowManager.swapBuffers();
    }

    return 0;
}
