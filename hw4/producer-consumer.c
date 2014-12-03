#include <errno.h>
#include <stdio.h>
#include <pthread.h>

#define THREAD_CNT 2

#define LIKELY_RET(X,Y,RET) \
    if((X)!= (Y)){ \
        perror(strerror(errno));  \
		return RET; \
	}

typedef struct sem_t{
        unsigned int value;
        pthread_mutex_t mutex;
}sem_t;

int sem_init(sem_t *sem, int pshared, unsigned int value){
	if(NULL == sem)
		return -1;
		
	sem->value = value;
	int err = pthread_mutex_init(&sem->mutex,NULL);
	LIKELY_RET(err,0,-1);
	return 0;
}


/*
EINVAL sem is not a valid semaphore.
*/
int sem_post(sem_t *sem){
	if(NULL == sem)
		return -1;	
		
	int err = pthread_mutex_lock(&sem->mutex);
	sem->value ++;
	err = pthread_mutex_unlock(&sem->mutex);
	LIKELY_RET(err,0,-1);
	return 0;
}

/*
EINVAL sem is not a valid semaphore.
*/
int sem_wait(sem_t *sem){
	if(NULL == sem)
		return -1;	

busywait:
	while(sem->value <=0); // busy wait. blocking here.
		
	/* critical setion upcomming.*/
	int err = pthread_mutex_lock(&sem->mutex);
	LIKELY_RET(err,0,-1);
	
	if(sem->value > 0){// my turn.
		sem->value --;
		err = pthread_mutex_unlock(&sem->mutex);
		LIKELY_RET(err,0,-1);		
	}
	else{// not my turn, go back to busy wait.
		err = pthread_mutex_unlock(&sem->mutex);
		LIKELY_RET(err,0,-1);
		goto busywait; 
	}
	
	return 0;
}



int buffer[4] ={0};
pthread_mutex_t buff_lock = PTHREAD_MUTEX_INITIALIZER;
int read_slot =0;   // inclusive
int write_slot =0;   // exclusive

void put(int val){
	int err = pthread_mutex_lock(&buff_lock);
	LIKELY_RET(err,0,-1);
	
	buffer[write_slot] = val;
	write_slot ++;
	write_slot %= 4;
	
	err= pthread_mutex_unlock(&buff_lock);
	LIKELY_RET(err,0,-1);
}

int get(){
	int err = pthread_mutex_lock(&buff_lock);
	LIKELY_RET(err,0,-1);
	
	int val = buffer[read_slot];
	read_slot++;
	read_slot%= 4;
	
	err= pthread_mutex_unlock(&buff_lock);
	LIKELY_RET(err,0,-1);
	
	return val;
}

sem_t sem_room;
sem_t sem_items;

void* producer_func(void* in){
	int i =1;
	for(i=1; ; i++){
		int err= sem_wait(&sem_room);  // block here until space available
		LIKELY_RET(err,0,-1);

		sleep( rand() % 5 ); //sleep
		put(i);

		err = sem_post(&sem_items);	
		LIKELY_RET(err,0,-1);
	}
}

void* consumer_func(void* in){
	while(1){
		int err= sem_wait(&sem_items); // block here until product available
		LIKELY_RET(err,0,-1);
		
		sleep( rand() % 5 );
		printf("%d\n",get());
		
		err = sem_post(&sem_room);	
		LIKELY_RET(err,0,-1);
	}

}


int main(){
	pthread_t consumers[THREAD_CNT];
	pthread_t producers[THREAD_CNT];	
	int err =0;
	
	err = sem_init(&sem_room, 0, 4); // 4 slots all empty
	LIKELY_RET(err,0,-1);
	err = sem_init(&sem_items, 0, 0);
	LIKELY_RET(err,0,-1);
	
	int i;
	for(i=0; i< THREAD_CNT; i++){ // make consumers
		err = pthread_create(&consumers[i], NULL, consumer_func, NULL);
		LIKELY_RET(err,0,-1);
	}

	for(i=0; i< THREAD_CNT; i++){ // make producers
		err = pthread_create(&producers[i], NULL, producer_func, NULL);
		LIKELY_RET(err,0,-1);

	}
	err = pthread_join(consumers[0],NULL);
	LIKELY_RET(err,0,-1);
	
	return 0;
}

