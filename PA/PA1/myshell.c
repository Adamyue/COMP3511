/*
    COMP3511 Spring 2024
    PA1: Simplified Linux Shell (MyShell)

    Your name: YUE, Pu
    Your ITSC email: pyue@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

/*
    Header files for MyShell
    Necessary header files are included.
    Do not include extra header files
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls

#define MYSHELL_WELCOME_MESSAGE "COMP3511 PA1 Myshell (Spring 2024)"

// Define template strings so that they can be easily used in printf
//
// Usage: assume pid is the process ID
//
//  printf(TEMPLATE_MYSHELL_START, pid);
//
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"
#define TEMPLATE_MYSHELL_CD_ERROR "Myshell cd command error\n"

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LENGTH 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the standard file descriptor IDs here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

// This function will be invoked by main()
void show_prompt(char *prompt, char *path)
{
    printf("%s %s> ", prompt, path);
}

// This function will be invoked by main()
// This function is given
int get_cmd_line(char *cmdline)
{
    int i;
    int n;
    if (!fgets(cmdline, MAX_CMDLINE_LENGTH, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(cmdline);
    cmdline[--n] = '\0';
    i = 0;
    while (i < n && cmdline[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}

// parse_arguments function is given
// This function helps you parse the command line
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char cmdline[MAX_CMDLINE_LENGTH]; // The input command line
//
// Sample usage:
//
//  parse_arguments(pipe_segments, cmdline, &num_pipe_segments, "|");
//
void parse_arguments(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

void process_cmd(char *cmdline)
{
    // Uncomment this line to show the content of the cmdline
    // printf("cmdline is: %s\n", cmdline);

    char test1[MAX_CMDLINE_LENGTH];
    char test2[MAX_CMDLINE_LENGTH];
    char* arguments1[MAX_ARGUMENTS];
    char* arguments2[MAX_ARGUMENTS];
    int num_arguments1;
    int num_arguments2;

    // copy cmdline
    strcpy(test1, cmdline);
    strcpy(test2, cmdline);

    // Handle redirection command
    parse_arguments(arguments1, test1, &num_arguments1, "<");
    parse_arguments(arguments2, test2, &num_arguments2, ">");

    // debug print
    // printf("%d\n%d\n", num_arguments1, num_arguments2);
    // fflush(stdout);
    // printf("%s\n", cmdline);
    // fflush(stdout);

    if (num_arguments1 != 1 || num_arguments2 != 1) {
        if (num_arguments1 == 1) {
            // only >
            char* output[MAX_CMDLINE_LENGTH];
            int num_output;
            int out; 

            parse_arguments(output, arguments2[1], &num_output, SPACE_CHARS);
            
            out = open(*output, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
            close(STDOUT_FILENO);
            dup2(out, STDOUT_FILENO);
            
            // Get arguments
            char* segments[MAX_ARGUMENTS_PER_SEGMENT];
            int num_segments;
            parse_arguments(segments, arguments2[0], &num_segments, SPACE_CHARS);
            segments[num_segments] = NULL;
            // execute the cmds
            execvp(*segments, segments);
        } else if (num_arguments2 == 1) {
            // only <
            char* input[MAX_CMDLINE_LENGTH];
            int num_input;
            int in;

            parse_arguments(input, arguments1[1], &num_input, SPACE_CHARS);

            in = open(*input, O_RDONLY, S_IRUSR | S_IWUSR);
            close(STDIN_FILENO);
            dup2(in, STDIN_FILENO);

            // Get arguments
            char* segments[MAX_ARGUMENTS_PER_SEGMENT];
            int num_segments;
            parse_arguments(segments, arguments1[0], &num_segments, SPACE_CHARS);
            segments[num_segments] = NULL;
            // execute the cmds
            execvp(*segments, segments);
        } else {
            // multiple
            char* check1[MAX_ARGUMENTS];
            char* check2[MAX_ARGUMENTS];
            int num_check1;
            int num_check2;
            int in, out;

            parse_arguments(check1, arguments1[0], &num_check1, ">");
            parse_arguments(check2, arguments1[1], &num_check2, ">");

            if (num_check1 == 1) {
                // num_check2 != 1
                // < >
                // At this time, check is the same as arguments[0], invoking parse_arguments should change it

                // debug print
                // printf("aaa\n%s\n%s\n", check2[0], check2[1]);
                // fflush(stdout);

                char* input_file[MAX_CMDLINE_LENGTH];
                char* output_file[MAX_CMDLINE_LENGTH];
                int num_input_file;
                int num_output_file;

                parse_arguments(input_file, check2[0], &num_input_file, SPACE_CHARS);
                parse_arguments(output_file, check2[1], &num_output_file, SPACE_CHARS);

                in = open(*input_file, O_RDONLY, S_IRUSR | S_IWUSR);
                out = open(*output_file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                dup2(in, STDIN_FILENO);
                dup2(out, STDOUT_FILENO);

                // Get arguments
                char* segments[MAX_ARGUMENTS_PER_SEGMENT];
                int num_segments;
                parse_arguments(segments, arguments1[0], &num_segments, SPACE_CHARS);
                segments[num_segments] = NULL;
                // execute the cmds
                execvp(*segments, segments);
            } else {
                // > <
                char* input_file[MAX_CMDLINE_LENGTH];
                char* output_file[MAX_CMDLINE_LENGTH];
                int num_input_file;
                int num_output_file;

                parse_arguments(input_file, arguments1[1], &num_input_file, SPACE_CHARS);
                parse_arguments(output_file, check1[1], &num_output_file, SPACE_CHARS);

                in = open(*input_file, O_RDONLY, S_IRUSR | S_IWUSR);
                out = open(*output_file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                dup2(in, STDIN_FILENO);
                dup2(out, STDOUT_FILENO);

                // Get arguments
                char* segments[MAX_ARGUMENTS_PER_SEGMENT];
                int num_segments;
                parse_arguments(segments, check1[0], &num_segments, SPACE_CHARS);
                segments[num_segments] = NULL;
                // execute the cmds
                execvp(*segments, segments);
            }
        }
    }

    // Get the arguments
    char* segments[MAX_ARGUMENTS_PER_SEGMENT];
    int num_segments;
    parse_arguments(segments, cmdline, &num_segments, SPACE_CHARS);
    segments[num_segments] = NULL;
    // execute the cmds
    execvp(*segments, segments);

    exit(0); // ensure the process cmd is finished
}

/* The main function implementation */
int main()
{
    // TODO: replace the shell prompt with your ITSC account name
    // For example, if you ITSC account is cspeter@connect.ust.hk
    // You should replace ITSC with cspeter
    char *prompt = "pyue";
    char cmdline[MAX_CMDLINE_LENGTH];
    char path[256]; // assume path has at most 256 characters

    printf("%s\n\n", MYSHELL_WELCOME_MESSAGE);
    printf(TEMPLATE_MYSHELL_START, getpid());

    // The main event loop
    while (1)
    {
        getcwd(path, 256);
        show_prompt(prompt, path);

        if (get_cmd_line(cmdline) == -1)
            continue; // empty line handling, continue and do not run process_cmd

        // TODO: Before running process_cmd
        //
        // (1) Handle the exit command
        // (2) Handle the cd command
        //
        // Note: These 2 commands should not be handled by process_cmd
        // Hint: You may call break or continue when handling (1) and (2) so that process_cmd below won't be executed

        // (1) Handle the exit command
        if (strcmp(cmdline, "exit") == 0) {
            printf(TEMPLATE_MYSHELL_END, getpid());
            break;
        } 

        // (2) Handle the cd commands
        char dest[2];
        if (strncmp(strncpy(dest, cmdline, 2), "cd", 2) == 0) {
            int i = 2;
            char thePath[MAX_CMDLINE_LENGTH] = "\0";

            for (; cmdline[i] == ' ' || cmdline[i] == '\t'; i++) {continue;}
            for (int k = 0; k < strlen(cmdline) - 3 && i < strlen(cmdline); i++, k++) {thePath[k] = cmdline[i];}

            if (chdir(thePath) == 0) {
                continue;
            } else {
                printf(TEMPLATE_MYSHELL_CD_ERROR);
                continue;
            }
        }
        
        // Handle multi-level pipe
        char* pipe_segments[MAX_PIPE_SEGMENTS];
        int num_pipe_segments;
        parse_arguments(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);

        // debug print
        // printf("%s\n", cmdline);
        // fflush(stdout);

        int pfds[num_pipe_segments * 2];

        for (int i = 0; i < num_pipe_segments; i++) {
            pipe(pfds + 2 * i);
        }

        for (int k = 0; k < num_pipe_segments; k++) {
            pid_t pid = fork();
            if (pid == 0)
            {
                // if not the first command
                if (k != 0) {
                    dup2(pfds[(k-1)*2], STDIN_FILENO);
                }

                // if not the last command
                if (k != num_pipe_segments - 1) {
                    dup2(pfds[k*2+1], STDOUT_FILENO);
                }

                for (int i = 0; i < num_pipe_segments * 2; i++) {
                    close(pfds[i]);
                }

                // the child process handles the command
                process_cmd(pipe_segments[k]);
            }
        }
           
        for (int i = 0; i < num_pipe_segments * 2; i++) {
            close(pfds[i]);
        }

        // the parent process simply wait for the child and do nothing
        for (int i = 0; i < num_pipe_segments; i++) {
            wait(0);
        };
            
        
    }

    return 0;
}