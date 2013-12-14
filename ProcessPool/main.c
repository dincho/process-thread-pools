//
//  main.c
//  ProcessPool
//
//  Created by Dincho Todorov on 11/17/13.
//  Copyright (c) 2013 Dincho Todorov. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

/* pipe descriptors */

//parent write to this pipe
int pd[2];

/* function declaration */
void worker();
void quit_handler(int num);

/* global constants */
const int POOL_SIZE = 5;
const int STOP_MARKER = POOL_SIZE * 10;

int main(int argc, const char * argv[])
{
    
    pid_t pool[POOL_SIZE];
    int marker = 0; //marker buffer
    
    //initialize pipe
    pipe(pd);
    write(pd[1], &marker, sizeof(marker));
    
    printf("parent pid: %d\n", getpid());
    
    //create the process pool
    for (int i = 0; i < POOL_SIZE; i++) {
        pid_t cpid = fork();
        if (cpid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        if (cpid) { //parent
            pool[i] = cpid;
            
            //when child is ready it stop itself, thus raisin SIGCHLD and wake up the parent
            //so parent only continue when child is ready and waiting the parent
            pause();
            
        } else { //child
            //start worker - it does not return
            worker();
            exit(EXIT_FAILURE);
        }
    }
    
    fprintf(stderr, "[%d] all children ready and wating parent\n", (unsigned)time(NULL));
    fprintf(stderr, "[%d] sending signals from parent ...\n", (unsigned)time(NULL));
    
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < POOL_SIZE; i++) {
            //send signal to wake up the child
            if (kill(pool[i], SIGCONT)) {
                perror("kill");
                exit(EXIT_FAILURE);
            }
            
            //when child has finished it's job, it stop itself, which generate SIGCHLD and wake up parent
            pause();
            
            fprintf(stderr, "[%d] parent waked child with pid: %d done\n", (unsigned)time(NULL), pool[i]);
        }
    }
    
    if (sizeof(marker) != read(pd[0], &marker, sizeof(marker))) {
        fprintf(stderr, "[%d] parent: error reading from pipe\n", (unsigned)time(NULL));
        exit(EXIT_FAILURE);
    }
    
    //exit all children
    for (int i = 0; i < POOL_SIZE; i++) {
        if (kill(pool[i], SIGINT)) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
    }
    
    //cleanup
    close(pd[0]);
    close(pd[1]);
    
    //@todo - handle zombies
    
    fprintf(stderr, "[%d] all done. marker value: %d\n", (unsigned)time(NULL), marker);
    return EXIT_SUCCESS;
}

void worker()
{
    int marker;
    
    //worker main loop, never quit
    //worker quits when SIGQUIT is received
    while (1) {
        fprintf(stderr, "[%d] worker with pid: %d waiting parent signal ...\n", (unsigned)time(NULL), getpid());
        
        //wait parent commands, this also send SIGCHLD to parent
        if (kill(getpid(), SIGSTOP)) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
        
        //waked up - do some stuff
        fprintf(stderr, "[%d] worker with pid: %d working ... \n", (unsigned)time(NULL), getpid());
        if (sizeof(marker) != read(pd[0], &marker, sizeof(marker))) {
            fprintf(stderr, "[%d] worker with pid: %d - error reading from pipe\n", (unsigned)time(NULL), getpid());
            exit(EXIT_FAILURE);
        }
        
        marker++;
        write(pd[1], &marker, sizeof(marker));
    }
}
