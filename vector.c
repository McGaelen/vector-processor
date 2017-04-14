/*
CIS 452 Program 2 - Streamed Vector Processing
Gaelen McIntee
2/15/2017

Usage: vector <number_size> <vector_size> <vector_A> <vector_B> <out_file>

This program takes two files with identical number_size and vector_size and
subtracts the numbers in vector_B from vector_A as a vector processor would,
using pipes.  The result is stored in out_file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

/* Preset read/write values for readable code */
#define READ 0
#define WRITE 1
#define VECTOR_SIZE_MAX 500

/* Catches the ^C signal to unpause the program. */
void unpause(int signum);
/* Adds two bit strings a and b and stroes the result in y. */
int add(char const *a, char const *b, char *y);
/* We know how much input we're working with */
int numberSize, vectorSize;
/* PID's for the incrementer and adder */
pid_t increm = -1, adder = -1;

int main(int argc, char *argv[])
{
    /* initial file descriptors for the pipes */
    int fd_compl_increm[2], fd_increm_adder[2];
    FILE *a, *b, *out;

    /* All processes get this signal installed to unpause later */
    signal(SIGINT, unpause);

    /* Capture command line arguments w/ error checking ––––––––––––––––– */
    /* must have 6 arguments */
    if (argc < 6) {
        fprintf(stderr, "Usage: vector <number_size> <vector_size> <vector_A> <vector_B> <out_file>\n");
        exit(1);
    }

    /* retrieve our number_size and vector_size */
    numberSize = atoi(argv[1]);
    vectorSize = atoi(argv[2]);
    if (vectorSize > VECTOR_SIZE_MAX) {
        fprintf(stderr, "Vector size too big, must be <=500.\n");
        exit(1);
    }
    fprintf(stderr, "Initializing vector processor with Number size %i and Vector size %i\n", numberSize, vectorSize);

    /* All files must open successfully to continue */
    a = fopen(argv[3], "r");
    b = fopen(argv[4], "r");
    out = fopen(argv[5], "w");
    if (a ==  NULL || b == NULL || out == NULL) {
        perror("File open error");
        exit(1);

    }
    fprintf(stderr, "Opened input files %s and %s.\n", argv[3], argv[4]);

    /* Pipe and Fork setup ––––––––––––––––––––––––––––––––––––––––––––– */
    if ( pipe(fd_compl_increm) < 0 || pipe(fd_increm_adder) < 0 ) {
        perror("Piping error");
        exit(1);
    }
    fprintf(stderr, "Pipes created.\n");

    /* The parent (Complementer) creates the Incrementer */
    increm = fork();
    if (increm < 0) {
        perror("Fork error");
        exit(1);
    }
    if (increm > 0)
        fprintf(stderr, "Incrementer child process created with PID %i.\n", increm);

    /* The Incrementer creates the Adder (Complementer is blocked from executing this) */
    if (!increm)
    {
        adder = fork();
        if (adder < 0) {
            perror("Fork error");
            exit(1);
        }
        if (adder > 0)
            fprintf(stderr, "Adder child process created with PID %i.\n", adder);
    }

    /* Processes now execute their independent code */

    /* Complementer (if our child process is the Incrementer) */
    if (increm > 0)
    {
        /* Redirect our stdout to the write end of the pipe connected to
        the Incrementer, and close all other references. */
        dup2(fd_compl_increm[WRITE], STDOUT_FILENO);
        close(fd_compl_increm[READ]);
        close(fd_compl_increm[WRITE]);
        close(fd_increm_adder[READ]);
        close(fd_increm_adder[WRITE]);
        fprintf(stderr, "Complementer: pipes redirected.\n");
        pause();

        char temp, complement[numberSize];
        /* Loop through each bit string in the file, and get each bit
        one by one.  Flip each bit that comes in.  If there's a newline,
        ignore it. */
        for (int j = 0; j < vectorSize; j++)
        {
            int i;
            for (i = 0; i < numberSize; i++)
            {
                temp = fgetc(b);
                switch (temp) {
                    case '0':
                    temp = '1'; break;
                    case '1':
                    temp = '0'; break;
                    case '\n':
                    i--; break; // Prevent loop from moving ahead if a newline is encountered.
                }
                complement[i] = temp;
            }
            complement[i] = '\0'; // Null terminate
            fprintf(stderr, "Complementer: complement is %s\n", complement);
            /* Send the complemented bit string through the pipe connected
            to the Incrementer */
            write(STDOUT_FILENO, complement, strlen(complement)+1);
        }
    }
    /* Incrementer (if our child process is the Adder) */
    else if (adder > 0)
    {
        /* Redirect our stdin to the read end of the pipe connected to
        the Complementer, redirect stdout to the write end of the pipe
        connected to the Adder.  Close all other references. */
        dup2(fd_compl_increm[READ], STDIN_FILENO);
        dup2(fd_increm_adder[WRITE], STDOUT_FILENO);
        close(fd_compl_increm[READ]);
        close(fd_compl_increm[WRITE]);
        close(fd_increm_adder[READ]);
        close(fd_increm_adder[WRITE]);
        fprintf(stderr, "Incrementer: redirected pipes.\n");
        pause();

        /* Before doing anything, make a bit string of the value 1 so
        we can use it to increment the bit string. */
        char compl[numberSize], one[numberSize], inc[numberSize];
        for (int i = 0; i < numberSize; i++) {
            if (i == numberSize-1)
                one[i] = '1';
            else
                one[i] = '0';
        }
        one[numberSize] = '\0';
        fprintf(stderr, "Incrementer: Bit string %s created.\n", one);

        /* Now we loop through the entire vector, reading in the value
        sent to us from the Complementer and calling add() with on it
        with our bit string of value 1, effectively incrementing it.
        to get the 2's complement. */
        for (int j = 0; j < vectorSize; j++)
        {
            read(STDIN_FILENO, compl, numberSize+1); // read pipe
            fprintf(stderr, "Incrementer: read in %s from Complementer.\n", compl);

            add(compl, one, inc); // Do "compl + 1" and save it in inc
            fprintf(stderr, "Incrementer: %s incremented by 1 is %s\n", compl, inc);

            /* Send the incremented value (2's comp) through the pipe connected
            to the Adder. */
            write(STDOUT_FILENO, inc, strlen(inc)+1);
        }
    }
    /* Adder (if we have no children) */
    else
    {
        /* Redirect our stdin to the read end of the pipe connected to
        the Incrementer, and close all other references. */
        dup2(fd_increm_adder[READ], STDIN_FILENO);
        close(fd_compl_increm[READ]);
        close(fd_compl_increm[WRITE]);
        close(fd_increm_adder[READ]);
        close(fd_increm_adder[WRITE]);
        fprintf(stderr, "Adder: redirected pipes.\n");
        pause();

        char temp, num_A[numberSize], num_B[numberSize], output[numberSize];
        int i;

        /* Loop through the entire vector, first reading in the 2's comp
        given from the Incrementer, then reading in the corresponding value
        from the vector in vector_A one character at a time. Newlines are
        ignored in the read. After both bit strings have been read, call add()
        on both of them to perform subtraction and then write the result to out_file. */
        for (int j = 0; j < vectorSize; j++)
        {
            read(STDIN_FILENO, num_B, numberSize+1);
            fprintf(stderr, "Adder: read in %s from Incrementer.\n", num_B);

            for (i = 0; i < numberSize; i++)
            {
                temp = fgetc(a);
                if (temp == '\n')
                    i--;	// ignore newline
                num_A[i] = temp;
            }
            num_A[i] = '\0'; // null terminate

            add(num_A, num_B, output); // Do "num_A + num_B" and save it in output

            /* Report our answer and send it to the file with a newline. */
            fprintf(stderr, "Adder: %s added to %s is %s\n", num_A, num_B, output);
            fputs(output, out);
            fputs("\n", out);
        }

        fprintf(stderr, "Vector subtraction complete.\n"); // we're done
    }

    return 0;
}

