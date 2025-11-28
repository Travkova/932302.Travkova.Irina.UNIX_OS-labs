#include<stdio.h> 
#include<pthread.h>
#include<unistd.h>

//условные переменные
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int ready = 0;//флаг состояния события
int running = 1; //флаг выполнения программы

//функция-поставщик
void* provider(void* arg) {
    int i = 0;
    while (running && i < 5) {
        sleep(1);
        pthread_mutex_lock(&lock);
        if (ready == 1) {
            pthread_mutex_unlock(&lock);
            continue;
        }
        ready = 1;
        i++;
        printf("Provider sent event with data: %d\n", i);
        pthread_cond_signal(&cond1);
        pthread_mutex_unlock(&lock);
    }
    running = 0;
    return NULL;
}
//функция-потребитель
void* consumer(void* arg) {
    int i = 0;
    while (running) {
        pthread_mutex_lock(&lock);

        while (ready == 0) {
            pthread_cond_wait(&cond1, &lock);
            if (ready == 1) {
                printf("Consumer awoke\n");
            }
        }

        if (ready == 1) {
            ready = 0;
            i++;
            printf("Consumer got event with data: %d\n", i);
        }

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}
int main() {
    pthread_t prov, cons;
    pthread_create(&prov, NULL, provider, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prov, NULL);
    pthread_join(cons, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond1);
    return 0;
}
