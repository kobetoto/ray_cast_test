#include "game.h"
#include <unistd.h>   /* write(), etc. */
#include <string.h>   /* strlen(), memset() */
#include <stdio.h>    /* snprintf() pour le helper de chargement */

/* ==========================================================================
**  Helpers — erreurs & pixels
**  --------------------------------------------------------------------------
**  panic       : affiche un message d'erreur sur stderr et quitte le programme
**  rgb         : compose un entier 0xRRGGBB depuis des composantes 0..255
**  put_pixel   : écrit un pixel (couleur) dans une image MLX (framebuffer)
** ========================================================================== */

void    panic(const char *msg)
{
    /* Écrit le message d'erreur sur la sortie d'erreur (descripteur 2) */
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
    /* Termine le programme avec un code de sortie d'erreur */
    exit(1);
}

int     rgb(int r, int g, int b)
{
    /* Sécurise les bornes des composantes entre 0 et 255
       (évite des dépassements si le calcul d'une couleur sort de l'intervalle) */
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (g > 255) g = 255;
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    /* Pack 3 octets R, G, B dans un entier 0xRRGGBB (format attendu par MLX) */
    return (r << 16) | (g << 8) | b;
}

void    put_pixel(t_img *img, int x, int y, int color)
{
    char *p;

    /* Ignore toute écriture hors-bord pour éviter un segfault */
    if (x < 0 || x >= img->w || y < 0 || y >= img->h)
        return ;
    /* Calcule l'adresse du pixel (ligne + colonne * taille pixel) */
    p = img->addr + (y * img->line_len + x * (img->bpp / 8));
    /* Écrit la couleur (int 0xRRGGBB) dans l'image MLX */
    *(int *)p = color;
}

/* ==========================================================================
**  Frame / Textures
**  --------------------------------------------------------------------------
**  clampi      : borne un entier entre lo et hi
**  wrapi       : fait un modulo positif (wrap) pour répéter une coordonnée
**  texel_at    : lit un pixel (texel) dans une texture MLX aux coords (u,v)
**  try_load_xpm_paths : tente de charger un XPM depuis plusieurs chemins
**  create_frame: crée une image MLX qui sert de framebuffer
**  destroy_frame: détruit ce framebuffer
**  load_xpm    : charge un fichier XPM dans une structure t_tex
**  destroy_tex : détruit l'image MLX d'une texture
** ========================================================================== */

static inline int  clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline int  wrapi(int v, int m)
{
    int r;

    /* Gestion de cas pathologique (évite division par 0) */
    if (m <= 0) return 0;
    /* Modulo classique, puis on s'assure d'un résultat positif */
    r = v % m;
    if (r < 0) r += m;
    return r;
}

static inline int  texel_at(t_tex *t, int u, int v)
{
    char *p;

    /* On répète horizontalement (u) et on borne verticalement (v) */
    u = wrapi(u, t->w);
    v = clampi(v, 0, t->h - 1);
    /* Adresse du texel = base + (ligne * stride + colonne * bytes_per_pixel) */
    p = t->addr + (v * t->line_len + u * (t->bpp / 8));
    /* Retourne la couleur lue (format MLX 0xRRGGBB) */
    return *(int *)p;
}

/* Essaie plusieurs emplacements standards pour trouver l’asset.
   - file       : nom du fichier (ex. "sky.xpm")
   - cand[]     : liste d'emplacements tentés ("src/", "assets/", etc.)
   - On concatène le chemin + fichier dans buf, puis on teste load_xpm.
   - Renvoie 1 si succès, 0 sinon. */
static int  try_load_xpm_paths(t_game *g, t_tex *dst, const char *file)
{
    const char  *cand[] = { file, "src/", "assets/", "./src/", "./assets/", NULL };
    char        buf[512];
    int         i;
    size_t      n;

    i = 0;
    while (cand[i])
    {
        n = strlen(cand[i]);
        if (n && cand[i][n - 1] == '/')
            snprintf(buf, sizeof(buf), "%s%s", cand[i], file);
        else
            snprintf(buf, sizeof(buf), "%s", cand[i]);
        if (load_xpm(g, dst, buf))
            return (1);
        i++;
    }
    return (0);
}

