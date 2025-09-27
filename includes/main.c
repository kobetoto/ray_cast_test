#include "game.h"

/* ==========================================================================
**  Helpers — erreurs & pixels
** ========================================================================== */

void    panic(const char *msg)
{
    /* Dans un projet 42 "propre", remplacez par un ft_putstr_fd + exit. */
    write(2, msg, __builtin_strlen(msg));
    write(2, "\n", 1);
    exit(1);
}

int rgb(int r, int g, int b)
{
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return (r << 16) | (g << 8) | b;
}

void    put_pixel(t_img *img, int x, int y, int color)
{
    if (x < 0 || x >= img->w || y < 0 || y >= img->h)
        return;
    char *p = img->addr + (y * img->line_len + x * (img->bpp / 8));
    *(int *)p = color;
}

/* ==========================================================================
**  Frame / Textures
** ========================================================================== */

int create_frame(t_game *g, int w, int h)
{
    g->frame.w = w;
    g->frame.h = h;
    g->frame.img = mlx_new_image(g->mlx, w, h);
    if (!g->frame.img)
        return (0);
    g->frame.addr = mlx_get_data_addr(g->frame.img, &g->frame.bpp,
                                      &g->frame.line_len, &g->frame.endian);
    if (!g->frame.addr)
        return (0);
    return (1);
}

void destroy_frame(t_game *g)
{
    if (g->frame.img)
        mlx_destroy_image(g->mlx, g->frame.img);
    g->frame.img = NULL;
}

int load_xpm(t_game *g, t_tex *dst, const char *path)
{
    dst->img = mlx_xpm_file_to_image(g->mlx, (char *)path, &dst->w, &dst->h);
    if (!dst->img)
        return (0);
    dst->addr = mlx_get_data_addr(dst->img, &dst->bpp, &dst->line_len, &dst->endian);
    return (!!dst->addr);
}

void destroy_tex(t_game *g, t_tex *t)
{
    if (t->img)
        mlx_destroy_image(g->mlx, t->img);
    t->img = NULL;
}

/* ==========================================================================
**  Carte — Version minimale codée en dur pour tester
**
**  Astuce: commencez avec une toute petite map, puis remplacez par un parser .ber
**  quand la partie rendu est bien comprise.
** ========================================================================== */

static const char *g_small_map[] = {
    "1111111111111",
    "1000000000001",
    "1000111100001",
    "1000100000001",
    "1000100111001",
    "1000000000001",
    "1111111111111",
    NULL
};

void setup_map_small(t_game *g)
{
    int h = 0;
    while (g_small_map[h])
        h++;
    g->map_h = h;
    g->map_w = 0;
    for (int i = 0; i < h; ++i)
    {
        int w = 0;
        while (g_small_map[i][w])
            w++;
        if (w > g->map_w) g->map_w = w;
    }
    g->map = (char **)malloc(sizeof(char *) * (h + 1));
    if (!g->map) panic("malloc map");
    for (int i = 0; i < h; ++i)
    {
        int w = 0;
        while (g_small_map[i][w]) w++;
        g->map[i] = (char *)malloc(w + 1);
        if (!g->map[i]) panic("malloc row");
        for (int j = 0; j < w; ++j)
            g->map[i][j] = g_small_map[i][j];
        g->map[i][w] = '\0';
    }
    g->map[h] = NULL;
}

/* ==========================================================================
**  Player Setup
** ========================================================================== */

static float deg_to_rad(float d) { return d * (float)M_PI / 180.0f; }

void setup_player(t_game *g, float px, float py, float dir_deg)
{
    /* Position initiale (au centre de la case) */
    g->p.pos.x = px + 0.5f;
    g->p.pos.y = py + 0.5f;

    /* Direction à partir d'un angle en degrés. */
    const float a = deg_to_rad(dir_deg);
    g->p.dir.x = cosf(a);
    g->p.dir.y = sinf(a);

    /* Plane = vecteur perpendiculaire à dir qui ouvre le champ de vision.
       Taille du plane = tan(FOV/2). */
    const float plane_len = tanf(deg_to_rad(FOV * 0.5f));
    g->p.plane.x = -g->p.dir.y * plane_len;
    g->p.plane.y =  g->p.dir.x * plane_len;
}

