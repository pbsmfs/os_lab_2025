#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

int thread_counter = 0;

void* thread1_function(void* arg) {
    int thread_id = __sync_fetch_and_add(&thread_counter, 1);
    printf("Thread %d: Starting...\n", thread_id);

    printf("Thread %d: Locking mutex1...\n", thread_id);
    pthread_mutex_lock(&mutex1);
    printf("Thread %d: mutex1 locked successfully\n", thread_id);

    sleep(1);

    printf("Thread %d: Trying to lock mutex2...\n", thread_id);
    pthread_mutex_lock(&mutex2);
    printf("Thread %d: mutex2 locked successfully\n", thread_id);

    printf("Thread %d: Critical section - doing work...\n", thread_id);
    sleep(1);

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);

    printf("Thread %d: Finished\n", thread_id);
    return NULL;
}

void* thread2_function(void* arg) {
    int thread_id = __sync_fetch_and_add(&thread_counter, 1);
    printf("Thread %d: Starting...\n", thread_id);

    printf("Thread %d: Locking mutex2...\n", thread_id);
    pthread_mutex_lock(&mutex2);
    printf("Thread %d: mutex2 locked successfully\n", thread_id);

    sleep(1);

    printf("Thread %d: Trying to lock mutex1...\n", thread_id);
    pthread_mutex_lock(&mutex1); 
    printf("Thread %d: mutex1 locked successfully\n", thread_id);

    printf("Thread %d: Critical section - doing work...\n", thread_id);
    sleep(1);

    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);

    printf("Thread %d: Finished\n", thread_id);
    return NULL;
}

void demonstrate_simple_deadlock() {
    pthread_t thread1, thread2;

    thread_counter = 0;

    pthread_create(&thread1, NULL, thread1_function, NULL);
    pthread_create(&thread2, NULL, thread2_function, NULL);

    sleep(3);

    printf("\nПрограмма заблокирована в состоянии deadlock!\n");
    printf("Потоки не могут продолжить выполнение.\n");
    printf("Для выхода нажмите Ctrl+C\n\n");

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
}

int main(int argc, char *argv[]) {

    demonstrate_simple_deadlock();

    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);

    return 0;
}
