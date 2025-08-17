/*
    Main usecase is to replace the environment of program 2 with passed variables from program 1
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    char *argVec[10] = {"welcome", "to", "lsp", NULL}; // argVec is an array of char*
    char *envVec[] = {"ENV1=10","ENV2=100","ENV3=1000","NULL"};

    execve(argv[1],argVec, envVec);
    printf("This  line won't be executed in program1\n");
    exit(0);
}