/* ==========================================================================
**  DDA — rendu d'une colonne
**
**  Pipeline:
**    1) On calcule la direction du rayon pour la colonne x.
**    2) On prépare deltaDist (distance pour traverser une case en X ou Y)
**       et sideDist (distance actuelle pour atteindre la première frontière).
**    3) On avance case par case (DDA) jusqu'à frapper un mur.
**    4) On calcule la "perpWallDist" (distance perpendiculaire) pour éviter
**       l'effet fish-eye, puis la hauteur de la bande verticale à dessiner.
**    5) On choisit la coordonnée de texture (tx) et on échantillonne la texture.
** ========================================================================== */

static bool is_wall(t_game *g, int mx, int my)
{
    if (mx < 0 || my < 0 || my >= g->map_h || mx >= g->map_w)
        return true; /* Hors carte = solide */
    return (g->map[my][mx] == '1');
}

void cast_column(t_game *g, int x)
{
    /* x_cam dans [-1, 1] (coordonnée "caméra" horizontale) */
    float x_cam = 2.0f * x / (float)WIN_W - 1.0f;

    /* Direction du rayon = dir + plane * x_cam */
    float ray_dir_x = g->p.dir.x + g->p.plane.x * x_cam;
    float ray_dir_y = g->p.dir.y + g->p.plane.y * x_cam;

    /* Case dans laquelle on se trouve (mapX, mapY). */
    int map_x = (int)g->p.pos.x;
    int map_y = (int)g->p.pos.y;

    /* Longueurs delta */
    float delta_x = (ray_dir_x == 0) ? 1e30f : fabsf(1.0f / ray_dir_x);
    float delta_y = (ray_dir_y == 0) ? 1e30f : fabsf(1.0f / ray_dir_y);

    /* stepX/stepY indiquent dans quel sens on avance (±1).
       sideDistX/sideDistY = distance courante pour atteindre la 1ère frontière. */
    int step_x;
    int step_y;
    float side_x;
    float side_y;

    if (ray_dir_x < 0)
    {
        step_x = -1;
        side_x = (g->p.pos.x - map_x) * delta_x;
    }
    else
    {
        step_x = 1;
        side_x = (map_x + 1.0f - g->p.pos.x) * delta_x;
    }
    if (ray_dir_y < 0)
    {
        step_y = -1;
        side_y = (g->p.pos.y - map_y) * delta_y;
    }
    else
    {
        step_y = 1;
        side_y = (map_y + 1.0f - g->p.pos.y) * delta_y;
    }

    /* DDA: avance case par case jusqu'à toucher un mur. "hit_side" retient
       si l'impact est venu d'un côté vertical (0) ou horizontal (1). */
    int hit_side = 0;
    while (!is_wall(g, map_x, map_y))
    {
        if (side_x < side_y)
        {
            side_x += delta_x;
            map_x += step_x;
            hit_side = 0; /* mur vertical */
        }
        else
        {
            side_y += delta_y;
            map_y += step_y;
            hit_side = 1; /* mur horizontal */
        }
    }

    /* Distance perpendiculaire (corrige le fish-eye). */
    float perp_dist;
    float wall_x;
    if (hit_side == 0)
    {
        perp_dist = (map_x - g->p.pos.x + (1 - step_x) * 0.5f) / ray_dir_x;
        wall_x = g->p.pos.y + perp_dist * ray_dir_y;
    }
    else
    {
        perp_dist = (map_y - g->p.pos.y + (1 - step_y) * 0.5f) / ray_dir_y;
        wall_x = g->p.pos.x + perp_dist * ray_dir_x;
    }
    wall_x -= floorf(wall_x); /* partie fractionnaire -> position du hit le long du mur [0..1) */

    if (perp_dist < 1e-6f) perp_dist = 1e-6f; /* éviter division par zéro */

    /* Hauteur de la bande verticale. */
    int line_h = (int)(WIN_H / perp_dist);
    int draw_start = -line_h / 2 + WIN_H / 2;
    int draw_end   =  line_h / 2 + WIN_H / 2;
    if (draw_start < 0) draw_start = 0;
    if (draw_end >= WIN_H) draw_end = WIN_H - 1;

    /* Dessin du ciel et du sol (ultra simple: aplats).
       Remplacez par une texture si vous voulez. */
    for (int y = 0; y < draw_start; ++y)
        put_pixel(&g->frame, x, y, rgb(120, 180, 255)); /* ciel */
    for (int y = draw_end + 1; y < WIN_H; ++y)
        put_pixel(&g->frame, x, y, rgb(60, 120, 60));   /* sol */

    /* Échantillonnage texture du mur. */
    int tex_x = (int)(wall_x * (float)g->tex_wall.w);
    if ((hit_side == 0 && ray_dir_x > 0) || (hit_side == 1 && ray_dir_y < 0))
        tex_x = g->tex_wall.w - tex_x - 1; /* inversions pour garder une cohérence visuelle */

    /* Étape dans la texture pour parcourir verticalement la bande */
    float step = (float)g->tex_wall.h / (float)line_h;
    float tex_pos = (draw_start - WIN_H / 2 + line_h / 2) * step;

    for (int y = draw_start; y <= draw_end; ++y)
    {
        int tex_y = (int)tex_pos & (g->tex_wall.h - 1); /* & si h est puissance de 2; sinon clamp */
        tex_pos += step;

        char *tp = g->tex_wall.addr + (tex_y * g->tex_wall.line_len + tex_x * (g->tex_wall.bpp / 8));
        int color = *(int *)tp;

        /* assombrir les faces "horizontales" pour un shading cheap */
        if (hit_side == 1)
            color = ((color >> 1) & 0x7F7F7F);

        put_pixel(&g->frame, x, y, color);
    }
}

