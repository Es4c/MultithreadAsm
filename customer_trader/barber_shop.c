#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int no_of_seats;
int no_of_customers;
int no_of_barber;
int occupied_seats = 0; 
int served = 0; // num of customer that has already been assigned with a barber
int passed = 0; // num of customer that has been to the shop
int wait_q = 0; // num of customer that has been to the shop and got a ticket
int barber_temp; // barber to be assigned
int work_pace[2], come_pace[2]; //min at [0], max at [1]


void * barber_routine(void *);
void * customer_routine(void *);
void * assistant_routine(void *);
int tickets;
int* barber_status; //index=barber's id, -1 means not working, otherwise it means the id of assigned customer

//declare global mutex and condition variables
pthread_mutex_t barber_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t assist_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pair_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t barber_customer_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t barber_assist_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t servedNseat_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t customer_assist_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t assist_customer_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t call_customer;
pthread_cond_t response_calling;
pthread_cond_t customer_come;
pthread_cond_t confirm_barber;
pthread_cond_t confirm_customer;
pthread_cond_t end_cut;
pthread_cond_t call_ticket;
pthread_cond_t start_cut;
pthread_cond_t job_finished;
pthread_cond_t end_of_day;

int get_randn_time(int min, int max){ //get random sleeping time between min and max
	if(min > max){
		int tmp = max;
		max = min;
		min = tmp;
	}
	return (int)rand() % (max + 1 - min) + min; 
}

int barber_checker(int arg){ //0 for check_empty, 1 for booking barber
	if(no_of_barber == 0){
		return -2; //err_check
	}
	int sum = 0; //num of available barber
	for(int i = 0;i<no_of_barber;i++){
		if(barber_status[i] == -1){
			if(arg == 1){
				printf("barber %d is available.\n",i);
				return i;
			}
			sum++;
		}
	}
	return arg == 0 ? sum == no_of_barber : -1; //-1 stands for all barbers are busy
}


