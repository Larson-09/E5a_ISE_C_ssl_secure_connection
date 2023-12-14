//
// Created by jordan on 15/11/23.
//

#include "com/com/com.h"
#include <stdio.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
    // Communication Init
    com_init();
    pause();
    printf("Hello Robot : Ctrl+C pour quitter\n");
    fflush(stdout);
}