/* ==========================================================================
**  Rendu complet
** ========================================================================== */

void render_frame(t_game *g)
{
    for (int x = 0; x < WIN_W; ++x)
        cast_column(g, x);
    mlx_put_image_to_window(g->mlx, g->win, g->frame.img, 0, 0);
}

/* ==========================================================================
**  Hooks clavier + boucle principale
** ========================================================================== */

int key_press(int keycode, t_game *g)
{
    if (keycode == KEY_ESC)
        close_window(g);
    else if (keycode == KEY_W) g->keys.w = true;
    else if (keycode == KEY_A) g->keys.a = true;
    else if (keycode == KEY_S) g->keys.s = true;
    else if (keycode == KEY_D) g->keys.d = true;
    else if (keycode == KEY_LEFT)  g->keys.left = true;
    else if (keycode == KEY_RIGHT) g->keys.right = true;
    return (0);
}

int key_release(int keycode, t_game *g)
{
    if (keycode == KEY_W) g->keys.w = false;
    else if (keycode == KEY_A) g->keys.a = false;
    else if (keycode == KEY_S) g->keys.s = false;
    else if (keycode == KEY_D) g->keys.d = false;
    else if (keycode == KEY_LEFT)  g->keys.left = false;
    else if (keycode == KEY_RIGHT) g->keys.right = false;
    return (0);
}

