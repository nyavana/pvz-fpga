/*
 * Plants vs Zombies — Main game loop
 *
 * Initializes the FPGA driver, keyboard input, and game state,
 * then runs a 60 Hz loop: input -> update -> render -> write to FPGA.
 *
 * Usage: ./pvz [/dev/input/eventN]
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "pvz.h"
#include "game.h"
#include "input.h"
#include "render.h"

#define FRAME_USEC 16667  /* ~60 fps */

static int pvz_fd;

/* Get current time in microseconds */
static long long get_time_usec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
}

static void process_input(game_state_t *gs)
{
    int key;

    while ((key = input_poll()) != INPUT_NONE) {
        switch (key) {
        case INPUT_UP:
            if (gs->cursor_row > 0) gs->cursor_row--;
            break;
        case INPUT_DOWN:
            if (gs->cursor_row < GRID_ROWS - 1) gs->cursor_row++;
            break;
        case INPUT_LEFT:
            if (gs->cursor_col > 0) gs->cursor_col--;
            break;
        case INPUT_RIGHT:
            if (gs->cursor_col < GRID_COLS - 1) gs->cursor_col++;
            break;
        case INPUT_SPACE:
            game_place_plant(gs);
            break;
        case INPUT_D:
            game_remove_plant(gs);
            break;
        case INPUT_TAB:
            gs->selected_plant_type =
                (gs->selected_plant_type == PLANT_PEASHOOTER)
                ? PLANT_SUNFLOWER : PLANT_PEASHOOTER;
            break;
        case INPUT_ESC:
            gs->state = -1; /* signal exit */
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    const char *input_dev = "/dev/input/event0";
    game_state_t gs;

    if (argc > 1)
        input_dev = argv[1];

    /* Seed random number generator */
    srand((unsigned)time(NULL));

    /* Open FPGA device */
    pvz_fd = open("/dev/pvz", O_RDWR);
    if (pvz_fd < 0) {
        perror("open /dev/pvz");
        fprintf(stderr, "Is the pvz_driver module loaded?\n");
        return 1;
    }

    /* Initialize keyboard input */
    if (input_init(input_dev) < 0) {
        fprintf(stderr, "Failed to open %s\n", input_dev);
        fprintf(stderr, "Usage: %s [/dev/input/eventN]\n", argv[0]);
        close(pvz_fd);
        return 1;
    }

    /* Initialize renderer */
    render_init(pvz_fd);

    /* Initialize game state */
    game_init(&gs);

    printf("Plants vs Zombies MVP\n");
    printf("Controls: Arrows=move, Tab=toggle plant, Space=place, D=remove, ESC=quit\n");
    printf("Sun: %d | Plant cost: %d\n\n", gs.sun, PLANT_COST);

    /* Main game loop */
    long long frame_start;

    while (gs.state >= 0) {
        frame_start = get_time_usec();

        /* 1. Process input */
        process_input(&gs);
        if (gs.state < 0)
            break;

        /* 2. Update game logic */
        game_update(&gs);

        /* 3. Render to FPGA */
        render_frame(&gs);

        /* Print status periodically */
        if (gs.frame_count % 60 == 0) {
            printf("\rSun: %3d | Zombies: %d/%d | Frame: %d",
                   gs.sun, gs.zombies_spawned, TOTAL_ZOMBIES,
                   gs.frame_count);
            fflush(stdout);
        }

        /* Check game over */
        if (gs.state == STATE_WIN) {
            printf("\n\n*** YOU WIN! All zombies defeated! ***\n");
            render_frame(&gs); /* Render win state */
            sleep(5);
            break;
        }
        if (gs.state == STATE_LOSE) {
            printf("\n\n*** GAME OVER! A zombie reached your house! ***\n");
            render_frame(&gs); /* Render lose state */
            sleep(5);
            break;
        }

        /* 4. Frame timing: sleep for remainder of frame */
        long long elapsed = get_time_usec() - frame_start;
        if (elapsed < FRAME_USEC)
            usleep(FRAME_USEC - elapsed);
    }

    printf("\nCleaning up...\n");
    input_close();
    close(pvz_fd);
    return 0;
}
