#include <stdlib.h>
#include <stdio.h>

// void *realloc(void *_Nullable ptr, size_t size);

int main()
{
    size_t num_of_ints = 10;
    
    int* p_nums = (int*) calloc(num_of_ints, sizeof(int));
    if (p_nums == NULL)
    {
        printf("Calloc failed!!\n");
        exit(-1);
    }

    printf("Calloc successful\n");

    for (size_t i = 0; i < num_of_ints; i++)
    {
        p_nums[i] = i+(i*5);
        printf("p_nums[%lu] : %d\n", i, p_nums[i]);
    }

    // ------------------------ Realloc - Expanding ------------------------------------

    num_of_ints = 15;
    int* p_temp = (int*) realloc(p_nums, (num_of_ints * sizeof(int)));

    if (p_temp == NULL)
    {
        printf("Realloc (expand) failed!! Original block is still valid, but exiting...");
        free(p_nums);
        p_nums = NULL;
        exit(-1);
    }
    else 
    {
        p_nums = p_temp;
        p_temp = NULL;
    }

    for (size_t i = 10; i < num_of_ints; i++)
    {
        p_nums[i] = i + (i*5);
        printf("-After realloc p_nums[%lu] : %d\n", i, p_nums[i]);
    }

    printf("--- Displaying p_nums from start after realloc---\n");
    for (size_t i = 0; i < num_of_ints; i++)
    {
        printf("p_nums[%lu] : %d\n", i, p_nums[i]);
    }

    /*
    // printf("--- Now display and compare the first 15 elements of old and new? heap areas pointed by p_nums* and p_temp* ---\n");

    
    for (size_t i = 0; i < num_of_ints; i++)
    {
        //printf("p_nums_1[%lu] : %d, ",i, p_nums_1[i]); // Dangerous!! Don't use the old pointer after realloc, it became dangling since memory might be moved to new region!!
    }
    

    // free(p_nums_1); // realloc may free the old pointer internally, you shouldn't free it again (??)
    */

    // ------------------------ Realloc - Shrinking ------------------------------------
    num_of_ints = 5;
    p_temp = (int*) realloc(p_nums, num_of_ints * sizeof(int));
    if (p_temp == NULL)
    {
        printf("Realloc (shrink) failed!!\n");
        free(p_nums);
        p_nums = NULL;
        exit(-1);
    }
    else 
    {
        p_nums = p_temp;
        p_temp = NULL;
    }

    printf("--- Displaying p_nums from start after realloc (shrink)---\n");
    for (size_t i = 0; i < num_of_ints; i++)
    {
        printf("p_nums[%lu] : %d\n", i, p_nums[i]);
    }    

    free(p_nums);
    p_nums = NULL;

    return 0;
}
