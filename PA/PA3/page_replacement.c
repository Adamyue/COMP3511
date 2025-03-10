// COMP3511 Spring 2024
// PA3: Page Replacement Algorithms
//
// Please use the print-related helper functions, instead of using your own printf function calls
// The print-related helper functions are clearly defined in the skeleton code
// The grader TA will probably use an autograder to grade this PA
//
// Your name: YUE, Pu
// Your ITSC email: pyue@connect.ust.hk
//
// Declaration:
//
// I declare that I am not involved in plagiarism
// I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

// ===
// Region: Header files
// Note: Necessary header files are included, do not include extra header files
// ===
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ===
// Region: Constants
// ===

#define UNFILLED_FRAME -1
#define MAX_QUEUE_SIZE 10
#define MAX_FRAMES_AVAILABLE 10
#define MAX_REFERENCE_STRING 30

#define ALGORITHM_FIFO "FIFO"
#define ALGORITHM_OPT "OPT"
#define ALGORITHM_LRU "LRU"


// Keywords (to be used when parsing the input)
#define KEYWORD_ALGORITHM "algorithm"
#define KEYWORD_FRAMES_AVAILABLE "frames_available"
#define KEYWORD_REFERENCE_STRING_LENGTH "reference_string_length"
#define KEYWORD_REFERENCE_STRING "reference_string"



// Assume that we only need to support 2 types of space characters:
// " " (space), "\t" (tab)
#define SPACE_CHARS " \t"

// ===
// Region: Global variables:
// For simplicity, let's make everything static without any dyanmic memory allocation
// In other words, we don't need to use malloc()/free()
// It will save you lots of time to debug if everything is static
// ===
char algorithm[10];
int reference_string[MAX_REFERENCE_STRING];
int reference_string_length;
int frames_available;
int frames[MAX_FRAMES_AVAILABLE]; // This is actually the output frame in FIFO
int FIFO_ordered_frames[MAX_FRAMES_AVAILABLE + 1]; // This is for sorting
int OPT_zero_dist_frames[MAX_FRAMES_AVAILABLE]; // This is to store frames that doesn't appear at the rest of the frames
int OPT_available_frames[MAX_FRAMES_AVAILABLE]; // This is to store frames that appears at the rest of the frames
int counter[MAX_FRAMES_AVAILABLE]; // This is for LRU algorithm

// Helper function: Check whether the line is a blank line (for input parsing)
int is_blank(char *line)
{
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch))
            return 0;
        ch++;
    }
    return 1;
}
// Helper function: Check whether the input line should be skipped
int is_skip(char *line)
{
    if (is_blank(line))
        return 1;
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}
// Helper: parse_tokens function
void parse_tokens(char **argv, char *line, int *numTokens, char *delimiter)
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

// Helper: parse the input file
void parse_input()
{
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2];                                 // buffer for 2 tokens
    char *reference_string_tokens[MAX_REFERENCE_STRING]; // buffer for the reference string
    int numTokens = 0, n = 0, i = 0;
    char equal_plus_spaces_delimiters[5] = "";

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters, SPACE_CHARS);

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        if (is_skip(line) == 0)
        {
            line = strtok(line, "\n");
            if (strstr(line, KEYWORD_ALGORITHM))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    strcpy(algorithm, two_tokens[1]);
                }
            }
            else if (strstr(line, KEYWORD_FRAMES_AVAILABLE))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &frames_available);
                }
            }
            else if (strstr(line, KEYWORD_REFERENCE_STRING_LENGTH))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &reference_string_length);
                }
            }
            else if (strstr(line, KEYWORD_REFERENCE_STRING))
            {
                parse_tokens(two_tokens, line, &numTokens, "=");
                if (numTokens == 2)
                {
                    parse_tokens(reference_string_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(reference_string_tokens[i], "%d", &reference_string[i]);
                    }
                }
            }
        }
    }
}
// Helper: Display the parsed values
void print_parsed_values() 
{
    int i;
    printf("%s = %s\n", KEYWORD_ALGORITHM, algorithm);
    printf("%s = %d\n", KEYWORD_FRAMES_AVAILABLE, frames_available);
    printf("%s = %d\n", KEYWORD_REFERENCE_STRING_LENGTH, reference_string_length);
    printf("%s = ", KEYWORD_REFERENCE_STRING);
    for (i = 0; i < reference_string_length; i++)
        printf("%d ", reference_string[i]);
    printf("\n");
}

// Useful string template used in printf()
// We will use diff program to auto-grade this project assignment
// Please use the following templates in printf to avoid formatting errors
//
// Example:
//
//   printf(template_total_page_fault, 0)    # Total Page Fault: 0 is printed on the screen
//   printf(template_no_page_fault, 0)       # 0: No Page Fault is printed on the screen

