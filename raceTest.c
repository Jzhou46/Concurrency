//
// Jeffrey Zhou
// JZhou46
// CS 361
// pThreads and Synchronization
// 12/5/14
//
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<time.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<pthread.h>
#include<unistd.h>
#include<math.h>
#include<time.h>

//semun union
union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *_buf;
};

//Worker struct
struct worker_struct{
	int nBuffers;
	int workerID;
	double sleepTime;
	int semID;
	int mutexID;
	int *buffers;
	int nReadErrors;
	int nWorkers; //Additional
	bool lockVal;//Additional
};

//Function to remove semaphores
void removeSemaphores(int semID, int mutexID){
        union semun semUnion;
        if(semctl(semID, 0, IPC_RMID, semUnion) == -1){
                perror("ERROR: semctl failed\n");
                exit(0);
        }
        if(semctl(mutexID, 0, IPC_RMID, semUnion) == -1){
                perror("ERROR: semctl failed\n");
                exit(0);
        }
}

//Thread Function
void * worker(void * thread){
	int i, bufferIndex, before, after, read, count = 0, count2 = 1;
	struct worker_struct *w = (struct worker_struct*)thread;
	bufferIndex = w->workerID;//Buffer Index
	int correct =  ( 1 << w->nWorkers ) - 1;
	int diff;
	struct sembuf lock = {bufferIndex, -1, 0};
	struct sembuf unlock = {bufferIndex, 1, 0};
	while(1){
		bufferIndex = bufferIndex % w->nBuffers;
		lock.sem_num = bufferIndex;
		unlock.sem_num = bufferIndex;
		if(count == 0 || count == 1){//Read operation
			if(w->lockVal == true){
				if(semop(w->semID, &lock, 1) == -1){//Lock semaphore
					printf("one\n");
					printf("ERROR: semop failed\n");
					removeSemaphores(w->semID, w->mutexID);
					pthread_exit(NULL);
				}
			}
			//Read the initial value in the buffer 
			before = w->buffers[bufferIndex];
			//Sleep for sleepTime seconds
			usleep(w->sleepTime * 1000000);
			//Read the value of the buffer again
			after = w->buffers[bufferIndex];
			if(w->lockVal == true){
				if(semop(w->semID, &unlock, 1) == -1){//Unlock semaphore
					printf("ERROR: semop failed\n");
					removeSemaphores(w->semID, w->mutexID);
					pthread_exit(NULL);
				}
			}
			if(before != after){//If there is a change
				//print a message to the screen if the value changed while the worker was sleeping.
				diff = before ^ after;
				int positive = before ^ diff;
				int negative = after ^ diff;
				if(w->lockVal == true){
					if(semop(w->mutexID, &lock, 1) == -1){//lock mutex
						printf("three\n");
						printf("ERROR: semop failed\n");
						removeSemaphores(w->semID, w->mutexID);
						pthread_exit(NULL);
					}
				}
				printf("Worker number %d reported change from %d to %d in buffer %d. Bad bits =", w->workerID, before, after, bufferIndex);
				for(i = 0; i < w->nWorkers; i++){
					if((negative & ( 1 << i)) && (positive & (1 << i)));//If the same bits, do nothing
					else{
						if(negative & ( 1 << i)){
							printf(" -%d", (int)log2((negative) & ( 1 << i)));
						}
						if(positive & (1 << i)){
							printf(" +%d", (int)log2((positive) & ( 1 << i)));
						}
					}
				}
				if(w->lockVal == true){
					if(semop(w->mutexID, &unlock, 1) == -1){//unlock mutex
						printf("ERROR: semop failed\n");
						removeSemaphores(w->semID, w->mutexID);
						pthread_exit(NULL);
					}
				}
				printf("\n");
				w->nReadErrors++;
			}//End of if clause
		}//End of else clause
		if(count == 2){//Write operation
			if(w->lockVal == true){
				if(semop(w->semID, &lock, 1) == -1){//lock semeaphore
					printf("ERROR: semop failed\n");
					removeSemaphores(w->semID, w->mutexID);
					pthread_exit(NULL);
				}
			}
			read = w->buffers[bufferIndex];
			usleep(w->sleepTime * 1000000);//Sleep for sleepTime seconds
			w->buffers[bufferIndex] = read + (1 << (w->workerID - 1));
			if(w->lockVal == true){
				if(semop(w->semID, &unlock, 1) == -1){//unlock semaphore
					printf("ERROR: semop failed\n");
					removeSemaphores(w->semID, w->mutexID);
					pthread_exit(NULL);
				}
			}	
			if(bufferIndex == 0){//If write[0]
				//printf("The number of acceses = %d\n", count2);
				pthread_exit(NULL);
			}
		}
		bufferIndex = bufferIndex + w->workerID;//Increment index value
		count2++;
		count = (count + 1) % 3;
	}//End of while loop
	pthread_exit(NULL);
} 

