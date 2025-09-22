#include "../includes/game.h"


// initialisation functions
char **get_map(void)
{
    char **map = malloc(sizeof(char *) * 11);
    map[0] = "111111111111111";
    map[1] = "100000000000001";
    map[2] = "100000000000001";
    map[3] = "100000100000001";
    map[4] = "100000000000001";
    map[5] = "100000010000001";
    map[6] = "100001000000001";
    map[7] = "100000000000001";
    map[8] = "100000000000001";
    map[9] = "111111111111111";
    map[10] = NULL;
    return (map);
}

int main(void)
{
    t_game *game;

    game = malloc(sizeof(t_game));
    if (!game)
        return (1);
    
    game->mlx = mlx_init();
    game->win = mlx_new_window(game->mlx, 1800, 1000, "Kobe");
    mlx_loop(game->mlx);
    free(game);
    return (0);
}
