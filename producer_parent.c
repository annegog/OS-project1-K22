#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>

#include "producer_parent.h"

#define SEM_NAME "semaphore1_project"
#define SEM2_NAME "semaphore2_project"
#define SEM3_NAME "semaphore3_project"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define INITIAL_VALUE 1

#define CHILD_PROGRAM "./consumer_child"

#define BUFSIZE 100

int main(int argc, char *argv[]){

    if (argc != 4){   // check for valid number of command-line arguments
        fprintf (stderr, "Usage: %s numprocesses\n", argv[0]);
        return 1;
    }

    /****************** semaphore *******************************/
    
    // We initialize the semaphore counter to 1 (INITIAL_VALUE)
    sem_t *semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, SEM_PERMS, INITIAL_VALUE);
    // initialize them to 0. 
    // So we are going to block some processes
    sem_t *semaphore2 = sem_open(SEM2_NAME, O_CREAT | O_EXCL, SEM_PERMS, 0);
    sem_t *semaphore3 = sem_open(SEM3_NAME, O_CREAT | O_EXCL, SEM_PERMS, 0);

    // sem_close(semaphore);
    // sem_unlink(SEM_NAME);
    // sem_close(semaphore2);
    // sem_unlink(SEM2_NAME);
    // sem_close(semaphore3);
    // sem_unlink(SEM3_NAME);

    if(semaphore == SEM_FAILED){
        perror("sem_open(3) error");
        exit(EXIT_FAILURE);
    }
    if(semaphore2 == SEM_FAILED){
        perror("sem2_open(3) error");
        exit(EXIT_FAILURE);
    }
    if(semaphore3 == SEM_FAILED){
        perror("sem3_open(3) error");
        exit(EXIT_FAILURE);
    }

    /****************** arguments *******************************/

    FILE *X = fopen(argv[1], "r"); // "r" = open for reading 
    int K = atoi(argv[2]); // childs 
    int N = atoi(argv[3]);  // processes 
    	
    char buff[BUFSIZE]; // a buffer to hold what we read in
    int counter = 0;    // lines of the file
    while(fgets(buff, BUFSIZE - 1, X) != NULL){
        counter++;
    }
    fclose(X);
    // printf("Lines of the file: %d\n", counter);

   /****************** shared memory *******************************/

	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;
	int shmid;

	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
	if(shmid == -1){
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}

    shared_memory = shmat(shmid, (void *)0, 0);
	if(shared_memory == (void *)-1){
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	shared_stuff = (struct shared_use_st *)shared_memory;

    /****************** create childs *******************************/

    pid_t pids[K];
    size_t i;

    if(sem_wait(semaphore) < 0)
        perror("sem_wait(3) failed on parent");
    
    for(i = 0; i < K; i++){
        if((pids[i] = fork()) < 0){
            perror("fork(2) failed");
            exit(EXIT_FAILURE);
        }

        if(pids[i] == 0){
            if (execl(CHILD_PROGRAM, CHILD_PROGRAM, NULL) < 0){
                perror("execl(2) failed");
                exit(EXIT_FAILURE);
            }
            break;
        }
    }

    if(sem_post(semaphore) < 0)
		perror("sem_post(3) failed on parent");

    /**************** start of critical section **********************/
   
    shared_stuff->N_processes = N;
    shared_stuff->lines_of_file = counter;
    shared_stuff->childs = K;
    shared_stuff->counter_childs = 0;
    
    int running = 1; 

	while(running){
        
        printf("parent: sem3 down\n");
        if(sem_wait(semaphore3) < 0){
		    perror("sem_wait(3) failed on parent");
		    continue;
	    }
        if(shared_stuff->counter_childs == shared_stuff->childs){
            running = 0;
            break;
        }
    
        int count = 0;
        fopen(argv[1], "r");
        while(fgets(buff, BUFSIZE - 1, X) != NULL){
            if (count == shared_stuff->line){
                strncpy(shared_stuff->some_text, buff, TEXT_SZ);
                printf("parent: sem2 up\n");
                if (sem_post(semaphore2) < 0) {
				    perror("sem_post(3) error on parent");
                }
                break;
            }   
            else{   
                count++;
            }   
        }
        fclose(X);
    }
    
    /****************** ******** *******************************/
    
    for(int y = 0; y < K; y++){
        // Remove the zombie process, and get the pid and return code
        if(waitpid(pids[y], NULL, 0) < 0)
            perror("waitpid(2) failed");
        if(pids[y] < 0){
            if (errno == ECHILD){
                printf("All children have exited\n");
                break;
            }
            else{
                perror("Could not wait");
            }
        }
        else{
            printf("Child %d exited!\n", y);
        }
    }

    /****************** ******** *******************************/


    if (shmdt(shared_memory) == -1){
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}

    sem_close(semaphore);
    sem_unlink(SEM_NAME);

    sem_close(semaphore2);
    sem_unlink(SEM2_NAME);

    sem_close(semaphore3);
    sem_unlink(SEM3_NAME);

    return 0;
}