//Function to initialize all fields of each struct in the array of structs
void initArrayStruct(struct worker_struct *structArray, double *sleepTime, int nWorkers, int nBuffers, double sleepMin, double sleepMax, int *buffers, int nReadErrors, int semID, int mutexID, bool lockVal){
	int i;
	for(i = 0; i < nWorkers; i++){
                double  x = sleepMin + (sleepMax - sleepMin) * rand() / RAND_MAX;
                structArray[i].nBuffers = nBuffers;
                structArray[i].workerID = i + 1;
                structArray[i].sleepTime = x;
                structArray[i].semID = semID;
                structArray[i].mutexID = mutexID;
                structArray[i].buffers = buffers;
                structArray[i].nReadErrors = nReadErrors;
		structArray[i].nWorkers = nWorkers;
		structArray[i].lockVal = lockVal;
		sleepTime[i] = x;
		
        }
}

//Function to create N threads
void createThreads(pthread_t *threads, struct worker_struct *structArray, int nWorkers){
	int i, ret;
	for(i = 0; i < nWorkers; i++){		
		ret = pthread_create(&threads[i], NULL, worker, &structArray[i]);
		if(ret != 0) {
			printf("pthread error!\n");
			exit(0);
		}
	}
}


//Function to wait for the threads
void waitForThreads(pthread_t *threads, int nWorkers){
	int i;
	for(i = 0; i < nWorkers; i++){
		pthread_join(threads[i], NULL);
	}
}

//Function to get the total number of read errors
int getTotalReadErrors(struct worker_struct *structArray, int nWorkers){
	int i, nReadErrors = 0;
	for(i = 0; i < nWorkers; i++){
		nReadErrors = nReadErrors + structArray[i].nReadErrors;
	}
	return nReadErrors;
}
//Function to get the total number of write errors as well as the bad bits
int getTotalWriteErrors(int *buffers, int nBuffers, int nWorkers){
	int nWriteErrors = 0;
	int correct =  ( 1 << nWorkers ) - 1;
	int i, j, diff;
	for(i = 0; i < nBuffers; i++){
		if(buffers[i] != correct){//If buffer value is not correct
			diff = buffers[i] ^ correct;
			printf("Error in buffer %d. Bad bits =", i);
			for(j = 0; j < nWorkers; j++){
				if(diff & ( 1 << j) ){
					printf(" %d", (int)log2(diff & ( 1 << j)));
					nWriteErrors++;
				}		
			}
			printf("\n");	
		}
	}
	return nWriteErrors;
}

//Recursive helper function to help the isPrime() function below
bool factorInRange(int m, int n){
        if(m >= n)
                return false;
        else if( n % m == 0)
                return true;
        else if( m == 2)
                return factorInRange(m + 1, n);
        else if( m != 2)
                return factorInRange(m + 2, n);
}

//Funtion to check if integer is prime
bool isPrime(int n){
        if(n < 2)
                return false;
        else if(factorInRange(2, n) == false)
                return true;
        else
                return false;
}

//Function to compare 2 doubles and return in decreasing order for qsort
int cmpfunc (const void * a, const void * b){
	if (*(double*)b > *(double*)a)
		return 1;
	else if (*(double*)b < *(double*)a)
		return -1;
	else
		return 0;  
}


