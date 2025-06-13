/* This is only a example program to demonstrate the different memory segment of a process.
This program shouldn't be considered as any use case */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----- Function Prototypes ----- */

int add_num(int num_1, int num_2);

/* ----- Global variables ----- */
int g_var = 20; // Stored in initialised data segment
int g_flag;     // Stored in un-initialised data segment

/* This code will be stored in "Text Segment" as read-only memory */
/* ----- Main Function ----- */
int main(void)
{
    int num_1, num_2, sum;              // Stored in stack frame of 'main' function - stack segment
    char *p_str;                        // p_str is a part of stack frame of main()
    char *p_buf = "welcome";            // p_buf is stored in stack frame of main(), but "welcome" string is stored in text segment, which is read-only!!
    char stack_buf[20] = {"stackData"}; // stack_buf is stored in stack frame and it contains value "stackData"

    // p_buf[0] = 'n'; // SEGMENTATION FAULT !!, p_buf is stored at stack but "welcome" string is stored in text segment. As p_buf[0] tries to write to text segment, which is read-only
    strcpy(stack_buf, "newString"); // possible as it's fine to change stored data on stack

    num_1 = 10; // 10 is stored in stack
    num_2 = 20; // 20 is stored in stack
    sum = add_num(num_1, num_2); // value of sum is stored in stack
    printf("The result of add is (%d)\n", sum);    

    num_1 = 100; 
    num_2 = 200; 
    sum = add_num(num_1, num_2); 
    printf("The result of add is (%d)\n", sum);    

    p_str = (char *) calloc(sizeof(char), 20); // p_str points to 20 bytes created on heap segment, but location of p_str itself is part of stack frame of main().
    
    if (p_str == NULL)
    {
        printf("Allocation failed!!");
        free(p_str);
        exit(1);
    }
    
    strcpy(p_str, "Hello");
    printf("String stored at p_str starting point is: %s\n", p_str);

    free(p_str);
    p_str = NULL;

    return 0;
}

int add_num(int num_1, int num_2) // function arguments num_1, num_2 are pushed to stack when the function is called.
{
    int res = 0; // stored in stack frame of 'add_num' function - stack segment
    static int count = 0; // stored in initialised data segment since it's static!! It's not stored in stack !!

    count = count + 1;
    res = num_1 + num_2;

    printf("Number of times the function add_num called is (%d)\n", count);
    return res;

    /* When a function returns, stack frame of function is deleted, so all data belonging to that function will become invalid. 
    So stack acts as a growing when new function is called, and shrinks when function return back to calling function.
    */
}

