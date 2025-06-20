#include <errno.h>
#include <locale.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#include "../include/lib_llist.h"

volatile sig_atomic_t gb_SIGINT_BOOL; // Boolean of whether CTRL+C (SIGINT) has been thrown
volatile sig_atomic_t g_WINSIZE_x = 1; // Horizontal size of the Terminal Window
volatile sig_atomic_t g_WINSIZE_y = 1; // Vertical size of the Terminal Window

#define HORIZ    0x2501 // '━'; // 0x2500 // '─'
#define VERTI    0x2503 // '┃'; // 0x2502 // '│'
#define TOPLEFT  0x250f // '┏'; // 0x256D // '╭'
#define TOPRIGHT 0x2513 // '┓'; // 0x256e // '╮'
#define BOTLEFT  0x2517 // '┗'; // 0x2570 // '╰'
#define BOTRIGHT 0x251b // '┛'; // 0x256f // '╯'
#define PLUS     0x254b // '╋'; // 0x253C // '┼'
// {'-': '━', '|': '┃', 'F': '┏', '7': '┓', 'L': '┗', 'J': '┛', '.': ':', 'S': 'S', '+': '╋'}

#define MILLIS_PER_SEC 1000000
#define MAX_COLOR_STEPS (128 + 128 + 256 + 256 + 256)

/**
 * @brief vertex_t - struct for containing vertex info
 * 
 * @param c     (wchar_t) vertex character
 * @param x     (int32_t) vertex x coordinate
 * @param y     (int32_t) vertex y coordinate
 * @param dir_x (int32_t) x direction of NEXT char
 * @param dir_y (int32_t) y direction of NEXT char
 */
typedef struct vertex_t
{
    wchar_t c; // vertex character
    int32_t x; // vertex x coordinate
    int32_t y; // vertex y coordinate
    int32_t dir_x; // x direction of NEXT char
    int32_t dir_y; // y direction of NEXT char
} vertex_t;

static void    sigint_h(int32_t sig);
static void    sigwinch_h(int sig);
static void    print_help(void);
static void    draw_border(void);
static int32_t print_char_c(vertex_t *vert, int32_t idx);
static int32_t print_char_w(vertex_t *vert);
static int32_t check_bounds(vertex_t *vert);
static void    debug_path_len(llist_t *path);

