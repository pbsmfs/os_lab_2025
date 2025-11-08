#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t partial_result = 1;
uint64_t mod = 0;

typedef struct {
    int thread_id;
    int k;
    int pnum;
    int start;
    int end;
} thread_data_t;

void* compute_partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    uint64_t local_result = 1;
    
    printf("Thread %d: computing range %d to %d\n", 
           data->thread_id, data->start, data->end);
    
    for (int i = data->start; i <= data->end; i++) {
        if (i == 0) continue;
        local_result = (local_result * i) % mod;
    }
    
    pthread_mutex_lock(&mutex);
    partial_result = (partial_result * local_result) % mod;
    printf("Thread %d: partial result = %llu (local), total = %llu\n", 
           data->thread_id, (unsigned long long)local_result, 
           (unsigned long long)partial_result);
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

uint64_t factorial_multithreaded(int k, int pnum) {
    if (k <= 1) return 1 % mod;
    
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].k = k;
        thread_data[i].pnum = pnum;
        thread_data[i].start = current_start;
        
        thread_data[i].end = current_start + numbers_per_thread - 1;
        if (i < remainder) {
            thread_data[i].end++;
        }
        
        if (thread_data[i].end > k) {
            thread_data[i].end = k;
        }
        
        printf("Creating thread %d for range %d-%d\n", 
               i, thread_data[i].start, thread_data[i].end);
        
        if (pthread_create(&threads[i], NULL, compute_partial_factorial, &thread_data[i]) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
        
        current_start = thread_data[i].end + 1;
        if (current_start > k) break;
    }
    
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
            exit(1);
        }
    }
    
    return partial_result;
}

uint64_t factorial_sequential(int k) {
    uint64_t result = 1;
    for (int i = 2; i <= k; i++) {
        result = (result * i) % mod;
    }
    return result;
}

int main(int argc, char *argv[]) {
    int k = -1;
    int pnum = 1;
    uint64_t mod_value = 0;
    
    static struct option long_options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "k:p:m:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'k':
                k = atoi(optarg);
                if (k < 0) {
                    printf("Error: k must be non-negative\n");
                    return 1;
                }
                break;
            case 'p':
                pnum = atoi(optarg);
                if (pnum <= 0) {
                    printf("Error: pnum must be positive\n");
                    return 1;
                }
                break;
            case 'm':
                mod_value = strtoull(optarg, NULL, 10);
                if (mod_value == 0) {
                    printf("Error: mod must be positive\n");
                    return 1;
                }
                break;
            default:
                printf("Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
                printf("Example: %s -k 10 --pnum=4 --mod=1000000007\n", argv[0]);
                return 1;
        }
    }
    
    if (k == -1 || mod_value == 0) {
        printf("Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
        printf("Example: %s -k 10 --pnum=4 --mod=1000000007\n", argv[0]);
        return 1;
    }
    
    mod = mod_value;
    
    printf("Computing %d! mod %llu using %d threads\n", 
           k, (unsigned long long)mod, pnum);
    
    partial_result = 1;
    
    uint64_t result = factorial_multithreaded(k, pnum);
    
    printf("\nFinal result: %d! mod %llu = %llu\n", 
           k, (unsigned long long)mod, (unsigned long long)result);
    
    if (k <= 20) { 
        uint64_t sequential_result = factorial_sequential(k);
        printf("Sequential verification: %llu\n", (unsigned long long)sequential_result);
        if (result != sequential_result) {
            printf("ERROR: Results don't match!\n");
            return 1;
        } else {
            printf("Verification: OK\n");
        }
    }
    
    pthread_mutex_destroy(&mutex);
    
    return 0;
}
