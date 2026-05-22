#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <string>

class ImGuiManager {
public:
    ImGuiManager(GLFWwindow* window, const std::string& ini_filepath);
    ~ImGuiManager();

    void newFrame();
    void render();
    ImGuiIO& getIO() { return ImGui::GetIO(); }

private:
    GLFWwindow* m_window;
};
