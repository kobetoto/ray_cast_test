#include "../includes/game.h"
/**
 * MAPPING
 */
char	**get_map(void)
{
	char	**m;

	m = (char **)malloc(sizeof(char *) * 20);
	if (!m)
		return (NULL);
	m[0] = "111111111111111";
	m[1] = "100000000000001";
	m[2] = "100000000000001";
	m[3] = "100000000000001";
	m[4] = "100000000000001";
	m[5] = "100000010000001";
	m[6] = "100000000000001";
	m[7] = "100000000000001";
	m[8] = "100000011111001";
    m[9] = "100000000000001";
	m[10] = "100000000000001";
	m[11] = "100000000000001";
	m[12] = "100000000000001";
    m[13] = "100111110000001";
	m[14] = "100000000000001";
	m[15] = "100000000000001";
	m[16] = "100000000000001";
	m[17] = "111111111111111";
	m[18] = NULL;
	return (m);
}


/**
 * INIT
 */
 int	init_game(t_game *g)
{
	init_player(&g->player);
	g->map = get_map();
	if (!g->map)
		return (1);
	g->mlx = mlx_init();
	if (!g->mlx)
		return (1);
	g->win = mlx_new_window(g->mlx, WIDTH, HEIGHT, "Game");
	if (!g->win)
		return (1);
	g->img = mlx_new_image(g->mlx, WIDTH, HEIGHT);
	if (!g->img)
		return (1);
	g->data = mlx_get_data_addr(g->img, &g->bpp, &g->size_line, &g->endian);
	if (!g->data)
		return (1);
	return (0);
}

/**
 * put pixel in a buffer memory (not in mlx window directly)
 */
 void	put_pixel(int x, int y, int color, t_game *g)
{
	int	offset;
	int	bpp8;

	if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT)
		return ;
	bpp8 = g->bpp / 8;
	offset = y * g->size_line + x * bpp8;
	g->data[offset + 0] = color & 0xFF;
	g->data[offset + 1] = (color >> 8) & 0xFF;
	g->data[offset + 2] = (color >> 16) & 0xFF;
}

/**
 * clearing
 */
 void	clear_image(t_game *g)
{
	int	x;
	int	y;

	y = 0;
	while (y < HEIGHT)
	{
		x = 0;
		while (x < WIDTH)
		{
			put_pixel(x, y, 0x000000, g);
			x++;
		}
		y++;
	}
}


/**
 * "solo block" UTILS
 */
static bool map_inside(t_game *g, int cy, int cx)
{
    if (cy < 0) return false;
    if (!g->map[cy]) return false;
    if (cx < 0) return false;
    if (!g->map[cy][cx]) return false;
    return true;
}

/**
 * 
 * true si la case est un bord de la map (au moins un "voisin" sort des bornes)
 * nord 
 * sud  
 * ouest
 * est  
 * */
static bool is_border_cell(t_game *g, int cy, int cx)
{
    return (!map_inside(g, cy-1, cx) ||
            !map_inside(g, cy+1, cx) ||
            !map_inside(g, cy, cx-1) ||
            !map_inside(g, cy, cx+1)); 
}

/* true si la case '1' est isolée: N,S,E,W ne sont PAS des murs,
   et la case n'est pas au bord de la map */
static bool is_solo_block(t_game *g, int cy, int cx)
{
    if (!map_inside(g, cy, cx))
		return false;
    if (g->map[cy][cx] != '1')
		return false;
    if (is_border_cell(g, cy, cx))
		return false;

    return (g->map[cy-1][cx] != '1' &&
            g->map[cy+1][cx] != '1' &&
            g->map[cy][cx-1] != '1' &&
            g->map[cy][cx+1] != '1');
}

