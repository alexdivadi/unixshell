#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

// Data: contains data necessary to compute dot product
struct Data {
  int* v1;
  int* v2;
  int n;
  int size;
  int* position;
};

// global variables needed by threads
struct Data* d;
int num_elements, thread_count;

/* Dot Product
 * Args: struct Data d
 * Returns: N/A
 * Function: calculates dot product given pointers to two vectors within matrix
 * */
void dot_product(struct Data d)
{
        int val = 0;
        for(int i=0; i < d.n ; i++) {
                //printf("%d, %d\n", d->v1[i], d->v2[i*d->size]);
                val += d.v1[i] * d.v2[i*d.size];
        }
        //printf("%d\n", val);
        *(d.position) = val;
}

/* Calcuulate Output
 * Args: int index
 * Returns: N/A
 * Function: runs the dot-product function for depending on number of threads
 * */
void *calc_output(void *ptr) {
        int *index = (int *) ptr;
        if (*index < 0 || *index > num_elements) {
                printf("Index error: %d\n", *index);
                exit(1);
        }
        for (int i = *index; i < num_elements; i += thread_count) {
                dot_product(d[i]);
        }
}

int main(int argc, char *argv[])
{
        if(argc != 8){
                printf("Error: Incorrect number of arguments.\n");
                printf("Usage: ./matmult u v w input1.txt input2.txt output.txt thread_count\n");
                exit(1);
        }

        int u, v, w;
        FILE *input1, *input2, *output;
        int *matrix1, *matrix2, *matrix_output;

        //printf("Step 1: Memory Allocation\n");

        /* do all input error checking */
        if ((u = atoi(argv[1])) < 0) {
            fprintf(stderr, "Error: Invalid argument (1)\n");
            exit(1);
        }
        if ((v = atoi(argv[2])) < 0) {
            fprintf(stderr, "Error: Invalid argument (2)\n");
            exit(1);
        }
        if ((w = atoi(argv[3])) < 0) {
            fprintf(stderr, "Error: Invalid argument (3)\n");
            exit(1);
        }
        if ((thread_count = atoi(argv[7])) < 0) {
            fprintf(stderr, "Error: Invalid argument (7)\n");
            exit(1);
        }
        if (thread_count > u * w || thread_count < 1) {
            fprintf(stderr, "Error: Thread count is incorrect.\n");
            exit(1);
        }
        if (u < 1 || v < 1 || w < 1) {
            fprintf(stderr, "Error: Must input non-negative numbers.");
            exit(1);
        }
        /* update values */
        num_elements = u * w;

        matrix1 = (int*)malloc(u * v * sizeof(int));
        matrix2 = (int*)malloc(v * w * sizeof(int));
        matrix_output = (int*)malloc(u * w * sizeof(int));

        //printf("Step 2: Read input files\n");

        input1 = fopen(argv[4], "r");
        if (!input1) {
                printf("File %s cannot be opened for reading.\n", argv[4]);
                exit(1);
        }
        for (int i = 0; i < u * v; i++) {
                if (!fscanf(input1, "%d", &matrix1[i])) {
                        printf("Error reading  first input matrix file.\n");
                        exit(1);
                }
        }
        fclose(input1);

        input2 = fopen(argv[5], "r");
        if (!input2) {
                printf("File %s cannot be opened for reading.\n", argv[5]);
                exit(1);
        }
        for (int i = 0; i < v * w; i++) {
                if (!fscanf(input2, "%d", &matrix2[i])) {
                        printf("Error reading second input matrix file.\n");
                        exit(1);
                }
        }
        fclose(input2);

        //printf("Step 3: Create Threads\n");

        /* allocate dynamic memory using new data */
        d = (struct Data *)malloc(u * w * sizeof(struct Data));
        pthread_t *threads = (pthread_t *) malloc(thread_count * sizeof(pthread_t));
        /* thread index is necessary to identify each thread */
        int *thread_index = (int *) malloc(thread_count * sizeof(int));

        /* populate values for Data */
        for (int row = 0; row < u; row++) {
                for (int col = 0; col < w; col++) {
                        int index = row * w + col; // index of output matrix
                        d[index].v1 = &matrix1[row*v]; // pointer to row in matrix 1
                        d[index].v2 = &matrix2[col]; // pointer to column in matrix 2
                        d[index].n = v; // number of elements in row/column
                        d[index].size = w; // number of columns in result matrix (used for calculating index)
                        d[index].position = &matrix_output[index]; // location in memory of output matrix cell
                }
        }

        /* create threads */
        for (int i = 0; i < thread_count; i++) {
                thread_index[i] = i;
                if (pthread_create( &threads[i], NULL, calc_output, (void *)&thread_index[i]) < 0) {
                        printf("Error creating thread (%d)\n", i);
                        exit(1);
                }
        }

       //printf("Step 4: Join Threads\n");

        /* join threads */
        for (int i = 0; i < thread_count; i++) {
                int err = 0;
                if ((err = pthread_join(threads[i], NULL)) < 0)
                        printf("Pthread_join failed (%d)\n", err);
        }

        //printf("Step 5: Write Output\n");

        output = fopen(argv[6], "w");
        if (!output) {
                printf("Output file %s cannot be opened for writing.\n", argv[6]);
                exit(1);
        }
        for (int i = 0; i < u; i++) {
                for (int j = 0; j < w; j++) {
                        fprintf(output, "%d ", matrix_output[i*w+j]);
                }
                fprintf(output, "\n");
        }
        fclose(output);
        printf("Program ran sucessfully!\n");
        exit(0);
}
