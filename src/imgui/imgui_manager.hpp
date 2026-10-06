#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>

class ImGuiManager {
public:
    ImGuiManager(GLFWwindow* window);
    ~ImGuiManager();

    void newFrame();
    void render();
    ImGuiIO& getIO() { return ImGui::GetIO(); }
    void rebuildFontTexture();

private:
    GLFWwindow* m_window;
};
