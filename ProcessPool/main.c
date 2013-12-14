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

//#define DEBUG 1
#define debug_print(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, "%s():%d: " fmt,  __func__, __LINE__, __VA_ARGS__); } while (0)

/* function declaration */
void worker(int readpd, int writepd);
void sigchld_handler(int sig);

int main(int argc, const char * argv[])
{
    /* constants */
    const int POOL_SIZE = 5;
    
    //process pool
    pid_t pool[POOL_SIZE];
    
    //marker buffer
    int marker = 0;
    
    //parent write - children read
    int write_pd[POOL_SIZE][2];
    
    //parent read - children write
    int read_pd[POOL_SIZE][2];
    
    signal(SIGCHLD, sigchld_handler);
    
    debug_print("parent pid: %d\n", getpid());
    
    //create the process pool
    for (int i = 0; i < POOL_SIZE; i++) {
        //initialize pipes
        pipe(write_pd[i]);
        pipe(read_pd[i]);
        
        pid_t cpid = fork();
        if (cpid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        if (cpid) { //parent
            pool[i] = cpid;
            
            close(write_pd[i][0]);
            close(read_pd[i][1]);
            
            //wait child to setup
//            pause();
        } else { //child
            close(write_pd[i][1]);
            close(read_pd[i][0]);
            
            //start worker - it does not return
            worker(write_pd[i][0], read_pd[i][1]);
            exit(EXIT_FAILURE);
        }
    }
    
    debug_print("[%d] all children ready and wating parent\n", (unsigned)time(NULL));
    debug_print("[%d] sending signals from parent ...\n", (unsigned)time(NULL));
    debug_print("%s------------------------------------------------\n", "");
    
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < POOL_SIZE; i++) {
            debug_print("[%d] wakeup child: %d\n", (unsigned)time(NULL), pool[i]);
            
            //send signal to wake up the child
//            if (kill(pool[i], SIGCONT)) {
//                perror("kill");
//                exit(EXIT_FAILURE);
//            }
            
            //send the marker to child, when it's done it writes it back
            //read/write are blocking, so parent wait until child is done and written to the pipe
            if(sizeof(marker) != write(write_pd[i][1], &marker, sizeof(marker))) {
                debug_print("[%d] error writing to pipe\n", (unsigned)time(NULL));
                perror("write");
                exit(EXIT_FAILURE);
            }
            
            if (sizeof(marker) != read(read_pd[i][0], &marker, sizeof(marker))) {
                debug_print("[%d] parent: error reading from pipe\n", (unsigned)time(NULL));
                perror("read");
                exit(EXIT_FAILURE);
            }
            
            debug_print("[%d] read marker %d from pid: %d\n", (unsigned)time(NULL), marker, pool[i]);
        }
    }
    
    //kill all children
    for (int i = 0; i < POOL_SIZE; i++) {
        if (kill(pool[i], SIGINT)) {
            perror("kill");
            exit(EXIT_FAILURE);
        }
    }
    
    printf("[%d] all done. marker value: %d\n", (unsigned)time(NULL), marker);
    return EXIT_SUCCESS;
}

void worker(int readpd, int writepd)
{
    int marker;
    
    //worker main loop, never quit
    //worker quits when SIGQUIT is received
    while (1) {
        debug_print("[%d] worker with pid: %d waiting parent signal ...\n", (unsigned)time(NULL), getpid());
        
        //wait parent commands, this also send SIGCHLD to parent so it knows we're ready
//        if (kill(getpid(), SIGSTOP)) {
//            perror("kill");
//            exit(EXIT_FAILURE);
//        }
        
        //waked up - do some stuff
        debug_print("[%d] worker with pid: %d working ... \n", (unsigned)time(NULL), getpid());
        if (sizeof(marker) != read(readpd, &marker, sizeof(marker))) {
            fprintf(stderr, "[%d] worker with pid: %d - error reading from pipe\n", (unsigned)time(NULL), getpid());
            perror("read");
            exit(EXIT_FAILURE);
        }
        
        marker++;
        
        if(sizeof(marker) != write(writepd, &marker, sizeof(marker))) {
            debug_print("[%d] worker with pid: %d - error writting to pipe\n", (unsigned)time(NULL), getpid());
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
}

void sigchld_handler(int sig)
{
    pid_t p;
    int status;
    
    while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Handle the death of pid p */
        debug_print("[%d] handling signal %d of: %d\n", (unsigned)time(NULL), sig, p);
    }
}