/* Crée une image MLX (framebuffer de rendu) de taille (w,h).
   - stocke son pointeur et les métadonnées (bpp, line_len, endian) */
int     create_frame(t_game *g, int w, int h)
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

/* Détruit l'image MLX utilisée comme framebuffer (si elle existe) */
void    destroy_frame(t_game *g)
{
    if (g->frame.img)
        mlx_destroy_image(g->mlx, g->frame.img);
    g->frame.img = NULL;
}

/* Charge un fichier XPM sur disque dans une structure t_tex.
   - mlx_xpm_file_to_image remplit t->w et t->h (dimensions)
   - mlx_get_data_addr permet d'accéder aux pixels (addr/bpp/stride/endian) */
int     load_xpm(t_game *g, t_tex *dst, const char *path)
{
    dst->img = mlx_xpm_file_to_image(g->mlx, (char *)path, &dst->w, &dst->h);
    if (!dst->img)
        return (0);
    dst->addr = mlx_get_data_addr(dst->img, &dst->bpp, &dst->line_len, &dst->endian);
    return (!!dst->addr);
}

/* Détruit l'image MLX d'une texture (si elle existe) */
void    destroy_tex(t_game *g, t_tex *t)
{
    if (t->img)
        mlx_destroy_image(g->mlx, t->img);
    t->img = NULL;
}

/* ==========================================================================
**  Carte — petite map codée en dur
**  --------------------------------------------------------------------------
**  g_small_map : tableau de chaînes représentant la carte (1 = mur, 0 = vide)
**  setup_map_small : copie g_small_map en mémoire dynamique dans g->map
**                    + calcule la largeur (map_w) et hauteur (map_h)
** ========================================================================== */

static const char *g_small_map[] = {
    "1111111111111111111111111111111111",
    "1000000000000000000000000000000001",
    "1000000000000000000000000000000001",
    "1000000000000000000000000000000001",
    "1000000000000000000000000000000001",
    "1000000000000000000000000000000001",
    "1000000000000000000000000000000001",
    "1000000000000000000000000000000001",
    "1000111100000000000000000000000001",
    "1000100000000000000000000000000001",
    "1000100111000000000000000000000001",
    "1000000000000000000000000000000001",
    "1111111111111111111111111111111111",
    NULL
};

void    setup_map_small(t_game *g)
{
    int h, i, w, j;

    /* Mesure la hauteur (nombre de lignes) de la carte statique */
    h = 0;
    while (g_small_map[h]) h++;
    g->map_h = h;

    /* Calcule la largeur maximale parmi les lignes (sécurité) */
    g->map_w = 0;
    i = 0;
    while (i < h)
    {
        w = 0;
        while (g_small_map[i][w]) w++;
        if (w > g->map_w) g->map_w = w;
        i++;
    }

    /* Alloue g->map (copie dynamique) + 1 pour un pointeur NULL final */
    g->map = (char **)malloc(sizeof(char *) * (h + 1));
    if (!g->map) panic("malloc map");

    /* Copie chaque ligne dans un buffer alloué (terminé par '\0') */
    i = 0;
    while (i < h)
    {
        w = 0;
        while (g_small_map[i][w]) w++;
        g->map[i] = (char *)malloc(w + 1);
        if (!g->map[i]) panic("malloc row");
        j = 0;
        while (j < w) { g->map[i][j] = g_small_map[i][j]; j++; }
        g->map[i][w] = '\0';
        i++;
    }
    /* Termine le tableau de lignes par NULL (facilite les parcours) */
    g->map[h] = NULL;
}

/* ==========================================================================
**  Player Setup
**  --------------------------------------------------------------------------
**  deg_to_rad  : conversion degrés -> radians
**  setup_player: initialise position, direction et plan caméra (FOV)
** ========================================================================== */

static float   deg_to_rad(float d) { return d * (float)M_PI / 180.0f; }

