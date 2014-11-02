#include <errno.h>
#include <stdio.h>
#include <pthread.h>

#define THREAD_CNT 3 

#define LIKELY_RET(X,Y,RET) \
    if((X)!= (Y)){ \
        perror(strerror(errno));  \
		return RET; \
	}




typedef struct sem_t{
        pthread_cond_t cond;
        unsigned int value;
        pthread_mutex_t mutex;
}sem_t;



int sem_init(sem_t *sem, int pshared, unsigned int value){
	if(NULL == sem)
		return -1;
		
	int err = pthread_cond_init(&sem->cond,NULL) ;

	sem->value = value;

	err = pthread_mutex_init(&sem->mutex,NULL);
	
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

	err = pthread_cond_signal(&sem->cond);
	
	return 0;
}

/*
EINVAL sem is not a valid semaphore.
*/
int sem_wait(sem_t *sem){
	if(NULL == sem)
		return -1;	
		
	int ret = pthread_mutex_lock(&sem->mutex);
	
	/*
	These  functions  atomically release mutex and cause the calling thread
       to block on the condition variable cond; atomically here means  "atomi-
       cally  with  respect  to access by another thread to the mutex and then
       the condition variable". That is, if another thread is able to  acquire
       the  mutex after the about-to-block thread has released it, then a sub-
       sequent call to pthread_cond_broadcast()  or  pthread_cond_signal()  in
       that  thread shall behave as if it were issued after the about-to-block
       thread has blocked.
	*/
	if(sem->value == 0){
		ret = pthread_cond_wait(&sem->cond,&sem->mutex);// auto release mutex
		//pthread_mutex_lock(&sem->mutex);
		sem->value --;
		pthread_mutex_unlock(&sem->mutex);
	}
	else{
		sem->value --;
		ret = pthread_mutex_unlock(&sem->mutex);
	}
	
	return 0;
}



int buffer[4] ={0};
pthread_mutex_t buff_lock = PTHREAD_MUTEX_INITIALIZER;
int read_slot =0;   // inclusive
int write_slot =0;   // exclusive

void put(int val){
	pthread_mutex_lock(&buff_lock);
	buffer[write_slot] = val;
	write_slot ++;
	write_slot %= 4;
	pthread_mutex_unlock(&buff_lock);
}

int get(){
	pthread_mutex_lock(&buff_lock);
	int val = buffer[read_slot];
	read_slot++;
	read_slot%= 4;
	pthread_mutex_unlock(&buff_lock);
	return val;
}

sem_t sem_room;
sem_t sem_items;

void* producer_func(void* in){
	int i =1;
	for(i=0; i <10; i++){
		printf("producer wait +\n");

		sem_wait(&sem_room);
//		sleep( rand() % 5 ); //sleep
		put(i);

		printf("producer post +\n");		
		sem_post(&sem_items);	
	}
}

void* consumer_func(void* in){
	while(1){
		printf("consumer wait +\n");

		sem_wait(&sem_items);
		printf("consumer sleep +\n");
//		sleep( rand() % 5 );
		printf("consumer sleep -\n");

		printf("num %d\n",get());
		
		printf("consumer post +\n");
		sem_post(&sem_room);	
	}

}


int main(){
	pthread_t consumers[THREAD_CNT];
	pthread_t producers[THREAD_CNT];	
	int err =0;
	
	err = sem_init(&sem_room, 0, 4); // 4 slots all empty
	err = sem_init(&sem_items, 0, 0);
	
	int i;
	for(i=0; i< THREAD_CNT; i++){ // make consumers
		err = pthread_create(&consumers[i], NULL, consumer_func, NULL);
	}

	for(i=0; i< THREAD_CNT; i++){ // make producers
		err = pthread_create(&producers[i], NULL, producer_func, NULL);

	}
	pthread_join(consumers[0],NULL);
	return 0;
}

