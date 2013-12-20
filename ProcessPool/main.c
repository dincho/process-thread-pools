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
#include <string.h> //strerror
#include <assert.h>

#define DEBUG 0
#define debug_print(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, "%s():%d: " fmt,  __func__, __LINE__, __VA_ARGS__); } while (0)

/* function declaration */
void worker(int readpd, int writepd);
void sigchld_handler(int sig);
void create_pool(pid_t *pool, int size, int pipes[][2]);
void destroy_pool(pid_t *pool, int size);
int run_pipeline(pid_t *pool, int size, int pipes[][2], int marker);

int main(int argc, const char * argv[])
{
    /* constants */
    const int POOL_SIZE = 5;
    
    //process pool
    pid_t pool[POOL_SIZE];
    
    //parent write - children read
    int pipes[POOL_SIZE][2];
    
    signal(SIGCHLD, sigchld_handler);
    
    debug_print("parent pid: %d\n", getpid());
    
    create_pool(pool, POOL_SIZE, pipes);
    
    int marker = run_pipeline(pool, POOL_SIZE, pipes, 0);
    assert(marker == 50);
    
    printf("[%d] all done. marker value: %d\n", (unsigned)time(NULL), marker);
    
    destroy_pool(pool, POOL_SIZE);
    return EXIT_SUCCESS;
}


void sigchld_handler(int sig)
{
    pid_t p;
    int status;
    
    //debug_print("[%d] handling signal %d\n", (unsigned)time(NULL), sig);
    
    while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Handle the death of pid p */
        debug_print("[%d] Handle the death of pid: %d\n", (unsigned)time(NULL), p);
    }
}

void create_pool(pid_t *pool, int size, int pipes[][2])
{
    for (int i = 0; i <= size; i++) {
        if (pipe(pipes[i])) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    
    for (int i = 0; i < size; i++) {
        pid_t cpid = fork();
        if (cpid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        if (cpid) { //parent
            pool[i] = cpid;
            pause();
        } else { //child
            
            //close other pipes
            for (int j = 0; j < size; j++) {
                if (j == i || j == i+1) {
                    continue;
                }
                
                //close unused pipes
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            //close other sides of working pipe
            close(pipes[i][1]);
            close(pipes[i+1][0]);
            
            //start worker - it does not return
            worker(pipes[i][0], pipes[i+1][1]);
            exit(EXIT_FAILURE);
        }
    }
    
    //this is parent only
    close(pipes[0][0]);
    close(pipes[size][1]);
    
    for (int j = 1; j < size; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }
    
    debug_print("[%d] all children ready and wating parent\n", (unsigned)time(NULL));
}

void destroy_pool(pid_t *pool, int size)
{
    //kill all children
    for (int i = 0; i < size; i++) {
        if (kill(pool[i], SIGINT)) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
    }
}

void worker(int readpd, int writepd)
{
    int marker;
    
    //worker main loop, never quit
    //worker quits when SIGQUIT is received
    while (1) {
        //wait parent commands, this also sends SIGCHLD to parent
        kill(getpid(), SIGSTOP);
        
        //waked up - do some stuff
        debug_print("[%d] worker with pid: %d working pipes ds: %d %d\n",
                    (unsigned)time(NULL), getpid(), readpd, writepd);
        
        if (sizeof(marker) != read(readpd, &marker, sizeof(marker))) {
            fprintf(stderr, "[%d] worker with pid: %d read error (%d): %s\n",
                    (unsigned)time(NULL), getpid(), errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        marker++;
        
        if(sizeof(marker) != write(writepd, &marker, sizeof(marker))) {
            fprintf(stderr, "[%d] worker with pid: %d {ds: %d} write error (%d): %s\n",
                    (unsigned)time(NULL), getpid(), writepd, errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

int run_pipeline(pid_t *pool, int size, int pipes[][2], int marker)
{
    debug_print("[%d] synchronously loop the marker ...\n", (unsigned)time(NULL));
    
    for (int j = 0; j < 10; j++) {
        //send marker to the first pipe
        if(sizeof(marker) != write(pipes[0][1], &marker, sizeof(marker))) {
            debug_print("[%d] error writing to pipe\n", (unsigned)time(NULL));
            perror("write");
            exit(EXIT_FAILURE);
        }
        
        for (int i = 0; i < size; i++) {
            debug_print("[%d] wakeup child: %d\n", (unsigned)time(NULL), pool[i]);
            
            //send signal to wake up the worker
            if (kill(pool[i], SIGCONT)) {
                fprintf(stderr, "[%d] kill pid %d error (%d): %s\n",
                        (unsigned)time(NULL), pool[i], errno, strerror(errno));
                exit(EXIT_FAILURE);
            }
            
            //wait worker to finish it's work (SIGCHLD is used)
            pause();
        }
        
        //read from the last pipe
        if(sizeof(marker) != read(pipes[size][0], &marker, sizeof(marker))) {
            debug_print("[%d] parent: error reading from pipe\n", (unsigned)time(NULL));
            perror("read");
            exit(EXIT_FAILURE);
        }
    }
    
    return marker;
}