void    setup_player(t_game *g, float px, float py, float dir_deg)
{
    float a = deg_to_rad(dir_deg);              /* angle initial en radians */
    float plane_len = tanf(deg_to_rad(FOV * 0.5f)); /* longueur du plan (FOV/2) */

    /* Centre le joueur au milieu de la case (px,py) */
    g->p.pos.x = px + 0.5f;
    g->p.pos.y = py + 0.5f;

    /* Vecteur direction (face où regarde le joueur) */
    g->p.dir.x = cosf(a);
    g->p.dir.y = sinf(a);

    /* Vecteur plan caméra (perpendiculaire à dir, définit l'ouverture du FOV) */
    g->p.plane.x = -g->p.dir.y * plane_len;
    g->p.plane.y =  g->p.dir.x * plane_len;
}

/* ==========================================================================
**  DDA — rendu d'une colonne
**  --------------------------------------------------------------------------
**  is_wall     : renvoie true si (mx,my) est un mur (ou hors carte = solide)
**  cast_column : RAYCASTING PAR DDA (Digital Differential Analyzer)
**                - Calcule la direction du rayon pour la colonne écran x
**                - Avance de case en case (grille) jusqu'à heurter un mur
**                - Calcule la distance perpendiculaire (anti fish-eye)
**                - Dessine CIEL (texturé), SOL (floor-casting), MUR (texturé)
** ========================================================================== */

static bool is_wall(t_game *g, int mx, int my)
{
    /* Toute coordonnée hors carte est considérée "mur" (solide) */
    if (mx < 0 || my < 0 || my >= g->map_h || mx >= g->map_w)
        return true;
    /* Mur si caractère == '1' */
    return (g->map[my][mx] == '1');
}

