#include <stdlib.h>
#include <stdio.h>

typedef struct User
{
    size_t salary;
    size_t id;
} user;

int main(void)
{
    size_t user_count = 10;

    user *p_user = (user *)calloc(user_count, sizeof(user));

    if (p_user == NULL)
    {
        printf("Calloc failed!\n");
        exit(-1);
    }

    printf("Calloc successful\n");

    for (size_t i = 0; i < user_count; i++)
    {
        p_user[i].id = i;
        p_user[i].salary = i + 100;

        printf("User %lu's salary: %lu\n", p_user[i].id, p_user[i].salary);
    }

    free(p_user);
    p_user = NULL;

    return 0;
}