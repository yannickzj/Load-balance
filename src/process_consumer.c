#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/sem.h>
#include <time.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>

#define MSGEND '!'

//produce [0,1] random numbers
double uRandom()   
{
	return rand()*1.0/RAND_MAX;
}

//produce poisson distrubuted random numbers
int poisson(int lambda)  
{
	if (lambda>100) {
		fprintf(stderr, "lambda is too large\n");
		return lambda;
	}
    int k = 0;
    long double p = 1.0;
	long double l=exp(-lambda);  
    while (p>=l)
    {
	    double u = uRandom();
	    p *= u;
	    k++;
	}
    return k-1;
}

//function to get integer random number
int randomInt (int parm) {
	//return rand()%parm+1;
	return poisson(parm);
} 

//function to produce random char
char randomChar() {
	return 'a'+rand()%26;
}

//function to  process request
void request_process (int Ct1_parm, int Ct2_parm, float p_i) {
	float t = uRandom();
	int process_time;
	if (t<p_i) {
		process_time = randomInt(Ct1_parm);
	} else {
		process_time = randomInt(Ct2_parm);	
	}
	usleep (process_time*1000);
}


//define union semun
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

//transmission state structure
struct stateInfo {
	int requestSnd;
	int requestPrc;
	int numBlocked;
	int numIdle;
	float timeBlocked;
	float timeIdle;
};


//wait on the 'consumer' semaphore
int consumer_wait (int semid)
{
	struct sembuf operations[1];
	operations[0].sem_num = 0;
	operations[0].sem_op = -1;
	operations[0].sem_flg = SEM_UNDO;
	return semop (semid, operations, 1);
}

//post to the 'consumer' semaphore
int consumer_post (int semid)
{
	struct sembuf operations[1];
	operations[0].sem_num = 0;
	operations[0].sem_op = 1;
	operations[0].sem_flg = SEM_UNDO;
	return semop (semid, operations, 1);
}

int msq_getQnum (int msqid) {
	struct msqid_ds msg_info;
	if (msgctl(msqid, IPC_STAT, &msg_info)!=0) {
		fprintf(stderr, "msgctl error\n");
		exit(-1);
	}
	return msg_info.msg_qnum;
}

time_t msq_getRtime (int msqid) {
	struct msqid_ds msg_info;
	if (msgctl(msqid, IPC_STAT, &msg_info)!=0) {
		fprintf(stderr, "msgctl error\n");
		exit(-1);
	}
	return msg_info.msg_rtime;

}

int main(int argc, char* argv[]) {
	
	//get input data
	int Ct1_parm = atoi(argv[1]);
	int Ct2_parm = atoi(argv[2]);
	float p_i = atof(argv[3]);
	key_t key = atoi(argv[4]);
	long mtype = atoi(argv[5]);
	key_t key_sem = atoi(argv[6]);
	int segmentID = atoi(argv[7]);
	srand((unsigned) time(NULL));	

	//attach the shared memory segment and initialization
	struct stateInfo* shared_memory = (struct stateInfo*) shmat(segmentID, 0, 0);
	
	//define buffer structure
	typedef struct msgbuf {
		long mtype;
		char randomChar;
	} msg_buf;

	//get existing message queue
	int msqid;
	if ((msqid = msgget(key, 0))<0) {
		fprintf(stderr, ("producer msgget error!\n"));
		exit(1);
	}

	//get existing semaphore
	int semid;
	if ((semid = semget(key_sem, 2, IPC_CREAT | 0666))<0) {
		fprintf(stderr, ("producer semget error!\n"));
		exit(1);
	}

	//set consumer parameters
	msg_buf msg_rbuf;
	msg_rbuf.mtype = mtype; 
	int flg_idle = 0;
	struct timeval t1;
	struct timeval t2;

	while(1) {
		//wait for sem_c
		consumer_wait(semid);

		//receive request from the message queue
		int message = 0;
		while (message!=MSGEND) {
			//check whether the queue is empty
			if (msq_getQnum(msqid)==0) {
				flg_idle = 1;
				shared_memory->numIdle++;
				gettimeofday(&t1, NULL);
			}


			if (msgrcv(msqid, (void*) &msg_rbuf, sizeof(char), mtype, 0)>0) {
				//record the idle info	
				if (flg_idle) {
					flg_idle = 0;
					gettimeofday(&t2, NULL);
					shared_memory->timeIdle += (t2.tv_sec + t2.tv_usec/1000000.0) - (t1.tv_sec + t1.tv_usec/1000000.0);
				}

				message = msg_rbuf.randomChar;
			} else {
				fprintf(stderr, ("consumer msgrcv error!\n"));
				exit(1);
			}

		}

		//post sem_c
		consumer_post(semid);

		//process request
		request_process(Ct1_parm, Ct2_parm, p_i);

		shared_memory->requestPrc++;
	
	}

	return 0;
}



