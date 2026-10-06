#include "window_manager.hpp"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <GLFW/glfw3native.h>
#include <iostream>

static bool s_monitors_changed = false;

static void onMonitorEvent(GLFWmonitor*, int) {
    s_monitors_changed = true;
}

WindowManager::WindowManager(const char* title)
    : m_window(nullptr), m_title(title) {
    m_glfw_ready = glfwInit();
    if (m_glfw_ready)
        glfwSetMonitorCallback(onMonitorEvent);
}

WindowManager::~WindowManager() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool WindowManager::init(int monitor) {
    if (!m_glfw_ready) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    if (!getWindowSize(monitor))
        return false;

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    m_window = glfwCreateWindow(m_width, m_height, m_title, NULL, NULL);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetWindowPos(m_window, m_x_pos, m_y_pos);

    Display* dpy = glfwGetX11Display();
    Window win = glfwGetX11Window(m_window);

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    XChangeWindowAttributes(dpy, win, CWOverrideRedirect, &attrs);


    Atom wm_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom type_desktop = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    XChangeProperty(dpy, win, wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&type_desktop, 1);

    XMapWindow(dpy, win);
    XLowerWindow(dpy, win);
    XFlush(dpy);

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    return true;
}

void WindowManager::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void WindowManager::pollEvents() {
    glfwPollEvents();
}

void WindowManager::waitEvents(double timeout_seconds) {
    glfwWaitEventsTimeout(timeout_seconds);
}

bool WindowManager::shouldClose() {
    return glfwWindowShouldClose(m_window);
}

int WindowManager::getMonitorCount() const {
    int count = 0;
    if (m_glfw_ready)
        glfwGetMonitors(&count);
    return count;
}

bool WindowManager::consumeMonitorChange() {
    bool changed = s_monitors_changed;
    s_monitors_changed = false;
    return changed;
}

void WindowManager::applyMonitor(int monitor) {
    int previous_width = m_width, previous_height = m_height;
    int previous_x = m_x_pos, previous_y = m_y_pos;

    getWindowSize(monitor);

    if (m_x_pos != previous_x || m_y_pos != previous_y)
        glfwSetWindowPos(m_window, m_x_pos, m_y_pos);
    if (m_width != previous_width || m_height != previous_height)
        glfwSetWindowSize(m_window, m_width, m_height);
}

// monitor is a GLFW monitor index, -2 for the primary one, -1 for the whole X screen
bool WindowManager::getWindowSize(int monitor) {

    int window_width = -1;
    int window_height = -1;
    int window_x_pos = 0;
    int window_y_pos = 0;

    GLFWmonitor* target_monitor = nullptr;
    if (monitor != -1) {
        int count = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&count);
        target_monitor = (monitor >= 0 && monitor < count) ? monitors[monitor] : glfwGetPrimaryMonitor();
    }

    const GLFWvidmode* mode = target_monitor ? glfwGetVideoMode(target_monitor) : nullptr;
    if (mode) {
        window_width = mode->width;
        window_height = mode->height;
        glfwGetMonitorPos(target_monitor, &window_x_pos, &window_y_pos);
    } else {
        Display* dpy = glfwGetX11Display();
        Screen* screen = dpy ? DefaultScreenOfDisplay(dpy) : nullptr;
        if (screen) {
            window_width = screen->width;
            window_height = screen->height;
        }
    }

    if (window_width == -1 || window_height == -1)
        std::cerr << "Erreur : Impossible de détecter la taille de l'écran via GLFW ou X11 !\n";

    m_width = window_width;
    m_height = window_height;
    m_x_pos = window_x_pos;
    m_y_pos = window_y_pos;

    return true;
}