const char template_total_page_fault[] = "Total Page Fault: %d\n";
const char template_no_page_fault[] = "%d: No Page Fault\n";

// Helper function:
// This function is useful for printing the fault frames in this format:
// current_frame: f0 f1 ...
//
// For example: the following 4 lines can use this helper function to print
//
// 7: 7
// 0: 7 0
// 1: 7 0 1
// 2: 2 0 1
//
// For the non-fault frames, you should use template_no_page_fault (see above)
//
void display_fault_frame(int current_frame)
{
    int j;
    printf("%d: ", current_frame);
    for (j = 0; j < frames_available; j++)
    {
        if (frames[j] != UNFILLED_FRAME)
            printf("%d ", frames[j]);
        else
            printf("  ");
    }
    printf("\n");
}

// Helper function: initialize arrays
void init_arrays(int array[], int length) {
    for (int i = 0; i < length; i++) {
        array[i] = UNFILLED_FRAME;
    }
}

// Helper function: initialize the frames
void init_frames()
{
    int i;
    for (i = 0; i < frames_available; i++)
        frames[i] = UNFILLED_FRAME;
}

void FIFO_algorithm()
{
    // TODO: Implement FIFO alogrithm here
    int num_of_no_page_fault = 0; int copy_frequency = 1;
    int num_of_unfiled = frames_available;

    for (int i = 0; i < reference_string_length; i++) {
        int k = 0; int current_frame = reference_string[i];

        // Find whether there is a frame matching the current_frame
        for (; k < frames_available; k++) {
            if (current_frame == frames[k]) {
                printf(template_no_page_fault, current_frame);
                num_of_no_page_fault++;
                break;
            }
        }
        // Does not find the frame, causing page fault
        if (k == frames_available) {
            // Find if there are any unfiled frames
            if (num_of_unfiled > 0) {
                for (int j = 0; j < frames_available; j++) {
                    if (frames[j] == UNFILLED_FRAME) {
                        frames[j] = current_frame;
                        num_of_unfiled--;
                        break;
                    }
                }
                display_fault_frame(current_frame);
            } else {
                // When there is no more unfiled frames
                // Copy the FIFO_ordered_frames from frames only once
                if (copy_frequency != 0) {
                    for (int m = 0; m < frames_available; m++) {
                        FIFO_ordered_frames[m] = frames[m];
                    }
                    copy_frequency--;
                } 
                int index = 0;

                // Change output frames using FIFO_ordered_frames
                FIFO_ordered_frames[frames_available] = current_frame;
                for (; index < frames_available; index++) {
                    if (frames[index] == FIFO_ordered_frames[0]) break;
                }
                frames[index] = current_frame;

                // display the current frame
                display_fault_frame(current_frame);

                // Update the FIFO_ordered_frames
                for (int n = 0; n < frames_available; n++) FIFO_ordered_frames[n] = FIFO_ordered_frames[n + 1];
            }
        }
    }
    printf(template_total_page_fault, reference_string_length - num_of_no_page_fault);
}