int
main (int argc, char **argv)
{
    srand(time(NULL));

    int32_t          end_ret = -1;
    struct sigaction saint   = { .sa_handler = sigint_h };
    struct sigaction sawinch = { .sa_handler = sigwinch_h };

    if (sigaction(SIGINT, &saint, NULL) == -1)
    {
        perror("sigint sigaction");
        errno = 0;
        goto END_RET;
    }

    if (sigaction(SIGWINCH, &sawinch, NULL) == -1)
    {
        perror("sigwinch sigaction");
        goto END_RET;
    }

    setlocale(LC_ALL, "en_US.UTF-8");
    fwide(stdout, 1); // set stdout to widechar mode

    bool b_colormode = false;

    int opt = 0;
    while ((opt = getopt(argc, argv, "ch")) != -1)
    {
        switch (opt)
        {
            case 'c':
                b_colormode = true;
                break;

            case 'h':
                print_help();
                goto END_RET;

            case '?':
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                goto END_RET;
            
            default:
                print_help();
                goto END_RET;
        }
    }

    llist_t *path = ll_create();

    gb_SIGINT_BOOL = 1;
    // hide cursor
    wprintf(L"\033[?25l");

    wchar_t *choices = calloc(4, sizeof(*choices));
    int32_t idx = rand() % UINT16_MAX;

    while (gb_SIGINT_BOOL)
    {
        vertex_t *prev  = NULL;
        vertex_t *curr  = NULL;
        vertex_t *start = NULL;

        sigwinch_h(0); // reuse sighandler to do inital window setup

        // start at direct middle with a '-'
        start        = calloc(1, sizeof(*start));
        start->c     = HORIZ;
        start->x     = g_WINSIZE_x / 2;
        start->y     = g_WINSIZE_y / 2;
        start->dir_y = 0;
        start->dir_x = (((rand() % 20) < 10) ? -1 : 1); // flip a coin for right or left
        ll_enq(path, start);

        if (b_colormode)
        {
            print_char_c(start, idx);
        }
        else
        {
            print_char_w(start);
        }
        prev = ll_tail(path);

        time_t t_start = { 0 };
        time_t t_end = { 0 };

        // for (int i = 0; i < UINT8_MAX*4; ++i) { // at 15fps, lasts ~60s
        // at 30fps, lasts ~1966s or ~32.77m  || 15fps, lasts ~3921s or 65.5m
        for (; idx < UINT16_MAX * 2; idx += 2)
        {
            if (!gb_SIGINT_BOOL)
            {
                break;
            }

            time(&t_start);
            curr = calloc(1, sizeof(*curr));

            curr->x = (prev->x + prev->dir_x);
            curr->y = (prev->y + prev->dir_y);

#ifdef DEBUG
            debug_path_len(path);
#endif

            // roll to pick the next direction
            // && update curr direction
            if (prev->c == HORIZ)
            {
                if (prev->dir_y == 0 && prev->dir_x == -1)
                {
                    choices[0] = HORIZ;
                    choices[1] = HORIZ;
                    choices[2] = TOPLEFT;
                    choices[3] = BOTLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == HORIZ)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == TOPLEFT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                    else if (curr->c == BOTLEFT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                }
                else if (prev->dir_y == 0 && prev->dir_x == 1)
                {
                    choices[0] = HORIZ;
                    choices[1] = HORIZ;
                    choices[2] = BOTRIGHT;
                    choices[3] = TOPRIGHT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == HORIZ)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == BOTRIGHT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                    else if (curr->c == TOPRIGHT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                }
            }
            else if (prev->c == VERTI)
            {
                if (prev->dir_y == -1 && prev->dir_x == 0)
                {
                    choices[0] = VERTI;
                    choices[1] = VERTI;
                    choices[2] = TOPRIGHT;
                    choices[3] = TOPLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == VERTI)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                    else if (curr->c == TOPRIGHT)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == TOPLEFT)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                }
                else if (prev->dir_y == 1 && prev->dir_x == 0)
                {
                    choices[0] = VERTI;
                    choices[1] = VERTI;
                    choices[2] = BOTRIGHT;
                    choices[3] = BOTLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == VERTI)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                    else if (curr->c == BOTRIGHT)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == BOTLEFT)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                }
            }
            else if (prev->c == TOPLEFT)
            {
                if (prev->dir_y == 0 && prev->dir_x == 1)
                {
                    choices[0] = HORIZ;
                    choices[1] = HORIZ;
                    choices[2] = BOTRIGHT;
                    choices[3] = TOPRIGHT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == HORIZ)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == BOTRIGHT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                    else if (curr->c == TOPRIGHT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                }
                else if (prev->dir_y == 1 && prev->dir_x == 0)
                {
                    choices[0] = VERTI;
                    choices[1] = VERTI;
                    choices[2] = BOTRIGHT;
                    choices[3] = BOTLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == VERTI)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                    else if (curr->c == BOTRIGHT)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == BOTLEFT)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                }
            }
            else if (prev->c == TOPRIGHT)
            {
                if (prev->dir_y == 0 && prev->dir_x == -1)
                {
                    choices[0] = HORIZ;
                    choices[1] = HORIZ;
                    choices[2] = TOPLEFT;
                    choices[3] = BOTLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == HORIZ)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == TOPLEFT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                    else if (curr->c == BOTLEFT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                }
                else if (prev->dir_y == 1 && prev->dir_x == 0)
                {
                    choices[0] = VERTI;
                    choices[1] = VERTI;
                    choices[2] = BOTRIGHT;
                    choices[3] = BOTLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == VERTI)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                    else if (curr->c == BOTRIGHT)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == BOTLEFT)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                }
            }
            else if (prev->c == BOTLEFT)
            {
                if (prev->dir_y == 0 && prev->dir_x == 1)
                {
                    choices[0] = HORIZ;
                    choices[1] = HORIZ;
                    choices[2] = BOTRIGHT;
                    choices[3] = TOPRIGHT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == HORIZ)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == BOTRIGHT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                    else if (curr->c == TOPRIGHT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                }
                else if (prev->dir_y == -1 && prev->dir_x == 0)
                {
                    choices[0] = VERTI;
                    choices[1] = VERTI;
                    choices[2] = TOPRIGHT;
                    choices[3] = TOPLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == VERTI)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                    else if (curr->c == TOPRIGHT)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == TOPLEFT)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                }
            }
            else if (prev->c == BOTRIGHT)
            {
                if (prev->dir_y == 0 && prev->dir_x == -1)
                {
                    choices[0] = HORIZ;
                    choices[1] = HORIZ;
                    choices[2] = TOPLEFT;
                    choices[3] = BOTLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == HORIZ)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == TOPLEFT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = 1;
                    }
                    else if (curr->c == BOTLEFT)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                }
                else if (prev->dir_y == -1 && prev->dir_x == 0)
                {
                    choices[0] = VERTI;
                    choices[1] = VERTI;
                    choices[2] = TOPRIGHT;
                    choices[3] = TOPLEFT;
                    curr->c    = choices[rand() % 4];
                    if (curr->c == VERTI)
                    {
                        curr->dir_x = 0;
                        curr->dir_y = -1;
                    }
                    else if (curr->c == TOPRIGHT)
                    {
                        curr->dir_x = -1;
                        curr->dir_y = 0;
                    }
                    else if (curr->c == TOPLEFT)
                    {
                        curr->dir_x = 1;
                        curr->dir_y = 0;
                    }
                }
            }

            ll_enq(path, curr);

            // print the char
            if (b_colormode)
            {
                print_char_c(curr, idx);
            }
            else
            {
                print_char_w(curr);
            }

            // check if next breaks map bounds
            if (0 != check_bounds(curr))
            {
                break;
            }
            prev = curr;
            time(&t_end);

            // usleep(30000 - (t_end - t_start)); // sleep(0.03) / 30fps
            usleep(60000 - (t_end - t_start)); // sleep(0.06) / 15fps
            // usleep(90000 - (t_end - t_start)); // sleep(0.09) / ~7fps
        }

        // only sleep and startover when not Ctrl+C/SIGINT
        if (gb_SIGINT_BOOL)
        {
            usleep(5 * MILLIS_PER_SEC); // 5 Seconds
            ll_destroy(&path, free);
            path = ll_create();
        }
    }

    // clear screen
    wprintf(L"\033[2J\033[;H");
    // show cursor
    wprintf(L"\033[?25h");
    fflush(stdout);

    ll_destroy(&path, free);
    free(choices);

    end_ret = 0;

