#define MEDIUM_NUMBER 3
#define T 5
#define PROCESS_NUMBER 4
#define SLEEP_BEFORE_ENTER 2
#define SLEEP_TIME 5
#define SLEEP_REST 10
#define MSG_MAX_SIZE 2
#define MSG_REQ_SIZE 2
#define MSG_PERMISSION_SIZE 2
#define MSG_REQ 1
#define MSG_RELEASE 2
#define MSG_PERMISSION 3

#include <mpi.h>
#include <vector>
#include <stdlib.h> 
#include <stdio.h> 
#include <time.h> 
#include <unistd.h>
#include <pthread.h>
#include "colors.h"

int rank, world_size;
int medium_req = -1;
int my_clock, req_clock = 0;
int medium_capacity = T;
int ack_counter = PROCESS_NUMBER - 1;
int msg_send[MSG_MAX_SIZE];
int msg_recv[MSG_MAX_SIZE];
std::vector<int> delay_buffer;
MPI_Status status;
pthread_mutex_t lock;
pthread_mutex_t clock_lock;
pthread_mutex_t release_lock;

void sendReq() {
	pthread_mutex_lock(&clock_lock);
	my_clock++;
	req_clock = my_clock;
	pthread_mutex_unlock(&clock_lock);
	msg_send[0] = req_clock;
	msg_send[1] = medium_req;
	for(int i = 0; i < world_size; i++) {
		if(i != rank) {
			MPI_Send(msg_send, MSG_REQ_SIZE, MPI_INT, i, MSG_REQ, MPI_COMM_WORLD);
			printf("%c[%dmTurysta %d wysyła zapytanie do turysty %d o wejscie do medium %d\n", 0x1B, CYAN, rank, i, medium_req);
		}
	}
}

void *RecvMsg(void *arg) {
	while(true) {
		MPI_Recv(msg_recv, MSG_MAX_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		int sender_clock = msg_recv[0];
		int sender_id = status.MPI_SOURCE;
		pthread_mutex_lock(&clock_lock);
		msg_recv[0] = my_clock;
		my_clock = std::max(my_clock, sender_clock) + 1;
		pthread_mutex_unlock(&clock_lock);
		switch(status.MPI_TAG) {
			case MSG_REQ: {
				int medium_id = msg_recv[1];
				pthread_mutex_lock(&release_lock);
				if(medium_id != medium_req) {
					printf("%c[%dmTurysta %d ubiega sie o inne medium niz turysta %d\n", 0x1B, 0, rank, sender_id);
					MPI_Send(msg_recv, MSG_PERMISSION_SIZE, MPI_INT, sender_id, MSG_PERMISSION, MPI_COMM_WORLD);
				} else {
					pthread_mutex_lock(&clock_lock);
					if(sender_clock < req_clock || (sender_clock == req_clock && sender_id < rank)) {
						printf("%c[%dmTurysta %d wsyla zgode turyscie %d na wejscie do medium %d\n", 0x1B, PINK, rank, sender_id, medium_id);
						MPI_Send(msg_recv, MSG_PERMISSION_SIZE, MPI_INT, sender_id, MSG_PERMISSION, MPI_COMM_WORLD);
					} else {
						printf("%c[%dmTurysta %d zapisuje turyste %d do bufora\n", 0x1B, GREEN, rank, sender_id);
						delay_buffer.push_back(sender_id);
					}
				}
				pthread_mutex_unlock(&clock_lock);
				pthread_mutex_unlock(&release_lock);
				break;
			}
			case MSG_PERMISSION: {
				printf("%c[%dmTurysta %d daje zgode turyscie %d\n", 0x1B, GREEN, rank, sender_id);
				ack_counter--;
				if(ack_counter == 0) {
					pthread_mutex_unlock(&lock);
				}
				break;
			}
		}
	}
}

void sendDelayAck() {
	pthread_mutex_lock(&clock_lock);
	my_clock++;
	pthread_mutex_unlock(&clock_lock);
	msg_send[0] = my_clock;
	for(int i = 0; i < delay_buffer.size(); ++i) {
		printf("%c[%dmTurysta %d wysyla zgode turyscie %d z bufora\n", 0x1B, YELLOW, rank, delay_buffer[i]);
		MPI_Send(msg_send, MSG_PERMISSION_SIZE, MPI_INT, delay_buffer[i], MSG_PERMISSION, MPI_COMM_WORLD);
	}
	delay_buffer.clear();
}

int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	pthread_t thread_id;
	errno = pthread_create(&thread_id, NULL, RecvMsg, NULL);

	srand(time(NULL));

	pthread_mutex_unlock(&lock);
	while(true) {
		medium_req = rand() % MEDIUM_NUMBER;
		printf("%c[%dmTurysta %d z zegarem %d chce uzyskac dostep do medium %d\n", 0x1B, RED, rank, my_clock, medium_req);
		sleep(SLEEP_BEFORE_ENTER);
		pthread_mutex_lock(&lock);
		sendReq();
		pthread_mutex_lock(&lock);
		printf("%c[%dmTurysta %d z zegarem %d wchodzi to tunelu %d\n", 0x1B, 0, rank, my_clock, medium_req);
		sleep(SLEEP_TIME);
		pthread_mutex_unlock(&lock);
		pthread_mutex_lock(&release_lock);
		sendDelayAck();
		pthread_mutex_unlock(&release_lock);
		medium_req = -1;
		ack_counter = PROCESS_NUMBER - 1;
	}

	errno = pthread_join(thread_id, NULL);
	MPI_Finalize();
}