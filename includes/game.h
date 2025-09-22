#ifndef GAME_H
#define GAME_H

# define WIDTH 1280
# define HEIGHT 720
# define BLOCK 64
# define DEBUG 0

# define W 119
# define A 97
# define S 115
# define D 100
# define LEFT 65361
# define RIGHT 65363

# define PI 3.14159265359

#include "./mlx/mlx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

typedef struct s_hit {
    float ray_x;
    float ray_y;
    int   side;   // 0 = vertical, 1 = horizontal
    float dist;   // distance corrigée (perp)
} t_hit;

typedef struct s_tex {
    void *img;
    char *data;
    int   w;
    int   h;
    int   bpp;
    int   size_line;
    int   endian;
} t_tex;

typedef struct s_player
{
    float x;
    float y;
    float angle;

    bool key_up;
    bool key_down;
    bool key_left;
    bool key_right;

    bool left_rotate;
    bool right_rotate;
}   t_player;

typedef struct s_game
{
    void *mlx;
    void *win;
    void *img;

    char *data;
    int bpp;
    int size_line;
    int endian;
    t_player player;
    t_tex wall;

    char **map;
} t_game;

void init_player(t_player *player);
bool  touch(float px, float py, t_game *game);
float dist2(float x, float y);
int key_release(int keycode, t_player *player);
int key_press(int keycode, t_player *player);
void move_player(t_player *player);
#endif
