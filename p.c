#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <direct.h>
#include <windows.h>

#define TEMP_FILE_NAME ".tempfile"
#define CLIPBOARD_FILE_NAME ".clipboard"

#define STR_MAX_LENGTH 4000

#define CREATEFILE_CODE 0
#define INSERTSTR_CODE 1
#define CAT_CODE 2

#define COUNT_MODE 0
#define BYWORD_MODE 1
#define ALL_MODE 2
#define ALL_BYWORD_MODE 3

#define RLIST_FILE_NAME ".removelist"
#define RECORD_COUNT_EXT "_UNDO_COUNT"
#define UNDO_EXT "_UNDO_NO_"

#define TAB_WIDTH 4

void create_remove_file();
void getcommand(char com[]);
void end_of_command();
int newline_handle();
void error_msg(char *message);
int track_changes(char fileaddress[]);
void cleanup();

int createfile();
int file_input(char fileaddress[], int caller_id);
int createfile_action(char fileaddress[]);
int makepath(char fileaddress[]);

int cat();
int cat_action(char fileaddress[]);

int insertstr();
int insertstr_input(char fileaddress[], char str[], int *pos_line, int *pos_char);
int file_input_by_word(char fileaddress[]);
int str_input(char str[]);
int pos_input(int *pos_line_ptr, int *pos_char_ptr);
int insertstr_action(char fileaddress[], char str[], int pos_line, int pos_char);

int removestr();
int removestr_input(char fileaddress[], int *pos_line_ptr, int *pos_char_ptr, int *size_ptr, char *direction_ptr);
int size_input(int *size_ptr);
int direction_input(char *direction_ptr);
int removestr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction);
int removestr_f(char fileaddress[], int pos_line, int pos_char, int size);
int removestr_b(char fileaddress[], int pos_line, int pos_char, int size);

int copystr();
int copystr_input(char fileaddress[], int *pos_line_ptr, int *pos_char_ptr, int *size_ptr, char *direction_ptr);
int copystr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction);
int copystr_f(char fileaddress[], int pos_line, int pos_char, int size);
int copystr_b(char fileaddress[], int pos_line, int pos_char, int size);

int cutstr();
int cutstr_input(char fileaddress[], int *pos_line_ptr, int *pos_char_ptr, int *size_ptr, char *direction_ptr);
int cutstr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction);

int pastestr();
int pastestr_input(char fileaddress[], int *pos_line_ptr, int *pos_char_ptr);
int pastestr_action(char fileaddress[], int pos_line, int pos_char);

int find();
int find_input(char str[], char fileaddress[], int *mode_ptr, int *at_ptr);
int find_action_vanilla(char str[], char fileaddress[], int at, int *end_ptr);
int find_action_next(char line[], char str[], int from, int *end_index_ptr, int star_start);
int find_action_count(char str[], char fileaddress[]);
int find_action_byword(char str[], char fileaddress[], int at);
int wordnum(char fileaddress[], int pos_char);
int find_action_all(char str[], char fileaddress[]);
int find_action_all_byword(char str[], char fileaddress[]);

int replace();
int replace_input(char fileaddress[], char str1[], char str2[], int *mode_ptr, int *at_ptr);
int pos_to_line_char(int pos, int *pos_line_ptr, int *pos_char_ptr, char fileaddress[]);
int replace_at(char fileaddress[], char str1[], char str2[], int at);
int replace_all(char fileaddress[], char str1[], char str2[]);

int tree();
int tree_action(char path[], int depth, int count, char branch[]);

int grep();
int grep_input(char str[], char *opt_ptr);
int grep_get_next_file(char fileaddress[]);
int grep_action(char fileaddress[], char str[], char opt);
int grep_search(char fileaddress[], char str[], char opt);

int undo();
int undo_action(char fileaddress[]);

int compare();
int compare_file_input(char fieladdress[]);
int compare_action(char fileaddress1[], char fileaddress2[]);
int compare_line_print(char line1[], char line2[]);

