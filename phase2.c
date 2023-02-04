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
#define STR_MAX_LENGTH 4000

// phase 2 macros
#define MODE_WIN_HEIGHT 1
#define MODE_WIN_WIDTH 6
#define COMMAND_WIN_HEIGHT 1
#define LINE_WIN_WIDTH 3

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
void create_remove_file();
void add_to_rlist(char fileaddress[]);
int track_changes(char fileaddress[]);
void end_of_command();

// phase 2 command functions
int open_f();
int open_action(char fileaddress[], int valid_input);
void close_cur_file();
void save_cur_file();

// phase 1 functions
int insertstr_action(char fileaddress[], char str[], int pos_line, int pos_char);
int removestr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction);
int removestr_f(char fileaddress[], int pos_line, int pos_char, int size);
int removestr_b(char fileaddress[], int pos_line, int pos_char, int size);
int create_file_input(char fileaddress[]);
int makepath(char fileaddress[]);
int createfile_action(char fileaddress[]);

FILE *command_file;
char cur_file_path[1000];
int cur_file_line, cur_file_char;
// files with more than 6000 lines?
int char_in_line[6000] = {0};
int line_count;
int saved;

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

void create_remove_file()
{
    // create and clear remove_list
    FILE *f = fopen(RLIST_FILE_NAME, "w");
    SetFileAttributesA(RLIST_FILE_NAME, FILE_ATTRIBUTE_HIDDEN);
    fclose(f);
}

void add_to_rlist(char fileaddress[])
{
    FILE *f = fopen(RLIST_FILE_NAME, "a");
    fprintf(f, "%s\n", fileaddress);
    fclose(f);
}

int track_changes(char fileaddress[])
{
    char record_count_address[1000];
    strcpy(record_count_address, fileaddress);
    strcat(record_count_address, RECORD_COUNT_EXT);

    // make file if it doesn't exist
    FILE *record_count_file = fopen(record_count_address, "r");

    int count;
    if (record_count_file == NULL)
    {
        record_count_file = fopen(record_count_address, "w");
        fprintf(record_count_file, "1");
        count = 1;
        SetFileAttributesA(record_count_address, FILE_ATTRIBUTE_HIDDEN);
        fclose(record_count_file);
        add_to_rlist(record_count_address);
    }
    else
    {
        fscanf(record_count_file, "%d", &count);
        count++;
        fclose(record_count_file);
        SetFileAttributesA(record_count_address, FILE_ATTRIBUTE_NORMAL);
        record_count_file = fopen(record_count_address, "w");
        fprintf(record_count_file, "%d", count);
        SetFileAttributesA(record_count_address, FILE_ATTRIBUTE_HIDDEN);
        fclose(record_count_file);
    }

    // DOESN'T SUPPORT MORE THAN 9999 UNDOS
    char num_str[5];
    sprintf(num_str, "%d", count);

    char new_record_address[1000];
    strcpy(new_record_address, fileaddress);
    strcat(new_record_address, UNDO_EXT);
    strcat(new_record_address, num_str);
    FILE *new_record_file = fopen(new_record_address, "w");

    FILE *original_file = fopen(fileaddress, "r");

    char line[2000];
    while (fgets(line, 2000, original_file) != NULL)
    {
        fprintf(new_record_file, line);
    }

    fclose(original_file);
    SetFileAttributesA(new_record_address, FILE_ATTRIBUTE_HIDDEN);
    fclose(new_record_file);
    add_to_rlist(new_record_address);

    return 0;
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

    FILE *file_backup = fopen("untitled_temp", "w");
    fclose(file_backup);
    strcpy(cur_file_path, "untitled");
    cur_file_line = cur_file_char = 0;
    line_count = 0;
    char_in_line[0] = 0;
    saved = 0;
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
    else if (c == KEY_UP)
    {
    }
    else if (c == KEY_DOWN)
    {
    }

    // movement
    // h: left
    // l: right
    // j:down
    // k:up
    if (c == 'h')
    {
        if (getcurx(text_win) - 1 < 0)
            return;
        wmove(text_win, getcury(text_win), getcurx(text_win) - 1);
        cur_file_line = getcury(text_win);
        cur_file_char = getcurx(text_win);
        wrefresh(text_win);
        return;
    }
    else if (c == 'l')
    {
        if (getcurx(text_win) > char_in_line[cur_file_line] - 1 || (cur_file_line != line_count - 1 && getcurx(text_win) == char_in_line[cur_file_line] - 1))
            return;
        wmove(text_win, getcury(text_win), getcurx(text_win) + 1);
        cur_file_line = getcury(text_win);
        cur_file_char = getcurx(text_win);
        wrefresh(text_win);
        return;
    }
    else if (c == 'j')
    {
        if (getcury(text_win) + 1 >= line_count)
            return;
        cur_file_line++;
        wmove(text_win, getcury(text_win) + 1, getcurx(text_win));
        if (getcurx(text_win) > char_in_line[cur_file_line] - 1 || (cur_file_line != line_count - 1 && getcurx(text_win) == char_in_line[cur_file_line] - 1))
        {
            wmove(text_win, getcury(text_win), char_in_line[cur_file_line] - 1);
            cur_file_line = getcury(text_win);
            cur_file_char = char_in_line[cur_file_line] - 1;
        }
        wrefresh(text_win);
        return;
    }
    else if (c == 'k')
    {
        if (getcury(text_win) - 1 < 0)
            return;
        cur_file_line--;
        wmove(text_win, getcury(text_win) - 1, getcurx(text_win));
        if (getcurx(text_win) > char_in_line[cur_file_line] - 1 || (cur_file_line != line_count - 1 && getcurx(text_win) == char_in_line[cur_file_line] - 1))
        {
            wmove(text_win, getcury(text_win), char_in_line[cur_file_line] - 1);
            cur_file_line = getcury(text_win);
            cur_file_char = char_in_line[cur_file_line] - 1;
        }
        wrefresh(text_win);
        return;
    }
}

