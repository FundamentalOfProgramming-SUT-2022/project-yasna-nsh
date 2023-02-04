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
#define HIGH_COLOR 5

#define NORMAL_MODE 0
#define INSERT_MODE 1
#define VISUAL_MODE 2

#define COUNT_MODE 0
#define BYWORD_MODE 1
#define ALL_MODE 2
#define ALL_BYWORD_MODE 3

void move_to_pos();
void show_prev_lines();
void show_next_lines();
void init_env();
void init_windows();
void refresh_view();
void print_highlighted_text(int from_line);
void print_selected_text(int from_line);
void normal_input(char c);
void insert_input(char c);
void visual_input(char c);
void command_input();
void set_mode_normal();
void set_mode_insert();
void set_mode_visual();
int line_char_to_pos(int cur_char, int cur_line);
void getcommand(char com[]);
void error_msg(char message[]);
int file_input(char fileaddress[]);
int dir_exist(char fileaddress[]);
void create_remove_file();
void add_to_rlist(char fileaddress[]);
void end_of_command();

// phase 2 command functions
int open_f();
int open_action(char fileaddress[], int valid_input);
void close_cur_file();
void save_cur_file();
void find_f();
void go_to_next_highlight();
void replace2();

// phase 1 functions
int insertstr_action(char fileaddress[], char str[], int pos_line, int pos_char);
int removestr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction);
int removestr_f(char fileaddress[], int pos_line, int pos_char, int size);
int removestr_b(char fileaddress[], int pos_line, int pos_char, int size);
int create_file_input(char fileaddress[]);
int makepath(char fileaddress[]);
int createfile_action(char fileaddress[]);
int undo_action(char fileaddress[]);
int track_changes(char fileaddress[]);
int auto_indent_action(char fileaddress[]);
int check_brace_validity(char fileaddress[]);
void process_line(char line[], char pline[]);
int next_non_wspace_index(char str[], int i);
void indent_line(FILE *f, char str[], int *depth_ptr);
void print_spaces(FILE *f, int depth);
int copystr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction);
int copystr_f(char fileaddress[], int pos_line, int pos_char, int size);
int copystr_b(char fileaddress[], int pos_line, int pos_char, int size);
int cutstr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction);
int pastestr_action(char fileaddress[], int pos_line, int pos_char);
int pos_to_line_char(int pos, int *pos_line_ptr, int *pos_char_ptr, char fileaddress[]);
int replace_at(char fileaddress[], char str1[], char str2[], int at);
int replace_all(char fileaddress[], char str1[], char str2[]);
int str_input(char str[]);
int find();
int find_input(char str[], int *mode_ptr, int *at_ptr);
int find_action_vanilla(char str[], char fileaddress[], int at, int *end_ptr);
int find_action_next(char line[], char str[], int from, int *end_index_ptr, int star_start);
int find_action_count(char str[], char fileaddress[]);
int find_action_byword(char str[], char fileaddress[], int at);
int wordnum(char fileaddress[], int pos_char);
int find_action_all(char str[], char fileaddress[]);
int find_action_all_byword(char str[], char fileaddress[]);
int pos_input(int *pos_line_ptr, int *pos_char_ptr);

FILE *command_file;
char cur_file_path[1000];
char fb_name[1000];
int cur_file_line, cur_file_char;
// files with more than 6000 lines?
int char_in_line[6000] = {0};
int line_count;
int saved;
int first_line_index;
int start_char_vis, start_line_vis;
int highlighted_line[1000];
int highlighted_char[1000];
int highlighted_length[1000];
int next_highlighted_line, next_highlighted_char, on_h_index;
int in_hmode = 0;

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
        else if (mode == VISUAL_MODE)
        {
            c = getch();
            visual_input(c);
        }
    }
}

void move_to_pos()
{
    // check logic
    int maxx = COLS - LINE_WIN_WIDTH;
    int maxy = LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT;

    int x = cur_file_char % (maxx);
    int y = 0;
    for (int i = first_line_index; i < cur_file_line; i++)
    {
        y += (char_in_line[i] / maxx) + ((char_in_line[i] % maxx) != 0);
    }
    y += (cur_file_char / maxx);

    if (y >= maxy)
    {
        show_next_lines();
        y--;
    }
    if (cur_file_line < first_line_index)
    {
        show_prev_lines();
    }

    wmove(text_win, y, x);
    wrefresh(text_win);
}

void show_prev_lines()
{
    if (first_line_index == 0)
        return;
    first_line_index--;
    refresh_view(first_line_index);
}

void show_next_lines()
{
    // check condition
    if (first_line_index + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT - 1 == line_count)
        return;
    first_line_index++;
    refresh_view(first_line_index);
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
    // wclear(command_win);
    // wprintw(command_win, message);
    // wrefresh(text_win);
    // napms(2000);

    printf("%s\n", message);
}

