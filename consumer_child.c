#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <sys/shm.h>
#include <time.h>

#include "consumer_child.h"

#define SEM_NAME "semaphore1_project"
#define SEM2_NAME "semaphore2_project"
#define SEM3_NAME "semaphore3_project"


int main(void){

    /****************** semaphore *******************************/
	
	sem_t *semaphore = sem_open(SEM_NAME, O_RDWR);
	sem_t *semaphore2 = sem_open(SEM2_NAME, O_RDWR);
	sem_t *semaphore3 = sem_open(SEM3_NAME, O_RDWR);
	
	if(semaphore == SEM_FAILED){
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }
	if(semaphore2 == SEM_FAILED){
        perror("sem2_open(3) failed");
        exit(EXIT_FAILURE);
    }
	if(semaphore3 == SEM_FAILED){
        perror("sem3_open(3) failed");
        exit(EXIT_FAILURE);
    }

   /****************** shared memory *******************************/

	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;
	int shmid;
	
	
	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
	// shmget failed
	if (shmid == -1){
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}

	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1){
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	
	shared_stuff = (struct shared_use_st *)shared_memory;

    /********************** critical section ************************/
	srand((unsigned int)getpid());
	time_t start_time, end_time;
	double diff_time;
	
	double m_o = 0;
	int random_num;
	for(int j=0; j < shared_stuff->N_processes; j++){

		printf("child: sem1 down\n");
		if(sem_wait(semaphore) < 0){
			perror("sem_wait(3) failed on child");
			continue;
		}

		random_num = rand()%((shared_stuff->lines_of_file)-1) + 1;
		shared_stuff->line = random_num;
		
		fprintf(stderr, "Child %ld - %d to parental process: I want the line %d.\n", (long)getpid(),j,random_num);
		time(&start_time);
		
		printf("child: sem3 up\n");
		if(sem_post(semaphore3) < 0){
			perror("sem_post(3) failed on child");
			continue;
		}

		printf("child: sem2 down\n");
		if (sem_wait(semaphore2) < 0){
			perror("sem_wait(3) failed on child");
			continue;
		}
		
		printf("Parent to child process %ld: %s\n",(long)getpid(), shared_stuff->some_text);
		
		time(&end_time);

		printf("child: sem1 up\n");
		if(sem_post(semaphore) < 0){
			perror("sem_post(3) failed on child");
			continue;
		}
		
		diff_time = difftime(end_time, start_time);
		m_o += diff_time ;
			
	}

	m_o = m_o/shared_stuff->N_processes;
	printf("Child %ld exit with %f avarage time\n", (long)getpid(), m_o);
	
	shared_stuff->counter_childs++;
	if(shared_stuff->counter_childs == shared_stuff->childs){
			printf("child: sem3 up\n");
			if(sem_post(semaphore3) < 0){
				perror("sem_post(3) failed on child");
			}
	}

	/*************************  *********  ****************************/
	
	if(shmdt(shared_memory) == -1){
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
	if(shmctl(shmid, IPC_RMID, 0) == -1){
		fprintf(stderr, "shmctl(IPC_RMID) failed\n");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);

	if(sem_close(semaphore) < 0)
        perror("sem_close(3) failed");		
	if(sem_close(semaphore2) < 0)
        perror("sem2_close(3) failed");
	if(sem_close(semaphore3) < 0)
        perror("sem3_close(3) failed");

    return 0;
}