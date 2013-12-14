//
//  main.c
//  ThreadPool
//
//  Created by Dincho Todorov on 11/26/13.
//  Copyright (c) 2013 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define DEBUG 0
#define debug_print(fmt, ...) \
do { if (DEBUG) fprintf(stderr, "%s():%d: " fmt,  __func__, __LINE__, __VA_ARGS__); } while (0)

void *worker(void *arg);
const int POOL_SIZE = 5;
int marker = 0;
sem_t *sem_done[POOL_SIZE], *sem_work[POOL_SIZE];

int main(int argc, const char * argv[])
{
    pthread_t pool[POOL_SIZE];
    
    //create the process pool
    for (int i = 0; i < POOL_SIZE; i++) {

        char sem_done_name[50];
        sprintf(sem_done_name, "/tmp/sem_done%d", i);
        
        sem_unlink(sem_done_name);
        if((sem_done[i] = sem_open(sem_done_name, O_CREAT, 0644, 0)) == SEM_FAILED) {
            perror("sem_open1"); //handle errno
            exit(EXIT_FAILURE);
        }
        
        char sem_work_name[50];
        sprintf(sem_work_name, "/tmp/sem_work%d", i);
        
        sem_unlink(sem_work_name);
        if((sem_work[i] = sem_open(sem_work_name, O_CREAT, 0644, 0)) == SEM_FAILED) {
            perror("sem_open2"); //handle errno
            exit(EXIT_FAILURE);
        }
        
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        
        if (pthread_create(&pool[i], NULL, worker, arg)) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    
    debug_print("[%d] pool created\n", (unsigned)time(NULL));
    
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < POOL_SIZE; i++) {
            sem_post(sem_work[i]); //let it work
            sem_wait(sem_done[i]); //wait it finish
            //fprintf(stderr, "[%d] marker: %d\n", (unsigned)time(NULL), marker);
        }
    }
    
    printf("[%d] all done. marker value: %d\n", (unsigned)time(NULL), marker);
    
    for (int i = 0; i < POOL_SIZE; i++) {
        //cancel the thread and wake up to it can exit
        pthread_cancel(pool[i]);
        sem_post(sem_work[i]);
        
        sem_close(sem_work[i]);
        sem_close(sem_done[i]);
    }
    

    pthread_exit(NULL);
}

void *worker(void *arg)
{
    //worker main loop, never quit
    int i = *((int *)arg);
    
    while (1) {
        debug_print("[%d] worker: %d waiting parent ...\n", (unsigned)time(NULL), i);
        
        if (-1 == sem_wait(sem_work[i])) {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }
        
        pthread_testcancel();
        
        debug_print("[%d] worker: %d working ...\n", (unsigned)time(NULL), i);
        marker++;
        
        if (-1 == sem_post(sem_done[i])) {
            perror("sem_post");
            exit(EXIT_FAILURE);
        }
    }
}