int file_input(char fileaddress[])
{
    // doesn't expect --file
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

void init_env()
{
    initscr();
    cbreak();
    keypad(stdscr, true);
    scrollok(text_win, true);
    start_color();
    init_color(COLOR_GREEN, 180, 234, 162);
    init_pair(TEXT_COLOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(MODE_COLOR, COLOR_BLACK, COLOR_GREEN);
    init_pair(FILE_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(COMMAND_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(LINE_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(HIGH_COLOR, COLOR_WHITE, COLOR_GREEN);
    refresh();
    init_windows();

    FILE *file_backup = fopen("untitled_temp", "w");
    fclose(file_backup);
    strcpy(cur_file_path, "untitled");
    strcpy(fb_name, "untitled_temp");
    cur_file_line = cur_file_char = 0;
    line_count = 0;
    char_in_line[0] = 0;
    saved = 0;
    wprintw(file_win, "untitled");
    wprintw(file_win, "  +");
    wprintw(line_win, "1 ");
    first_line_index = 0;
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

void refresh_view(int from_line)
{
    // print edited text again to prevent overwrite and update line and char count
    wclear(text_win);
    FILE *f = fopen(fb_name, "r");
    char line[1000];
    int i = 0;
    char_in_line[0] = 0;
    wclear(line_win);
    wprintw(line_win, "%d", from_line + 1);
    while (fgets(line, 1000, f) != NULL)
    {
        // stop printing text after reaching border
        if (i >= from_line && i < from_line + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT)
        {
            wprintw(text_win, line);
        }
        char_in_line[i] += strlen(line);
        if (line[strlen(line) - 1] == '\n' || feof(f))
        {
            i++;
            char_in_line[i] = 0;
            if (i > from_line && i < from_line + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT && line[strlen(line) - 1] == '\n')
                wprintw(line_win, "\n%d", i + 1);
        }
    }
    fclose(f);
    line_count = i;
    first_line_index = from_line;
    wrefresh(line_win);
    move_to_pos();
    wrefresh(text_win);
}

// add highlight to more than
void print_highlighted_text(int from_line)
{
    wclear(text_win);
    FILE *f = fopen(fb_name, "r");
    char line[1000];
    int i = 0;
    int h_count = 0;
    char_in_line[0] = 0;
    wclear(line_win);
    wprintw(line_win, "%d", from_line + 1);
    while (fgets(line, 1000, f) != NULL)
    {
        // stop printing text after reaching border
        if (i >= from_line && i < from_line + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT)
        {
            for (int j = 0; line[j] != 0; j++)
            {
                if (j == highlighted_char[h_count] && i == highlighted_line[h_count])
                {
                    wattron(text_win, COLOR_PAIR(HIGH_COLOR));
                    for (; j <= highlighted_char[h_count] + highlighted_length[h_count] - 1; j++)
                    {
                        wprintw(text_win, "%c", line[j]);
                    }
                    j--;
                    h_count++;
                    wattroff(text_win, COLOR_PAIR(HIGH_COLOR));
                }
                else
                    wprintw(text_win, "%c", line[j]);
            }
            // wprintw(text_win, line);
        }
        char_in_line[i] += strlen(line);
        if (line[strlen(line) - 1] == '\n' || feof(f))
        {
            i++;
            char_in_line[i] = 0;
            if (i > from_line && i < from_line + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT && line[strlen(line) - 1] == '\n')
                wprintw(line_win, "\n%d", i + 1);
        }
    }
    fclose(f);
    line_count = i;
    first_line_index = from_line;
    wrefresh(line_win);
    move_to_pos();
    wrefresh(text_win);
}

void print_selected_text(int from_line)
{
    wclear(text_win);
    FILE *f = fopen(fb_name, "r");
    char line[1000];
    int i = 0;
    int h_count = 0;
    char_in_line[0] = 0;

    int start_line, start_char;
    int end_line, end_char;
    if (cur_file_line > start_line_vis)
    {
        end_line = cur_file_line;
        end_char = cur_file_char;
        start_line = start_line_vis;
        start_char = start_char_vis;
    }
    else if (cur_file_line < start_line_vis)
    {
        start_line = cur_file_line;
        start_char = cur_file_char;
        end_line = start_line_vis;
        end_char = start_char_vis;
    }
    else
    {
        start_line = end_line = cur_file_line;
        if (start_char_vis < cur_file_char)
        {
            start_char = start_char_vis;
            end_char = cur_file_char;
        }
        else
        {
            start_char = cur_file_char;
            end_char = start_char_vis;
        }
    }

    wclear(line_win);
    wprintw(line_win, "%d", from_line + 1);
    while (fgets(line, 1000, f) != NULL)
    {
        // stop printing text after reaching border
        if (i >= from_line && i < from_line + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT)
        {
            for (int j = 0; line[j] != 0; j++)
            {
                if (start_line == end_line)
                {
                    if (i == start_line && j >= start_char && j <= end_char)
                    {
                        wattron(text_win, COLOR_PAIR(HIGH_COLOR));
                        wprintw(text_win, "%c", line[j]);
                        wattroff(text_win, COLOR_PAIR(HIGH_COLOR));
                    }
                    else
                        wprintw(text_win, "%c", line[j]);
                }
                else if ((i == start_line && j >= start_char) ||
                         (i == end_line && j <= end_char) ||
                         (i > start_line && i < end_line))
                {
                    wattron(text_win, COLOR_PAIR(HIGH_COLOR));
                    wprintw(text_win, "%c", line[j]);
                    wattroff(text_win, COLOR_PAIR(HIGH_COLOR));
                }
                else
                    wprintw(text_win, "%c", line[j]);
            }
            // wprintw(text_win, line);
        }
        char_in_line[i] += strlen(line);
        if (line[strlen(line) - 1] == '\n' || feof(f))
        {
            i++;
            char_in_line[i] = 0;
            if (i > from_line && i < from_line + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT && line[strlen(line) - 1] == '\n')
                wprintw(line_win, "\n%d", i + 1);
        }
    }
    fclose(f);
    line_count = i;
    first_line_index = from_line;
    wrefresh(text_win);
    wrefresh(line_win);
    move_to_pos();
    wrefresh(text_win);
}

void normal_input(char c)
{
    if (c != 'n' && in_hmode)
    {
        in_hmode = 0;

        // remove_highlights
        refresh_view(first_line_index);
    }
    if (c == 'i') /*insert*/
    {
        set_mode_insert();
        return;
    }
    else if (c == 'v') /*visual*/
    {
        set_mode_visual();
        return;
    }
    else if (c == ':') /*command*/
    {
        command_input();
        return;
    }
    else if (c == '/') /*find*/
    {
        find_f();
        return;
    }
    else if (in_hmode && c == 'n') /*next in find*/
    {
        go_to_next_highlight();
        return;
    }
    else if (c == 'u') /*undo*/
    {
        undo_action(fb_name);
        refresh_view(first_line_index);
        if (line_count - first_line_index < LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT)
        {
            if (line_count == 0)
                cur_file_line = 0;
            else
                cur_file_line = line_count - 1;
        }
        else
            cur_file_line = first_line_index + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT - 1;

        if (cur_file_line == line_count - 1 || line_count == 0)
            cur_file_char = char_in_line[cur_file_line];
        else
            cur_file_char = char_in_line[cur_file_line] - 1;
        move_to_pos();
        return;
    }
    else if (c == '=') /*auto-indent*/
    {
        track_changes(fb_name);
        auto_indent_action(fb_name);
        refresh_view(first_line_index);
        if (line_count == 0)
        {
            cur_file_line = 0;
            cur_file_char = 0;
        }
        else
        {
            cur_file_line = line_count - 1;
            cur_file_char = char_in_line[cur_file_line];
        }

        // show file's save state
        if (saved)
        {
            wprintw(file_win, "  +");
            saved = 0;
            wrefresh(file_win);
        }

        move_to_pos();
        return;
    }
    else if (c == 'p') /*paste*/
    {
        track_changes(fb_name);
        pastestr_action(fb_name, cur_file_line, cur_file_char);
        refresh_view(first_line_index);

        // show file's save state
        if (saved)
        {
            wprintw(file_win, "  +");
            saved = 0;
            wrefresh(file_win);
        }
        return;
    }
    else if (c == 3) /*up arrow*/
    {
        if (cur_file_line < first_line_index + 4)
            show_prev_lines();
        move_to_pos();
        return;
    }
    else if (c == 2) /*down arrow*/
    {
        if (cur_file_line > first_line_index + LINES - MODE_WIN_HEIGHT - COMMAND_WIN_HEIGHT - 5)
            show_next_lines();
        move_to_pos();
        return;
    }

    // movement
    // h: left
    // l: right
    // j:down
    // k:up
    if (c == 'h')
    {
        if (cur_file_char == 0)
            return;
        cur_file_char--;
        move_to_pos();
        return;
    }
    else if (c == 'l')
    {
        if (line_count == 0)
            return;
        if ((cur_file_char == char_in_line[cur_file_line] - 1 && cur_file_line != line_count - 1) || (cur_file_line == line_count - 1 && cur_file_char == char_in_line[cur_file_line]))
            return;
        cur_file_char++;
        move_to_pos();
        return;
    }
    else if (c == 'j')
    {
        if (cur_file_line == line_count - 1)
            return;
        cur_file_line++;
        if ((cur_file_char >= char_in_line[cur_file_line] && cur_file_line != line_count - 1))
            cur_file_char = char_in_line[cur_file_line] - 1;
        else if (cur_file_line == line_count - 1 && cur_file_char > char_in_line[cur_file_line])
            cur_file_char = char_in_line[cur_file_line];
        move_to_pos();
        return;
    }
    else if (c == 'k')
    {
        if (cur_file_line == 0)
            return;
        cur_file_line--;
        if ((cur_file_char >= char_in_line[cur_file_line] && cur_file_line != line_count - 1))
            cur_file_char = char_in_line[cur_file_line] - 1;
        else if (cur_file_line == line_count - 1 && cur_file_char > char_in_line[cur_file_line])
            cur_file_char = char_in_line[cur_file_line];
        move_to_pos();
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
    if (c == 8) /*BACKSPACE*/
    {
        if (cur_file_line != 0 && cur_file_char == 0)
        {
            removestr_action(fb_name, cur_file_line, cur_file_char, 1, 'b');
        }
        else if (cur_file_char + cur_file_line != 0)
        {
            removestr_action(fb_name, cur_file_line, cur_file_char, 1, 'b');
        }
    }
    else if (c == 3 || c == 2 || c == 4 || c == 5)
    {
        // movement
        // down:2
        // up: 3
        // left: 4
        // right: 5

        if (c == 4)
        {
            if (cur_file_char == 0)
                return;
            cur_file_char--;
            move_to_pos();
            return;
        }
        else if (c == 5)
        {
            if (line_count == 0)
                return;
            if ((cur_file_char == char_in_line[cur_file_line] - 1 && cur_file_line != line_count - 1) || (cur_file_line == line_count - 1 && cur_file_char == char_in_line[cur_file_line]))
                return;
            cur_file_char++;
            move_to_pos();
            return;
        }
        else if (c == 2)
        {
            if (cur_file_line == line_count - 1)
                return;
            cur_file_line++;
            if ((cur_file_char >= char_in_line[cur_file_line] && cur_file_line != line_count - 1))
                cur_file_char = char_in_line[cur_file_line] - 1;
            else if (cur_file_line == line_count - 1 && cur_file_char > char_in_line[cur_file_line])
                cur_file_char = char_in_line[cur_file_line];
            move_to_pos();
            return;
        }
        else if (c == 3)
        {
            if (cur_file_line == 0)
                return;
            cur_file_line--;
            if ((cur_file_char >= char_in_line[cur_file_line] && cur_file_line != line_count - 1))
                cur_file_char = char_in_line[cur_file_line] - 1;
            else if (cur_file_line == line_count - 1 && cur_file_char > char_in_line[cur_file_line])
                cur_file_char = char_in_line[cur_file_line];
            move_to_pos();
            return;
        }
    }
    else /*normal keys*/
    {
        // add text to temp file
        char temp[2];
        temp[0] = c;
        temp[1] = 0;
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
    refresh_view(first_line_index);

    // check logic
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

    move_to_pos();
    wrefresh(text_win);
}

void visual_input(char c)
{

    if (c == 27) /*ESC*/
    {
        set_mode_normal();
        refresh_view(first_line_index);
        return;
    }
    else if (c == 'y')
    {
        track_changes(fb_name);

        int pos_start = line_char_to_pos(start_char_vis, start_line_vis);
        int pos_end = line_char_to_pos(cur_file_char, cur_file_line);
        int size;
        if (pos_start > pos_end)
        {
            size = pos_start - pos_end + 1;
            copystr_action(fb_name, cur_file_line, cur_file_char, size, 'f');
        }
        else
        {
            size = pos_end - pos_start + 1;
            copystr_action(fb_name, start_line_vis, start_char_vis, size, 'f');
        }
        set_mode_normal();
        refresh_view(first_line_index);
        return;
    }
    else if (c == 'd')
    {
        track_changes(fb_name);

        int pos_start = line_char_to_pos(start_char_vis, start_line_vis);
        int pos_end = line_char_to_pos(cur_file_char, cur_file_line);
        int size;
        if (pos_start > pos_end)
        {
            size = pos_start - pos_end + 1;
            cutstr_action(fb_name, cur_file_line, cur_file_char, size, 'f');
        }
        else
        {
            size = pos_end - pos_start + 1;
            cutstr_action(fb_name, start_line_vis, start_char_vis, size, 'f');
        }
        set_mode_normal();
        refresh_view(first_line_index);
        return;
    }

    // selection
    // h: left
    // l: right
    // j:down
    // k:up
    if (c == 'h')
    {
        if (cur_file_char == 0)
            return;
        cur_file_char--;
    }
    else if (c == 'l')
    {
        if ((cur_file_char == char_in_line[cur_file_line] - 1 && cur_file_line != line_count - 1) || (cur_file_line == line_count - 1 && cur_file_char == char_in_line[cur_file_line]))
            return;
        cur_file_char++;
    }
    else if (c == 'j')
    {
        if (cur_file_line == line_count - 1)
            return;
        cur_file_line++;
        if ((cur_file_char >= char_in_line[cur_file_line] && cur_file_line != line_count - 1))
            cur_file_char = char_in_line[cur_file_line] - 1;
        else if (cur_file_line == line_count - 1 && cur_file_char > char_in_line[cur_file_line])
            cur_file_char = char_in_line[cur_file_line];
    }
    else if (c == 'k')
    {
        if (cur_file_line == 0)
            return;
        cur_file_line--;
        if ((cur_file_char >= char_in_line[cur_file_line] && cur_file_line != line_count - 1))
            cur_file_char = char_in_line[cur_file_line] - 1;
        else if (cur_file_line == line_count - 1 && cur_file_char > char_in_line[cur_file_line])
            cur_file_char = char_in_line[cur_file_line];
    }
    move_to_pos();
    print_selected_text(first_line_index);
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
            wclear(command_win);
            wrefresh(command_win);
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
        char fb_name2[1000];

        // keep previous backup's name
        strcpy(fb_name2, cur_file_path);
        strcat(fb_name2, "_temp");

        strcpy(cur_file_path, fileaddress);

        strcpy(fb_name, cur_file_path);
        strcat(fb_name, "_temp");

        createfile_action(cur_file_path);
        rename(fb_name2, fb_name);
        save_cur_file();
        saved = 1;
        wclear(file_win);
        wprintw(file_win, cur_file_path);
        wrefresh(file_win);
    }
    else if (!strcmp(command, "undo"))
    {
        char word[100];
        if (fscanf(command_file, "%s", word) != EOF)
        {
            // undo other file
            char fileaddress[1000];
            file_input(fileaddress);
            if (!strcmp(fileaddress, cur_file_path))
            {
                undo_action(fb_name);
                // show file's save state
                if (saved)
                {
                    wprintw(file_win, "  +");
                    saved = 0;
                    wrefresh(file_win);
                }
                refresh_view(first_line_index);
                if (line_count == 0)
                {
                    cur_file_line = 0;
                    cur_file_char = 0;
                }
                else
                {
                    cur_file_line = line_count - 1;
                    cur_file_char = char_in_line[cur_file_line];
                }
                move_to_pos();
            }
            else
            {
                undo_action(fileaddress);
            }
        }
        else
        {
            undo_action(fb_name);
            // show file's save state
            if (saved)
            {
                wprintw(file_win, "  +");
                saved = 0;
                wrefresh(file_win);
            }
            refresh_view(first_line_index);
        }
    }
    else if (!strcmp(command, "auto-indent"))
    {
        if (fgetc(command_file) == '\n')
        {
            track_changes(fb_name);
            auto_indent_action(fb_name);
            refresh_view(first_line_index);
            if (line_count == 0)
            {
                cur_file_line = 0;
                cur_file_char = 0;
            }
            else
            {
                cur_file_line = line_count - 1;
                cur_file_char = char_in_line[cur_file_line];
            }

            // show file's save state
            if (saved)
            {
                wprintw(file_win, "  +");
                saved = 0;
                wrefresh(file_win);
            }

            move_to_pos();
        }
        else
        {
            char fileaddress[1000];
            file_input(fileaddress);
            if (!strcmp(fileaddress, cur_file_path))
            {
                track_changes(fb_name);
                auto_indent_action(fb_name);
                refresh_view(first_line_index);
                if (line_count == 0)
                {
                    cur_file_line = 0;
                    cur_file_char = 0;
                }
                else
                {
                    cur_file_line = line_count - 1;
                    cur_file_char = char_in_line[cur_file_line];
                }

                // show file's save state
                if (saved)
                {
                    wprintw(file_win, "  +");
                    saved = 0;
                    wrefresh(file_win);
                }

                move_to_pos();
            }
            else
            {
                track_changes(fileaddress);
                auto_indent_action(fileaddress);
            }
        }
    }
    else if (!strcmp(command, "replace"))
    {
        // ADD REPLACE PHASE 1 LATER
        replace2();
        refresh_view(first_line_index);
    }
    else if (!strcmp(command, "createfile"))
    {
        // NO --file BECAUSE OF DOC 2 (BUT THERE IS A --file IN DOC 1)
        char fileaddress[1000];
        file_input(fileaddress);
        createfile_action(fileaddress);
    }
    else if (!strcmp(command, "insertstr"))
    {
        char word[100];

        char fileaddress[1000];
        // --file
        fscanf(command_file, "%s", word);
        file_input(fileaddress);

        char str[STR_MAX_LENGTH];
        str_input(str);

        int pos_line, pos_char;
        pos_input(&pos_line, &pos_char);

        if (!strcmp(fileaddress, cur_file_path))
        {
            track_changes(fb_name);
            insertstr_action(fb_name, str, pos_line, pos_char);
            // show file's save state
            if (saved)
            {
                wprintw(file_win, "  +");
                saved = 0;
                wrefresh(file_win);
            }
            refresh_view(first_line_index);
            move_to_pos();
        }
        else
        {
            track_changes(fileaddress);
            insertstr_action(fileaddress, str, pos_line, pos_char);
        }
    }
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
        endwin();
        exit(0);
    }
    end_of_command();
    wclear(command_win);
    wrefresh(command_win);
    wrefresh(text_win);
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
    track_changes(fb_name);
}

void set_mode_visual()
{
    mode = VISUAL_MODE;
    wclear(mode_win);
    wprintw(mode_win, "VISUAL");
    wrefresh(mode_win);
    wrefresh(text_win);
    start_char_vis = cur_file_char;
    start_line_vis = cur_file_line;
}

int line_char_to_pos(int cur_char, int cur_line)
{
    // check logic
    int pos = 0;
    for (int i = 0; i < cur_line; i++)
        pos += char_in_line[i];
    pos += cur_char;
    return pos;
}

// phase 2 command functions
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
        while (fgets(line, 1000, f) != NULL)
        {
            fprintf(file_backup, line);
        }
        fclose(f);
        fclose(file_backup);

        first_line_index = 0;
        refresh_view(first_line_index);
    }
    else
    {
        saved = 0;
        f = fopen(fileaddress, "w");
        wprintw(line_win, "1");
        line_count = 0;
        first_line_index = 0;
        fclose(file_backup);
        fclose(f);
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
        cur_file_line = line_count - 1;
        cur_file_char = char_in_line[line_count - 1] - 1;
        move_to_pos();
    }
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
    remove(fb_name);
}

void save_cur_file()
{
    FILE *file_backup = fopen(fb_name, "r");
    FILE *org = fopen(cur_file_path, "w");
    char line[1000];
    while (fgets(line, 1000, file_backup) != NULL)
    {
        fprintf(org, line);
    }
    fclose(org);
    fclose(file_backup);
    saved = 1;
}

void find_f()
{
    in_hmode = 1;
    wmove(command_win, 0, 0);
    wprintw(command_win, "/");
    wrefresh(command_win);
    echo();
    char exp[1000];
    char word[1000];
    wscanw(command_win, "%s", word);

    if (word[0] != '\"')
    {
        strcpy(exp, word);
    }
    else
    {
        for (int i = 1; i < strlen(word); i++)
        {
            exp[i - 1] = word[i];
        }

        int i = strlen(word) - 1;
        char c;
        while (1)
        {
            c = wgetch(command_win);
            if (c == '\"' && exp[i - 1] != '\\')
                break;
            exp[i] = c;
            i++;
        }
        exp[i] = 0;
    }

    find_action_all(exp, fb_name);
    print_highlighted_text(first_line_index);

    if (highlighted_line[0] != -1)
    {
        if (highlighted_line[on_h_index] == -1)
        {
            on_h_index = 0;
        }

        cur_file_char = highlighted_char[on_h_index];
        cur_file_line = highlighted_line[on_h_index];
        move_to_pos();
    }

    wclear(command_win);
    wrefresh(command_win);
    wrefresh(text_win);
    noecho();
}

void go_to_next_highlight()
{
    on_h_index++;
    if (highlighted_line[on_h_index] == -1)
    {
        on_h_index = 0;
        if (highlighted_line[0] == -1)
            return;
    }

    cur_file_char = highlighted_char[on_h_index];
    cur_file_line = highlighted_line[on_h_index];
    move_to_pos();
}

void replace2()
{
    // phase 2 replace (no --file)
    char fileaddress[1000];
    char str1[STR_MAX_LENGTH], str2[STR_MAX_LENGTH];
    str_input(str1);
    str_input(str2);
    strcpy(fileaddress, fb_name);

    char word[100] = {0};
    fscanf(command_file, "%s", word);
    // phase 1 replace -> get file address
    if (!strcmp(word, "--file"))
    {
        file_input(fileaddress);
        fscanf(command_file, "%s", word);
    }

    // mode
    int mode = 0, at = 1;
    if (!strcmp(word, "-at"))
    {
        fscanf(command_file, "%d", &at);
    }
    else if (!strcmp(word, "-all"))
    {
        mode = 1;
    }

    track_changes(fileaddress);

    if (mode == 0)
    {
        if (replace_at(fb_name, str1, str2, at) == -1)
        {
            error_msg("match not found");
            undo_action(fileaddress);
        }
        refresh_view(first_line_index);
        move_to_pos();
    }
    else
    {
        int temp = replace_all(fileaddress, str1, str2);
        if (temp == -1)
            undo_action(fileaddress);
        refresh_view(first_line_index);
        move_to_pos();
    }
}

// phase 1 funcitons
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
        // error_msg("this file already exists");
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

int undo_action(char fileaddress[])
{
    char record_count_address[1000];
    strcpy(record_count_address, fileaddress);
    strcat(record_count_address, RECORD_COUNT_EXT);

    // make file if it doesn't exist
    FILE *record_count_file = fopen(record_count_address, "r");

    int count;
    if (record_count_file == NULL)
    {
        error_msg("history not found");
        return -1;
    }
    else
    {
        // get and adjust the number of records
        fscanf(record_count_file, "%d", &count);
        if (count == 0)
        {
            error_msg("history not found");
            SetFileAttributesA(record_count_address, FILE_ATTRIBUTE_HIDDEN);
            fclose(record_count_file);
            return -1;
        }
        fclose(record_count_file);
        SetFileAttributesA(record_count_address, FILE_ATTRIBUTE_NORMAL);
        record_count_file = fopen(record_count_address, "w");
        fprintf(record_count_file, "%d", count - 1);
        SetFileAttributesA(record_count_address, FILE_ATTRIBUTE_HIDDEN);
        fclose(record_count_file);
        if (!strcmp(fileaddress, fb_name) && saved)
        {
            saved = 0;
            wprintw(file_win, "  +");
            wrefresh(file_win);
        }
    }

    // DOESN'T SUPPORT MORE THAN 9999 UNDOS
    char num_str[5];
    sprintf(num_str, "%d", count);

    char record_address[1000];
    strcpy(record_address, fileaddress);
    strcat(record_address, UNDO_EXT);
    strcat(record_address, num_str);
    FILE *record_file = fopen(record_address, "r");
    FILE *original_file = fopen(fileaddress, "w");

    char line[2000];
    while (fgets(line, 2000, record_file) != NULL)
    {
        fprintf(original_file, line);
    }

    fclose(original_file);
    fclose(record_file);
    remove(record_address);

    return 0;
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

int auto_indent_action(char fileaddress[])
{
    int valid_brace = check_brace_validity(fileaddress);
    if (valid_brace == -1)
    {
        error_msg("closing pairs don't match");
        return -1;
    }

    FILE *f = fopen(fileaddress, "r");
    FILE *temp = fopen(".autoindent", "w");
    char line[2000];
    char pline[2000] = {0};
    int depth = 0;
    while (fgets(line, 2000, f) != NULL)
    {
        process_line(line, pline);
        indent_line(temp, pline, &depth);
    }
    fclose(temp);
    fclose(f);
    temp = fopen(".autoindent", "r");
    f = fopen(fileaddress, "w");
    while (fgets(line, 2000, temp) != NULL)
    {
        // print line if it isn't empty
        if (next_non_wspace_index(line, 0) != -1)
            fprintf(f, line);
    }
    fclose(f);
    fclose(temp);
    remove(".autoindent");
    return 0;
}

int check_brace_validity(char fileaddress[])
{
    FILE *f = fopen(fileaddress, "r");
    char line[2000];
    // { == +1
    // } == -1
    int count = 0;

    while (fgets(line, 2000, f) != NULL)
    {
        for (int i = 0; i < strlen(line); i++)
        {
            if (line[i] == '{')
                count++;
            if (line[i] == '}')
                count--;
            if (count < 0)
            {
                fclose(f);
                return -1;
            }
        }
    }
    fclose(f);
    if (count != 0)
        return -1;
    return 0;
}

void process_line(char line[], char pline[])
{
    int p_i, l_i;
    p_i = l_i = 0;
    int temp;

    while (l_i < strlen(line))
    {
        if (line[l_i] == ' ' || line[l_i] == '\n')
        {
            temp = next_non_wspace_index(line, l_i);
            if (temp == -1)
                break;
            if (line[temp] == ';' || line[temp] == '{' || line[temp] == '}' || l_i == 0)
            {
                l_i = temp;
                continue;
            }
            else
            {
                for (; l_i < temp; p_i++, l_i++)
                {
                    pline[p_i] = line[l_i];
                }
            }
        }
        else
        {
            pline[p_i] = line[l_i];
            if (line[l_i] == '{' || line[l_i] == '}' || line[l_i] == ';')
            {
                l_i = next_non_wspace_index(line, l_i + 1);
            }
            else
                l_i++;

            p_i++;
        }
    }

    pline[p_i] = 0;

    if (pline[strlen(pline) - 1] == '\n')
        pline[strlen(pline) - 1] = 0;
}

int next_non_wspace_index(char str[], int i)
{
    for (; i < strlen(str); i++)
    {
        if (str[i] != ' ' && str[i] != '\n')
            return i;
    }

    return -1;
}

void indent_line(FILE *f, char str[], int *depth_ptr)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] == '{')
        {
            if (i == 0 || str[i - 1] == '{' || str[i - 1] == '}')
            {
                print_spaces(f, *depth_ptr);
                fprintf(f, "{\n");
            }
            else
                fprintf(f, " {\n");
            (*depth_ptr)++;
        }
        else if (str[i] == '}')
        {
            if (str[i - 1] != '{' && str[i - 1] != '}')
                fprintf(f, "\n");
            (*depth_ptr)--;
            print_spaces(f, *depth_ptr);
            fprintf(f, "}\n");
        }
        else
        {
            if (i == 0 || str[i - 1] == '{' || str[i - 1] == '}' || str[i - 1] == ';')
            {
                fprintf(f, "\n");
                print_spaces(f, *depth_ptr);
            }
            fprintf(f, "%c", str[i]);
        }
    }
}

void print_spaces(FILE *f, int depth)
{
    for (int i = 0; i < depth * TAB_WIDTH; i++)
    {
        fprintf(f, " ");
    }
}

int copystr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction)
{
    if (direction == 'f')
        return copystr_f(fileaddress, pos_line, pos_char, size);
    else
        return copystr_b(fileaddress, pos_line, pos_char, size);
}

