#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>

int fd[2]; // array to read and write in the pipe
int a, b, p, n;
bool *mark;   // size is of 'b', used mainly for Sieve's algorithm to calculate prime numbers.
int *primes;  // size is of (b+p), used to store all the prime numbers needed.
int *wpapx;   // array to store the 'wpapx' values of 'n' rows
int *pid_arr; // array to store the PID of all worker processes

// This function handles the signal generated by worker process on termination
void handelingChild(int sig)
{
    signal(SIGCHLD, handelingChild);
    int a[2]; // gets the index and value of wpapx
    int status;
    read(fd[0], a, 2 * sizeof(int)); // reading the wpapx values through the pipe
    if (a[0] == -1 || a[1] == -1)
    {
        printf("Killing every worker process...\n");
        for (int i = 0; i < n; i++)
            if (pid_arr[i] != 0)
                kill(pid_arr[i], SIGABRT); // killing all the worker processes
        printf("Killing the Controller process...\n");
        kill(getpid(), SIGABRT); // killing the controller process
        printf("TERMINATING...\n");
    }
    else
    {
        wpapx[a[0]] = a[1];
    }
}

// This function checks if the argument passed is in the range of 'a' and 'b'
void checkValidArgument(int *mat, int row)
{
    int r[2];
    r[0] = -1;
    r[1] = -1;
    for (int i = 0; i < n; i++)
    {
        // process is terminatied if the condition is satisfied
        if (mat[i] < a || mat[i] > b)
        {
            printf("An argument passed in row %d does not lie in the range of %d and %d. \n", row, a, b);
            printf("Terminating the worker process %d...\n", row);
            write(fd[1], r, 2 * sizeof(int));
            signal(SIGCHLD, handelingChild);
            exit(1);
        }
    }
}

// This function is to calculate the average of all the elements in a 1D Array of size 'n'.
int averageCalculator(int *arr, int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++)
    {
        sum += arr[i];
    }
    int avg = floor(sum / n);
    return avg;
}

// This function computes and stores all the primes in the range of '0' to 'b'
// and an additional of 'p' number of primes in a 1D array names 'primes'.
void simpleSieve(int range)
{
    memset(primes, -1, b * sizeof(primes[0]));
    for (int i = 0; i < b; i++)
    {
        mark[i] = true;
    }
    for (int p = 2; p * p < b; p++)
    {
        if (mark[p] == true)
        {
            for (int i = p * p; i < b; i += p)
                mark[i] = false;
        }
    }
    int c = 0;
    for (int p = 2; p < b; p++)
        if (mark[p] == true)
            primes[c++] = p;
    int nextPrime = b;
    while (range)
    {
        bool flag = true;
        for (int i = 2; i <= nextPrime / 2; ++i)
        {
            if (nextPrime % i == 0)
            {
                flag = false;
                break;
            }
        }
        if (flag)
        {
            primes[c++] = nextPrime;
            --range;
        }
        nextPrime++;
    }
}

// This function computes px for a give value of 'x' and returns the 'thapx'.
void *thapxCalculation(void *val)
{
    flockfile(stdout);
    printf("Worker thread %lu is now executing... \n\n", pthread_self());
    funlockfile(stdout);
    int *x = (int *)val;
    int px[(2 * p) + 1];
    memset(px, -1, (2 * p + 1) * sizeof(px[0]));
    int i = 0;
    while (primes[i] < (*x))
        i++;
    int end, c = 0;
    if (mark[*x] == true)
        end = i + p;
    else
        end = i + p - 1;
    i = i - p;
    while (i < 0)
        i++;
    printf("px of %d is: ", *x);
    bool check = false;
    while (i <= end)
    {
        px[c] = primes[i++];
        flockfile(stdout);
        printf("%d ", px[c]);
        funlockfile(stdout);
        ++c;
    }
    printf("\n\n");
    int thapx = averageCalculator(px, c);
    return (void *)(intptr_t)thapx;
}