/**
 * Convertit la distance perpendiculaire en hauteur projetée (h) (perspective).
 * Calcule le segment vertical (du haut y0 au bas y1) où dessiner la colonne du mur, centré verticalement.
 */
 void	compute_strip_bounds(float dist, int *y0, int *y1, int *h)
{
	float	hf;
	int		top;
	int		botom;

	/*hf = hauteur projetée du mur à l’écran en pixels*/
	hf = (BLOCK / dist) * (WIDTH / 1.7f);
	/*hauteur du mur en pixels à dessiner dans la fenêtre.*/
	*h = (int)hf;
	top = (HEIGHT - *h) / 2;
	botom = top + *h;
	/*si tres proche de la map (clamping)*/
	if (top < 0)
		top = 0;
	if (botom > HEIGHT)
		botom = HEIGHT;
	*y0 = top;
	*y1 = botom;
}

 void	compute_tex_vscroll(const t_tex *t, int h, int y0,
				float *step, float *tex_pos)
{
	/*taux d’étirement de la texture*/
	*step = (float)t->h / (float)h;
	/*position de départ dans la texture (première ligne à lire)*/
	*tex_pos = (y0 - (HEIGHT - h) / 2) * (*step);
}

/* --------------------------- Texture utils ------------------------------- */

int	pack_rgb_24(unsigned char r, unsigned char g, unsigned char b)
{
	return ((int)r << 16 | (int)g << 8 | (int)b);
}

 int	tex_read(const t_tex *t, int tx, int ty)
{
	int					bpp8;
	const unsigned char	*p;
	unsigned char		r;
	unsigned char		g;
	unsigned char		b;

	if (tx < 0)
		tx = 0;
	if (tx >= t->w)
		tx = t->w - 1;
	if (ty < 0)
		ty = 0;
	if (ty >= t->h)
		ty = t->h - 1;
	bpp8 = t->bpp / 8;
	p = (const unsigned char *)t->data + ty * t->size_line + tx * bpp8;
	b = p[0];
	g = p[1];
	r = p[2];
	return (pack_rgb_24(r, g, b));
}

 int	load_texture(t_game *g, t_tex *t, const char *path)
{
	t->img = mlx_xpm_file_to_image(g->mlx, (char *)path, &t->w, &t->h);
	if (!t->img)
		return (1);
	t->data = mlx_get_data_addr(t->img, &t->bpp, &t->size_line, &t->endian);
	if (!t->data)
		return (1);
	return (0);
}

/* --------------------------- Raycast utils --------------------------------*/

int	is_close_to_grid(float v, float cell, float gap_epsilion)
{
	float	mod;

	mod = fmodf(v, cell);
	if (mod < gap_epsilion)
		return (1);
	if ((cell - mod) < gap_epsilion)
		return (1);
	return (0);
}

/* --------------------------- Raycast utils (DDA) -------------------------- */

/* test direct sur la grille (en indices de cellules) */
static inline int map_cell_is_wall(t_game *g, int mx, int my)
{
	if (mx < 0 || my < 0) return 1;
	if (!g->map[my]) return 1;
	if (!g->map[my][mx]) return 1;
	return (g->map[my][mx] == '1');
}

/* Lancer de rayon version DDA :
   - travaille en coordonnées "cases" pour le DDA,
   - renvoie une distance perpendiculaire (anti-fisheye) en PIXELS,
   - et les coordonnées d'impact h.ray_x / h.ray_y en PIXELS aussi. */