int copystr_f(char fileaddress[], int pos_line, int pos_char, int size)
{
    FILE *original_file = fopen(fileaddress, "r");
    SetFileAttributesA(CLIPBOARD_FILE_NAME, FILE_ATTRIBUTE_NORMAL);
    FILE *clipboard = fopen(CLIPBOARD_FILE_NAME, "w");

    char line[2000];
    line[0] = 0;
    int cur_line = 0;
    while (cur_line < pos_line)
    {
        if (fgets(line, 2000, original_file) == NULL)
        {
            error_msg("the file doesn't have this many lines");
            fclose(original_file);
            fclose(clipboard);
            SetFileAttributesA(CLIPBOARD_FILE_NAME, FILE_ATTRIBUTE_HIDDEN);
            return -1;
        }

        // if reached end of a line (since lines can be longer than 2000 characters)
        if (line[strlen(line) - 1] == 10)
            cur_line++;
    }

    int count = 0;
    while (count < pos_char)
    {
        fgetc(original_file);
        count++;
    }

    char temp;
    for (int i = 0; i < size; i++)
    {
        temp = fgetc(original_file);
        if (temp == EOF)
        {
            error_msg("this line doesn't have enough characters");
            fclose(original_file);
            fclose(clipboard);
            SetFileAttributesA(CLIPBOARD_FILE_NAME, FILE_ATTRIBUTE_HIDDEN);
            return -1;
        }
        fputc(temp, clipboard);
    }

    fclose(original_file);
    fclose(clipboard);
    SetFileAttributesA(CLIPBOARD_FILE_NAME, FILE_ATTRIBUTE_HIDDEN);
    return 0;
}

