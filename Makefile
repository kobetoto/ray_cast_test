# **************************************************************************** #
#                                   Makefile                                   #
# **************************************************************************** #

# Nom du binaire
NAME        := poke3d

# Dossiers
INCDIR      := includes
SRCDIR      := src
OBJDIR      := obj

# Sources
SRC         := $(SRCDIR)/main.c

# Objets
OBJ         := $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Compilateur & flags
CC          := cc
CFLAGS      := -Wall -Wextra -Werror -I$(INCDIR)

# MiniLibX (version Linux) 
MLX_DIR     := includes/mlx
MLX_INC     := -I$(MLX_DIR)
MLX_LIB     := -L$(MLX_DIR) -lmlx_Linux -lXext -lX11 -lm -lz


# Couleurs (facultatif)
GREEN       := \033[0;32m
GRAY        := \033[0;90m
RESET       := \033[0m

# Règle par défaut
all: $(NAME)

# Link final
$(NAME): $(OBJ)
	@printf "$(GRAY)Linking $(NAME)...$(RESET)\n"
	@$(CC) $(CFLAGS) $(OBJ) $(MLX_LIB) -o $(NAME)
	@printf "$(GREEN)Built $(NAME) ✓$(RESET)\n"

# Compilation .c -> .o dans OBJDIR
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(@D)
	@printf "$(GRAY)Compiling $<...$(RESET)\n"
	@$(CC) $(CFLAGS) $(MLX_INC) -c $< -o $@

# Dossier des objets
$(OBJDIR):
	@mkdir -p $(OBJDIR)

clean:
	@rm -rf $(OBJDIR)
	@printf "$(GREEN)Cleaned objects$(RESET)\n"

fclean: clean
	@rm -f $(NAME)
	@printf "$(GREEN)Removed $(NAME)$(RESET)\n"

re: fclean all

.PHONY: all clean fclean re