t_hit	cast_ray(t_game *g, float ang)
{
	t_hit		h;
	double		dirX, dirY;
	double		posX, posY;
	int			mapX, mapY;
	double		deltaDistX, deltaDistY;
	double		sideDistX, sideDistY;
	int			stepX, stepY;
	int			side; /* 0 => mur vertical (on a franchi en X), 1 => mur horizontal (en Y) */
	double		perpWallDist;
	double		hitX, hitY;

	/* direction du rayon */
	dirX = cos(ang);
	dirY = sin(ang);

	/* position caméra en UNITÉS DE CASES (pas en pixels) */
	posX = g->player.x / (double)BLOCK;
	posY = g->player.y / (double)BLOCK;

	/* case de départ */
	mapX = (int)posX;
	mapY = (int)posY;

	/* longueurs de rayons jusqu’à la prochaine séparation de cases */
	deltaDistX = (dirX == 0.0) ? 1e30 : fabs(1.0 / dirX);
	deltaDistY = (dirY == 0.0) ? 1e30 : fabs(1.0 / dirY);

	/* initialisation des pas et sideDist */
	if (dirX < 0) { stepX = -1; sideDistX = (posX - mapX) * deltaDistX; }
	else          { stepX =  1; sideDistX = (mapX + 1.0 - posX) * deltaDistX; }
	if (dirY < 0) { stepY = -1; sideDistY = (posY - mapY) * deltaDistY; }
	else          { stepY =  1; sideDistY = (mapY + 1.0 - posY) * deltaDistY; }

	/* DDA : avance de frontière en frontière jusqu’à toucher un mur */
	for (;;)
	{
		if (sideDistX < sideDistY)
		{
			sideDistX += deltaDistX;
			mapX += stepX;
			side = 0; /* mur “vertical” */
		}
		else
		{
			sideDistY += deltaDistY;
			mapY += stepY;
			side = 1; /* mur “horizontal” */
		}
		if (map_cell_is_wall(g, mapX, mapY))
			break;
	}

	/* distance perpendiculaire (pas besoin de correction de fish-eye) */
	if (side == 0) perpWallDist = sideDistX - deltaDistX;
	else           perpWallDist = sideDistY - deltaDistY;
	if (perpWallDist < 1e-6) perpWallDist = 1e-6;

	/* point d’impact en PIXELS (utile pour l’alignement texture existant) */
	hitX = (posX + perpWallDist * dirX) * (double)BLOCK;
	hitY = (posY + perpWallDist * dirY) * (double)BLOCK;

	h.ray_x = (float)hitX;
	h.ray_y = (float)hitY;
	h.side  = side;

	/* ta pipeline attend une dist en pixels (cf. compute_strip_bounds) */
	h.dist  = (float)(perpWallDist * (double)BLOCK);

	return h;
}


int compute_tex_x(const t_tex *t, const t_hit *h, float ray_ang)
{
    float offset;

    if (h->side == 0) offset = fmodf(h->ray_y, BLOCK) / BLOCK; // mur vertical
    else              offset = fmodf(h->ray_x, BLOCK) / BLOCK; // mur horizontal

    int tx = (int)(offset * (float)t->w);

    // flip optionnel pour cohérence
    if ((h->side == 0 && cosf(ray_ang) > 0.0f) || (h->side == 1 && sinf(ray_ang) < 0.0f))
        tx = t->w - 1 - tx;
    return tx;
}

/* --------------------------- Map utils ----------------------------------- */

 bool	touch(float px, float py, t_game *g)
{
	int	cx;
	int	cy;

	cx = (int)(px / BLOCK);
	cy = (int)(py / BLOCK);
	if (cx < 0 || cy < 0)
		return (true);
	if (!g->map[cy])
		return (true);
	if (!g->map[cy][cx])
		return (true);
	return (g->map[cy][cx] == '1');
}

 void	draw_square(int x, int y, int s, int color, t_game *g)
{
	int	i;

	i = 0;
	while (i < s)
	{
		put_pixel(x + i, y, color, g);
		put_pixel(x, y + i, color, g);
		put_pixel(x + s, y + i, color, g);
		put_pixel(x + i, y + s, color, g);
		i++;
	}
}

 void	draw_map(t_game *g)
{
	int		y;
	int		x;
	char	**m;

	m = g->map;
	y = 0;
	while (m[y])
	{
		x = 0;
		while (m[y][x])
		{
			if (m[y][x] == '1')
				draw_square(x * BLOCK, y * BLOCK, BLOCK, 0x0000FF, g);
			x++;
		}
		y++;
	}
}

/* --------------------------- Maths --------------------------------------- */

 float	dist2(float x, float y)
{
	return (sqrtf(x * x + y * y));
}

/* --------------------------- Draw column --------------------------------- */

void draw_line(t_player *p, t_game *g, float ray_ang, int col)
{
    (void)p;
    t_hit h = cast_ray(g, ray_ang);

    int y0, y1, hh;
    compute_strip_bounds(h.dist, &y0, &y1, &hh);

    /* -> indices de cellule (la position d’impact est déjà dans le mur) */
    int cx = (int)(h.ray_x / BLOCK);
    int cy = (int)(h.ray_y / BLOCK);

    /* -> choisir la texture : “solo” si bloc isolé, sinon V/H selon le côté */
    const t_tex *tex;
    if (is_solo_block(g, cy, cx))
        tex = &g->wall_solo;
    else
        tex = (h.side == 0) ? &g->wall_v : &g->wall_h;

    int   tx = compute_tex_x(tex, &h, ray_ang);
    float step, tex_pos;
    compute_tex_vscroll(tex, hh, y0, &step, &tex_pos);

    while (y0 < y1)
    {
        int ty = (int)tex_pos;  tex_pos += step;
        int color = tex_read(tex, tx, ty);

        put_pixel(col, y0, color, g);
        y0++;
    }
}