int copystr_b(char fileaddress[], int pos_line, int pos_char, int size)
{
    FILE *original_file = fopen(fileaddress, "r");

    if (pos_line == 0)
    {
        fclose(original_file);
        return copystr_f(fileaddress, pos_line, pos_char - size, size);
    }

    int char_count[pos_line];
    for (int i = 0; i < pos_line; i++)
        char_count[i] = 0;

    char line[2000];
    int i = 0;
    while (i < pos_line)
    {
        fgets(line, 2000, original_file);
        char_count[i] += strlen(line);
        if (line[strlen(line) - 1] == '\n')
            i++;
    }

    long long sum = 0;
    int j;
    for (j = pos_line - 1; j >= 0; j--)
    {
        if (sum + char_count[j] >= size - pos_char)
            break;
        sum += char_count[j];
    }

    int pos_start = char_count[j] - (size - pos_char - sum);
    fclose(original_file);

    return copystr_f(fileaddress, j, pos_start, size);
}

int cutstr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction)
{
    if (copystr_action(fileaddress, pos_line, pos_char, size, direction) == -1)
        return -1;
    else
        return removestr_action(fileaddress, pos_line, pos_char, size, direction);
}

int pastestr_action(char fileaddress[], int pos_line, int pos_char)
{
    FILE *clipboard = fopen(CLIPBOARD_FILE_NAME, "r");
    if (clipboard == NULL)
    {
        error_msg("clipboard is empty");
        return -1;
    }

    char line[2000];
    while (1)
    {
        if (fgets(line, 2000, clipboard) == NULL)
            break;
        if (insertstr_action(fileaddress, line, pos_line, pos_char) == -1)
        {
            fclose(clipboard);
            return -1;
        }
        if (line[strlen(line) - 1] == '\n')
        {
            pos_line++;
            pos_char = 0;
        }
        else
        {
            pos_char += strlen(line);
        }
    }

    fclose(clipboard);
    return 0;
}