void insert_input(char c)
{
    char fb_name[1000];
    // special chars
    if (c == 27) /*ESC*/
    {
        set_mode_normal();
        return;
    }
    else if (c == 8) /*BACKSPACE*/
    {
        strcpy(fb_name, cur_file_path);
        strcat(fb_name, "_temp");
        if (cur_file_line != 0 && cur_file_char == 0)
        {
            removestr_action(fb_name, cur_file_line, cur_file_char, 1, 'b');
        }
        else if (cur_file_char + cur_file_line != 0)
        {
            removestr_action(fb_name, cur_file_line, cur_file_char, 1, 'b');
        }
    }
    else if (c == KEY_EXIT)
    {
        // ?
        return;
    }
    else if (c == KEY_UP || c == KEY_DOWN || c == KEY_LEFT || c == KEY_RIGHT)
    {
        // does it automatically change pos?
        return;
    }
    else /*normal keys*/
    {
        // add text to temp file
        char temp[2];
        temp[0] = c;
        temp[1] = 0;
        strcpy(fb_name, cur_file_path);
        strcat(fb_name, "_temp");
        if (cur_file_line != 0 && char_in_line[cur_file_line] == 0)
        {
            insertstr_action(fb_name, temp, cur_file_line - 1, char_in_line[cur_file_line - 1]);
        }
        else
            insertstr_action(fb_name, temp, cur_file_line, cur_file_char);
    }

    // show file's save state
    if (saved)
    {
        wprintw(file_win, "  +");
        saved = 0;
        wrefresh(file_win);
    }

    // print edited text again to prevent overwrite and update line and char count
    wclear(text_win);
    FILE *f = fopen(fb_name, "r");
    char line[1000];
    int i = 0;
    char_in_line[0] = 0;
    wclear(line_win);
    wprintw(line_win, "1");
    while (fgets(line, 1000, f) != NULL)
    {
        // stop printing text after reaching border
        if (i < LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT)
        {
            wprintw(text_win, line);
        }
        char_in_line[i] += strlen(line);
        // IS THIS IF CORRECT?
        if (line[strlen(line) - 1] == '\n' || feof(f))
        {
            i++;
            if (line[strlen(line) - 1] == '\n')
                wprintw(line_win, "\n%d", i + 1);
            char_in_line[i] = 0;
        }
    }
    fclose(f);
    line_count = i;
    wrefresh(line_win);
    if (c == 8)
    {
        if (cur_file_line != 0 && cur_file_char == 0)
        {
            cur_file_line--;
            cur_file_char = char_in_line[cur_file_line];
        }
        else if (cur_file_char + cur_file_line != 0)
        {
            cur_file_char--;
        }
    }
    else
    {
        if (c == '\n')
        {
            cur_file_char = 0;
            cur_file_line++;
        }
        else
        {
            cur_file_char++;
        }
    }

    wmove(text_win, cur_file_line, cur_file_char);
    // cur_file_line = getcury(text_win);
    // cur_file_char = getcurx(text_win);
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
    wclear(command_win);
    wrefresh(command_win);
    if (!strcmp(command, "open"))
    {
        // path should start with /root
        open_f();
    }
    else if (!strcmp(command, "save"))
    {
        if (!strcmp(cur_file_path, "untitled"))
        {
            wclear(command_win);
            wprintw(command_win, "enter file's name and address: ");
            wrefresh(command_win);
            char filepath[1000];
            create_file_input(filepath);
            createfile_action(filepath);
            strcpy(cur_file_path, filepath);

            FILE *file_backup = fopen("untitled_temp", "r");
            FILE *org = fopen(cur_file_path, "w");
            char line[1000];
            while (fgets(line, 1000, file_backup) != NULL)
            {
                fprintf(org, line);
            }
            fclose(org);
            fclose(file_backup);

            char fb_name[1000];
            strcpy(fb_name, cur_file_path);
            strcat(fb_name, "_temp");
            rename("untitled_temp", fb_name);
        }
        else
        {
            save_cur_file();
        }
        saved = 1;
        wclear(file_win);
        wprintw(file_win, cur_file_path);
        wrefresh(file_win);
    }
    else if (!strcmp(command, "saveas"))
    {
        char fileaddress[1000];
        file_input(fileaddress);
        char fb_name[1000];
        strcpy(fb_name, cur_file_path);
        strcat(fb_name, "_temp");

        strcpy(cur_file_path, fileaddress);
        char fb_name2[1000];
        strcpy(fb_name2, cur_file_path);
        strcat(fb_name2, "_temp");

        rename(fb_name, fb_name2);
        save_cur_file();
        saved = 1;
        wclear(file_win);
        wprintw(file_win, cur_file_path);
        wrefresh(file_win);
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
    else if (!strcmp(command, "exit"))
    {
        close_cur_file();
        cleanup();
    }
    end_of_command();
    wclear(command_win);
    wrefresh(command_win);
    wrefresh(text_win);
    // wmove(text_win, 0, 0);
    // wrefresh(text_win);
    // cursor position?
    noecho();
}

void set_mode_normal()
{
    mode = NORMAL_MODE;
    wclear(mode_win);
    wprintw(mode_win, "NORMAL");
    wrefresh(mode_win);
    wrefresh(text_win);
}

void set_mode_insert()
{
    mode = INSERT_MODE;
    wclear(mode_win);
    wprintw(mode_win, "INSERT");
    wrefresh(mode_win);
    if (line_count == 0)
    {
        wmove(text_win, 0, 0);
        cur_file_char = cur_file_line = 0;
    }
    wrefresh(text_win);
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
    FILE *file_backup = fopen(fb_name, "w");
    cur_file_char = cur_file_line = 0;
    wclear(text_win);
    wclear(line_win);
    if (valid_input == 0) /*file exists*/
    {
        saved = 1;
        f = fopen(fileaddress, "r");
        char line[1000];
        int i = 0;
        char_in_line[0] = 0;
        wclear(line_win);
        wprintw(line_win, "1");
        while (fgets(line, 1000, f) != NULL)
        {
            // stop printing text after reaching border
            if (i < LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT)
            {
                wprintw(text_win, line);
                fprintf(file_backup, line);
            }
            char_in_line[i] += strlen(line);
            if (line[strlen(line) - 1] == '\n' || feof(f))
            {
                i++;
                if (line[strlen(line) - 1] == '\n')
                    wprintw(line_win, "\n%d", i + 1);
                char_in_line[i] = 0;
            }
        }
        line_count = i;
        fclose(f);
    }
    else
    {
        saved = 0;
        f = fopen(fileaddress, "w");
        wprintw(line_win, "1");
        line_count = 0;
    }
    wrefresh(text_win);
    wrefresh(line_win);

    wclear(file_win);
    wprintw(file_win, fileaddress);
    if (!saved)
        wprintw(file_win, "  +");
    wrefresh(file_win);

    wrefresh(text_win);
    if (line_count == 0)
    {
        wmove(text_win, 0, 0);
        cur_file_line = cur_file_char = 0;
    }
    else
    {
        wmove(text_win, line_count - 1, char_in_line[line_count - 1]);
        cur_file_line = line_count - 1;
        cur_file_char = char_in_line[line_count - 1];
    }

    fclose(file_backup);
    fclose(f);
}

void close_cur_file()
{
    wclear(command_win);
    wrefresh(text_win);
    if (!saved)
    {
        wprintw(command_win, "do you want to save the current file? (y/n) ");
        wrefresh(command_win);
        char c;
        c = wgetch(command_win);
        if (c == 'y')
        {
            save_cur_file();
        }
    }
    char fb_name[1000];
    strcpy(fb_name, cur_file_path);
    strcat(fb_name, "_temp");
    remove(fb_name);
}

void save_cur_file()
{
    char fb_name[1000];
    strcpy(fb_name, cur_file_path);
    strcat(fb_name, "_temp");
    FILE *file_backup = fopen(fb_name, "r");
    FILE *org = fopen(cur_file_path, "w");
    char line[1000];
    while (fgets(line, 1000, file_backup) != NULL)
    {
        fprintf(org, line);
    }
    fclose(org);
    fclose(file_backup);
}

int insertstr_action(char fileaddress[], char str[], int pos_line, int pos_char)
{
    FILE *original_file = fopen(fileaddress, "r");
    FILE *temp_file = fopen(TEMP_FILE_NAME, "w");

    char line[2000];
    line[0] = 0;
    int cur_line = 0;
    while (cur_line < pos_line)
    {
        if (fgets(line, 2000, original_file) == NULL)
        {
            error_msg("the file doesn't have this many lines");
            fclose(original_file);
            fclose(temp_file);
            remove(TEMP_FILE_NAME);
            return -1;
        }

        fprintf(temp_file, line);

        // if reached end of a line (since lines can be longer than 2000 characters)
        if (line[strlen(line) - 1] == 10)
            cur_line++;
    }

    // check validity of line number
    if (fgets(line, 2000, original_file) == NULL && pos_line != 0)
    {
        error_msg("the file doesn't have this many lines");
        fclose(original_file);
        fclose(temp_file);
        remove(TEMP_FILE_NAME);
        return -1;
    }

    // check validity of start pos
    if (pos_char != 0 && pos_char > strlen(line))
    {
        printf("line %d of the file doesn't have this many characters\n", pos_line + 1);
        fclose(original_file);
        fclose(temp_file);
        remove(TEMP_FILE_NAME);
        return -1;
    }

    // add str and the rest of the line to temp_file
    char temp_char = line[pos_char];
    line[pos_char] = 0;
    if (pos_char != 0)
        fprintf(temp_file, line);

    // handle \n \\ \" \'
    char processed_str[STR_MAX_LENGTH] = {0};
    int i_str1 = 0;
    int i_str2 = 0;
    while (i_str1 < strlen(str))
    {
        if (str[i_str1] == '\\')
        {
            i_str1++;

            if (str[i_str1] == 'n')
            {
                processed_str[i_str2] = '\n';
            }
            else if (str[i_str1] == '\"' || str[i_str1] == '\'' || str[i_str1] == '\\')
            {
                processed_str[i_str2] = str[i_str1];
            }
            else
            {
                printf("unrecognized character \\%c\n", str[i_str1]);
                fclose(original_file);
                fclose(temp_file);
                remove(TEMP_FILE_NAME);
                return -1;
            }
        }
        else
            processed_str[i_str2] = str[i_str1];

        i_str1++;
        i_str2++;
    }
    fprintf(temp_file, processed_str);

    line[pos_char] = temp_char;
    fprintf(temp_file, line + pos_char);
    cur_line++;

    // add the rest of original_file to temp_file
    while (fgets(line, 2000, original_file) != NULL)
    {
        fprintf(temp_file, line);
    }
    fclose(original_file);
    fclose(temp_file);

    // copy temp_file's contents to original_file
    original_file = fopen(fileaddress, "w");
    temp_file = fopen(TEMP_FILE_NAME, "r");
    while (fgets(line, 2000, temp_file) != NULL)
    {
        fprintf(original_file, line);
    }

    fclose(original_file);
    fclose(temp_file);
    remove(TEMP_FILE_NAME);

    return 0;
}

int removestr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction)
{
    if (direction == 'f')
        return removestr_f(fileaddress, pos_line, pos_char, size);
    else
        return removestr_b(fileaddress, pos_line, pos_char, size);
}