END_RET:
    return end_ret;
}


// =============================================================================
//                              STATIC FUNCTIONS
// =============================================================================

/**
 * @brief SIGINT Handler function. Sets global bool to false/off.
 * 
 * @param   sig     (int32_t)   SIGNAL Caught
 * 
 * @returns N/A     (void)
 */
static void
sigint_h (int32_t sig)
{
    (void)sig;
    gb_SIGINT_BOOL = 0;
}

/**
 * @brief SIGWINCH Handler function. Capture new window sizes, clear the screen,
 *  and redraw the border.
 * 
 * @param   sig     (int32_t)   SIGNAL Caught
 * 
 * @returns N/A     (void)
 */
static void
sigwinch_h (int sig)
{
    (void)sig;
    struct winsize ws;

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0
        && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0
        && ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) != 0)
    {
        perror("sigwinch ioctl() sig:");
    }

    g_WINSIZE_x = ws.ws_col;
    g_WINSIZE_y = ws.ws_row;
    // clear screen
    wprintf(L"\033[2J\033[;H");
    draw_border();
}

/**
 * @brief Print the Help Menu.
 * 
 * @returns N/A     (void)
 */
static void
print_help (void)
{
    wprintf(L"Usage: ./pipes\n");
    wprintf(L"Display some pipes just like ye olden Windows Screensavers!\n");
    wprintf(L"\n OPTIONS:\n");
    wprintf(L"\t-c\n\t\tUse RGB-256 color mode\n");
    wprintf(L"\t-h\n\t\tPrint this Help Menu and Exit\n");
    wprintf(L"\n");
}

/** 
 * @brief Draw the border around the Terminal Window.
 * 
 * @returns N/A     (void)
 */
static void
draw_border (void)
{
    int i = 0;
    int x = 0;

    wprintf(L"\033[1m"); // bold

    // top
    wprintf(L"%lc", TOPLEFT);
    for (i = 1; i < g_WINSIZE_x - 1; ++i)
    {
        wprintf(L"%lc", HORIZ);
    }
    wprintf(L"%lc", TOPRIGHT);

    // mid
    for (i = 1; i < g_WINSIZE_y - 1; ++i) // for row
    {
        for (x = 0; x < g_WINSIZE_x; ++x) // for column
        {
            if ((x == 0) || (x == g_WINSIZE_x - 1))
            {
                wprintf(L"%lc", VERTI);
            }
            else
            {
                putwchar(' ');
            }
        }
        putwchar('\n');
    }

    // bottom
    wprintf(L"%lc", BOTLEFT);
    for (i = 1; i < g_WINSIZE_x - 1; ++i)
    {
        wprintf(L"%lc", HORIZ);
    }
    wprintf(L"%lc", BOTRIGHT);
    fflush(stdout);

    wprintf(L"\033[0m"); // reset
}