int main(int argc, char *argv[]){
	int i, nBuffers, nWorkers, randSeed, semID, mutexID;
	double sleepMin, sleepMax;
	char *lock;
	int nReadErrors = 0;//Counter
	union semun semUnion;
	bool lockVal;
	printf("-------------------------\n");
	printf("  Jeffrey Zhou\n  Jzhou46\n");
	printf("-------------------------\n");
	if(argc < 5 || argc > 7){
		printf("ERROR: THE AMOUNT OF ARGUMENTS ENTER IS INVALID\n");
		printf("YOUR COMMAND LINE SHOULD CONTAIN THESE ARGUMENTS: raceTest nBuffers nWorkers sleepMin sleepMax [ randSeed ] [ -lock | -nolock ]\n");
		return 0;
	}
	else if (argc == 7){	
		nBuffers = atoi(argv[1]);
		if(nBuffers < 2 || nBuffers > 32){
			printf("ERROR: Value for nBuffers must be between 2 and 32\n");
			return 0;
		}
		if(isPrime(nBuffers) == false){
			printf("ERROR: Value for nBuffers must be prime\n");
			return 0;
		}
		nWorkers = atoi(argv[2]);
		if(nWorkers < 0 || nWorkers >= nBuffers){
			printf("ERROR: Value for nWorkers must be a positive number less than nBuffers.\n");
			return 0; 
		}
		sleepMin = atof(argv[3]);
		if(sleepMin < 0.0){
			printf("ERROR: sleepMin can not be negative\n");
			return 0;
		}
		sleepMax = atof(argv[4]);
		if(sleepMax < 0.0){
			printf("ERROR: sleepMax can not be negative\n");
			return 0;
		}
		if(sleepMin >= sleepMax){
			printf("ERROR: sleepMin can not be greater than or equal to sleepMax\n");
			return 0;
		}
		randSeed = atoi(argv[5]);
		if(randSeed < 0){
			printf("ERROR: randSeed can not be a negative number\n");
			return 0;
		}
		if(randSeed == 0){
			srand (time(NULL));
			printf("NOTE: The random number generator seeded with time( NULL )\n");
		}
		else{
			srand(randSeed);
		}
		lock = argv[6];
		if(strcmp(lock, "-lock") != 0  && strcmp(lock, "-nolock") != 0){
			printf("ERROR: invalid string values for this argument: [ -lock | -nolock ]\n");
			return 0;
		}
	}
	else if(argc == 6){
		nBuffers = atoi(argv[1]);
		if(nBuffers < 2 || nBuffers > 32){
			printf("ERROR: Value for nBuffers must be between 2 and 32\n");
			return 0;
		}
		if(isPrime(nBuffers) == false){
			printf("ERROR: Value for nBuffers must be prime\n");
			return 0;
		}
		nWorkers = atoi(argv[2]);
		if(nWorkers < 0 || nWorkers >= nBuffers){
			printf("ERROR: Value for nWorkers must be a positive number less than nBuffers.\n");
			return 0;
		}
		sleepMin = atof(argv[3]);
		if(sleepMin < 0.0){
			printf("ERROR: sleepMin can not be negative\n");
			return 0;
		}
		sleepMax = atof(argv[4]);
		if(sleepMax < 0.0){
			printf("ERROR: sleepMax can not be negative\n");
			return 0;
		}
		if(sleepMin >= sleepMax){
			printf("ERROR: sleepMin can not be greater than or equal to sleepMax\n");
			return 0;
		}
		randSeed = atoi(argv[5]);
		if(randSeed < 0){
			printf("ERROR: randSeed can not be a negative number\n");
			return 0;
		}
		if(randSeed == 0){
			srand (time(NULL));
			printf("NOTE: The random number generator seeded with time( NULL )\n");
		}
		else{
			srand(randSeed);
		}
		lock = "-nolock";
		printf("NOTE: Defaulted to -nolock, i.e. without using semaphores\n");

	}
	else if(argc == 5){
		nBuffers = atoi(argv[1]);
		if(nBuffers < 2 || nBuffers > 32){
			printf("ERROR: Value for nBuffers must be between 2 and 32\n");
			return 0;
		}
		if(isPrime(nBuffers) == false){
			printf("ERROR: Value for nBuffers must be prime\n");
			return 0;
		}
		nWorkers = atoi(argv[2]);
		if(nWorkers < 0 || nWorkers >= nBuffers){
			printf("ERROR: Value for nWorkers must be a positive number less than nBuffers.\n");
			return 0;
		}
		sleepMin = atof(argv[3]);
		if(sleepMin < 0.0){
			printf("ERROR: sleepMin can not be negative\n");
			return 0;
		}
		sleepMax = atof(argv[4]);
		if(sleepMax < 0.0){
			printf("ERROR: sleepMax can not be negative\n");
			return 0;
		}
		if(sleepMin >= sleepMax){
			printf("ERROR: sleepMin can not be greater than or equal to sleepMax\n");
			return 0;
		}
		srand (time(NULL));
		printf("NOTE: The random number generator is seeded with time( NULL )\n");
		lock = "-nolock";
		printf("NOTE: Defaulted to -nolock, i.e. without using semaphores\n");
	}
	char string[10];
	if(strcmp(lock, "-nolock") == 0){
		strcpy(string, "without");
		lockVal = false;
	}
	else if(strcmp(lock, "-lock") == 0){
		strcpy(string, "with");
		lockVal = true;
	}
	printf("\nRunning simulation for %d thread(s) accessing %d buffers, %s locking.....\n\n", nWorkers, nBuffers, string);

	//create a local array of N integers, and initialize them all to zero. 
	int *buffers = malloc(sizeof(int) * nBuffers);
	for(i = 0; i < nBuffers; i++){
		buffers[i] = 0;
	}

	//create an array of M doubles, and fill the array with random values in the range from sleepMin to sleepMax. 	
	double *sleepTime = malloc(sizeof(double) * nWorkers);

	//Main( ) should create a single semaphore using semget( ), and use semctl( ) to initialize it to 1. The ID should be stored in the structs passed to the worker( ) functions
	if((semID = semget(IPC_PRIVATE, nBuffers, 0600 | IPC_CREAT)) == -1){//Create semaphore
		perror("ERROR: semget failed\n");
		exit(0);
	}
	if((mutexID = semget(IPC_PRIVATE, 1, 0600 | IPC_CREAT)) == -1){//Create mutex
		perror("ERROR: semget failed\n");
		exit(0);
	}
	unsigned short *semVals = malloc(sizeof(unsigned short) * nBuffers);
	for(i = 0; i <  nBuffers; i++){
		semVals[i] = 1;
	}
	semUnion.array = semVals;
	if(semctl(semID, 0, SETALL, semUnion) == -1){
		printf("hi\n");
		removeSemaphores(semID, mutexID);
		perror("ERROR: semctl failed\n");
		exit(0);
	}
	semUnion.val = 1;
	if(semctl(mutexID, 0, SETVAL, semUnion) == -1){
		removeSemaphores(semID, mutexID);
		perror("ERROR: semctl failed\n");
		exit(0);
	}
	//create an array of M structs, and populate each struct with the values needed for one of the threads.	
	struct worker_struct *structArray = malloc(sizeof(struct worker_struct) * nWorkers); 
	initArrayStruct(structArray, sleepTime, nWorkers, nBuffers, sleepMin, sleepMax, buffers, nReadErrors, semID, mutexID, lockVal);	
	qsort(sleepTime, nWorkers, sizeof(double), cmpfunc);//Sort the random values array

	//Printing the results of sleepTime 
	printf("---RANDOM SlEEPTIMES---\n");
	for(i = 0; i < nWorkers; i++){
		printf("sleepTime[%d] = %lf\n", i, sleepTime[i]);
	}

	//Create an array of threads
	pthread_t *threads = malloc(sizeof(pthread_t) * nWorkers);	
	createThreads(threads, structArray, nWorkers);

	printf("\nNOTE: All bits start at 0\n");

	printf("\n-----READ ERRORS-----\n");	
	printf("NOTE: A negative value represents a bit that was lost\n");
	printf("NOTE: A positive value represents a bit that was gained\n\n");

	//After having created all the workers, main( ) should then call pthread_join( ) for each of the threads to wait for them to finish
	waitForThreads(threads, nWorkers);	

	//After all the threads have completed, main( ) should report the values stored in each of the buffers 
	printf("\n---BUFFER VALUES---\n");
	for(i = 0; i < nBuffers; i++){
		printf("buffers[%d] = %d\n", i, buffers[i]);
	}
	printf("\n");

	//After all threads have finished, main( ) should examine the contents of the shared buffers, and report any write errors detected. 	
	int nWriteErrors = 0;
	int correct =  ( 1 << nWorkers ) - 1;
	printf("All buffers should hold %d\n\n", correct);
	nReadErrors = getTotalReadErrors(structArray, nWorkers);	
	printf("-----WRITE ERRORS-----\n");
	nWriteErrors = getTotalWriteErrors(buffers, nBuffers, nWorkers);
	printf("\n%d Read errors and %d write errors encountered\n\n", nReadErrors, nWriteErrors);
	//Printing the values in each workers
	/*
	   printf("\n---WORKER VALUES---\n");	
	   for(i = 0; i < nWorkers; i ++){
	   printf("nBuffers = %d\n", structArray[i].nBuffers);
	   printf("workerID = %d\n", structArray[i].workerID);
	   printf("sleepTime = %lf\n", structArray[i].sleepTime);
	   printf("semID = %d\n", structArray[i].semID);
	   printf("mutexID = %d\n", structArray[i].mutexID);
	   printf("buffers = %d\n", *structArray[i].buffers);
	   printf("nReadErrors = %d\n", structArray[i].nReadErrors);
	   printf("nWorkers = %d\n", structArray[i].nWorkers);
	   printf("\n");
	   }
	 */
	removeSemaphores(semID, mutexID);

	//Deallocate arrays
	free(buffers);
	free(sleepTime);
	free(semVals);
	free(structArray);
	free(threads);
	return 0;
}