int main(int argc, char ** argv)
{
	pthread_t *thread_customer; //system thread id
	pthread_t *thread_barber;
	pthread_t thread_assistant;
	int *t_ids_customer, *t_ids_barber; //user-defined thread id
 	int rc;

	printf("Enter number of seats capacity (int): \n");
	scanf("%d", &no_of_seats);
	tickets = no_of_seats;
	
	printf("Enter the total number of customers (int): \n");
	scanf("%d", &no_of_customers);

	printf("Enter the total number of barbers (int): \n");
	scanf("%d", &no_of_barber);
	
	if(no_of_barber <= 0 || no_of_seats <= 0){
		printf("Number of barber/seat must be larger than 0.\n");
		return 0;
	}

	
	printf("Enter the minimum number of time units for barber to service a customer (int): \n");
	scanf("%d", &work_pace[0]);

	printf("Enter the maximum number of time units for barber to service a customer: \n");
	scanf("%d", &work_pace[1]);

	printf("Enter the minimum number of time units between two customer arrivals (int): \n");
	scanf("%d", &come_pace[0]);

	printf("Enter the maximum number of time units between two customer arrivals (int): \n");
	scanf("%d", &come_pace[1]);

	if(no_of_customers <= 0){ 
		//if there is no customer, there is no need to waste time and resource executing lots of codes.
		printf("Assistant: Hi barbers. We've finished the work for the day. See you all tomorrow!\n");
		printf("Main thread: All customers have now been served. Salon is closed now.\n");
		return 0;
	}
	
	barber_status = (int*) calloc(no_of_barber, sizeof(int)); //-1 for not working, -2 for booked but not assigned yet, other postive numbers means the id of assigned customer

	for(int i=0;i<no_of_barber;i++){ //initialze all barbers to be "not working"
		barber_status[i] = -1;
	}
		
	//Initialize mutex and condition variable objects 
	rc = pthread_cond_init(&call_customer,NULL);
	rc = pthread_cond_init(&response_calling,NULL);
	rc = pthread_cond_init(&end_cut,NULL);
	rc = pthread_cond_init(&customer_come,NULL);
	rc = pthread_cond_init(&call_ticket,NULL);
	rc = pthread_cond_init(&start_cut,NULL);
	rc = pthread_cond_init(&confirm_barber,NULL);
	rc = pthread_cond_init(&confirm_customer,NULL);
	rc = pthread_cond_init(&end_of_day,NULL);
	rc = pthread_cond_init(&job_finished,NULL);
				
	
	thread_barber = malloc((no_of_barber) * sizeof(pthread_t));
	thread_customer = malloc((no_of_customers) * sizeof(pthread_t));
	if(thread_barber == NULL || thread_customer == NULL){
		fprintf(stderr, "threads out of memory\n");
		exit(1);
	}	
	t_ids_customer = malloc((no_of_customers) * sizeof(int)); 
	t_ids_barber = malloc((no_of_barber) * sizeof(int));
 	if(t_ids_customer == NULL || t_ids_barber == NULL ){
		fprintf(stderr, "t out of memory\n");
		exit(1);
	}	
	
	//create the barber thread.
	for(int o = 0;o<no_of_barber;o++){
		t_ids_barber[o] = o;
		rc = pthread_create(&thread_barber[o], NULL, barber_routine, (void *) &t_ids_barber[o]); //barber routine takes thread_id as the arg
		if (rc) {
			printf("ERROR; return code from pthread_create() (barber) is %d\n", rc);
			exit(-1);
		}
	}

	rc = pthread_create(&thread_assistant,NULL,assistant_routine,NULL);

	//create customers according to the arrival rate
	srand(time(0));
	for (int k = 0; k<no_of_customers; k++)
	{	
		sleep(get_randn_time(come_pace[0],come_pace[1])); //sleep a few second before creating a thread
		t_ids_customer[k] = k;
		printf("k is %d",k);
		rc = pthread_create(&thread_customer[k], NULL, customer_routine, (void *)&t_ids_customer[k]); //customr routine takes thread id as the arg
		if (rc) {
			printf("ERROR; return code from pthread_create() (consumer) is %d\n", rc);
			exit(-1);
		}
	}
	//jwait consumer threads to terminate
	for (int k = 0; k<no_of_customers; k++) 
	{
		pthread_join(thread_customer[k], NULL);
		printf("thread %d quitted.\n",k);
	}
	//wait barber threads to terminate
	for (int k=0;k<no_of_barber;k++){
		pthread_join(thread_barber[k],NULL);
		printf("barber thread %d quitted.\n",k);
	} 
	//terminate assistance thread
	pthread_cancel(thread_assistant);

	//deallocate allocated memory
	free(thread_barber);
	free(thread_customer);
	free(t_ids_barber);
	free(t_ids_customer);
	free(barber_status);
	//destroy mutex and condition variable object
	pthread_mutex_destroy(&barber_mutex);
	pthread_mutex_destroy(&servedNseat_mutex);
	pthread_mutex_destroy(&assist_mutex);
	pthread_mutex_destroy(&pair_mutex);
	pthread_mutex_destroy(&barber_customer_cond_mutex);
	pthread_mutex_destroy(&barber_assist_cond_mutex);
	pthread_mutex_destroy(&customer_assist_cond_mutex);
	pthread_mutex_destroy(&assist_customer_cond_mutex);
	pthread_cond_destroy(&call_customer);
	pthread_cond_destroy(&response_calling);
	pthread_cond_destroy(&end_cut);
	pthread_cond_destroy(&start_cut);
	pthread_cond_destroy(&customer_come);
	pthread_cond_destroy(&call_ticket);
	pthread_cond_destroy(&confirm_barber);
	pthread_cond_destroy(&confirm_customer);
	pthread_cond_destroy(&end_of_day);
	pthread_cond_destroy(&job_finished);
	printf("Main thread: All customers have now been served. Salon is closed now.\n");
	return 0;
}