int pos_to_line_char(int pos, int *pos_line_ptr, int *pos_char_ptr, char fileaddress[])
{
    FILE *f = fopen(fileaddress, "r");

    int line_count = 0;
    int char_count = 0;
    int count = 0;
    char c;

    while (count < pos)
    {
        c = fgetc(f);
        if (c == '\n')
        {
            line_count++;
            char_count = -1;
        }
        count++;
        char_count++;
    }

    *pos_line_ptr = line_count;
    *pos_char_ptr = char_count;
    return 0;
}

int replace_at(char fileaddress[], char str1[], char str2[], int at)
{
    int end_index;
    int start = find_action_vanilla(str1, fileaddress, at, &end_index);
    if (start == -1)
    {
        return -1;
    }

    int pos_line, pos_char;
    pos_to_line_char(start, &pos_line, &pos_char, fileaddress);
    cur_file_char = pos_char;
    cur_file_line = pos_line;
    removestr_action(fileaddress, pos_line, pos_char, end_index - start + 1, 'f');
    insertstr_action(fileaddress, str2, pos_line, pos_char);
    return 0;
}

int replace_all(char fileaddress[], char str1[], char str2[])
{
    int start[1000] = {0};
    int length[1000] = {0};
    int end;
    int i;
    int temp;
    int offset = 0;

    // find start position and length
    for (i = 1;; i++)
    {
        temp = find_action_vanilla(str1, fileaddress, i, &end);
        if (temp == -1)
        {
            i--;
            break;
        }
        length[i - 1] = end - temp + 1;
        start[i - 1] = temp + offset;
        offset += (strlen(str2) - length[i - 1]);
    }

    // replace
    int pos_line, pos_char;
    for (int j = 0; j < i; j++)
    {
        pos_to_line_char(start[j], &pos_line, &pos_char, fileaddress);
        if (j == 0)
        {
            cur_file_char = pos_char;
            cur_file_line = pos_line;
        }
        removestr_action(fileaddress, pos_line, pos_char, length[j], 'f');
        insertstr_action(fileaddress, str2, pos_line, pos_char);
    }

    // message
    if (i == 0)
    {
        error_msg("no matches found");
        return -1;
    }
    else if (i == 1)
        printf("1 match found and replaced\n");
    else
        printf("%d matches found and replaced\n", i);
    return 0;
}