void    cast_column(t_game *g, int x)
{
    /* x_cam ∈ [-1,1] : position horizontale normalisée de la colonne x */
    float   x_cam = 2.0f * x / (float)WIN_W - 1.0f;

    /* Direction du rayon pour cette colonne :
       dir_cam = dir_joueur + plane * x_cam (modifie selon l’angle écran) */
    float   ray_dir_x = g->p.dir.x + g->p.plane.x * x_cam;
    float   ray_dir_y = g->p.dir.y + g->p.plane.y * x_cam;

    /* Case de départ dans la grille (projection entière de la position joueur) */
    int     map_x = (int)g->p.pos.x;
    int     map_y = (int)g->p.pos.y;

    /* delta_x/delta_y = distance pour traverser une case entière en X/Y
       (pré-calcul nécessaire pour comparer les distances dans la DDA) */
    float   delta_x = (ray_dir_x == 0) ? 1e30f : fabsf(1.0f / ray_dir_x);
    float   delta_y = (ray_dir_y == 0) ? 1e30f : fabsf(1.0f / ray_dir_y);

    /* step_x/step_y : sens d’incrémentation (±1) selon le signe du rayon
       side_x/side_y : distance actuelle jusqu’à la première frontière X/Y */
    int     step_x, step_y;
    float   side_x, side_y;

    if (ray_dir_x < 0) { step_x = -1; side_x = (g->p.pos.x - map_x) * delta_x; }
    else               { step_x =  1; side_x = (map_x + 1.0f - g->p.pos.x) * delta_x; }
    if (ray_dir_y < 0) { step_y = -1; side_y = (g->p.pos.y - map_y) * delta_y; }
    else               { step_y =  1; side_y = (map_y + 1.0f - g->p.pos.y) * delta_y; }

    /* Boucle DDA : on avance toujours du côté le plus proche (X ou Y)
       jusqu’à heurter une case "mur". hit_side = 0 si impact vertical, 1 si horizontal */
    int     hit_side = 0;
    while (!is_wall(g, map_x, map_y))
    {
        if (side_x < side_y) { side_x += delta_x; map_x += step_x; hit_side = 0; }
        else                 { side_y += delta_y; map_y += step_y; hit_side = 1; }
    }

    /* Distance perpendiculaire au mur (corrige le fish-eye) + point d’impact le long du mur */
    float   perp_dist, wall_x;
    if (hit_side == 0)
    {
        /* Impact sur une paroi verticale : on calcule la distance le long de X */
        perp_dist = (map_x - g->p.pos.x + (1 - step_x) * 0.5f) / ray_dir_x;
        /* Position d’impact sur l’axe Y (pour texturer correctement) */
        wall_x = g->p.pos.y + perp_dist * ray_dir_y;
    }
    else
    {
        /* Impact sur une paroi horizontale : distance le long de Y */
        perp_dist = (map_y - g->p.pos.y + (1 - step_y) * 0.5f) / ray_dir_y;
        /* Position d’impact sur l’axe X (pour texturer) */
        wall_x = g->p.pos.x + perp_dist * ray_dir_x;
    }
    /* Garde la partie fractionnaire (0..1) pour connaître l’offset sur le mur */
    wall_x -= floorf(wall_x);
    /* Évite une division par 0 dans le calcul de hauteur de bande */
    if (perp_dist < 1e-6f) perp_dist = 1e-6f;

    /* Taille de la bande verticale (plus le mur est proche, plus c’est haut) */
    int line_h = (int)(WIN_H / perp_dist);
    /* Y de début/fin de la bande mur (centrage vertical) */
    int draw_start = -line_h / 2 + WIN_H / 2;
    int draw_end   =  line_h / 2 + WIN_H / 2;
    /* Borne le dessin à l’écran */
    if (draw_start < 0) draw_start = 0;
    if (draw_end >= WIN_H) draw_end = WIN_H - 1;

    /* ---------- CIEL TEXTURÉ (panorama horizontal + léger défilement) ---------- */
    if (g->tex_sky.img)
    {
        /* Convertit la direction du rayon en angle (-pi..pi) puis en [0..1) */
        float ang = atan2f(ray_dir_y, ray_dir_x);
        float u01 = (ang / (2.0f * (float)M_PI)) + 0.5f;
        /* Colonne dans la texture du ciel, avec un décalage qui varie lentement (g->tick/2) */
        int   tx  = wrapi((int)(u01 * (float)g->tex_sky.w)
                          + (g->tick / 2) % g->tex_sky.w, g->tex_sky.w); /* défilement lent */

        /* Paramètre d’horizon : plus grand => horizon plus bas (plus de ciel visible en haut) */
        const float horizon_y = WIN_H * 0.70f; /* ajuste 0.60..0.75 pour placer l’horizon */

        int y = 0;
        while (y < draw_start)  /* Dessine le ciel de la ligne 0 à draw_start-1 */
        {
            /* v01 mappe la hauteur écran vers la hauteur texture du ciel */
            float v01 = (float)y / horizon_y;
            int   ty  = (int)(v01 * (float)g->tex_sky.h);
            ty = clampi(ty, 0, g->tex_sky.h - 1);
            /* Lit la couleur du ciel à (tx, ty) et dessine le pixel */
            int col = texel_at(&g->tex_sky, tx, ty);
            put_pixel(&g->frame, x, y, col);
            y++;
        }
    }
    else
    {
        /* Fallback : si pas de texture ciel, on remplit le haut avec un bleu */
        int y = 0;
        while (y < draw_start) { put_pixel(&g->frame, x, y, rgb(120,180,255)); y++; }
    }

    /* ---------- SOL TEXTURÉ (floor casting corrigé) ----------
       Technique : pour chaque ligne sous l’horizon, on calcule la distance
       au "plan du sol" et on en déduit la coordonnée monde (floorX, floorY)
       qu'on convertit en coordonnées texture (wrap).
       Ici, on prend la *partie fractionnaire* de (floorX,floorY) pour éviter
       les étirements (aliasing) à grande distance. */
    if (g->tex_floor.img)
    {
        /* Directions des rayons aux extrémités gauche/droite de l’écran
           (constantes pour une même ligne d’écran) */
        float dir0x = g->p.dir.x - g->p.plane.x;
        float dir0y = g->p.dir.y - g->p.plane.y;
        float dir1x = g->p.dir.x + g->p.plane.x;
        float dir1y = g->p.dir.y + g->p.plane.y;

        /* Position “caméra” en Z = moitié de la hauteur d’écran (convention) */
        float posZ = 0.5f * (float)WIN_H;

        int y = draw_end + 1;    /* On démarre sous le mur affiché */
        while (y < WIN_H)
        {
            /* p = distance verticale (pixels) entre la ligne y et l’horizon (centre) */
            float p = (float)y - (float)WIN_H / 2.0f;
            if (p == 0.0f) p = 0.0001f;       /* évite une division par 0 */
            /* rowDist = distance au plan du sol correspondant à la ligne y */
            float rowDist = posZ / p;

            /* Position monde au bord gauche, puis interpolation linéaire vers x
               (équivalent : floorX = pos + rowDist * dir0 + step * x) */
            float floorX_left  = g->p.pos.x + rowDist * dir0x;
            float floorY_left  = g->p.pos.y + rowDist * dir0y;
            float floorX = floorX_left + (rowDist * (dir1x - dir0x)) * ((float)x / (float)WIN_W);
            float floorY = floorY_left + (rowDist * (dir1y - dir0y)) * ((float)x / (float)WIN_W);

            /* IMPORTANT : on ne garde que la fraction (0..1) pour échantillonner
               la tuile de sol de manière répétée et stable (évite “l’étirement”). */
            float fx = floorX - floorf(floorX);
            float fy = floorY - floorf(floorY);
            int   tx = (int)(fx * (float)g->tex_floor.w);
            int   ty = (int)(fy * (float)g->tex_floor.h);

            /* Lit la couleur dans la texture de sol */
            int color = texel_at(&g->tex_floor, tx, ty);

            /* Assombrissement doux avec la distance pour donner de la profondeur
               (fonction 1 / (1 + k * d^2) ) */
            float shade = 1.0f / (1.0f + 0.02f * rowDist * rowDist);
            int rr = (int)(((color >> 16) & 255) * shade);
            int gg = (int)(((color >> 8)  & 255) * shade);
            int bb = (int)(( color        & 255) * shade);
            color = rgb(rr, gg, bb);

            /* Dessine le pixel de sol */
            put_pixel(&g->frame, x, y, color);
            y++;
        }
    }
    else
    {
        /* Fallback : si pas de texture sol, on remplit la zone basse en vert */
        int y = draw_end + 1;
        while (y < WIN_H) { put_pixel(&g->frame, x, y, rgb(60,120,60)); y++; }
    }

    /* ---------- MUR TEXTURÉ ----------
       On récupère l'abscisse de texture (tex_x) via wall_x (fraction 0..1)
       + on parcourt verticalement la bande pour peindre chaque pixel du mur. */
    {
        /* Coordonnée horizontale dans la texture de mur (colonne) */
        int   tex_x = (int)(wall_x * (float)g->tex_wall.w);
        /* Inversion de texture selon la face impactée pour garder une cohérence
           gauche/droite (évite d’avoir la texture “miroir” selon l’angle) */
        if ((hit_side == 0 && ray_dir_x > 0) || (hit_side == 1 && ray_dir_y < 0))
            tex_x = g->tex_wall.w - tex_x - 1;

        /* step = combien de pixels texture on avance par pixel écran vertical */
        float step = (float)g->tex_wall.h / (float)line_h;
        /* Position de départ dans la texture (alignement vertical de la bande) */
        float tex_pos = (draw_start - WIN_H / 2 + line_h / 2) * step;

        int y = draw_start;
        while (y <= draw_end)
        {
            /* Coordonnée verticale dans la texture (borne pour la sécurité) */
            int tex_y = (int)tex_pos;
            tex_y = clampi(tex_y, 0, g->tex_wall.h - 1);
            tex_pos += step;

            /* Adresse du texel (mur) à (tex_x, tex_y) */
            char *tp = g->tex_wall.addr + (tex_y * g->tex_wall.line_len
                         + tex_x * (g->tex_wall.bpp / 8));
            int color = *(int *)tp;

            /* Dessine le pixel de mur sur la colonne x, ligne y */
            put_pixel(&g->frame, x, y, color);
            y++;
        }
    }
}

