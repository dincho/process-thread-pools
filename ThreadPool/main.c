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

void *worker(void *arg);
const int POOL_SIZE = 5;
int marker = 0;
sem_t *sem_free, *sem_work;
pthread_mutex_t work_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, const char * argv[])
{
    pthread_t pool[POOL_SIZE];
    
    if((sem_free = sem_open("/tmp/sem_free", O_CREAT, 0644, 0)) == SEM_FAILED) {
        perror("sem_open1"); //handle errno
        exit(EXIT_FAILURE);
    }
    
    if((sem_work = sem_open("/tmp/sem_work", O_CREAT, 0644, 0)) == SEM_FAILED) {
        perror("sem_open2"); //handle errno
        exit(EXIT_FAILURE);
    }
    
    printf("parent pid: %d\n", getpid());
    
    //create the process pool
    for (int i = 0; i < POOL_SIZE; i++) {
        if (pthread_create(&pool[i], NULL, worker, (void *)i)) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    
    if (sem_post(sem_work)) { //let the first thread work
        perror("sem_post1");
        exit(EXIT_FAILURE);
    }
    
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < POOL_SIZE; i++) {
            sem_wait(sem_free); //if there is free thread
            sem_post(sem_work); //let it work
        }
    }
    
    fprintf(stderr, "[%d] all done. marker value: %d\n", (unsigned)time(NULL), marker);
    
    sem_close(sem_work);
    sem_unlink("/tmp/sem_work");
    sem_close(sem_free);
    sem_unlink("/tmp/sem_free");
    pthread_exit(NULL);
}

void *worker(void *arg)
{
    //worker main loop, never quit
    int id = (int)arg;
    
    while (1) {
        fprintf(stderr, "[%d] worker: %d waiting parent ...\n", (unsigned)time(NULL), id);
        
        sem_wait(sem_work);
        fprintf(stderr, "[%d] worker: %d working ...\n", (unsigned)time(NULL), id);
        marker++;
//        sem_post(sem_free);
    }
}


