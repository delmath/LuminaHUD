NAME     = LuminaHUD

# Style
GREY     = \033[0;30m
RED      = \033[0;31m
GREEN    = \033[0;32m
YELLOW   = \033[0;33m
BLUE     = \033[0;34m
PURPLE   = \033[0;35m
CYAN     = \033[0;36m
WHITE    = \033[0;37m
BOLD     = \033[1m
NC       = \033[0m

# Installation path
PREFIX   = /usr/local

# Compiler and flags
CC       = c++
DEPFLAGS = -MMD -MP
CFLAGS   = $(DEPFLAGS) -Wall -Wextra -std=c++17

ifeq ($(MAKECMDGOALS), fast)
	CFLAGS += -O3
endif

# Flags pour l'éditeur de liens (Linker) - Requis pour GLFW, OpenGL et X11
LDFLAGS  = -lglfw -lGL -lX11 -lpthread -ldl

# Includes : Points de départ pour les recherches de headers
INCLUDES = -I src \
           -I src/imgui \
           -I src/window \
           -I src/drawSystem \
           -I src/config \
           -I lib/imgui \
           -I lib/imgui/backends

# Fichiers sources de TON projet
SRC_FILES = src/main.cpp \
            src/imgui/imgui_manager.cpp \
            src/window/window_manager.cpp \
            src/drawSystem/draw_system.cpp \
            src/config/config_manager.cpp

# Fichiers sources de la bibliothèque ImGui
IMGUI_FILES = lib/imgui/imgui.cpp \
              lib/imgui/imgui_draw.cpp \
              lib/imgui/imgui_widgets.cpp \
              lib/imgui/imgui_tables.cpp \
              lib/imgui/backends/imgui_impl_glfw.cpp \
              lib/imgui/backends/imgui_impl_opengl3.cpp

# Union de tous les fichiers sources à compiler
ALL_SRCS  = $(SRC_FILES) $(IMGUI_FILES)

# Object files directory
OBJ_DIR   = .obj/
OBJ       = $(patsubst %.cpp, $(OBJ_DIR)%.o, $(ALL_SRCS))
DEPS      = $(OBJ:%.o=%.d)

COMPILED_FILES := 0

all : $(NAME) nothing_to_be_done

# Pattern rule universelle pour compiler les fichiers .cpp
$(OBJ_DIR)%.o : %.cpp
	@if [ $(COMPILED_FILES) -eq 0 ]; then \
		echo "\n$(YELLOW)╔══════════════════════════════════════════════╗$(NC)"; \
		echo "$(YELLOW)║        Starting $(NAME) compilation...       ║$(NC)"; \
		echo "$(YELLOW)╚══════════════════════════════════════════════╝$(NC)"; \
	fi
	@$(eval COMPILED_FILES := 1)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@printf "$(GREEN)[Compiling] $(NC)%s\n" "$<"

# Règle d'édition de liens finale
$(NAME) : $(OBJ)
	@echo "\n$(GREEN)[Compiling program] $(NC)$(NAME)"
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LDFLAGS)
	@make --no-print-directory end_message

nothing_to_be_done:
	@if [ $(COMPILED_FILES) -eq 0 ]; then \
		echo "\n$(YELLOW)╔══════════════════════════════════════════════╗$(NC)"; \
		echo "$(YELLOW)║        Nothing to be done for $(NAME).        ║$(NC)"; \
		echo "$(YELLOW)╚══════════════════════════════════════════════╝$(NC)\n"; \
	fi

end_message:
	@echo "$(GREEN)╔══════════════════════════════════════════════╗$(NC)"
	@echo "$(GREEN)║    $(NAME) compiled successfully!          ║$(NC)"
	@echo "$(GREEN)╚══════════════════════════════════════════════╝$(NC)"

# Règle d'installation du binaire
install : $(NAME)
	@echo "$(URPLE)[Installing]$(NC) Moving $(NAME) to $(PREFIX)/bin/"
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@cp -f $(NAME) $(DESTDIR)$(PREFIX)/bin/
	@chmod 755 $(DESTDIR)$(PREFIX)/bin/$(NAME)
	@echo "$(GREEN)[Success]$(NC) $(NAME) is now installed!"

# Règle de désinstallation
uninstall :
	@echo "$(RED)[Uninstalling]$(NC) Removing $(NAME) from $(PREFIX)/bin/"
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(NAME)

clean :
	@echo "$(RED)[Removing] $(NC)object files"
	@rm -rf $(OBJ_DIR)

fclean : clean
	@if [ -f $(NAME) ]; then \
		echo "$(RED)[Removing] $(NC)program $(NAME)"; \
		rm -f $(NAME); \
	fi

re : fclean all

.PHONY: all clean fclean re nothing_to_be_done end_message install uninstall
-include $(DEPS)