int auto_indent();
int auto_indent_action(char fileaddress[]);
int check_brace_validity(char fileaddress[]);
void process_line(char line[], char pline[]);
int next_non_wspace_index(char str[], int i);
void indent_line(FILE *f, char str[], int *depth_ptr);
void print_spaces(FILE *f, int depth);

int arman();

FILE *command_file;
int is_arman = 0;
int first_time = 1;
char arman_result[10000];
char prev_output[10000];

int main()
{
    create_remove_file();
    char command[100];

    while (1)
    {
        printf("> ");
        getcommand(command);

        if (!strcmp(command, "createfile"))
        {
            createfile();
        }
        else if (!strcmp(command, "insertstr"))
        {
            insertstr();
        }
        else if (!strcmp(command, "cat"))
        {
            cat();
        }
        else if (!strcmp(command, "removestr"))
        {
            removestr();
        }
        else if (!strcmp(command, "copystr"))
        {
            copystr();
        }
        else if (!strcmp(command, "cutstr"))
        {
            cutstr();
        }
        else if (!strcmp(command, "pastestr"))
        {
            pastestr();
        }
        else if (!strcmp(command, "find"))
        {
            find();
        }
        else if (!strcmp(command, "replace"))
        {
            replace();
        }
        else if (!strcmp(command, "tree"))
        {
            tree();
        }
        else if (!strcmp(command, "grep"))
        {
            grep();
        }
        else if (!strcmp(command, "undo"))
        {
            undo();
        }
        else if (!strcmp(command, "compare"))
        {
            compare();
        }
        else if (!strcmp(command, "auto-indent"))
        {
            auto_indent();
        }
        else if (!strcmp(command, "arman"))
        {
            arman();
        }
        else if (!strcmp(command, "exit"))
        {
            cleanup();
            break;
        }
        else
        {
            error_msg("invalid input");
        }

        end_of_command();
    }
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

void getcommand(char com[])
{
    command_file = fopen(".commandfile", "w");
    char full_line[6000];
    gets(full_line);
    fprintf(command_file, full_line);
    fclose(command_file);

    command_file = fopen(".commandfile", "r");
    if (strstr(full_line, "=D") != NULL)
        strcpy(com, "arman");
    else
        fscanf(command_file, "%s", com);
}

void end_of_command()
{
    fclose(command_file);
}

int newline_handle()
{
    if (fgetc(command_file) == '\n')
        return 1;

    // this function is called after scanf(%s) -> the input character is either '\n' or ' '
    return 0;
}

void error_msg(char *message)
{
    printf("%s\n", message);
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

int createfile()
{
    char fileaddress[1000];
    int valid_input = file_input(fileaddress, CREATEFILE_CODE);
    if (valid_input == -1)
    {
        return -1;
    }

    int valid_action = createfile_action(fileaddress);
    if (valid_action == -1)
    {
        return -1;
    }

    return 0;
}

int file_input(char fileaddress[], int caller_code)
{
    // nothing entered after the command's first word
    if (newline_handle() == 1)
    {
        if (caller_code == CREATEFILE_CODE)
            error_msg("enter createfile --file <file path> to make a file");
        else if (caller_code == CAT_CODE)
            error_msg("enter cat --file <file path> to view the contents of a file");
        return -1;
    }

    char keyword[1000];
    fscanf(command_file, "%s", keyword);

    // check input validity (-1 is invalid)
    if (strcmp(keyword, "--file"))
    {
        error_msg("invalid input");
        error_msg("enter --file to specify the file address");
        return -1;
    }

    // incomplete input
    if (newline_handle() == 1)
    {
        error_msg("invalid input");
        error_msg("enter the file address after --file");
        return -1;
    }

    fgets(fileaddress, 1000, command_file);
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

int cat()
{
    char fileaddress[1000];
    int valid_input = file_input(fileaddress, CAT_CODE);

    if (valid_input == -1)
    {
        return -1;
    }

    track_changes(fileaddress);

    int valid_action = cat_action(fileaddress);
    if (valid_action == -1)
    {
        return -1;
    }

    return 0;
}

int cat_action(char fileaddress[])
{
    FILE *f = fopen(fileaddress, "r");
    if (f == NULL)
    {
        error_msg("file doesn't exist");
        return -1;
    }

    char line[1000];

    while (fgets(line, 1000, f) != NULL)
    {
        if (!is_arman)
            printf("%s", line);
        else
            sprintf(arman_result + strlen(arman_result), line);
    }
    if (!is_arman)
        printf("\n");
    fclose(f);
    return 0;
}

int insertstr()
{
    char fileaddress[1000], str[STR_MAX_LENGTH];
    int pos_line, pos_char;

    fileaddress[0] = str[0] = 0;
    int valid_input = insertstr_input(fileaddress, str, &pos_line, &pos_char);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int valid_action = insertstr_action(fileaddress, str, pos_line, pos_char);
    if (valid_action == -1)
        return -1;

    return 0;
}

int insertstr_input(char fileaddress[], char str[], int *pos_line_ptr, int *pos_char_ptr)
{
    int valid_file = file_input_by_word(fileaddress);
    if (valid_file == -1)
        return -1;

    int valid_str = 0;
    if (!first_time)
    {
        strcpy(str, prev_output);
    }
    else
        valid_str = str_input(str);

    if (valid_str == -1)
        return -1;

    int valid_pos = pos_input(pos_line_ptr, pos_char_ptr);
    if (valid_pos == -1)
        return -1;

    return 0;
}

int file_input_by_word(char fileaddress[])
{
    char word[1000];

    // first word is --file
    fscanf(command_file, "%s ", word);

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

    FILE *f = fopen(fileaddress, "r");
    if (f == NULL)
    {
        error_msg("this file doesn't exist");
        return -1;
    }
    else
    {
        fclose(f);
    }

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
        error_msg("the start position can't be negative");
        return -1;
    }

    // make the line number zero-based
    (*pos_line_ptr)--;

    return 0;
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

int removestr()
{
    char fileaddress[1000];
    int pos_line, pos_char, size;
    char direction;

    int valid_input = removestr_input(fileaddress, &pos_line, &pos_char, &size, &direction);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int valid_action = removestr_action(fileaddress, pos_line, pos_char, size, direction);
    if (valid_action == -1)
        return -1;

    return 0;
}

int removestr_input(char fileaddress[], int *pos_line_ptr, int *pos_char_ptr, int *size_ptr, char *direction_ptr)
{
    int valid_file = file_input_by_word(fileaddress);
    if (valid_file == -1)
        return -1;

    int valid_pos = pos_input(pos_line_ptr, pos_char_ptr);
    if (valid_pos == -1)
        return -1;

    int valid_size = size_input(size_ptr);
    if (valid_size == -1)
        return -1;

    int valid_dir = direction_input(direction_ptr);
    if (valid_dir == -1)
        return -1;

    return 0;
}

int size_input(int *size_ptr)
{
    char word[1000];

    // -size
    fscanf(command_file, "%s", word);

    fscanf(command_file, "%d", size_ptr);
    return 0;
}

int direction_input(char *direction_ptr)
{
    char in[100];
    fscanf(command_file, "%s", in);
    if (!strcmp(in, "-f"))
    {
        *direction_ptr = 'f';
        return 0;
    }
    if (!strcmp(in, "-b"))
    {
        *direction_ptr = 'b';
        return 0;
    }

    error_msg("enter -f or -b to specify the direction of removal");
    return -1;
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
    while (count < pos_char)
    {
        fputc(fgetc(original_file), temp_file);
        count++;
    }
    for (int i = 0; i < size; i++)
        fgetc(original_file);

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
        return removestr_f(fileaddress, pos_line, pos_char - size, size);
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

    return removestr_f(fileaddress, j, pos_start, size);
}

int copystr()
{
    char fileaddress[1000];
    int pos_line, pos_char, size;
    char direction;

    int valid_input = copystr_input(fileaddress, &pos_line, &pos_char, &size, &direction);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int valid_action = copystr_action(fileaddress, pos_line, pos_char, size, direction);
    if (valid_action == -1)
        return -1;

    return 0;
}

int copystr_input(char fileaddress[], int *pos_line_ptr, int *pos_char_ptr, int *size_ptr, char *direction_ptr)
{
    return removestr_input(fileaddress, pos_line_ptr, pos_char_ptr, size_ptr, direction_ptr);
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

int cutstr()
{
    char fileaddress[1000];
    int pos_line, pos_char, size;
    char direction;

    int valid_input = cutstr_input(fileaddress, &pos_line, &pos_char, &size, &direction);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int valid_action = cutstr_action(fileaddress, pos_line, pos_char, size, direction);
    if (valid_action == -1)
        return -1;

    return 0;
}

int cutstr_input(char fileaddress[], int *pos_line_ptr, int *pos_char_ptr, int *size_ptr, char *direction_ptr)
{
    return removestr_input(fileaddress, pos_line_ptr, pos_char_ptr, size_ptr, direction_ptr);
}

int cutstr_action(char fileaddress[], int pos_line, int pos_char, int size, char direction)
{
    if (copystr_action(fileaddress, pos_line, pos_char, size, direction) == -1)
        return -1;
    else
        return removestr_action(fileaddress, pos_line, pos_char, size, direction);
}

int pastestr()
{
    char fileaddress[1000];
    int pos_line, pos_char, size;
    char direction;

    int valid_input = pastestr_input(fileaddress, &pos_line, &pos_char);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int valid_action = pastestr_action(fileaddress, pos_line, pos_char);
    if (valid_action == -1)
        return -1;

    return 0;
}

int pastestr_input(char fileaddress[], int *pos_line_ptr, int *pos_char_ptr)
{
    int valid_file = file_input_by_word(fileaddress);
    if (valid_file == -1)
        return -1;

    int valid_pos = pos_input(pos_line_ptr, pos_char_ptr);
    if (valid_pos == -1)
        return -1;

    return 0;
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
            error_msg("unknown error");
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

int find()
{
    // ADD \n FEATURE

    char str[STR_MAX_LENGTH], fileaddress[1000];
    int mode, at;
    at = 1;
    mode = -1;

    int valid_input = find_input(str, fileaddress, &mode, &at);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int found;

    if (mode == -1)
    {
        int end_index;
        found = find_action_vanilla(str, fileaddress, at, &end_index);
        if (found == -2)
            return -1;
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
            return -1;

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
        return -1;
    return 0;
}

int find_input(char str[], char fileaddress[], int *mode_ptr, int *at_ptr)
{
    int valid_str = 0;
    if (!first_time)
        strcpy(str, prev_output);
    else
        valid_str = str_input(str);
    if (valid_str == -1)
        return -1;
    int valid_file = file_input_by_word(fileaddress);
    if (valid_file == -1)
        return -1;

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
    for (at = 1;; at++)
    {
        index[at - 1] = find_action_vanilla(str, fileaddress, at, &end_index);
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

int replace()
{
    char fileaddress[1000], str1[STR_MAX_LENGTH], str2[STR_MAX_LENGTH];
    int mode = 0, at = 1;

    int valid_input = replace_input(fileaddress, str1, str2, &mode, &at);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    if (mode == 0)
    {
        if (replace_at(fileaddress, str1, str2, at) == -1)
        {
            error_msg("match not found");
            return -1;
        }
        else
        {
            printf("match found and replaced\n");
            return 0;
        }
    }
    else
    {
        int temp = replace_all(fileaddress, str1, str2);
        return temp;
    }
}

int replace_input(char fileaddress[], char str1[], char str2[], int *mode_ptr, int *at_ptr)
{
    int valid_str1 = 1;
    if (!first_time)
        strcpy(str1, prev_output);
    else
        valid_str1 = str_input(str1);
    if (valid_str1 == -1)
    {
        return -1;
    }

    int valid_str2 = str_input(str2);
    if (valid_str2 == -1)
    {
        return -1;
    }

    int valid_file = file_input_by_word(fileaddress);
    if (valid_file == -1)
    {
        return -1;
    }

    // mode
    char c = fgetc(command_file);
    if (c == '\n')
        return 0;
    char word[100];
    fscanf(command_file, "%s", word);
    if (!strcmp(word, "-at"))
    {
        fscanf(command_file, "%d", at_ptr);
    }
    else if (!strcmp(word, "-all"))
    {
        *mode_ptr = 1;
    }

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

int tree()
{
    int depth = 0;
    fscanf(command_file, "%d", &depth);
    if (depth < -1)
    {
        error_msg("invalid depth");
        return -1;
    }

    char path[1000] = {0};
    char branch[100] = {0};
    strcpy(path, "root");

    if (depth != -1)
        tree_action(path, depth, 0, branch);
    else
        tree_action(path, 1001, 0, branch);
}

int tree_action(char path[], const int depth, int count, char branch[])
{
    // reached depth
    if (count > depth)
        return 0;

    DIR *d = opendir(path);
    // reached a file
    if (d == NULL)
        return 0;

    // . and ..
    struct dirent *data = readdir(d);
    data = readdir(d);
    int num = 0;
    while ((data = readdir(d)) != NULL)
    {
        num++;
    }
    closedir(d);

    d = opendir(path);
    data = readdir(d);
    data = readdir(d);
    char next_path[1000] = {0};
    char this_branch[100] = {0};
    char next_branch[100] = {0};
    while ((data = readdir(d)) != NULL)
    {
        if (count != 0)
        {
            strcpy(this_branch, branch);
            if (num == 1)
            {
                strcat(this_branch, "L---");
                strcpy(next_branch, branch);
                strcat(next_branch, "    ");
            }
            else
            {
                strcat(this_branch, "T---");
                strcpy(next_branch, branch);
                strcat(next_branch, "|   ");
            }
        }

        for (int i = 0; i < strlen(this_branch); i++)
        {
            // PROBLEM IN WRITING AND SIZE OF CHARACTERS??
            if (this_branch[i] == 'T')
            {
                if (!is_arman)
                    printf("%c", 195);
                else
                    sprintf(arman_result + strlen(arman_result), "|");
            }
            else if (this_branch[i] == 'L')
            {
                if (!is_arman)
                    printf("%c", 192);
                else
                    sprintf(arman_result + strlen(arman_result), "L");
            }
            else if (this_branch[i] == '|')
            {
                if (!is_arman)
                    printf("%c", 179);
                else
                    sprintf(arman_result + strlen(arman_result), "|");
            }
            else if (this_branch[i] == '-')
            {
                if (!is_arman)
                    printf("%c", 196);
                else
                    sprintf(arman_result + strlen(arman_result), ".");
            }
            else
            {
                if (!is_arman)
                    printf(" ");
                else
                    sprintf(arman_result + strlen(arman_result), " ");
            }
        }

        if (!is_arman)
            printf("%s\n", data->d_name);
        else
            sprintf(arman_result + strlen(arman_result), "%s\n", data->d_name);

        strcpy(next_path, path);
        strcat(next_path, "/");
        strcat(next_path, data->d_name);
        num--;
        tree_action(next_path, depth, count + 1, next_branch);
    }
    closedir(d);

    return 0;
}

int grep()
{
    char fileaddress[1000] = {0};
    char str[STR_MAX_LENGTH];

    // none, c, l
    char opt = 'n';

    int valid_input = grep_input(str, &opt);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int valid_action = grep_action(fileaddress, str, opt);
    if (valid_action == -1)
        return -1;

    return 0;
}

int grep_input(char str[], char *opt_ptr)
{
    char word[1000];
    fscanf(command_file, "%s", word);
    if (!strcmp(word, "--str") || !strcmp(word, "--files"))
    {
        *opt_ptr = 'n';
    }
    else
    {
        if (!strcmp(word, "-c"))
            *opt_ptr = 'c';
        else
            *opt_ptr = 'l';

        // get --str
        if (first_time)
            fscanf(command_file, "%s", word);
    }

    // get string
    if (!first_time)
        strcpy(str, prev_output);
    else
    {
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
    }

    // get --files
    fscanf(command_file, "%s", word);
    return 0;
}

int grep_get_next_file(char fileaddress[])
{
    if (fgetc(command_file) == '\n')
        return -1;
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

    FILE *f = fopen(fileaddress, "r");
    if (f == NULL)
    {
        error_msg("this file doesn't exist");
        return -1;
    }
    else
    {
        fclose(f);
    }
    return 0;
}

int grep_action(char fileaddress[], char str[], char opt)
{
    int count = 0;

    while (grep_get_next_file(fileaddress) != -1)
    {
        count += grep_search(fileaddress, str, opt);
    }

    if (count == 0)
        error_msg("no cases found");
    else if (opt == 'c')
    {
        if (count == 1)
        {
            if (!is_arman)
                printf("1 case found\n");
            else
                sprintf(arman_result + strlen(arman_result), "1 case found");
        }
        else
        {
            if (!is_arman)
                printf("%d cases found\n", count);
            else
                sprintf(arman_result + strlen(arman_result), "%d cases found", count);
        }
    }

    return 0;
}

int grep_search(char fileaddress[], char str[], char opt)
{
    FILE *f = fopen(fileaddress, "r");

    int num = 0;
    // LINE LONGER THAN 4000 CHARACTERS
    char line[4000];
    int line_count = 0;
    while (fgets(line, 4000, f))
    {
        line_count++;
        if (strstr(line, str) == NULL)
        {
            continue;
        }

        num++;
        if (opt == 'n')
        {
            // prevent 2 enters in a row
            if (line[strlen(line) - 1] == '\n')
                line[strlen(line) - 1] = 0;
            if (!is_arman)
                printf("[%s] <#%d>: %s\n", fileaddress, line_count, line);
            else
                sprintf(arman_result + strlen(arman_result), "[%s] <#%d>: %s\n", fileaddress, line_count, line);
        }
        else if (opt == 'l')
        {
            if (num == 1)
            {
                if (!is_arman)
                    printf("%s\n", fileaddress);
                else
                    sprintf(arman_result + strlen(arman_result), "%s\n", fileaddress);
            }
        }
    }

    fclose(f);
    return num;
}

int undo()
{
    char fileaddress[1000];
    int valid_input = file_input_by_word(fileaddress);
    if (valid_input == -1)
        return -1;
    int valid_action = undo_action(fileaddress);
    if (valid_action == -1)
        return -1;
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

int compare()
{
    char fileaddress1[1000], fileaddress2[1000];
    int valid_file1 = compare_file_input(fileaddress1);
    int valid_file2 = compare_file_input(fileaddress2);
    if (valid_file1 == -1 || valid_file2 == -1)
        return -1;

    int valid_action = compare_action(fileaddress1, fileaddress2);
    if (valid_action == -1)
        return -1;
    return 0;
}

int compare_file_input(char fileaddress[])
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

    FILE *f = fopen(fileaddress, "r");
    if (f == NULL)
    {
        error_msg("this file doesn't exist");
        return -1;
    }
    else
    {
        fclose(f);
    }

    return 0;
}

int compare_action(char fileaddress1[], char fileaddress2[])
{
    FILE *f1 = fopen(fileaddress1, "r");
    FILE *f2 = fopen(fileaddress2, "r");

    char line1[2000];
    char line2[2000];
    int reached_end1, reached_end2;
    reached_end1 = reached_end2 = 0;
    int line_num = 1;

    while (1)
    {
        if (fgets(line1, 2000, f1) == NULL)
            reached_end1 = 1;
        if (fgets(line2, 2000, f2) == NULL)
            reached_end2 = 1;
        if (reached_end1 || reached_end2)
            break;

        if (line1[strlen(line1) - 1] == '\n')
            line1[strlen(line1) - 1] = 0;
        if (line2[strlen(line2) - 1] == '\n')
            line2[strlen(line2) - 1] = 0;

        if (strcmp(line1, line2))
        {
            if (!is_arman)
                printf("============ #%d ============\n", line_num);
            else
                sprintf(arman_result + strlen(arman_result), "============ #%d ============\n", line_num);
            compare_line_print(line1, line2);
        }
        line_num++;
    }

    if (reached_end1 && reached_end2)
        return 0;

    int start = line_num;
    int end = line_num;
    char longer_file_address[1000];

    if (reached_end1 && !reached_end2)
    {
        while (fgets(line2, 2000, f2) != NULL)
            end++;
        if (!is_arman)
            printf(">>>>>>>>>>>> #%d - #%d >>>>>>>>>>>>\n", start, end);
        else
            sprintf(arman_result + strlen(arman_result), ">>>>>>>>>>>> #%d - #%d >>>>>>>>>>>>\n", start, end);

        strcpy(longer_file_address, fileaddress2);
    }
    else if (!reached_end1 && reached_end2)
    {
        while (fgets(line1, 2000, f1) != NULL)
            end++;

        if (!is_arman)
            printf("<<<<<<<<<<<< #%d - #%d <<<<<<<<<<<<\n", start, end);
        else
            sprintf(arman_result + strlen(arman_result), "<<<<<<<<<<<< #%d - #%d <<<<<<<<<<<<\n", start, end);

        strcpy(longer_file_address, fileaddress1);
    }
    fclose(f1);
    fclose(f2);

    FILE *longer_file = fopen(longer_file_address, "r");
    int count = 0;

    while (fgets(line1, 2000, longer_file) != NULL)
    {
        count++;
        if (count < start)
        {
            continue;
        }
        if (!is_arman)
            printf("%s", line1);
        else
            sprintf(arman_result + strlen(arman_result), "%s", line1);
    }
    if (!is_arman)
        printf("\n");
    else
        sprintf(arman_result + strlen(arman_result), "\n");
    fclose(longer_file);
    return 0;
}

int compare_line_print(char line1[], char line2[])
{
    int space_index1[1000] = {0};
    int space_index2[1000] = {0};

    space_index1[0] = -1;
    int s_index = 1;
    for (int i = 0; i < strlen(line1); i++)
    {
        if (line1[i] == ' ')
        {
            space_index1[s_index] = i;
            s_index++;
        }
    }
    space_index1[s_index] = strlen(line1);
    space_index1[s_index + 1] = -2;

    space_index2[0] = -1;
    s_index = 1;
    for (int i = 0; i < strlen(line2); i++)
    {
        if (line2[i] == ' ')
        {
            space_index2[s_index] = i;
            s_index++;
        }
    }
    space_index2[s_index] = strlen(line2);
    space_index2[s_index + 1] = -2;

    int s1, s2;
    s1 = s2 = 0;
    int count_diff = 0;
    int len1, len2;
    int equal;
    int start1, end1, start2, end2;
    while (space_index1[s1 + 1] != -2 && space_index2[s2 + 1] != -2)
    {
        len1 = space_index1[s1 + 1] - space_index1[s1] - 1;
        len2 = space_index2[s2 + 1] - space_index2[s2] - 1;

        // multiple spaces in a row
        if (len1 == 0)
        {
            s1++;
            continue;
        }
        if (len2 == 0)
        {
            s2++;
            continue;
        }

        // words have different lengths
        if (len1 != len2)
        {
            count_diff++;

            start1 = space_index1[s1] + 1;
            end1 = start1 + len1 - 1;

            start2 = space_index2[s2] + 1;
            end2 = start2 + len2 - 1;

            if (count_diff > 1)
            {
                break;
            }
            else
            {
                s1++;
                s2++;
                continue;
            }
        }

        equal = 1;
        for (int i = 1; i <= len1; i++)
        {
            if (line1[space_index1[s1] + i] != line2[space_index2[s2] + i])
            {
                equal = 0;
                break;
            }
        }

        if (!equal)
        {
            count_diff++;

            start1 = space_index1[s1] + 1;
            end1 = start1 + len1 - 1;

            start2 = space_index2[s2] + 1;
            end2 = start2 + len2 - 1;

            if (count_diff > 1)
            {
                break;
            }
        }

        s1++;
        s2++;
    }

    // more than one different word (count_diff>1)
    // only different in number of spaces (count_diff==0)
    // different number of words
    if (count_diff != 1 || space_index1[s1 + 1] != -2 || space_index2[s2 + 1] != -2)
    {
        if (!is_arman)
        {
            printf("%s\n", line1);
            printf("%s\n", line2);
        }
        else
        {
            sprintf(arman_result + strlen(arman_result), "%s\n", line1);
            sprintf(arman_result + strlen(arman_result), "%s\n", line2);
        }
        return 0;
    }

    for (int i = 0; i < strlen(line1); i++)
    {
        if (i == start1)
        {
            if (!is_arman)
                printf(">>");
            else
                sprintf(arman_result + strlen(arman_result), ">>");
        }
        if (!is_arman)
            printf("%c", line1[i]);
        else
            sprintf(arman_result + strlen(arman_result), "%c", line1[i]);

        if (i == end1)
        {
            if (!is_arman)
                printf("<<");
            else
                sprintf(arman_result + strlen(arman_result), "<<");
        }
    }
    if (!is_arman)
        printf("\n");
    else
        sprintf(arman_result + strlen(arman_result), "\n");

    for (int i = 0; i < strlen(line2); i++)
    {
        if (i == start2)
        {
            if (!is_arman)
                printf(">>");
            else
                sprintf(arman_result + strlen(arman_result), ">>");
        }
        if (!is_arman)
            printf("%c", line2[i]);
        else
            sprintf(arman_result + strlen(arman_result), "%c", line2[i]);

        if (i == end2)
        {
            if (!is_arman)
                printf("<<");
            else
                sprintf(arman_result + strlen(arman_result), "<<");
        }
    }
    if (!is_arman)
        printf("\n");
    else
        sprintf(arman_result + strlen(arman_result), "\n");
}

int arman()
{
    is_arman = 1;
    first_time = 1;

    char command_line[6000];
    fgets(command_line, 6000, command_file);
    fclose(command_file);
    char *command_pos = command_line;
    char *prev_command_pos = command_line;
    char command[100];

    while ((command_pos = strstr(prev_command_pos, " =D ")) != NULL)
    {
        command_file = fopen(".commandfile", "w");
        *command_pos = 0;
        fprintf(command_file, "%s\n", prev_command_pos);
        fclose(command_file);
        command_file = fopen(".commandfile", "r");
        fscanf(command_file, "%s", command);
        arman_result[0] = 0;

        if (!strcmp(command, "cat"))
        {
            cat();
        }
        else if (!strcmp(command, "find"))
        {
            find();
        }
        else if (!strcmp(command, "tree"))
        {
            tree();
        }
        else if (!strcmp(command, "grep"))
        {
            grep();
        }
        else if (!strcmp(command, "compare"))
        {
            compare();
        }
        else
        {
            error_msg("invalid input");
            return -1;
        }
        first_time = 0;
        prev_command_pos = command_pos + 4;
        strcpy(prev_output, arman_result);
        fclose(command_file);
    }

    is_arman = 0;

    command_file = fopen(".commandfile", "w");
    fprintf(command_file, "%s\n", prev_command_pos);
    fclose(command_file);
    command_file = fopen(".commandfile", "r");
    fscanf(command_file, "%s", command);
    arman_result[0] = 0;
    if (!strcmp(command, "insertstr"))
    {
        insertstr();
    }
    else if (!strcmp(command, "find"))
    {
        find();
    }
    else if (!strcmp(command, "replace"))
    {
        replace();
    }
    else if (!strcmp(command, "grep"))
    {
        grep();
    }
    else
    {
        error_msg("invalid input");
    }
    fclose(command_file);

    first_time = 1;
    return 0;
}

int auto_indent()
{
    // auto-indent should start with --file
    char fileaddress[1000];
    int valid_input = file_input_by_word(fileaddress);
    if (valid_input == -1)
        return -1;

    track_changes(fileaddress);

    int valid_action = auto_indent_action(fileaddress);
    if (valid_action == -1)
        return -1;

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
            if (line[temp] == '{' || line[temp] == '}' || l_i == 0)
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
            if (line[l_i] == '{' || line[l_i] == '}')
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
        // else if (str[i] == ';')
        // {
        //     if (str[i + 1] != '}')
        //     {
        //         fprintf(f, ";\n");
        //         print_spaces(f, *depth_ptr);
        //     }
        //     else
        //         fprintf(f, ";");
        // }
        else
        {
            if (i == 0 || str[i - 1] == '{' || str[i - 1] == '}')
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