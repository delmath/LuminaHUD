#include "window_manager.hpp"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <GLFW/glfw3native.h>
#include <iostream>

WindowManager::WindowManager(const char* title)
    : m_title(title), m_window(nullptr) {
}

WindowManager::~WindowManager() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool WindowManager::init() {
    if (!getWindowSize())
        return false;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

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

bool WindowManager::shouldClose() {
    return glfwWindowShouldClose(m_window);
}

bool WindowManager::getWindowSize() {

	int window_width = -1;
    int window_height = -1;
    int window_x_pos = 0;
    int window_y_pos = 0;

    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
    if (primary_monitor) {
        const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
        if (mode) {
            window_width = mode->width;
            window_height = mode->height;
            window_x_pos = 0;
            window_y_pos = 0;
        }
    } else {
        Display* dpy = XOpenDisplay(NULL);
        if (dpy) {
            Screen* screen = DefaultScreenOfDisplay(dpy);
            if (screen) {
                window_width = screen->width;
                window_height = screen->height;
                window_x_pos = 0;
                window_y_pos = 0;
            }
            XCloseDisplay(dpy);
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