int str_input(char str[])
{
    char word[1000];
    int dq = 0;

    // first word is --str or --str1 or --str2
    fscanf(command_file, "%s ", word);

    fscanf(command_file, "%s", word);

    if (word[0] != '\"')
    {
        strcpy(str, word);
    }
    else
    {
        for (int i = 1; i < strlen(word); i++)
        {
            str[i - 1] = word[i];
        }

        int i = strlen(word) - 1;
        char c;
        while (1)
        {
            c = fgetc(command_file);
            if (c == '\"' && str[i - 1] != '\\')
                break;
            str[i] = c;
            i++;
        }
        str[i] = 0;
    }

    return 0;
}

int find()
{
    // ADD \n FEATURE

    char str[STR_MAX_LENGTH], fileaddress[1000];
    int mode, at;
    at = 1;
    mode = -1;

    int valid_input = find_input(str, &mode, &at);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int found;

    if (mode == -1)
    {
        int end_index;
        found = find_action_vanilla(str, fileaddress, at, &end_index);
        // -2 means error
        if (found == -2)
        {
            undo_action(fileaddress);
            return -1;
        }
        if (!is_arman)
            printf("%d\n", found);
        else
            sprintf(arman_result + strlen(arman_result), "%d", found);
    }
    else if (mode == COUNT_MODE)
    {
        found = find_action_count(str, fileaddress);
    }
    else if (mode == BYWORD_MODE)
    {
        found = find_action_byword(str, fileaddress, at);
        if (found == -2)
        {
            undo_action(fileaddress);
            return -1;
        }

        if (!is_arman)
            printf("%d\n", found);
        else
            sprintf(arman_result + strlen(arman_result), "%d", found);
    }
    else if (mode == ALL_MODE)
    {
        found = find_action_all(str, fileaddress);
    }
    else if (mode == ALL_BYWORD_MODE)
    {
        found = find_action_all_byword(str, fileaddress);
    }

    if (found == -1)
    {
        return -2;
    }
    return 0;
}