int removestr_f(char fileaddress[], int pos_line, int pos_char, int size)
{
    FILE *original_file = fopen(fileaddress, "r");
    FILE *temp_file = fopen(TEMP_FILE_NAME, "w");

    char line[2000];
    line[0] = 0;
    int cur_line = 0;
    while (cur_line < pos_line)
    {
        if (fgets(line, 2000, original_file) == NULL)
        {
            error_msg("the file doesn't have this many lines");
            fclose(original_file);
            fclose(temp_file);
            remove(TEMP_FILE_NAME);
            return -1;
        }

        fprintf(temp_file, line);

        // if reached end of a line (since lines can be longer than 2000 characters)
        if (line[strlen(line) - 1] == 10)
            cur_line++;
    }

    int count = 0;
    char temp;
    while (count < pos_char)
    {
        temp = fgetc(original_file);
        if (temp == EOF)
        {
            fclose(original_file);
            fclose(temp_file);
            remove(TEMP_FILE_NAME);
            error_msg("this line doesn't have enough characters");
            return -1;
        }
        fputc(temp, temp_file);
        count++;
    }
    for (int i = 0; i < size; i++)
    {
        if (fgetc(original_file) == EOF)
        {
            fclose(original_file);
            fclose(temp_file);
            remove(TEMP_FILE_NAME);
            error_msg("no changes made");
            error_msg("couldn't remove this many characters");
            return -1;
        }
    }

    // add the rest of original_file to temp_file
    while (fgets(line, 2000, original_file) != NULL)
    {
        fprintf(temp_file, line);
    }
    fclose(original_file);
    fclose(temp_file);

    // copy temp_file's contents to original_file
    original_file = fopen(fileaddress, "w");
    temp_file = fopen(TEMP_FILE_NAME, "r");
    while (fgets(line, 2000, temp_file) != NULL)
    {
        fprintf(original_file, line);
    }

    fclose(original_file);
    fclose(temp_file);
    remove(TEMP_FILE_NAME);
    return 0;
}

