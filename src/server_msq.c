#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/stat.h>


#define MSQKEY 1085
#define SEMKEY 2345
#define TIMEINTERVAL 1

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
	
	//timer initialization
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	srand((unsigned) time(NULL));	
	
	//set timer1
	gettimeofday(&tv1, NULL);

	//input check
	if (argc != 10) {
		fprintf(stderr, ("Input error!\n"));
		exit(1);
	}

	int T = atof(argv[1]);
	int B = atoi(argv[2]);
	int P = atoi(argv[3]);
	int C = atoi(argv[4]);
	int Pt_parm = atof(argv[5]);
	int Rs_parm = atoi(argv[6]);
	int Ct1_parm = atof(argv[7]);
	int Ct2_parm = atof(argv[8]);
	float p_i = atof(argv[9]);

	//define message structure
	typedef struct msgbuf {
		long mtype;
		char randomChar;
	} msg_buf;
	
	//creat a message queue
	int msqid;
	key_t key_msq = MSQKEY;
	int gflag = IPC_CREAT | IPC_EXCL | 0666;
	
	if ((msqid = msgget(key_msq, gflag))<0) {
		fprintf(stderr, ("msgget error!\n"));
		exit(1);
	} 
	
	//initialize the size of the message queue
	struct msqid_ds bufInit;
	if (msgctl(msqid, IPC_STAT, &bufInit)==-1) {
		fprintf(stderr, ("msgctl IPC_STAT error!\n"));
		exit(1);
	}
	bufInit.msg_qbytes = B * sizeof(char);
	if (msgctl( msqid, IPC_SET, &bufInit)==-1) {
		fprintf(stderr, ("msgctl IPC_SET error!\n"));
		exit(1);
	}

	//allocate a semaphore
	key_t key_sem = SEMKEY;
	int semid; 
	semid = semget (key_sem, 2, IPC_CREAT | 0666); 
	if (semid == -1) {
		fprintf(stderr, ("semget error: existing semaphore!\n"));
		union semun ignored_argument;
		semctl(semid, 2, IPC_RMID, ignored_argument);
		exit(1);
	}

	//semaphore initialization
	union semun argument;
	unsigned short values[2];
	values[0] = 1;
	values[1] = 1;
	argument.array = values;
	int tempValue = semctl(semid, 0, SETALL, argument);
	if (tempValue == -1) {
		fprintf(stderr, ("semctl initialization error!\n"));	
		union semun ignored_argument;
		semctl(semid, 2, IPC_RMID, ignored_argument);
		exit(1);
	}

	//shared memery allocation 
	int segmentID;
	const int shared_segment_size = sizeof(struct stateInfo);
	segmentID = shmget(IPC_PRIVATE, shared_segment_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
				
	//attach the shared memory segment and initialization
	struct stateInfo* shared_memory = (struct stateInfo*) shmat(segmentID, 0, 0);
	shared_memory->requestSnd = 0;
	shared_memory->requestPrc = 0;
	shared_memory->numBlocked = 0;
	shared_memory->numIdle = 0;
	shared_memory->timeBlocked = 0.0;
	shared_memory->timeIdle = 0.0;

	//set producer process arguments
	char* producerFile = (char*) "./process_producer";
	char msqkeyText[20];
	sprintf(msqkeyText, "%d", key_msq);
	char semkeyText[20];
	sprintf(semkeyText, "%d", key_sem);
	long mtype = 1;
	char mtypeText[10];
	sprintf(mtypeText, "%ld", mtype);
	char shmkeyText[20];
	sprintf(shmkeyText, "%d", segmentID);
	char* producer_argv[] = {
		producerFile,
		argv[5],
		argv[6],
		msqkeyText,
		mtypeText,
		semkeyText,
		shmkeyText,
		argv[2],
		NULL		
	};

	//set consumer process arguments
	char* consumerFile = (char*) "./process_consumer";
	char* consumer_argv[] = {
		consumerFile,
		argv[7],
		argv[8],
		argv[9],
		msqkeyText,
		mtypeText,
		semkeyText,
		shmkeyText,
		NULL		
	};

	//set timer2
	gettimeofday(&tv2, NULL);

	//creat producer process
	pid_t* producer_list;
	pid_t producer_pid;
	producer_list = (pid_t*) malloc(P*sizeof(pid_t));
	
	int i;
	for (i=0; i<P; i++) {
		producer_pid = fork();
		producer_list[i] = producer_pid;
		if(producer_pid<0) {
			fprintf(stderr, ("Error occured on forking.\n"));		
			exit(1);
		}
		else if(producer_pid==0){	
			//producer process	
			execvp(producerFile, producer_argv);
			fprintf(stderr, ("an error occurred in execv.\n"));
			abort();
		}
		producer_list[i] = producer_pid;
	}	

	//creat consumer process
	pid_t* consumer_list;
	pid_t consumer_pid;
	consumer_list = (pid_t*) malloc(C*sizeof(pid_t));
	
	for (i=0; i<C; i++) {
		consumer_pid = fork();
		consumer_list[i] = consumer_pid;
		if(consumer_pid<0) {
			fprintf(stderr, ("Error occured on forking.\n"));		
			exit(1);
		}
		else if(consumer_pid==0){	
			//consumer process	
			execvp(consumerFile, consumer_argv);
			fprintf(stderr, ("an error occurred in execv.\n"));
			abort();
		}
		consumer_list[i] = consumer_pid;
	}

	//set timer3
	gettimeofday(&tv3, NULL);

	//print time differences
	double t1 = tv1.tv_usec/1000000.0 + tv1.tv_sec;
	double t2 = tv2.tv_usec/1000000.0 + tv2.tv_sec;
	double t3 = tv3.tv_usec/1000000.0 + tv3.tv_sec;
	double dt1;
	double dt2;
	dt1 = t2-t1;
	dt2 = t3-t2;

	//wait for cetain execution time and display processing info
	printf("\n");
	int numSnd0 = 0;
	int numSnd1 = 0;
	int numPrc0 = 0;
	int numPrc1 = 0;
	int numBlk0 = 0;
	int numBlk1 = 0;
	int numIdle0 = 0;
	int numIdle1 = 0;
	float timeBlk0 = 0.0;
	float timeBlk1 = 0.0;
	float timeIdle0 = 0.0;
	float timeIdle1 = 0.0;
	int time=0;
	int numQueue;
	//store time differences in local file
	FILE *fp;
	char *fileName = (char*) "output_msq.txt";
	if ((fp=fopen(fileName, "a+"))==NULL) {
		fprintf(stderr, ("cannot open file!\n"));
		exit(1);
	}
	fprintf(fp, "T=%d, B=%d, P=%d, C=%d, Pt_parm=%d, Rs_parm=%d, Ct1_parm=%d, Ct2_parm=%d, p_i=%f\n", T, B, P, C, Pt_parm, Rs_parm, Ct1_parm, Ct2_parm, p_i);
	fprintf(fp, "initailization_time=%.6fs, fork_time=%.6fs\n", dt1, dt2);
	fprintf(fp, "%-15s%-15s%-15s%-15s%-15s%-15s%-15s%-15s\n", "T", "occupied", "empty", "numPrc", "numBlk", "timeBlk(s)", "numIdle", "timeIdle(s)");

	while (time<T) {
		sleep(TIMEINTERVAL);
		time += TIMEINTERVAL;
		//printf("T=%d\n", time);
		numSnd1 = shared_memory->requestSnd;
		numPrc1 = shared_memory->requestPrc;
		numBlk1 = shared_memory->numBlocked;
		numIdle1 = shared_memory->numIdle;
		timeBlk1 = shared_memory->timeBlocked;	
		timeIdle1 = shared_memory->timeIdle;	
		numQueue = msq_getQnum(msqid);
		fprintf(fp, "%-15d%-15d%-15d%-15d%-15d%-15.6f%-15d%-15.6f\n", time, numQueue, B-numQueue, numPrc1-numPrc0, numBlk1-numBlk0, timeBlk1-timeBlk0, numIdle1-numIdle0, timeIdle1-timeIdle0);
		numSnd0 = shared_memory->requestSnd;
		numPrc0 = shared_memory->requestPrc;
		numBlk0 = shared_memory->numBlocked;
		numIdle0 = shared_memory->numIdle;
		timeBlk0 = shared_memory->timeBlocked;	
		timeIdle0 = shared_memory->timeIdle;
	}
	
	fprintf(fp, "\n");

	//detach the shared memory segment
	shmdt(shared_memory);

	//terminate the child processes
	for (i=0; i<P; i++) {
		kill(producer_list[i], SIGTERM);		
	}

	for (i=0; i<C; i++) {
		kill(consumer_list[i], SIGTERM);		
	}

	//remove the message queue
	struct msqid_ds msg_info;
	msgctl(msqid, IPC_RMID, &msg_info);

	//remove the semaphore
	union semun ignored_argument;
	semctl(semid, 1, IPC_RMID, ignored_argument);

	//deallocate the shared memory segment
	shmctl (segmentID, IPC_RMID, 0);
	
	fclose(fp);

	return 0;
}