void * barber_routine(void * arg)
{
	int* id = (int*) arg;
	int assigned_customer;
	while (1)
	{
		
		printf("Barber %d: I'm now ready to accept a customer.\n",*id);
		pthread_cond_signal(&call_customer);
		
		pthread_mutex_lock(&barber_assist_cond_mutex);
		pthread_cond_wait(&call_ticket,&barber_assist_cond_mutex);
		pthread_mutex_unlock(&barber_assist_cond_mutex);

		if(barber_status[*id] != -1){ //means if this barber has been booked
			pthread_mutex_lock(&barber_assist_cond_mutex);
			pthread_cond_wait(&start_cut,&barber_assist_cond_mutex);//this signal means the assignment of customer has been done.
			printf("barber %d received call_customer.\n",*id);
			pthread_mutex_unlock(&barber_assist_cond_mutex);
			assigned_customer = barber_status[*id];
			printf("Barber %d: Hello, Customer %d.\n",*id, assigned_customer);
			sleep(get_randn_time(work_pace[0], work_pace[1])); //cutting
			printf("Barber %d: finished cutting. Good bye Customer%d!\n",*id, assigned_customer);

			barber_status[*id] = -1; //set status to "not working"
			
			pthread_cond_signal(&end_cut);
		}
		
		if(passed == no_of_customers && occupied_seats == 0){ //means all customers has been to the shop and no more customer will come
			printf("Barber %d, I'm waiting to leave.\n",*id);
			pthread_cond_signal(&job_finished);
			pthread_mutex_lock(&assist_mutex);
			pthread_cond_wait(&end_of_day,&assist_mutex);
			pthread_mutex_unlock(&assist_mutex);
			printf("barber %d: I've released the lock\n",*id);
			break;
		}
		printf("%d people has been served.\n",served);
		
	}
	pthread_exit(EXIT_SUCCESS);
	return NULL;
}

void * customer_routine(void * arg)
{
	int *id;
	id = (int *) arg;
	int ticket, assigned_barber;
	printf("Customer %d: I have arrived at the barber shop.\n",*id);
	passed++;
	pthread_mutex_lock(&servedNseat_mutex);
	printf("Customer %d: I'm holding the lock\n",*id);
	if(occupied_seats >= 0 && occupied_seats < no_of_seats){
		ticket = wait_q++%no_of_seats;
		printf("Customer %d: I'm lucky to get a free seat, and a ticket numbered %d.\n",*id,ticket);
		occupied_seats++;
		
		pthread_mutex_unlock(&servedNseat_mutex);
		printf("Customer %d: I've released the lock\n",*id);
		pthread_cond_signal(&customer_come);
		

		while(1){ //waiting for calling
			pthread_mutex_lock(&customer_assist_cond_mutex);
			pthread_cond_wait(&call_ticket,&customer_assist_cond_mutex);
			pthread_mutex_unlock(&customer_assist_cond_mutex);

			printf("%d received calling. Ticket is %d, and called ticked is %d\n",*id,ticket,served%no_of_seats);
			
			if(served%no_of_seats == ticket){
				printf("%d has been called\n",*id);

				pthread_mutex_lock(&pair_mutex);
				
				assigned_barber = barber_temp;
				barber_status[barber_temp] = *id;
				pthread_cond_signal(&response_calling);

				pthread_mutex_lock(&customer_assist_cond_mutex);
				pthread_cond_wait(&confirm_customer, &customer_assist_cond_mutex);
				pthread_mutex_unlock(&customer_assist_cond_mutex);

				printf("Customer %d: My ticket number %d has been called. Hello, Barber %d.\n", *id, ticket,assigned_barber);
				pthread_cond_signal(&start_cut);
			
				pthread_mutex_unlock(&pair_mutex);

				while(barber_status[assigned_barber]  != -1){
					pthread_mutex_lock(&barber_customer_cond_mutex);
					pthread_cond_wait(&end_cut,&barber_customer_cond_mutex);
					pthread_mutex_unlock(&barber_customer_cond_mutex);
					if(barber_status[assigned_barber] != -1){
						printf("all busy, call again.\n");
						pthread_cond_signal(&end_cut);
					}
				}
				
				printf("Customer %d: Well done. Thank barber %d,z bye!\n",*id, assigned_barber);
				break;
			}
			

		}
		
	}else{
		pthread_mutex_unlock(&servedNseat_mutex);
		printf("Customer %d: I've released the lock\n",*id);
		printf("Customer %d: oh no! all seats have been taken and I'll leave now!\n",*id);
	}
	
	
	pthread_exit(EXIT_SUCCESS);
	return NULL;
}