/* ==========================================================================
**  Rendu complet
**  --------------------------------------------------------------------------
**  render_frame : itère les colonnes x=0..WIN_W-1, appelle cast_column,
**                 puis affiche l'image MLX en fenêtre
** ========================================================================== */

void    render_frame(t_game *g)
{
    int x = 0;
    while (x < WIN_W) { cast_column(g, x); x++; }
    /* Affiche le framebuffer (image MLX) dans la fenêtre à la position (0,0) */
    mlx_put_image_to_window(g->mlx, g->win, g->frame.img, 0, 0);
}

/* ==========================================================================
**  Hooks clavier + boucle principale
**  --------------------------------------------------------------------------
**  key_press   : met à true les flags de touches pressées (WASD, flèches)
**  key_release : met à false les flags de touches relâchées
**  move_and_rotate : applique les déplacements (avec collision) et la rotation
**  loop_hook   : tick d'animation + mouvement + rendu (appelé chaque frame)
** ========================================================================== */

int     key_press(int keycode, t_game *g)
{
    if (keycode == KEY_ESC) close_window(g); /* Fermeture immédiate */
    else if (keycode == KEY_W) g->keys.w = true;
    else if (keycode == KEY_A) g->keys.a = true;
    else if (keycode == KEY_S) g->keys.s = true;
    else if (keycode == KEY_D) g->keys.d = true;
    else if (keycode == KEY_LEFT)  g->keys.left = true;
    else if (keycode == KEY_RIGHT) g->keys.right = true;
    return (0);
}