static void move_and_rotate(t_game *g)
{
    /* Mouvement avant/arrière */
    t_v2f forward = g->p.dir;
    t_v2f right = (t_v2f){ g->p.dir.y, -g->p.dir.x }; /* rotation 90° */

    float nx, ny;

    if (g->keys.w)
    {
        nx = g->p.pos.x + forward.x * MOVE_SPEED;
        ny = g->p.pos.y + forward.y * MOVE_SPEED;
        if (!is_wall(g, (int)nx, (int)g->p.pos.y)) g->p.pos.x = nx;
        if (!is_wall(g, (int)g->p.pos.x, (int)ny)) g->p.pos.y = ny;
    }
    if (g->keys.s)
    {
        nx = g->p.pos.x - forward.x * MOVE_SPEED;
        ny = g->p.pos.y - forward.y * MOVE_SPEED;
        if (!is_wall(g, (int)nx, (int)g->p.pos.y)) g->p.pos.x = nx;
        if (!is_wall(g, (int)g->p.pos.x, (int)ny)) g->p.pos.y = ny;
    }
    if (g->keys.a)
    {
        nx = g->p.pos.x - right.x * MOVE_SPEED;
        ny = g->p.pos.y - right.y * MOVE_SPEED;
        if (!is_wall(g, (int)nx, (int)g->p.pos.y)) g->p.pos.x = nx;
        if (!is_wall(g, (int)g->p.pos.x, (int)ny)) g->p.pos.y = ny;
    }
    if (g->keys.d)
    {
        nx = g->p.pos.x + right.x * MOVE_SPEED;
        ny = g->p.pos.y + right.y * MOVE_SPEED;
        if (!is_wall(g, (int)nx, (int)g->p.pos.y)) g->p.pos.x = nx;
        if (!is_wall(g, (int)g->p.pos.x, (int)ny)) g->p.pos.y = ny;
    }

    /* Rotation gauche/droite (mise à jour dir et plane). */
    if (g->keys.left || g->keys.right)
    {
        float ang = (g->keys.left ? -ROT_SPEED : ROT_SPEED);
        float odx = g->p.dir.x;
        g->p.dir.x = g->p.dir.x * cosf(ang) - g->p.dir.y * sinf(ang);
        g->p.dir.y = odx          * sinf(ang) + g->p.dir.y * cosf(ang);

        float opx = g->p.plane.x;
        g->p.plane.x = g->p.plane.x * cosf(ang) - g->p.plane.y * sinf(ang);
        g->p.plane.y = opx           * sinf(ang) + g->p.plane.y * cosf(ang);
    }
}

int loop_hook(t_game *g)
{
    move_and_rotate(g);
    render_frame(g);
    return (0);
}

/* ==========================================================================
**  Quitter proprement
** ========================================================================== */

int close_window(t_game *g)
{
    destroy_tex(g, &g->tex_wall);
    destroy_tex(g, &g->tex_floor);
    destroy_tex(g, &g->tex_sky);
    destroy_frame(g);
    if (g->win) mlx_destroy_window(g->mlx, g->win);
    if (g->mlx) mlx_destroy_display(g->mlx), free(g->mlx);
    exit(0);
    return (0);
}

/* ==========================================================================
**  main — ultra commenté pour comprendre le flux.
** ========================================================================== */

int main(void)
{
    t_game g;

    /* 1) Zero-initialisation pour éviter les surprises. */
    __builtin_memset(&g, 0, sizeof(g));

    /* 2) Initialiser MLX et créer une fenêtre. */
    g.mlx = mlx_init();
    if (!g.mlx) panic("mlx_init failed");
    g.win = mlx_new_window(g.mlx, WIN_W, WIN_H, "poke3D — DDA engine");
    if (!g.win) panic("mlx_new_window failed");

    /* 3) Créer un framebuffer où l'on dessine chaque frame. */
    if (!create_frame(&g, WIN_W, WIN_H)) panic("create_frame failed");

    /* 4) Charger au moins une texture de mur (mur XPM à la racine du projet).
       Vous pouvez utiliser vos fichiers fournis: "wall.xpm", "floor.xpm", "sky.xpm".
       Ici on ne s'en sert visuellement que pour les murs (simple). */
    if (!load_xpm(&g, &g.tex_wall, "wall.xpm")) panic("load wall.xpm failed");
    /* Optionnel: si vous voulez remplacer les aplats "ciel/sol" par textures:
    load_xpm(&g, &g.tex_sky, "sky.xpm");
    load_xpm(&g, &g.tex_floor, "floor.xpm");
    */

    /* 5) Construire une petite carte (vous changerez par un parser .ber). */
    setup_map_small(&g);

    /* 6) Placer le joueur. dir_deg = 0 veut dire "vers +x" (à droite). */
    setup_player(&g, 2, 2, 0.0f);

    /* 7) Hooker clavier et fermeture, et lancer la boucle. */
    mlx_hook(g.win, DestroyNotify, StructureNotifyMask, close_window, &g);
    mlx_hook(g.win, KeyPress, KeyPressMask, key_press, &g);
    mlx_hook(g.win, KeyRelease, KeyReleaseMask, key_release, &g);
    mlx_loop_hook(g.mlx, loop_hook, &g);

    render_frame(&g); /* premier rendu */

    mlx_loop(g.mlx);
    return (0);
}
