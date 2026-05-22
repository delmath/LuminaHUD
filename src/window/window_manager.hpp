#pragma once

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>

class WindowManager {
public:
    WindowManager(const char* title);
    ~WindowManager();

    bool init();
    GLFWwindow* getWindow() const { return m_window; }
    void swapBuffers();
    void pollEvents();
    bool shouldClose();
    bool getWindowSize();
    void setMousePassthrough(bool passthrough);

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    const char* m_title;
    int m_x_pos;
    int m_y_pos;

};