/**
 * @brief Print the associated character in 256 - RGB Color mode.
 *
 * @param   vert        (vertex_t *) Vertex PTR of the associated vertex to
 * print.
 * @param   idx         (int32_t)    Index INT for correct iterative stepping
 * through RGB Values. 
 * 
 * @note    Only 1024 possible RGB values; any given index will be 
 * modulo'd down to with the acceptable range of values. Have the calling 
 * function iterate/loop through values sequentually for the proper 
 * RGB-rainbow effect.
 * 
 * @returns retval      (int32_t)    0 if Success; -1 if Failed.
 */
static int32_t
print_char_c (vertex_t *vert, int32_t idx)
{
    if (NULL == vert)
    {
        return -1;
    }

    // move cursor to vertex row and col
    wprintf(L"\033[%d;%dH", vert->y, vert->x);

    int32_t red = 0;
    int32_t grn = 0;
    int32_t blu = 0;

    /* RGB Values
    red          255,   0,   0   #FF0000
    orange       255, 127,   0   #FF7F00
    yellow       255, 255,   0   #FFFF00
    green          0, 255,   0   #00FF00
    blue           0,   0, 255   #0000FF
    indigo        75,   0, 130   #4B0082
    violet       148,   0, 211   #9400D3
    */

    if ((idx % MAX_COLOR_STEPS) < 256) // rd > ye
    {
        red = 255;
        grn = 0 + (idx % 256);
        blu = 0;
    }
    else if ((idx % MAX_COLOR_STEPS) < 512) // ye > gr
    {
        red = 255 - (idx % 256);
        grn = 255;
        blu = 0;
    }
    else if ((idx % MAX_COLOR_STEPS) < 768) // gr > bl
    {
        red = 0;
        grn = 255 - (idx % 256);
        blu = 0 + (idx % 256);
    }
    else if ((idx % MAX_COLOR_STEPS) < 1025) // bl > rd
    {
        red = 0 + (idx % 256);
        grn = 0;
        blu = 255 - (idx % 256);
    }
    else
    {
        red = 255;
        blu = 255;
        grn = 255;
    }

    wprintf(L"\033[1m"); // bold
    wprintf(L"\033[38;2;%d;%d;%dm", red, grn, blu);
    wprintf(L"%lc", vert->c);

    wprintf(L"\033[1D"); // move 1 left
    wprintf(L"\033[0m"); // reset
    fflush(stdout);

    return 0;
}

/**
 * @brief Print the associated character in non-color mode.
 *
 * @param   vert        (vertex_t *) Vertex PTR of the associated vertex to
 * print.
 * 
 * @returns retval      (int32_t)    0 if Success; -1 if Failed.
 */
static int32_t
print_char_w (vertex_t *vert)
{
    if (NULL == vert)
    {
        return -1;
    }

    // move cursor to vertex row and col
    wprintf(L"\033[%d;%dH", vert->y, vert->x);

    int32_t red = 255;
    int32_t blu = 255;
    int32_t grn = 255;

    wprintf(L"\033[1m"); // bold
    wprintf(L"\033[38;2;%d;%d;%dm", red, grn, blu);
    wprintf(L"%lc", vert->c);

    wprintf(L"\033[1D"); // move 1 left
    wprintf(L"\033[0m"); // reset
    fflush(stdout);

    return 0;
}

/**
 * @brief Checkes whether the associated character vertex is within the bounds of
 * the Terminal Window.
 * 
 * @param   vert        (vertex_t *) Vertex PTR of the associated vertex to
 * check.
 * 
 * @returns retval      (int32_t)    0 if In-Bounds (Success); -1 if Out-of-bounds (Failed).
 */
static int32_t
check_bounds (vertex_t *vert)
{
    int32_t ret_val = -1;
    if (NULL == vert)
    {
        goto CHECK_BNDS_RET;
    }

    if ((vert->x + vert->dir_x) <= 1 || (vert->x + vert->dir_x) >= g_WINSIZE_x)
    {
        goto CHECK_BNDS_RET;
    }

    if ((vert->y + vert->dir_y) <= 1
        || (vert->y + vert->dir_y) >= g_WINSIZE_y - 1)
    {
        goto CHECK_BNDS_RET;
    }

    ret_val = 0;

CHECK_BNDS_RET:
    return ret_val;
}

/**
 * Print the current length of the pipe in a Debug string in the top left corner.
 * 
 * @param   path        (llist_t *)  LinkedList PTR of the associated pipe.
 * 
 * @retuns  N/A         (void)
 */
static void
debug_path_len (llist_t *path)
{
    if (NULL == path) 
    {
        return;
    }

    wprintf(L"\033[2;0H");
    wprintf(L"%lc %5d", VERTI, ll_len(path));

    fflush(stdout);
}