int main(int argc, char *args[])
{
    if (argc == 1)
    {
        printf("Invalid input format\n");
        return -1;
    }
    else if (argc != atoi(args[1]) * atoi(args[1]) + 5)
    {
        printf("Invalid input format\n");
        return -1;
    }
    n = atoi(args[1]);
    a = atoi(args[2]);
    b = atoi(args[3]);
    p = atoi(args[4]);
    signal(SIGCHLD, handelingChild);
    mark = (bool *)malloc(b * sizeof(bool));
    primes = (int *)malloc((b + p) * sizeof(int));
    wpapx = (int *)malloc((n) * sizeof(int));
    pid_arr = (int *)malloc((n) * sizeof(int));
    for (int i = 0; i < n; i++)
    {
        pid_arr[i] = 0;
        wpapx[i] = -1;
    }
    simpleSieve(p); // calculating all primes
    int mat[n][n];
    int i, j, c = 5;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            mat[i][j] = atoi(args[c]); // creating the input matrix
            c++;
        }
    }
    printf("\nThis is the input matrix:\n");
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    if (pipe(fd) == -1)
    {
        perror("Error in creating pipe\n\n");
        exit(1);
    }
    else
        printf("Pipe created successfully.\n\n");
    pid_t p, wpid;
    for (int i = 0; i < n; i++)
    {
        if ((p = fork()) == 0)
            printf("Worker process %d is successfully created.\n\n", i); // process creation
        if (p < 0)
            perror("Error in fork\n");
        else if (p == 0)
        {
            flockfile(stdout);
            printf("Worker process %d is now executing... \n\n", i);
            funlockfile(stdout);
            int thapx[n];
            int wpapxArr[2];
            checkValidArgument(mat[i], i);
            pthread_t threads[n];
            void *status; // takes the return value from a pthread
            int ret;
            for (int j = 0; j < n; j++)
            {
                int *arg = malloc(sizeof(*arg));
                if (arg == NULL)
                {
                    fprintf(stderr, "Couldn't allocate memory for thread arg.\n");
                    exit(EXIT_FAILURE);
                }
                *arg = mat[i][j];
                if (pthread_create(&threads[j], NULL, thapxCalculation, arg) == 0) // thread creation
                {
                    flockfile(stdout);
                    printf("Worker thread %d with id %lu of Worker process %d has been successfully created.\n\n", j, threads[j], i);
                    funlockfile(stdout);
                }
                else
                {
                    fprintf(stderr, "Couldn't create a thread.\n");
                    exit(EXIT_FAILURE);
                }
            }
            for (int j = 0; j < n; j++)
            {
                ret = pthread_join(threads[j], &status);
                if (ret)
                {
                    fprintf(stderr, "pthread_join() failed\n");
                    return -1;
                }
                else
                {
                    thapx[j] = (intptr_t)status;
                    flockfile(stdout);
                    printf("Worker thread %d with id %lu of worker process %d has completed execution. \n\n", j, threads[j], i);
                    funlockfile(stdout);
                }
            }
            printf("Values of thapx for index %d is: ", i);
            for (int j = 0; j < n; j++)
            {
                flockfile(stdout);
                printf("%d ", thapx[j]);
                funlockfile(stdout);
            }
            printf("\n\n");
            int wpapx = averageCalculator(thapx, n);
            wpapxArr[0] = i;
            wpapxArr[1] = wpapx;
            write(fd[1], wpapxArr, 2 * sizeof(int));
            flockfile(stdout);
            printf("Worker process %d has finished execution.\n\n", i);
            funlockfile(stdout);
            signal(SIGCHLD, handelingChild); // signaling the termination of a worker process
            exit(0);
        }
        else if (p > 0)
        {
            pid_arr[i] = p; // saving the PID of the worker process
            wait(NULL);
        }
    }
    printf("All the wpapx values have been calculated and all worker processes have completed execution.\n\n");
    printf("All wpapx values have been written to the controller and this is the wpapx array: ");
    for (int i = 0; i < n; i++)
    {
        flockfile(stdout);
        printf("%d ", wpapx[i]);
        funlockfile(stdout);
    }
    printf("\n\n");
    int fapx = averageCalculator(wpapx, n);
    printf("The fapx value has been computed by the controller and this is the value: %d\n\n", fapx);
    return 0;
}