int find_input(char str[], int *mode_ptr, int *at_ptr)
{
    int valid_str = 0;
    if (!first_time)
        strcpy(str, prev_output);
    else
        valid_str = str_input(str);
    if (valid_str == -1)
        return -1;
    // int valid_file = file_input_by_word(fileaddress);
    // if (valid_file == -1)
    //     return -1;

    char temp;
    if (fgetc(command_file) != '\n')
    {
        char att[30];
        int count, all, byword, at;
        count = all = byword = at = 0;
        do
        {
            fscanf(command_file, "%s", att);
            if (!strcmp(att, "-at"))
            {
                at = 1;
                fscanf(command_file, "%d", at_ptr);
            }
            else if (!strcmp(att, "-count"))
            {
                count = 1;
            }
            else if (!strcmp(att, "-all"))
            {
                all = 1;
            }
            else if (!strcmp(att, "-byword"))
            {
                byword++;
            }
        } while ((temp = fgetc(command_file)) != '\n' && temp != EOF);

        if (count)
        {
            if (byword)
            {
                error_msg("count and byword can't be used together");
                return -1;
            }
            if (all)
            {
                error_msg("count and all can't be used together");
                return -1;
            }
            if (at)
            {
                error_msg("count and at can't be used together");
                return -1;
            }
            *mode_ptr = COUNT_MODE;
        }
        else if (all)
        {
            if (at)
            {
                error_msg("all and at can't be used together");
                return -1;
            }
            if (byword)
            {
                *mode_ptr = ALL_BYWORD_MODE;
            }
            else
            {
                *mode_ptr = ALL_MODE;
            }
        }
        else if (byword)
        {
            *mode_ptr = BYWORD_MODE;
        }
    }

    return 0;
}

int find_action_vanilla(char str[], char fileaddress[], int at, int *end_ptr)
{
    FILE *f = fopen(fileaddress, "r");
    if (f == NULL)
    {
        error_msg("file doesn't exist");
        return -2;
    }

    // change *'s to 0's
    char copy[STR_MAX_LENGTH];
    for (int i = 0; i < STR_MAX_LENGTH; i++)
    {
        copy[i] = str[i];
        if (str[i] == 0)
            break;
    }

    char *pos = copy;
    int star_index[1000] = {0};
    int i = 0;
    for (int k = 0; copy[k] != 0; k++)
    {
        if (copy[k] == '*' && (k == 0 || copy[k - 1] != '\\'))
        {
            star_index[i] = k;
            i++;
        }
    }
    pos = copy;
    for (int k = 0; k < i; k++)
    {
        pos[star_index[k]] = 0;
    }

    char line[STR_MAX_LENGTH];
    int start_point = -1;
    int end_point = -1;
    int result;
    int j = 0;
    int chars_so_far = 0;
    pos = copy - 1;
    fgets(line, 4000, f);
    while (1)
    {
        while (j <= i)
        {
            result = find_action_next(line, pos + 1, end_point + 1, &end_point, (j != 0));
            if (result == -1)
                break;
            else if (start_point == -1) /*searched the first part*/
                start_point = result;

            pos = copy + star_index[j] * sizeof(char);
            j++;
        }

        if (result != -1)
        {
            if (at == 1)
            {
                fclose(f);
                *end_ptr = end_point + chars_so_far;
                return start_point + chars_so_far;
            }
            else
            {
                at--;
                pos = copy - 1;
                j = 0;
                start_point = -1;
            }
        }
        else
        {
            pos = copy - 1;
            j = 0;
            start_point = -1;
        }

        // not found in this line or found but not enough times -> go to the next line
        if (/*result == -1 ||*/ end_point == strlen(line) - 1)
        {
            chars_so_far += strlen(line);
            if (fgets(line, 2000, f) == NULL)
                break;
            pos = copy - 1;
            j = 0;
            start_point = end_point = -1;
        }
    }

    fclose(f);
    *end_ptr = -1;
    return -1;
}

int find_action_next(char line[], char str[], int from, int *end_index_ptr, int star_start)
{
    // find the end index of cases like a*
    if (star_start && strlen(str) == 0)
    {
        for (int i = from;; i++)
        {
            if (i == strlen(line) || line[i] == ' ')
            {
                *end_index_ptr = i - 1;
                return from;
            }
        }
    }

    // handle \n \\ \" \' and \*
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
            else
            {
                processed_str[i_str2] = str[i_str1];
            }
        }
        else
            processed_str[i_str2] = str[i_str1];

        i_str1++;
        i_str2++;
    }

    int str_index = 0;
    int text_index = from;
    int starting_point = text_index;
    int fixed_start = -1;
    while (1)
    {
        // find the last occurence if there is a star before pattern (*a)
        if (str_index == strlen(processed_str))
        {
            if (!star_start)
                break;
            fixed_start = starting_point;
            starting_point++;
            text_index = starting_point;
            str_index = 0;
        }
        if (text_index >= strlen(line))
        {
            if (fixed_start != -1)
                str_index = strlen(processed_str);
            break;
        }
        if (line[text_index] != processed_str[str_index])
        {
            if (line[text_index] == ' ')
            {
                if (fixed_start != -1)
                {
                    str_index = strlen(processed_str);
                    break;
                }
                int next_word_index;
                for (int i = text_index;; i++)
                {
                    if (line[i] != ' ')
                    {
                        next_word_index = i;
                        break;
                    }
                }
                *end_index_ptr = next_word_index - 1;
                return -1;
            }
            else
            {
                starting_point++;
                text_index = starting_point;
                str_index = 0;
            }
        }
        else
        {
            text_index++;
            str_index++;
        }
    }

    if (str_index == strlen(processed_str))
    {
        if (!star_start)
        {
            fixed_start = starting_point;
        }
        *end_index_ptr = fixed_start + strlen(processed_str) - 1;
        return fixed_start;
    }
    else
    {
        *end_index_ptr = strlen(line) - 1;
        return -1;
    }
}