int removestr_b(char fileaddress[], int pos_line, int pos_char, int size)
{
    FILE *original_file = fopen(fileaddress, "r");

    if (pos_line == 0)
    {
        fclose(original_file);
        if (pos_char - size < 0)
        {
            error_msg("no changes made");
            error_msg("couldn't remove this many characters");
            return -1;
        }
        return removestr_f(fileaddress, pos_line, pos_char - size, size);
    }
    int char_count[pos_line];
    for (int i = 0; i < pos_line; i++)
        char_count[i] = 0;

    char line[2000];
    int i = 0;
    while (i < pos_line)
    {
        if (fgets(line, 2000, original_file) == NULL)
        {
            fclose(original_file);
            error_msg("the file doesn't have this many lines");
            return -1;
        }
        char_count[i] += strlen(line);
        if (line[strlen(line) - 1] == '\n')
            i++;
    }
    fclose(original_file);

    long long sum = 0;
    int j;
    for (j = pos_line - 1; j >= 0; j--)
    {
        if (sum + char_count[j] >= size - pos_char)
            break;
        sum += char_count[j];
    }

    if (j == -1)
    {
        error_msg("no changes made");
        error_msg("couldn't remove this many characters");
        return -1;
    }
    int pos_start = char_count[j] - (size - pos_char - sum);

    return removestr_f(fileaddress, j, pos_start, size);
}

