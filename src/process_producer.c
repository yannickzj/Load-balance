#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <sys/sem.h>
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

//function to producer random char
char randomChar() {
	return 'a'+rand()%26;
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


//wait on the 'producer' semaphore
int producer_wait (int semid)
{
	struct sembuf operations[1];
	operations[0].sem_num = 1;
	operations[0].sem_op = -1;
	operations[0].sem_flg = SEM_UNDO;
	return semop (semid, operations, 1);
}

//post to the 'producer' semaphore
int producer_post (int semid)
{
	struct sembuf operations[1];
	operations[0].sem_num = 1;
	operations[0].sem_op = 1;
	operations[0].sem_flg = SEM_UNDO;
	return semop (semid, operations, 1);
}

//function to get the message number in the queue
int msq_getQnum (int msqid) {
	struct msqid_ds msg_info;
	if (msgctl(msqid, IPC_STAT, &msg_info)!=0) {
		fprintf(stderr, "msgctl error\n");
		exit(-1);
	}
	return msg_info.msg_qnum;
}

int main(int argc, char* argv[]) {
	
	//get input data
	int Pt_parm = atoi(argv[1]);
	int Rs_parm = atoi(argv[2]);
	key_t key_msq = atoi(argv[3]);
	long mtype = atoi(argv[4]);
	key_t key_sem = atoi(argv[5]);
	int segmentID = atoi(argv[6]);
	int B = atoi(argv[7]);
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
	if ((msqid = msgget(key_msq, 0))<0) {
		fprintf(stderr, ("producer msgget error!\n"));
		exit(1);
	}

	//get existing semaphore
	int semid;
	if ((semid = semget(key_sem, 2, IPC_CREAT | 0666))<0) {
		fprintf(stderr, ("producer semget error!\n"));
		exit(1);
	}

	//set producer parameters
	msg_buf msg_sbuf;
	msg_sbuf.mtype = mtype; 
	int flg_blk = 0;
	struct timeval t1;
	struct timeval t2;

	while(1) {
		//produce P_t and R_s
		int P_t = randomInt(Pt_parm);
		int R_s = randomInt(Rs_parm);
	
		//delay P_t
		usleep(P_t*1000);
	
		//wait for producer semaphore
		producer_wait(semid);

		//put the request in the message queue
		int countSnd = 0;
		while(countSnd<R_s) {
			msg_sbuf.randomChar = randomChar();
			
			//check whether the queue is full
			if (msq_getQnum(msqid)==B) {
				flg_blk = 1;
				shared_memory->numBlocked++;
				gettimeofday(&t1, NULL);
			}
			
			if (msgsnd(msqid, &msg_sbuf, sizeof(char), 0)==0) {
				//record the blocked info
				if (flg_blk) {
					flg_blk = 0;
					gettimeofday(&t2, NULL);
					shared_memory->timeBlocked += (t2.tv_sec + t2.tv_usec/1000000.0) - (t1.tv_sec + t1.tv_usec/1000000.0);
				}
				countSnd++;	
			} else {
				fprintf(stderr, ("producer msgsnd error!\n"));
				exit(1);
			}
		}

		//put the ending signal in the message queue
		msg_sbuf.randomChar = MSGEND;
		if (msgsnd(msqid, &msg_sbuf, sizeof(char), 0)!=0) {
			fprintf(stderr, ("producer msgsnd error!\n"));
			exit(1);
		}

		shared_memory->requestSnd++;

		//post to  producer semaphore
		producer_post(semid);

	}

	//detach the shared memory segment
	shmdt(shared_memory);

	return 0;
}


