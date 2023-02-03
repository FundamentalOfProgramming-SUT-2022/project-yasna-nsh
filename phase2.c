#include <ncurses.h>
#include <string.h>
#include <direct.h>
#include <windows.h>
#include <sys/stat.h>

// phase 1 macros
#define TEMP_FILE_NAME ".tempfile"
#define CLIPBOARD_FILE_NAME ".clipboard"

#define RLIST_FILE_NAME ".removelist"
#define RECORD_COUNT_EXT "_UNDO_COUNT"
#define UNDO_EXT "_UNDO_NO_"

#define TAB_WIDTH 4

// phase 2 macros
#define MODE_WIN_HEIGHT 1
#define MODE_WIN_WIDTH 6
#define COMMAND_WIN_HEIGHT 1
#define LINE_WIN_WIDTH 2

#define TEXT_COLOR 0
#define MODE_COLOR 1
#define FILE_COLOR 2
#define COMMAND_COLOR 3
#define LINE_COLOR 4

#define NORMAL_MODE 0
#define INSERT_MODE 1
#define VISUAL_MODE 2

void init_env();
void init_windows();
void normal_input(char c);
void insert_input(char c);
void command_input();
void set_mode_normal();
void set_mode_insert();
void getcommand(char com[]);
void error_msg(char message[]);
int file_input(char fileaddress[]);
int dir_exist(char fileaddress[]);
void end_of_command();

// command functions
int open_f();
int open_action(char fileaddress[], int valid_input);
void close_cur_file();
void save_cur_file();

FILE *command_file;
FILE *file_backup;
char cur_file_path[1000];
int cur_file_line, cur_file_char;
int is_arman = 0;
int first_time = 1;
char arman_result[10000];
char prev_output[10000];

WINDOW *text_win;
WINDOW *line_win;
WINDOW *mode_win;
WINDOW *file_win;
WINDOW *command_win;
int mode;

int main()
{
    init_env();
    char c;
    while (1)
    {
        if (mode == NORMAL_MODE)
        {
            c = getch();
            normal_input(c);
        }
        else if (mode == INSERT_MODE)
        {
            c = getch();
            insert_input(c);
        }
    }
    endwin();
}