/* --------------------------- Loop ---------------------------------------- */

 int	draw_loop(t_game *g)
{
	t_player	*p;
	float		step_ang;
	float		ang;
	int			col;

	p = &g->player;
	move_player(p);
	draw_background(g);
	if (DEBUG)
	{
		draw_square((int)p->x, (int)p->y, 10, 0x00FF00, g);
		draw_map(g);
	}
	step_ang = PI / 3.0f / (float)WIDTH;
	ang = p->angle - (PI / 6.0f);
	col = 0;
	while (col < WIDTH)
	{
		draw_line(p, g, ang, col);
		ang += step_ang;
		col++;
	}
	mlx_put_image_to_window(g->mlx, g->win, g->img, 0, 0);
	return (0);
}
void	draw_floor_scaled_tile(t_game *g, float scale)
{
	int x, y, tx, ty, color, tw, th, half;

	if (!g->floor_tex.data || scale <= 0.f)
		return ;
	tw = g->floor_tex.w;
	th = g->floor_tex.h;
	half = HEIGHT / 2;

	y = half;
	while (y < HEIGHT)
	{
		ty = ((int)((y - half) / scale)) % th;
		if (ty < 0) ty += th;
		x = 0;
		while (x < WIDTH)
		{
			tx = ((int)(x / scale)) % tw;
			if (tx < 0) tx += tw;
			color = tex_read(&g->floor_tex, tx, ty);
			put_pixel(x, y, color, g);
			x++;
		}
		y++;
	}
}
void	draw_floor(t_game *g)
{
	int	x;
	int	y;
	int	tx;
	int	ty;
	int	color;
	int	half;
	int	tw;
	int	th;

	if (!g->floor_tex.data)
		return ;
	tw = g->floor_tex.w;
	th = g->floor_tex.h;
	half = HEIGHT / 2;

	/* moitié basse de l'écran = sol (image tuilée, écran-fixé) */
	y = half;
	while (y < HEIGHT)
	{
		/* répète verticalement */
		ty = (y - half) % th;
		if (ty < 0)
			ty += th;
		x = 0;
		while (x < WIDTH)
		{
			/* répète horizontalement */
			tx = x % tw;
			if (tx < 0)
				tx += tw;

			color = tex_read(&g->floor_tex, tx, ty);
			put_pixel(x, y, color, g);
			x++;
		}
		y++;
	}
}


int	draw_background(t_game *g)
{
	int   x, y;
	int   half = HEIGHT / 2;

	/* SKY (garde comme tu l'as) */
	y = 0;
	while (y < half)
	{
		int ty = (int)((float)y * (float)g->sky.h / (float)half);
		x = 0;
		while (x < WIDTH)
		{
			int tx = (int)((float)x * (float)g->sky.w / (float)WIDTH);
			int color = tex_read(&g->sky, tx, ty);
			put_pixel(x, y, color, g);
			x++;
		}
		y++;
	}

	/* FLOOR (remplace l'appel actuel par celui-ci) */
	draw_floor(g);
	return (0);
}
/* --------------------------- Main ---------------------------------------- */

int	main(void)
{
	t_game	g;

	if (init_game(&g) != 0)
		return (1);
	if (load_texture(&g, &g.wall_v, "src/wall.xpm") != 0) 
		return (1);
	if (load_texture(&g, &g.wall_h, "src/wall.xpm") != 0)
		return (1);
    if (load_texture(&g, &g.sky, "src/sky.xpm") != 0)
        return (1);
    if (load_texture(&g, &g.floor_tex, "src/floor.xpm") != 0)
        return (1);
	if (load_texture(&g, &g.wall_solo,"src/wall.xpm"))
		return (1);
	mlx_hook(g.win, 2, 1L << 0, key_press, &g.player);
	mlx_hook(g.win, 3, 1L << 1, key_release, &g.player);
	mlx_loop_hook(g.mlx, draw_loop, &g);
	mlx_loop(g.mlx);
	return (0);
}