int     key_release(int keycode, t_game *g)
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
    /* Vecteur avant (direction du joueur) et droite (dir tournée de 90°) */
    t_v2f forward = g->p.dir;
    t_v2f right = (t_v2f){ g->p.dir.y, -g->p.dir.x };
    float nx, ny;

    /* Avancer (W) : on essaie d'avancer sur X puis Y en vérifiant les murs (collision AABB simple) */
    if (g->keys.w)
    {
        nx = g->p.pos.x + forward.x * MOVE_SPEED;
        ny = g->p.pos.y + forward.y * MOVE_SPEED;
        if (!is_wall(g, (int)nx, (int)g->p.pos.y)) g->p.pos.x = nx;
        if (!is_wall(g, (int)g->p.pos.x, (int)ny)) g->p.pos.y = ny;
    }
    /* Reculer (S) : pareil mais en sens inverse */
    if (g->keys.s)
    {
        nx = g->p.pos.x - forward.x * MOVE_SPEED;
        ny = g->p.pos.y - forward.y * MOVE_SPEED;
        if (!is_wall(g, (int)nx, (int)g->p.pos.y)) g->p.pos.x = nx;
        if (!is_wall(g, (int)g->p.pos.x, (int)ny)) g->p.pos.y = ny;
    }
    /* Strafe gauche (A) : déplacement latéral à gauche (vector right négatif) */
    if (g->keys.a)
    {
        nx = g->p.pos.x - right.x * MOVE_SPEED;
        ny = g->p.pos.y - right.y * MOVE_SPEED;
        if (!is_wall(g, (int)nx, (int)g->p.pos.y)) g->p.pos.x = nx;
        if (!is_wall(g, (int)g->p.pos.x, (int)ny)) g->p.pos.y = ny;
    }
    /* Strafe droit (D) : déplacement latéral à droite (vector right positif) */
    if (g->keys.d)
    {
        nx = g->p.pos.x + right.x * MOVE_SPEED;
        ny = g->p.pos.y + right.y * MOVE_SPEED;
        if (!is_wall(g, (int)nx, (int)g->p.pos.y)) g->p.pos.x = nx;
        if (!is_wall(g, (int)g->p.pos.x, (int)ny)) g->p.pos.y = ny;
    }

    /* Rotation gauche/droite via matrices de rotation 2D sur dir et plane */
    if (g->keys.left || g->keys.right)
    {
        float ang = (g->keys.left ? -ROT_SPEED : ROT_SPEED);
        /* Rotation du vecteur direction */
        float odx = g->p.dir.x;
        g->p.dir.x = g->p.dir.x * cosf(ang) - g->p.dir.y * sinf(ang);
        g->p.dir.y = odx          * sinf(ang) + g->p.dir.y * cosf(ang);

        /* Rotation du plan caméra (même angle) */
        float opx = g->p.plane.x;
        g->p.plane.x = g->p.plane.x * cosf(ang) - g->p.plane.y * sinf(ang);
        g->p.plane.y = opx           * sinf(ang) + g->p.plane.y * cosf(ang);
    }
}

