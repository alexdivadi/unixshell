#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct Data {
int* v1;
int* v2;
int n;
int size;
};

void *dot_product(void *ptr)
{
        struct Data* d = (struct Data *) ptr;
        int val = 0;
        for(int i=0; i < d->n ; i++) {
                //printf("%d, %d\n", d->v1[i], d->v2[i*d->size]);
                val += d->v1[i] * d->v2[i*d->size];
        }
        //printf("%d\n", val);
        return (void *) val;
}

int main(int argc, char *argv[])
{
        if(argc != 8){
                printf("Error: Incorrect number of arguments.\n");
                printf("Usage: ./matmult u v w input1.txt input2.txt output.txt thread_count\n");
                exit(1);
        }

        int u, v, w, thread_count;
        FILE *input1, *input2, *output;
        int *matrix1, *matrix2, *matrix_output;

        printf("Step 1: Memory Allocation\n");

        if ((u = atoi(argv[1])) < 0)
                fprintf(stderr, "Invalid argument (1)");
        if ((v = atoi(argv[2])) < 0)
                fprintf(stderr, "Invalid argument (2)");
        if ((w = atoi(argv[3])) < 0)
                fprintf(stderr, "Invalid argument (3)");
        if ((thread_count = atoi(argv[7])) < 0)
                fprintf(stderr, "Invalid argument (7)");
        if (thread_count != u * w)
                fprintf(stderr, "Thread count is incorrect.");
        if (u < 1 || v < 1 || w < 1)
                fprintf(stderr, "Must input non-negative numbers.");

        matrix1 = (int*)malloc(u * v * sizeof(int));
        matrix2 = (int*)malloc(v * w * sizeof(int));
        matrix_output = (int*)malloc(u * w * sizeof(int));

        printf("Step 2: Read files\n");

        input1 = fopen(argv[4], "r");
        if (!input1) {
                printf("File %s cannot be opened for reading.\n", argv[4]);
                exit(1);
        }
        for (int i = 0; i < u * v; i++) {
                if (!fscanf(input1, "%d", &matrix1[i])) {
                        printf("Error reading file.\n");
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
                        printf("Error reading file.\n");
                        exit(1);
                }
        }
        fclose(input2);

        printf("Step 3: Create Threads\n");

        struct Data *d;
        d = (struct Data *)malloc(thread_count * sizeof(struct Data));
        pthread_t *threads = (pthread_t *) malloc(thread_count * sizeof(pthread_t));

    /* Create independent threads each of which will execute function */
        for(int i=0; i < u; i++) {
                for(int j=0; j < w; j++) {
                        int index = i * w + j;
                        d[index].v1 = &matrix1[i*v];
                        d[index].v2 = &matrix2[j];
                        d[index].n = v;
                        d[index].size = w;

                        pthread_create( &threads[index], NULL, dot_product, (void *)&d[index]);
                }
        }

        printf("Step 4: Join Threads\n");

        for (int i = 0; i < thread_count; i++) {
                void * ptr;
                int err = 0;
                if ((err = pthread_join(threads[i], &ptr)) < 0)
                        printf("Pthread_join failed (%d)\n", err);
                //printf("%d\n", (int) ptr);
                matrix_output[i] = (int) ptr;
        }

        printf("Step 5: Write Output\n");

        output = fopen(argv[6], "w");
        if (!output) {
                printf("File %s cannot be opened for writing.\n", argv[6]);
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

