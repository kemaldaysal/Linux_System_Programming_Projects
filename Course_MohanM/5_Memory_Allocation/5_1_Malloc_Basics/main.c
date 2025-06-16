#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    size_t nums = 10;

    int *p_int = (int *) malloc(nums * sizeof(int));

    if (p_int == NULL)
    {
        printf("Malloc failed!\n");
        exit(-1);
    }

    printf("Malloc successful\n");

    for (int count = 0; count < nums; count++)
    {
        p_int[count] = count;
        printf("p_int[%d] : %d\n", count, p_int[count]);
    }

    free(p_int);
    p_int = NULL;

    return 0;
}