/* The add() function checks through all possible combinations of
the bits at position i of the bit strings a and b, while also
considering a carry flag.  The output bit string at position i
is set according to which case was triggered. */
int add(char const *a, char const *b, char *y)
{
    int carry = 0;

    for (int i = (numberSize-1); i >= 0; i--)
    {
        if (a[i]=='0' && b[i]=='0' && carry==0)
            { y[i] = '0'; carry = 0; }
        else if (a[i]=='1' && b[i]=='0' && carry==0)
            { y[i] = '1'; carry = 0; }
        else if (a[i]=='0' && b[i]=='1' && carry==0)
            { y[i] = '1'; carry = 0; }
        else if (a[i]=='0' && b[i]=='0' && carry==1)
            { y[i] = '1'; carry = 0; }
        else if (a[i]=='1' && b[i]=='0' && carry==1)
            { y[i] = '0'; carry = 1; }
        else if (a[i]=='0' && b[i]=='1' && carry==1)
            { y[i] = '0'; carry = 1; }
        else if (a[i]=='1' && b[i]=='1' && carry==1)
            { y[i] = '1'; carry = 1; }
        else if (a[i]=='1' && b[i]=='1' && carry==0)
            { y[i] = '0'; carry = 1; }
        else {
            fprintf(stderr, "Unrecognized pattern in add(). a=%s, b=%s, y=%s. PID: %i\n", a, b, y, getpid());
            return -1;
        }
    }
    y[numberSize] = '\0'; // Null terminate

    return 0;
}

/* Report that we're unpaused and return. */
void unpause(int signum)
{
    if (increm > 0)
    fprintf(stderr, " Program unpaused.\n\n\n");
    return;
}