int     loop_hook(t_game *g)
{
    g->tick++;              /* Incrémente un compteur (sert au défilement du ciel) */
    move_and_rotate(g);     /* Applique les entrées clavier et met à jour la caméra */
    render_frame(g);        /* Recalcule toute la frame et l'affiche */
    return (0);
}

/* ==========================================================================
**  Quitter proprement
**  --------------------------------------------------------------------------
**  close_window : libère textures, frame, fenêtre et display MLX puis exit
** ========================================================================== */

int     close_window(t_game *g)
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
**  main — flux global
**  --------------------------------------------------------------------------
**  - init structures (memset)
**  - init MLX (contexte + fenêtre)
**  - créer framebuffer
**  - charger textures (mur/sky/sol) depuis différents chemins possibles
**  - construire une petite carte et placer le joueur
**  - installer les hooks (clavier, fermeture, boucle)
**  - lancer la boucle MLX
** ========================================================================== */

int     main(void)
{
    t_game  g;

    /* Met la structure à zéro (évite des pointeurs “sauvages”) */
    __builtin_memset(&g, 0, sizeof(g));

    /* Initialisation MLX : ouvre une connexion au serveur X */
    g.mlx = mlx_init();
    if (!g.mlx) panic("mlx_init failed");

    /* Crée une fenêtre de taille WIN_W × WIN_H avec un titre */
    g.win = mlx_new_window(g.mlx, WIN_W, WIN_H, "poke3DDA engine");
    if (!g.win) panic("mlx_new_window failed");

    /* Crée l'image (framebuffer) dans laquelle on dessinera chaque frame */
    if (!create_frame(&g, WIN_W, WIN_H)) panic("create_frame failed");

    /* Mur: mets "wall.xpm" si tu veux un mur classique — ici on utilise "tree1.xpm"
       pour bloquer comme un “arbre” impassable (façon barrière naturelle). */
    if (!try_load_xpm_paths(&g, &g.tex_wall, "tree1.xpm"))
        panic("load wall texture failed");

    /* Ciel panoramique (défilement doux dans cast_column) */
    if (!try_load_xpm_paths(&g, &g.tex_sky, "sky.xpm"))
        panic("load sky.xpm failed");

    /* Sol texturé (floor casting) : idéalement une tuile “herbe” seamless */
    if (!try_load_xpm_paths(&g, &g.tex_floor, "floor.xpm"))
        panic("load floor.xpm failed");

    /* Construit la mini-carte et place le joueur en (2,2), regardant vers +X (0°) */
    setup_map_small(&g);
    setup_player(&g, 2, 2, 0.0f);

    /* Installe les hooks :
       - DestroyNotify: fermeture de la fenêtre
       - KeyPress/KeyRelease: gestion des entrées clavier
       - loop_hook: callback appelé en boucle chaque “frame” */
    mlx_hook(g.win, DestroyNotify, StructureNotifyMask, close_window, &g);
    mlx_hook(g.win, KeyPress, KeyPressMask, key_press, &g);
    mlx_hook(g.win, KeyRelease, KeyReleaseMask, key_release, &g);
    mlx_loop_hook(g.mlx, loop_hook, &g);

    /* Démarre le tick et dessine une frame initiale (optionnel, pour éviter un flash noir) */
    g.tick = 0;
    render_frame(&g);

    /* Boucle événementielle MLX (ne retourne pas avant la fermeture) */
    mlx_loop(g.mlx);
    return (0);
}