void * assistant_routine(void * arg){
	while(1){
		pthread_mutex_trylock(&servedNseat_mutex);

		printf("assist: I'm holding the lock\n");
		if(occupied_seats == 0){
			pthread_mutex_unlock(&servedNseat_mutex);
			printf("Assistant: I'm waiting for customers.\n");
			pthread_mutex_lock(&assist_customer_cond_mutex);
			printf("Assistant: I'm waiting for a barber to become available.\n");
			pthread_cond_wait(&customer_come,&assist_customer_cond_mutex);
			pthread_mutex_unlock(&assist_customer_cond_mutex);
		}
			
		pthread_mutex_unlock(&servedNseat_mutex);
		printf("assist: I've released the lock\n");
		printf("Assistant: There are someone waiting for calling, let me check the barber.\n");
		pthread_mutex_lock(&pair_mutex);
		barber_temp = barber_checker(1);
		while(barber_temp == -1){ //wait a barber to be available
			pthread_cond_wait(&call_customer,&pair_mutex); //call_customer is explicitly saying a barber becomes available
			barber_temp = barber_checker(1);
		}
		pthread_mutex_unlock(&pair_mutex);
		
		barber_status[barber_temp] = -2; //temporarily 'lock' a barber for the customer
		//served%no_of_seats gives the circular order of tickets.
		printf("Assistant: Call one customer with a ticket numbered %d.\n",served%no_of_seats);
		pthread_cond_broadcast(&call_ticket);

		pthread_mutex_lock(&assist_customer_cond_mutex);
		pthread_cond_wait(&response_calling,&assist_customer_cond_mutex);
		pthread_mutex_unlock(&assist_customer_cond_mutex);
		printf("Assistant: Assign Customer %d to Barber %d.\n",barber_status[barber_temp],barber_temp);
		
		pthread_mutex_trylock(&servedNseat_mutex);
		printf("2assist: I'm holding the lock2\n");
		served++;
		occupied_seats--;
		pthread_mutex_unlock(&servedNseat_mutex);
		printf("2assist: I've released the lock2\n");

		pthread_cond_signal(&confirm_customer);
			printf("pair: [%d, %d]\n",barber_temp,barber_status[barber_temp]);
		
		
		printf("passed: %d\n",passed);
		if(passed == no_of_customers && occupied_seats == 0){
			printf("waiting for barber finishing\n");
			pthread_cond_broadcast(&call_ticket);
			/* the line above is a work-around to deal with last customer assigning to barbers.
			the occupied_seat changes after the barber checking for remained customers, 
			making a barber waits for calling after all customers are assigned. So I broadcast again
			to ask the barber check the occupied_seat again.  */
			while(!barber_checker(0)){
				pthread_mutex_lock(&assist_mutex);
				pthread_cond_wait(&job_finished, &assist_mutex);
				pthread_mutex_unlock(&assist_mutex);
			}
			printf("Assistant: Hi barbers. We've finished the work for the day. See you all tomorrow!\n");
			pthread_cond_broadcast(&end_of_day);		
			pthread_exit(EXIT_SUCCESS);
			
		}
	}
}