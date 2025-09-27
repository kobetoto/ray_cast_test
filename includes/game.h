#ifndef GAME_H
#define GAME_H

/* =============================
**  POKE3D — Header principal
**  Objectif: moteur "à la cub3D" utilisant la DDA (Digital Differential Analyzer)
**  pour marcher de case en case sur une grille 2D et rendre une vue 3D style Wolfenstein.
**
**  Ce fichier définit:
**    - Les constantes (taille de fenêtre, FOV...)
**    - Les structures (vecteurs, images, textures, player, game)
**    - Les prototypes des fonctions
**
**  L'idée à retenir:
**    1) La carte est une grille de '0' (vide) et '1' (mur).
**    2) Pour chaque colonne de pixels de l'écran, on lance un rayon depuis la caméra.
**    3) On avance étape par étape de case en case avec la DDA jusqu'à toucher un mur.
**    4) On calcule la distance "perpendiculaire" au mur et on dessine une bande verticale.
** ============================= */

# define WIN_W 1280
# define WIN_H 720

/* FOV (Field Of View). 60° donne un rendu confortable. */
# define FOV (60.0f)

/* Vitesse de déplacement / rotation (à adapter à votre goût). */
# define MOVE_SPEED 0.08f
# define ROT_SPEED  0.045f

/* Codes touches (Linux/X11 avec minilibX Linux). */
# define KEY_ESC   65307
# define KEY_W     119
# define KEY_A      97
# define KEY_S     115
# define KEY_D     100
# define KEY_LEFT  65361
# define KEY_RIGHT 65363

/* ---------- INCLUDES SYSTÈME / MLX ---------- */
# include <stdlib.h>
# include <stdbool.h>
# include <unistd.h>
# include <math.h>
# include <X11/keysym.h>
# include <X11/X.h>
# include "./mlx/mlx.h"
#include <string.h>

/* =============================
**  Structures utilitaires
** ============================= */

typedef struct s_v2f
{
    float x;
    float y;
}   t_v2f;

/* Une image MLX qu'on peut lire/écrire pixel par pixel */
typedef struct s_img
{
    void    *img;       /* handle MLX */
    char    *addr;      /* pointeur sur les pixels */
    int     bpp;        /* bits par pixel */
    int     line_len;   /* nombre d'octets par ligne */
    int     endian;
    int     w;
    int     h;
}   t_img;

/* Texture = image 2D utilisée pour recouvrir les murs/sols/ciels */
typedef t_img t_tex;

/* État des touches pour un mouvement fluide (on évite la logique "à l'événement"). */
typedef struct s_keys
{
    bool    w;
    bool    a;
    bool    s;
    bool    d;
    bool    left;
    bool    right;
}   t_keys;

/* Joueur/caméra. "dir" est le vecteur direction; "plane" est le plan caméra perpendiculaire.
** L'écran représente l'intervalle [-1, 1] sur ce plan (projection perspective). */
typedef struct s_player
{
    t_v2f   pos;        /* position en cases (x,y), ex: (3.5, 2.2) */
    t_v2f   dir;        /* direction du regard (unitaire) */
    t_v2f   plane;      /* vecteur perpendiculaire à dir qui définit l'ouverture du FOV */
}   t_player;

/* Contexte global du jeu. */
typedef struct s_game
{
    void        *mlx;
    void        *win;

    /* Framebuffer: on dessine ici chaque image puis on "push" vers la fenêtre. */
    t_img       frame;

    /* Textures (au minimum un mur). Vous pouvez en ajouter d'autres. */
    t_tex       tex_wall;
    t_tex       tex_sky;     /* optionnel: simple bandeau pour le ciel */
    t_tex       tex_floor;   /* optionnel: couleur/texture pour le sol */

    /* Carte: tableau de chaînes "0"/"1".
    ** map_h = nombre de lignes; map_w = nombre de colonnes (rectangulaire). */
    char        **map;
    int         map_w;
    int         map_h;

    int         tick; /* NEW: compteur simple pour des effets (parallaxe ciel) */
    
    /* Joueur + état des touches */
    t_player    p;
    t_keys      keys;
}   t_game;


/* === SPRITES COLLECTIBLES (Pokéballs) + HUD + ZBUFFER === */
typedef struct s_sprite {
    float x;         /* position monde (centre de case) */
    float y;
    bool  active;    /* true tant que pas ramassé */
}   t_sprite;

t_tex      tex_pokeball;        /* sprite pokeball (transparence par clé couleur) */
t_tex      tex_pokeball_small;  /* petite pokeball pour le HUD (ou fallback) */

t_sprite  *sprites;      /* tableau dynamique de sprites détectés dans la map */
int        sprite_count; /* combien au total */

int        collected;    /* combien ramassées (pour le HUD) */

float     *zbuf;         /* z-buffer par colonne: distance perpendicular du mur pour occlusion sprites */


/* =============================
**  Prototypes (utils)
** ============================= */
int     close_window(t_game *g);
void    panic(const char *msg);

int     create_frame(t_game *g, int w, int h);
void    destroy_frame(t_game *g);
void    put_pixel(t_img *img, int x, int y, int color);
int     rgb(int r, int g, int b);

int     load_xpm(t_game *g, t_tex *dst, const char *path);
void    destroy_tex(t_game *g, t_tex *t);

/* =============================
**  Prototypes (entrée / hooks)
** ============================= */
int     key_press(int keycode, t_game *g);
int     key_release(int keycode, t_game *g);
int     loop_hook(t_game *g);

/* =============================
**  Prototypes (rendu / DDA)
** ============================= */
void    render_frame(t_game *g);
void    cast_column(t_game *g, int x);

/* =============================
**  Prototypes (setup)
** ============================= */
void    setup_player(t_game *g, float px, float py, float dir_deg);
void    setup_map_small(t_game *g);

#endif
