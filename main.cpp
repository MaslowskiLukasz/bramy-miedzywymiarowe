#define MEDIUM_NUMBER 2
#define T 5
#define PROCESS_NUMBER 4
#define SLEEP_BEFORE_ENTER 2
#define SLEEP_REST 10
#define MSG_MAX_SIZE 2
#define MSG_REQ_SIZE 2
#define MSG_REQ 1

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
int ack_counter = PROCESS_NUMBER;
int msg_send[MSG_MAX_SIZE];
int msg_recv[MSG_MAX_SIZE];
std::vector<int> delay_buffer;
MPI_Status status;
pthread_mutex_t lock;
pthread_mutex_t clock_lock;

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
		printf("%c[%dmTurysta %d otrzymal wiadomosc od %d z zegarem %d\n", 0x1B, GREEN, rank, sender_id, sender_clock);
	}

	}

int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	pthread_t thread_id;
	errno = pthread_create(&thread_id, NULL, RecvMsg, NULL);

	srand(time(NULL));

	while(true) {
		medium_req = rand() % MEDIUM_NUMBER;
		printf("%c[%dmTurysta %d z zegarem %d chce uzyskac dostep do medium %d\n", 0x1B, RED, rank, my_clock, medium_req);
		sleep(SLEEP_BEFORE_ENTER);
		sendReq();
	}

	errno = pthread_join(thread_id, NULL);
	MPI_Finalize();
}