int find_action_count(char str[], char fileaddress[])
{
    int index[100] = {0};
    int at;
    int end_index;
    for (at = 1;; at++)
    {
        index[at - 1] = find_action_vanilla(str, fileaddress, at, &end_index);
        if (index[at - 1] == -1)
            break;
    }
    if (at == 1)
    {
        if (!is_arman)
            printf("-1\n");
        else
            sprintf(arman_result + strlen(arman_result), "-1");
        return -1;
    }
    if (!is_arman)
        printf("%d\n", at - 1);
    else
        sprintf(arman_result + strlen(arman_result), "%d", at - 1);
    return 0;
}

int find_action_byword(char str[], char fileaddress[], int at)
{
    FILE *f = fopen(fileaddress, "r");
    if (f == NULL)
    {
        error_msg("file doesn't exist");
        return -2;
    }

    // change *'s to 0's
    char copy[STR_MAX_LENGTH];
    for (int i = 0; i < STR_MAX_LENGTH; i++)
    {
        copy[i] = str[i];
        if (str[i] == 0)
            break;
    }

    char *pos = copy;
    int star_index[1000] = {0};
    int i = 0;
    for (int k = 0; copy[k] != 0; k++)
    {
        if (copy[k] == '*')
        {
            star_index[i] = k;
            i++;
        }
    }
    pos = copy;
    for (int k = 0; k < i; k++)
    {
        pos[star_index[k]] = 0;
    }

    char line[STR_MAX_LENGTH];
    int start_point = -1;
    int end_point = -1;
    int result;
    int j = 0;
    int chars_so_far = 0;
    pos = copy - 1;
    fgets(line, 4000, f);
    while (1)
    {
        while (j <= i)
        {
            result = find_action_next(line, pos + 1, end_point + 1, &end_point, (j != 0));
            if (result == -1)
                break;
            else if (start_point == -1) /*searched the first part*/
                start_point = result;

            pos = copy + star_index[j] * sizeof(char);
            j++;
        }

        if (result != -1)
        {
            if (at == 1)
            {
                fclose(f);
                return wordnum(fileaddress, start_point + chars_so_far);
            }
            else
            {
                at--;
                pos = copy - 1;
                j = 0;
                start_point = -1;
            }
        }
        else
        {
            pos = copy - 1;
            j = 0;
            start_point = -1;
        }

        // not found in this line or found but not enough times -> go to the next line
        if (/*result == -1 ||*/ end_point == strlen(line) - 1)
        {
            chars_so_far += strlen(line);
            if (fgets(line, 2000, f) == NULL)
                break;
            pos = copy - 1;
            j = 0;
            start_point = end_point = -1;
        }
    }

    fclose(f);
    return -1;
}

int wordnum(char fileaddress[], int pos_char)
{
    FILE *f = fopen(fileaddress, "r");

    // word index is 1-based
    int count = 1;
    char c;
    char prevc = 0;
    for (int i = 0; i < pos_char; i++)
    {
        c = fgetc(f);

        if (i == 0)
        {
            prevc = c;
            continue;
        }
        if (c == ' ' || c == '\n')
        {
            if (prevc != ' ' && prevc != '\n')
                count++;
        }

        prevc = c;
    }

    fclose(f);
    return count;
}

int find_action_all(char str[], char fileaddress[])
{
    int index[100] = {0};
    int at;
    int end_index;
    int found_next_h = 0;
    for (at = 1;; at++)
    {
        index[at - 1] = find_action_vanilla(str, fileaddress, at, &end_index);
        if (index[at - 1] == -1)
        {
            highlighted_char[at - 1] = highlighted_line[at - 1] = highlighted_length[at - 1] = -1;
            break;
        }
        else
        {
            pos_to_line_char(index[at - 1], &highlighted_line[at - 1], &highlighted_char[at - 1], fileaddress);
            highlighted_length[at - 1] = end_index - index[at - 1] + 1;
            if (!found_next_h && index[at - 1] >= line_char_to_pos(cur_file_char, cur_file_line))
            {
                found_next_h = 1;
                on_h_index = at - 1;
                next_highlighted_char = highlighted_char[at - 1];
                next_highlighted_line = highlighted_line[at - 1];
            }
        }
    }

    // no match found
    if (at == 1)
    {
        highlighted_char[0] = highlighted_line[0] = highlighted_length[0] = -1;
        on_h_index = 0;
        if (!is_arman)
            printf("-1\n");
        else
            sprintf(arman_result + strlen(arman_result), "-1");
        return -1;
    }

    for (int i = 0; i < at - 2; i++)
    {
        if (!is_arman)
            printf("%d, ", index[i]);
        else
            sprintf(arman_result + strlen(arman_result), "%d, ", index[i]);
    }
    if (!is_arman)
        printf("%d\n", index[at - 2]);
    else
        sprintf(arman_result + strlen(arman_result), "%d", index[at - 2]);
    return 0;
}

int find_action_all_byword(char str[], char fileaddress[])
{
    int index[100] = {0};
    int at;
    for (at = 1;; at++)
    {
        index[at - 1] = find_action_byword(str, fileaddress, at);
        if (index[at - 1] == -1)
            break;
    }

    // no match found
    if (at == 1)
    {
        if (!is_arman)
            printf("-1\n");
        else
            sprintf(arman_result + strlen(arman_result), "-1");
        return -1;
    }

    for (int i = 0; i < at - 2; i++)
    {
        if (!is_arman)
            printf("%d, ", index[i]);
        else
            sprintf(arman_result + strlen(arman_result), "%d, ", index[i]);
    }
    if (!is_arman)
        printf("%d\n", index[at - 2]);
    else
        sprintf(arman_result + strlen(arman_result), "%d", index[at - 2]);
    return 0;
}

int pos_input(int *pos_line_ptr, int *pos_char_ptr)
{
    *pos_line_ptr = *pos_char_ptr = -1;

    // first word is --pos
    char word[100];
    fscanf(command_file, "%s", word);

    if (fscanf(command_file, "%d:%d", pos_line_ptr, pos_char_ptr) != 2)
    {
        error_msg("give the position with this format: --pos <line no>:<start position>");
        return -1;
    }
    if (*pos_line_ptr <= 0)
    {
        error_msg("the line number can't be zero or negative");
        return -1;
    }
    if (*pos_char_ptr < 0)
    {
        error_msg("the starting position can't be negative");
        return -1;
    }

    // make the line number zero-based
    (*pos_line_ptr)--;

    return 0;
}