int createfile_action(char fileaddress[])
{
    FILE *f;

    if (fileaddress[0] == '\\' || fileaddress[0] == '/')
        f = fopen(fileaddress + 1, "r");
    else
        f = fopen(fileaddress, "r");

    // check if the file already exists
    if (f != NULL)
    {
        fclose(f);
        error_msg("this file already exists");
        return -1;
    }

    int validpath = makepath(fileaddress);

    if (validpath == -1)
        return -1;

    f = fopen(fileaddress, "w");
    fclose(f);

    return 0;
}

int makepath(char fileaddress[])
{
    char addresscopy[1000];
    strcpy(addresscopy, fileaddress);

    char filename[256];

    // default file name is the whole address
    strcpy(filename, addresscopy);

    // extract file name
    for (int i = strlen(addresscopy); i >= 0; i--)
    {
        // if reached / or \ or first char of address(address contains no / or \)
        if (addresscopy[i] == '/' || addresscopy[i] == '\\')
        {
            strcpy(filename, addresscopy + i + 1);
            break;
        }
    }
    //?? do I need to check if the file name has a format

    // check if the file name is valid (doesn't contain forbidden characters)
    FILE *test = fopen(filename, "w");
    if (test == NULL)
    {
        // SHOULD BE CHANGED: files which have the same name as a folder give this unrelated message
        error_msg("invalid file name.");
        error_msg("file names can't contain the following characters: < > : \" / \\ | ? *");
        return -1;
    }
    else
    {
        fclose(test);
        remove(filename);
    }

    // create directory
    // PROBLEM: if a folder name in the middle is not invalid the previous folders are made
    // EXAMPLE: createfile --file "f1/f*2/t.txt" -> f1 is made and the error is shown afterwards

    // file address has a / or \\ at the beginning

    for (int i = 0; addresscopy[i] != 0; i++)
    {
        if (addresscopy[i] == '/' || addresscopy[i] == '\\')
        {
            addresscopy[i] = 0;
            // didn't handle invalid folder names and folders that have the same name as a file
            _mkdir(addresscopy);
            addresscopy[i] = '/';
        }
    }

    return 0;
}

int create_file_input(char fileaddress[])
{
    wgetstr(command_win, fileaddress);
    if (fileaddress[strlen(fileaddress) - 1] == '\n')
        fileaddress[strlen(fileaddress) - 1] = 0;

    if (fileaddress[0] == '\"')
    {
        if (fileaddress[strlen(fileaddress) - 1] != '\"')
        {
            error_msg("incorrect use of \"\"");
            return -1;
        }

        // delete the closing " from file path
        fileaddress[strlen(fileaddress) - 1] = 0;

        // delete the opening " from file path (by shifting the file address to the left)
        for (int i = 1; i < strlen(fileaddress); i++)
        {
            fileaddress[i - 1] = fileaddress[i];
        }
        fileaddress[strlen(fileaddress) - 1] = 0;
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

    return 0;
}