void init_env()
{
    initscr();
    cbreak();
    keypad(stdscr, true);
    scrollok(text_win, true);
    start_color();
    init_pair(TEXT_COLOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(MODE_COLOR, COLOR_WHITE, COLOR_GREEN);
    init_pair(FILE_COLOR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COMMAND_COLOR, COLOR_BLUE, COLOR_BLACK);
    init_pair(LINE_COLOR, COLOR_CYAN, COLOR_BLACK);
    refresh();
    init_windows();

    file_backup = fopen("untitled_temp", "w");
    strcpy(cur_file_path, "untitled");
    cur_file_line = cur_file_char = 0;
    wprintw(mode_win, "NORMAL");
    wprintw(file_win, "untitled");
    wprintw(file_win, "  +");
    wprintw(line_win, "1 ");
    wrefresh(mode_win);
    wrefresh(file_win);
    wrefresh(line_win);
}

void init_windows()
{
    text_win = newwin(LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT, COLS - LINE_WIN_WIDTH, 0, LINE_WIN_WIDTH);
    mode_win = newwin(MODE_WIN_HEIGHT, MODE_WIN_WIDTH, LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT, 0);
    file_win = newwin(MODE_WIN_HEIGHT, COLS - MODE_WIN_WIDTH, LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT, MODE_WIN_WIDTH + 1);
    command_win = newwin(COMMAND_WIN_HEIGHT, COLS, LINES - MODE_WIN_HEIGHT, 0);
    line_win = newwin(LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT, LINE_WIN_WIDTH, 0, 0);
    wbkgd(text_win, COLOR_PAIR(TEXT_COLOR));
    wbkgd(mode_win, COLOR_PAIR(MODE_COLOR));
    wbkgd(file_win, COLOR_PAIR(FILE_COLOR));
    wbkgd(command_win, COLOR_PAIR(COMMAND_COLOR));
    wbkgd(line_win, COLOR_PAIR(LINE_COLOR));

    wmove(text_win, 0, 0);
    wrefresh(text_win);
    wrefresh(mode_win);
    wrefresh(file_win);
    wrefresh(command_win);
    wrefresh(line_win);

    noecho();
    set_mode_normal();
}

void normal_input(char c)
{
    if (c == 'i')
    {
        set_mode_insert();
        return;
    }
    else if (c == ':')
    {
        command_input();
        return;
    }

    // movement
    // h: left
    // l: right
    // j:down
    // k:up
    if (c == 'h')
    {
        wmove(text_win, getcury(text_win), getcurx(text_win) - 1);
        wrefresh(text_win);
        return;
    }
    else if (c == 'l')
    {
        wmove(text_win, getcury(text_win), getcurx(text_win) + 1);
        wrefresh(text_win);
        return;
    }
    else if (c == 'j')
    {
        wmove(text_win, getcury(text_win) + 1, getcurx(text_win));
        while (mvwinch(text_win, getcury(text_win), getcurx(text_win)) == ' ')
        {
            wmove(text_win, getcury(text_win), getcurx(text_win) - 1);
        }
        wrefresh(text_win);
        return;
    }
    else if (c == 'k')
    {
        wmove(text_win, getcury(text_win) - 1, getcurx(text_win));
        while (mvwinch(text_win, getcury(text_win), getcurx(text_win)) == ' ')
        {
            wmove(text_win, getcury(text_win), getcurx(text_win) - 1);
        }
        wrefresh(text_win);
        return;
    }
}

void insert_input(char c)
{
    // special chars
    if (c == 27) /*ESC*/
    {
        set_mode_normal();
        return;
    }
    if (c == KEY_BACKSPACE)
    {
        // REMOVE CHAR
    }
    wprintw(text_win, "%c", c);
    wrefresh(text_win);
}

void command_input()
{
    wmove(command_win, 0, 0);
    wprintw(command_win, ":");
    wrefresh(command_win);
    echo();
    char command[100];
    getcommand(command);
    if (!strcmp(command, "open"))
    {
        // path should start with /root
        open_f();
    }
    // else if (!strcmp(command, "createfile"))
    // {
    //     createfile();
    // }
    // else if (!strcmp(command, "insertstr"))
    // {
    //     insertstr();
    // }
    // else if (!strcmp(command, "cat"))
    // {
    //     cat();
    // }
    // else if (!strcmp(command, "removestr"))
    // {
    //     removestr();
    // }
    // else if (!strcmp(command, "copystr"))
    // {
    //     copystr();
    // }
    // else if (!strcmp(command, "cutstr"))
    // {
    //     cutstr();
    // }
    // else if (!strcmp(command, "pastestr"))
    // {
    //     pastestr();
    // }
    // else if (!strcmp(command, "find"))
    // {
    //     find();
    // }
    // else if (!strcmp(command, "replace"))
    // {
    //     replace();
    // }
    // else if (!strcmp(command, "tree"))
    // {
    //     tree();
    // }
    // else if (!strcmp(command, "grep"))
    // {
    //     grep();
    // }
    // else if (!strcmp(command, "undo"))
    // {
    //     undo();
    // }
    // else if (!strcmp(command, "compare"))
    // {
    //     compare();
    // }
    // else if (!strcmp(command, "auto-indent"))
    // {
    //     auto_indent();
    // }
    // else if (!strcmp(command, "arman"))
    // {
    //     arman();
    // }
    // else if (!strcmp(command, "exit"))
    // {
    //     cleanup();
    // }
    end_of_command();
    wclear(command_win);
    wrefresh(command_win);
    wrefresh(text_win);
    // cursor position?
    noecho();
}

void set_mode_normal()
{
    mode = NORMAL_MODE;
    wclear(mode_win);
    wprintw(mode_win, "NORMAL");
    wrefresh(mode_win);
}

void set_mode_insert()
{
    mode = INSERT_MODE;
    wclear(mode_win);
    wprintw(mode_win, "INSERT");
    wrefresh(mode_win);
    // PUT CURSOR AT THE END OF THE TEXT?
    wmove(text_win, 0, 0);
    wrefresh(text_win);
}

void getcommand(char com[])
{
    command_file = fopen(".commandfile", "w");
    char full_line[6000];
    wgetstr(command_win, full_line);
    fprintf(command_file, full_line);
    fprintf(command_file, "\n");
    fclose(command_file);

    command_file = fopen(".commandfile", "r");
    if (strstr(full_line, "=D") != NULL)
        strcpy(com, "arman");
    else
        fscanf(command_file, "%s", com);
}

void error_msg(char message[])
{
    wclear(command_win);
    wprintw(command_win, message);
    wrefresh(text_win);
    napms(2000);
}

int file_input(char fileaddress[])
{
    char word[1000];

    fscanf(command_file, "%s", word);

    if (word[0] != '\"')
    {
        strcpy(fileaddress, word);
    }
    else
    {
        for (int i = 1; i < strlen(word); i++)
        {
            fileaddress[i - 1] = word[i];
        }

        int i = strlen(word) - 1;
        char c;
        while (1)
        {
            c = fgetc(command_file);
            if (c == '\"' && fileaddress[i - 1] != '\\')
                break;
            fileaddress[i] = c;
            i++;
        }
        fileaddress[i] = 0;
    }

    // remove the starting \ or /
    if (fileaddress[0] == '\\' || fileaddress[0] == '/')
    {
        for (int i = 1; i < strlen(fileaddress); i++)
        {
            fileaddress[i - 1] = fileaddress[i];
        }
        fileaddress[strlen(fileaddress) - 1] = 0;
    }

    // does directory exist
    if (!dir_exist(fileaddress))
    {
        error_msg("directory doesn't exist");
        return -1;
    }

    // does file exist
    FILE *f = fopen(fileaddress, "r");
    if (f == NULL)
    {
        return -2;
    }
    else
    {
        fclose(f);
    }

    return 0;
}

void cleanup()
{
    remove(CLIPBOARD_FILE_NAME);

    FILE *f = fopen(RLIST_FILE_NAME, "r");
    char address[1000];
    while (fgets(address, 1000, f) != NULL)
    {
        // reached end of the list
        if (address[0] == '\n')
            break;

        // remove the \n from the end
        address[strlen(address) - 1] = 0;
        remove(address);
    }
    fclose(f);
    remove(RLIST_FILE_NAME);
    fclose(command_file);
    remove(".commandfile");
}

int dir_exist(char fileaddress[])
{
    int i;
    for (i = strlen(fileaddress) - 1; i >= 0; i--)
    {
        if (fileaddress[i] == '/' || fileaddress[i] == '\\')
            break;
    }
    if (i < 0)
    {
        // no directory
        return 1;
    }

    fileaddress[i] = 0;
    struct stat s;
    stat(fileaddress, &s);
    fileaddress[i] = '/';

    if (S_ISDIR(s.st_mode))
        return 1;
    return 0;
}

void end_of_command()
{
    fclose(command_file);
}

// command functions
int open_f()
{
    char fileaddress[1000];
    int valid_input = file_input(fileaddress);
    if (valid_input == -1) /*invalid file address*/
        return -1;
    open_action(fileaddress, valid_input);
}

int open_action(char fileaddress[], int valid_input)
{
    close_cur_file();
    strcpy(cur_file_path, fileaddress);
    FILE *f;
    char fb_name[1000];
    strcpy(fb_name, fileaddress);
    strcat(fb_name, "_temp");
    file_backup = fopen(fb_name, "w");
    cur_file_char = cur_file_line = 0;
    wclear(text_win);
    if (valid_input == 0) /*file exists*/
    {
        f = fopen(fileaddress, "r");
        char line[1000];
        // FIX WRAP PROBLEM
        while (fgets(line, 1000, f) != NULL)
        {
            wprintw(text_win, line, -1);
            fprintf(file_backup, line);
        }
        wrefresh(text_win);
    }
    wmove(text_win, 0, 0);

    wclear(file_win);
    wprintw(file_win, fileaddress);
    wprintw(file_win, "  +");
    wrefresh(file_win);
}

void close_cur_file()
{
    wclear(command_win);
    wprintw(command_win, "do you want to save the current file? (y/n) ");
    wrefresh(text_win);
    char c;
    c = wgetch(command_win);
    if (c == 'y')
    {
        save_cur_file();
    }
    fclose(file_backup);
    char fb_name[1000];
    strcpy(fb_name, cur_file_path);
    strcat(fb_name, "_temp");
    remove(fb_name);
}

void save_cur_file()
{
    fclose(file_backup);
    char fb_name[1000];
    strcpy(fb_name, cur_file_path);
    strcat(fb_name, "_temp");
    file_backup = fopen(fb_name, "r");
    FILE *org = fopen(cur_file_path, "w");
    char line[1000];
    while (fgets(line, 1000, file_backup) != NULL)
    {
        fprintf(org, line);
    }
    fclose(org);
    fclose(file_backup);
}