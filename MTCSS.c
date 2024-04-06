//DISCLAIMER: I DO NOT SUPPORT PEOPLE PLAGIARIZING MY CODE. I DO NOT TAKE RESPONSIBILITY FOR THE UNLAWFUL ACTIONS OF OTHERS.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

// globals
int *job_pool;              // where jobs are stored
int num_created_jobs = 0;   // for making sure we don't ake too many jobs
int num_processed_jobs = 0; // for letting the server threads know when to stop
int processing_time;
int total_jobs;
int job_pool_size = 0;
int in = 0;  // points to the first free position (for producers)
int out = 0; // points to the first full position (for consumers)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_produce = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_consume = PTHREAD_COND_INITIALIZER;
FILE *log_file;

// prototypes
void *make_job(void *arg);
void *process_job(void *arg);
void log_action(const char *thread_type, int thread_id, int job_pool_index, int job_id);

// main
int main(int argc, char *argv[])
{
    // process prog params
    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s <num_client_threads> <num_server_threads> <job_pool_size> <total_jobs> <job_processing_time_ms>\n", argv[0]);
        return 1;
    }

    int params[5];

    for (int i = 1; i < argc; i++)
    {
        char *endPtr;
        params[i - 1] = strtol(argv[i], &endPtr, 10);
        if (*endPtr != '\0' || params[i - 1] < 0)
        {
            fprintf(stderr, "All arguments must be positive integers.\n");
            return 1;
        }
    }

    printf("Client Threads: %d\n", params[0]);
    printf("Server Threads: %d\n", params[1]);
    printf("Job Pool Size: %d\n", params[2]);
    printf("Total Jobs: %d\n", params[3]);
    printf("Job Processing Time (ms): %d\n", params[4]);

    // we need to make these global as they're going to be used for making and processing threads
    processing_time = params[4];
    total_jobs = params[3];
    job_pool_size = params[2];

    // job pool just me dynamically allocated as you can't create an array in the stack with a variable length
    job_pool = (int *)malloc(params[2] * sizeof(int));
    if (job_pool == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for the job pool.\n");
        return 1;
    }

    // make thread arrays
    pthread_t *client_threads = (pthread_t *)malloc(params[0] * sizeof(pthread_t));
    if (client_threads == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for client threads\n");
        return 1;
    }
    pthread_t *server_threads = (pthread_t *)malloc(params[1] * sizeof(pthread_t));
    if (server_threads == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for server threads\n");
        free(client_threads); // clean up previously allocated memory
        return 1;
    }

    // make indices so each tread knows who it is
    int *client_indices = (int *)malloc(params[0] * sizeof(int));
    if (client_indices == NULL)
    {
        free(client_threads);
        return 1;
    }
    int *server_indices = (int *)malloc(params[1] * sizeof(int));
    if (server_indices == NULL)
    {
        free(client_threads);
        free(server_indices);
        return 1;
    }

    log_file = fopen("service.log", "w");

    // execute threads
    for (int i = 0; i < params[0]; i++)
    {
        client_indices[i] = i;
        if (pthread_create(&client_threads[i], NULL, make_job, (void *)&client_indices[i]) != 0)
        {
            fprintf(stderr, "Failed to create client thread\n");
            return 1;
        }
    }

    for (int i = 0; i < params[1]; i++)
    {
        server_indices[i] = i;
        if (pthread_create(&server_threads[i], NULL, process_job, (void *)&server_indices[i]) != 0)
        {
            fprintf(stderr, "Failed to create server thread\n");
            return 1;
        }
    }

    // reaps threads upon completion
    for (int i = 0; i < params[0]; i++)
    {
        int *result;
        pthread_join(client_threads[i], (void **)&result);
        printf("Client thread %d created %d jobs.\n", i, *result);
        free(result); // Free the dynamically allocated result
    }

    for (int i = 0; i < params[1]; i++)
    {
        int *result;
        pthread_join(server_threads[i], (void **)&result);
        printf("Server thread %d processed %d jobs.\n", i, *result);
        free(result); // Free the dynamically allocated result
    }

    // end code
    fclose(log_file);
    free(job_pool);
    free(client_indices);
    free(server_indices);
    free(client_threads);
    free(server_threads);
    pthread_mutex_destroy(&mutex);

    return 0;
}

void *make_job(void *arg)
{
    int *created_jobs = malloc(sizeof(int));
    if (!created_jobs)
    {
        perror("Failed to allocate memory for created_jobs");
        pthread_exit(NULL);
    }
    *created_jobs = 0;

    while (1)
    {
        pthread_mutex_lock(&mutex);

        if (num_created_jobs >= total_jobs)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        while (((in + 1) % job_pool_size) == out && num_created_jobs < total_jobs)
        {
            pthread_cond_wait(&cond_produce, &mutex);
        }

        if (num_created_jobs >= total_jobs)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        job_pool[in] = num_created_jobs;
        log_action("Client", *((int *)arg), in, num_created_jobs);
        in = (in + 1) % job_pool_size;
        num_created_jobs++;
        (*created_jobs)++;

        pthread_cond_signal(&cond_consume);
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(created_jobs);
}

void *process_job(void *arg)
{
    int *processed_jobs = malloc(sizeof(int));
    if (!processed_jobs)
    {
        perror("Failed to allocate memory for processed_jobs");
        pthread_exit(NULL);
    }
    *processed_jobs = 0;

    while (1)
    {
        pthread_mutex_lock(&mutex);

        while (in == out && num_processed_jobs < total_jobs)
        {
            pthread_cond_wait(&cond_consume, &mutex);
        }

        if (num_processed_jobs >= total_jobs)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        log_action("Server", *((int *)arg), out, job_pool[out]);
        out = (out + 1) % job_pool_size;
        num_processed_jobs++;
        (*processed_jobs)++;

        pthread_cond_signal(&cond_produce);

        pthread_mutex_unlock(&mutex);

        usleep(processing_time * 1000);
    }

    pthread_exit(processed_jobs);
}

void log_action(const char *thread_type, int thread_id, int job_pool_index, int job_id)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long long timestamp = (long long)ts.tv_sec * 1000000000 + ts.tv_nsec; // convert to nanoseconds (second + nanosecond)

    // make sure log_file is valid
    if (log_file != NULL)
    {
        fprintf(log_file, "%lld %s %d %d %d\n", timestamp, thread_type, thread_id, job_pool_index, job_id);
        fflush(log_file); // clear buffer
    }
    else
    {
        printf("Error: log_file is not open.\n");
    }
}
