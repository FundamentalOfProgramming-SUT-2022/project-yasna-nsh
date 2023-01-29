#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>

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

void getcommand(char com[]);
void error_msg(char *message);
int newline_handle();
void clearline();

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

int main()
{
    char command[1000];
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
        else if (!strcmp(command, "exit"))
        {
            // ADD EXIT FUNCTION TO CONTROL INPUTS LIKE "exit slfkj lsfjk lsjkf"
            break;
        }
        else
        {
            error_msg("invalid input");
            clearline();
        }
    }
}

void getcommand(char com[])
{
    scanf("%s", com);
}

void error_msg(char *message)
{
    printf("%s\n", message);
}

// next character newline (1) or space (0)
int newline_handle()
{
    if (getchar() == '\n')
        return 1;

    // this function is called after scanf(%s) -> the input character is either '\n' or ' '
    return 0;
}

void clearline()
{
    if (newline_handle() == 0)
    {
        char tmp[1000];
        gets(tmp);
    }
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
    scanf("%s", keyword);

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

    gets(fileaddress);
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
    FILE *test = fopen(filename, "w+");
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
        printf("%s", line);
    }
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

    int valid_str = str_input(str);
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
    scanf("%s ", word);

    scanf("%s", word);

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
            c = getchar();
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
        clearline();
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
    scanf("%s ", word);

    scanf("%s", word);

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
            c = getchar();
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
    scanf("%s", word);

    if (scanf("%d:%d", pos_line_ptr, pos_char_ptr) != 2)
    {
        error_msg("give the position with this format: --pos <line no>:<start position>");
        return -1;
    }
    if (*pos_line_ptr <= 0)
    {
        error_msg("the line number can't be zero or negative");
        clearline();
        return -1;
    }
    if (*pos_char_ptr < 0)
    {
        error_msg("the start position can't be negative");
        clearline();
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
    scanf("%s", word);

    scanf("%d", size_ptr);
    return 0;
}

int direction_input(char *direction_ptr)
{
    char in[100];
    scanf("%s", in);
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
            remove(CLIPBOARD_FILE_NAME);
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
    for (int i = 0; i < size; i++)
        fputc(fgetc(original_file), clipboard);

    fclose(original_file);
    fclose(clipboard);
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

    int found;

    if (mode == -1)
    {
        int end_index;
        found = find_action_vanilla(str, fileaddress, at, &end_index);
        if (found == -2)
            return -1;

        printf("%d\n", found);
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

        printf("%d\n", found);
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
    int valid_str = str_input(str);
    if (valid_str == -1)
        return -1;
    int valid_file = file_input_by_word(fileaddress);
    if (valid_file == -1)
        return -1;
    if (getchar() != '\n')
    {
        char att[30];
        int count, all, byword, at;
        count = all = byword = at = 0;
        do
        {
            scanf("%s", att);
            if (!strcmp(att, "-at"))
            {
                at = 1;
                scanf("%d", at_ptr);
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
        } while (getchar() != '\n');

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
        printf("-1\n");
        return -1;
    }
    printf("%d\n", at - 1);
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
        printf("-1\n");
        return -1;
    }

    for (int i = 0; i < at - 2; i++)
    {
        printf("%d, ", index[i]);
    }
    printf("%d\n", index[at - 2]);
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
        printf("-1\n");
        return -1;
    }

    for (int i = 0; i < at - 2; i++)
    {
        printf("%d, ", index[i]);
    }
    printf("%d\n", index[at - 2]);
    return 0;
}

int replace()
{
    char fileaddress[1000], str1[STR_MAX_LENGTH], str2[STR_MAX_LENGTH];
    int mode = 0, at = 1;

    int valid_input = replace_input(fileaddress, str1, str2, &mode, &at);
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
        return replace_all(fileaddress, str1, str2);
}

int replace_input(char fileaddress[], char str1[], char str2[], int *mode_ptr, int *at_ptr)
{
    int valid_str1 = str_input(str1);
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
    char c = getchar();
    if (c == '\n')
        return 0;
    char word[100];
    scanf("%s", word);
    if (!strcmp(word, "-at"))
    {
        scanf("%d", at_ptr);
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