void OPT_algorithm()
{
    // TODO: Implement OPT replacement here
    int num_of_no_page_fault = 0;
    int num_of_unfiled = frames_available;
    int distances[MAX_FRAMES_AVAILABLE];
    
    for (int i = 0; i < reference_string_length; i++) {
        int k = 0; int current_frame = reference_string[i];

        // These arrays need to be refreshed for each loop
        init_arrays(OPT_zero_dist_frames, MAX_FRAMES_AVAILABLE);
        init_arrays(OPT_available_frames, MAX_FRAMES_AVAILABLE);
        init_arrays(distances, MAX_FRAMES_AVAILABLE);

        // Find whether there is a frame matching the current_frame
        for (; k < frames_available; k++) {
            if (current_frame == frames[k]) {
                printf(template_no_page_fault, current_frame);
                num_of_no_page_fault++;
                break;
            }
        }
        // Does not find the frame, causing page fault
        if (k == frames_available) {
            // Find if there are any unfiled frames
            if (num_of_unfiled > 0) {
                for (int j = 0; j < frames_available; j++) {
                    if (frames[j] == UNFILLED_FRAME) {
                        frames[j] = current_frame;
                        num_of_unfiled--;
                        break;
                    }
                }
                display_fault_frame(current_frame);
            } else {
                // When there are no more unfiled frames
                // First find each distance, if finally some of element in [distances] are 0, it means this frame doesn't appear anymore.
                for (int x = 0; x < frames_available; x++) {
                    int dist = 0;
                    for(int index = i; index < reference_string_length; index++) {
                        if (reference_string[index] != frames[x]) {dist++;} else {break;}
                    }
                    if (dist == 0) {
                        for (int m = 0; m < MAX_FRAMES_AVAILABLE; m++) {
                            if (OPT_zero_dist_frames[m] == UNFILLED_FRAME) {
                                OPT_zero_dist_frames[m] = frames[x];
                                break;
                            }
                        }
                    } else {
                        for (int m = 0; m < MAX_FRAMES_AVAILABLE; m++) {
                            if (OPT_available_frames[m] == UNFILLED_FRAME) {
                                OPT_available_frames[m] = frames[x];
                                distances[m] = dist;
                                break;
                            }
                        }
                    }
                }
                // Now find the optimal one to replace
                if (OPT_zero_dist_frames[0] != UNFILLED_FRAME) {
                    int OPT_frame = OPT_zero_dist_frames[0];
                    // Find the OPT one
                    for (int v = 1; v < MAX_FRAMES_AVAILABLE && OPT_zero_dist_frames[v] != UNFILLED_FRAME; v++) {
                        if (OPT_frame > OPT_zero_dist_frames[v]) OPT_frame = OPT_zero_dist_frames[v];
                    }
                    // Replace
                    for (int y = 0; y < frames_available; y++) {
                        if (frames[y] == OPT_frame) {
                            frames[y] = current_frame;
                            break;
                        }
                    }
                    display_fault_frame(current_frame);
                } else {
                    int OPT_frame = OPT_available_frames[0];
                    int OPT_dist = distances[0];
                    // Find the OPT one
                    for (int v = 1; v < MAX_FRAMES_AVAILABLE && OPT_available_frames[v] != UNFILLED_FRAME && distances[v] != UNFILLED_FRAME; v++) {
                        if (OPT_dist < distances[v]) {
                            OPT_frame = OPT_available_frames[v];
                            OPT_dist = distances[v];
                        }
                    }
                    // Replace
                    for (int y = 0; y < frames_available; y++) {
                        if (frames[y] == OPT_frame) {
                            frames[y] = current_frame;
                            break;
                        }
                    }
                    display_fault_frame(current_frame);
                }
            }
        }
    }
    printf(template_total_page_fault, reference_string_length - num_of_no_page_fault);
}

void LRU_algorithm()
{
    // TODO: Implement LRU replacement here
    int num_of_no_page_fault = 0;
    int num_of_unfiled = frames_available;

    // initialize the counter
    for (int v = 0; v < MAX_FRAMES_AVAILABLE; v++) counter[v] = 0;

    for (int i = 0; i < reference_string_length; i++) {
        int isFind = 0; int k = 0; 
        int current_frame = reference_string[i];

        // Find whether there is a frame matching the current_frame
        for (; k < frames_available; k++) {
            if (current_frame == frames[k]) {
                counter[k] = 0; // update the counter
                printf(template_no_page_fault, current_frame);
                num_of_no_page_fault++;
                isFind++;
            } else if (frames[k] != UNFILLED_FRAME) {
                counter[k]++;
            }
        }

        if (isFind == 1) {
            isFind--;
            continue;
        }

        if (k == frames_available) {
            // Find if there are any unfiled frames
            if (num_of_unfiled > 0) {
                for (int j = 0; j < frames_available; j++) {
                    if (frames[j] == UNFILLED_FRAME) {
                        frames[j] = current_frame;
                        num_of_unfiled--;
                        break;
                    } 
                }
                display_fault_frame(current_frame);
            } else {
                // When there are no more unfiled frames
                // Find the maximum counter in counter[MAX_FRAMES_AVAILABLE], 
                // then update it to zero and replace the corresponding frame in frames
                int index = 0;
                int max_ctr = counter[0]; 

                for (int m = 0; m < frames_available; m++) {
                    if (m == 0) continue;

                    if (counter[m] > max_ctr) {
                        max_ctr = counter[m];
                        index = m;
                    }

                    if (m == frames_available - 1) break;
                }

                for (int n = 0; n < frames_available; n++) {
                    if (n == index) {
                        // Update counter and replace frame
                        frames[n] = current_frame;
                        counter[index] = 0;
                        display_fault_frame(current_frame);
                    } else counter[n]++;
                }
            }
        }
    }
    printf(template_total_page_fault, reference_string_length - num_of_no_page_fault);
}


int main()
{
    parse_input();              
    print_parsed_values();      
    init_frames();              
    if (strcmp(algorithm, ALGORITHM_FIFO) == 0)
    {
        FIFO_algorithm();
    }
    else if (strcmp(algorithm, ALGORITHM_OPT) == 0)
    {
        OPT_algorithm();
    }
    else if (strcmp(algorithm, ALGORITHM_LRU) == 0)
    {
        LRU_algorithm();
    }

    return 0;
}