#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>

#define STR_MAX_LENGTH 4000
#define CREATEFILE_CODE 0
#define INSERTSTR_CODE 1
#define CAT_CODE 2

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

    char *address_ptr;
    if (fileaddress[0] == '\\' || fileaddress[0] == '/')
        address_ptr = fileaddress + 1;
    else
        address_ptr = fileaddress;
    f = fopen(address_ptr, "w");
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
    char *address_ptr;
    if (addresscopy[0] == '/' || addresscopy[0] == '\\')
        address_ptr = addresscopy + 1;
    else
        address_ptr = addresscopy;
    for (int i = 0; address_ptr[i] != 0; i++)
    {
        if (address_ptr[i] == '/' || address_ptr[i] == '\\')
        {
            address_ptr[i] = 0;
            // didn't handle invalid folder names and folders that have the same name as a file
            _mkdir(address_ptr);
            address_ptr[i] = '/';
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
    // NO CHECK FOR CORRECT INPUT FORMAT YET
    char word[1000];

    // first word is --file
    scanf("%s ", word);

    char c;
    c = getchar();

    if (c != '\"')
    {
        fileaddress[0] = c;
        scanf("%s", fileaddress + 1);
    }
    else
    {
        int i = 0;
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
    // MORE THAN ONE SPACE IN A ROW

    // get string
    char word[1000];
    int dq = 0;

    // first word is --str
    scanf("%s ", word);

    char c;
    c = getchar();

    if (c != '\"')
    {
        str[0] = c;
        scanf("%s", str + 1);
    }
    else
    {
        int i = 0;
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
    FILE *temp_file = fopen("tempfile", "w");

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
            remove("tempfile");
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
        remove("tempfile");
        return -1;
    }

    // check validity of start pos
    if (pos_char != 0 && pos_char > strlen(line))
    {
        printf("line %d of the file doesn't have this many characters\n", pos_line + 1);
        fclose(original_file);
        fclose(temp_file);
        remove("tempfile");
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
    temp_file = fopen("tempfile", "r");
    while (fgets(line, 2000, temp_file) != NULL)
    {
        fprintf(original_file, line);
    }

    fclose(original_file);
    fclose(temp_file);
    remove("tempfile");

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
    FILE *temp_file = fopen("tempfile", "w");

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
            remove("tempfile");
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
    temp_file = fopen("tempfile", "r");
    while (fgets(line, 2000, temp_file) != NULL)
    {
        fprintf(original_file, line);
    }

    fclose(original_file);
    fclose(temp_file);
    remove("tempfile");
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
    FILE *clipboard = fopen("clipboard", "w");

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
            remove("clipboard");
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
    FILE *clipboard = fopen("clipboard", "r");
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