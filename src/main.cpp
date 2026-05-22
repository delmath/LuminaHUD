#include <chrono>
#include <string>
#include <thread>
#include <X11/Xlib.h>

#include "imgui.h"
#include "window/window_manager.hpp"
#include "config/config_manager.hpp"
#include "imgui/imgui_manager.hpp"
#include "drawSystem/draw_system.hpp"

int main() {
    ConfigManager configManager;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    configManager.loadAndPrepareConfig(io);

    WindowManager windowManager("Process Window Dashboard");
    if (!windowManager.init()) {
        return -1;
    }

    GLFWwindow* window = windowManager.getWindow();

    ImGuiManager imGuiManager(window, "/tmp/imgui_trash.ini");
    io = imGuiManager.getIO();

    while (!windowManager.shouldClose()) {
        windowManager.pollEvents();
        imGuiManager.newFrame();
        configManager.updateBashBlocks();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("ProcessWindowOverlay", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        for (const auto& block : configManager.getIDTextBlocks()) {
            drawTextBlock(block);
        }

        for (auto& btn : configManager.getButtonBlocks()) {
            drawButtonBlock(btn, configManager.getVariables());
        }

        ImGui::End();
        imGuiManager.render();
        windowManager.swapBuffers();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
