/********************************************************/
/* Nome: Paolo Menezes Carrara                          */
/* NUSP: 7577789                                        */
/* Nome: Caio Salgado                                   */
/* NUSP:                                                */
/********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define SPEED_CONST	0x00
#define SPEED_RANDOM 	0x01

#define SPEED_FAST	0x00	/* Velocity of 25km/h */
#define SPEED_FASTER	0x01	/* Velocity of 50km/h */

#define LAPS_CONFIG		0x00
#define LAPS_FINISHED		0x01
#define LAPS_ELIMINATED		0X02
#define LAPS_PENULTIMATE	0x03
#define LAPS_NOT_FINISHED	0x04
#define LAPS_ANTE_PENULTIMATE	0x05

#define YES	0x01;
#define NO	0x00;

struct Biker {
	int speed;
	int lap;
	int *id;
};

struct Track {
	int bikers[4];
	int track_position;
	int total_occupied;
};

struct Lap {
	int lastone;
	int penultimate;
	int ante_penultimate;
	int finished;
	int expected;
};

/* Static functions */
static void *init_biker		(void *);
static struct Track *get_track	(void);
static struct Lap *get_laps 	(void);
static void start_race 		(pthread_t *const);
static int get_speed 		(void);
static void set_biker_in_track	(int);
static int run_72ms 		(const int);
static int lap_tracker		(const int, const int);
static int go_foward 		(const int, const int);
static int free_way		(const int);
static int get_biker_position	(const int);
static void print_lap_rank	(const int);

/* Global variables */
struct Track *track;
struct Lap *laps;
int speed_change;
int track_size;
int remaining_bikers;
int bikers_num;

/* Barreira */
pthread_barrier_t start_race_barrier;

/* Semáforos */
sem_t ids_semaphore;
sem_t laps_semaphore;
sem_t foward_semaphore;

int main (int argc, char **argv) {

	int option;
	pthread_t *bikers = NULL;

	while ( (option = getopt (argc, argv, "d:n:vu") ) != -1 ) {

		switch (option) {

			case 'd':
				track_size = atoi (optarg);
				break;
			case 'n':
				bikers_num = atoi (optarg);
				break;
			case 'v':
				speed_change = SPEED_RANDOM;
				break;
			case 'u':
				speed_change = SPEED_CONST;
				break;
			default:
				break;
		}

	}

	/* Get the bikers track */
	track = get_track ();

	/* Get the laps monitoring variable */
	laps = get_laps ();

	/* Malloc the bikers thread */
	bikers = malloc ( bikers_num*sizeof (pthread_t) );
	if (bikers == NULL) {
		printf ("malloc problem\n");
	}

	/* Initiates the barrier that will hold the threads before at the beginning*/
	if ( ( pthread_barrier_init (&start_race_barrier, NULL, bikers_num) ) != 0) {
		printf ("deu pau na barreira\n");
		exit (-1);
	}

	/* Initiates the semaphore that will help in the threads id distribution */
	sem_init (&ids_semaphore, 0, 1);

	/* Initiates the semaphore that will help the threads change track positions without colisions with other bikers ;)*/
	sem_init (&foward_semaphore, 0, 1);

	/* Sets that the number of biker in the track is equal to the number of bikers starting the race*/
	remaining_bikers = bikers_num;

	/* Start the race! */
	start_race (bikers);

	return 0;
}

static void start_race (pthread_t *const bikers)
{
	int i;

	/* Creates the bikers_threads */
	for (i = 1; i <= bikers_num; i++) {
		if ( pthread_create (&bikers[i], NULL, init_biker, &i) ) {
			printf("error creating thread.");
			abort();
		}

		if ( pthread_join (bikers[i], NULL) ) {
			printf("error joining thread.");
			abort();
		}
	}
}

static int run_72ms (const int biker_id)
{
	int go_foward_result;
	int biker_position;

	/* Pega a posição do ciclista na pista*/
	biker_position = get_biker_position (biker_id);

	/* Verifica se à sua frente há passagem */
	if ( free_way (biker_position) ) {

		/* Semáforo para os ciclistas não trocarem de posições simultâneamente */
		sem_wait (&foward_semaphore);

		/* Anda uma posição */
		go_foward_result = go_foward (biker_id, biker_position);

		sem_post (&foward_semaphore);

		if (go_foward_result == LAPS_FINISHED) {
			
			return LAPS_FINISHED;
		}
	}

	return LAPS_NOT_FINISHED;
}

static int lap_tracker (const int type, const int var)
{

	if (type == LAPS_CONFIG) {
		int i;

		/* Inicializa o semáforo quer controla o total de ciclistas que terminaram a volta */
		sem_init (&laps_semaphore, 0, 1);

		/* Mudar isso para que o total de ciclistas diminua a cada duas voltas e não a cada uma volta */
		for (i = 0; i < var; i++) {
			laps[i].expected = var-i;
		}
	
		return 1;
	}
	if (type == LAPS_FINISHED) {

		/* Semáforo para mexer na variável laps e remaining_bikers que são compartilhadas */
		sem_wait (&laps_semaphore);

		remaining_bikers--;
		laps[var].finished++;

		if (laps[var].expected == laps[var].finished) {			/* Verifica se é o último da volta */
			return LAPS_ELIMINATED;
		}
		else if ((laps[var].expected-1) == laps[var].finished) {	/* Verifica se é o penúltimo da volta */
			return LAPS_PENULTIMATE;
		}
		else if ((laps[var].expected-2) == laps[var].finished) {	/* Verifica se é o antepenúltimo da volta */
			return LAPS_ANTE_PENULTIMATE;
		}

		sem_post (&laps_semaphore);
	}
	
	return 0;
}

static void *init_biker (void *args)
{
	struct Biker biker;

	/* Pega valores iniciais para os ciclistas */
	biker.id = (int *) args;
	biker.speed = get_speed ();
	biker.lap = 0;

	/* Posiciona os ciclistas na pista */
	set_biker_in_track (*biker.id);

	/* Barreira, coloca todos em posição para iniciar a corrida */
	if ( (pthread_barrier_wait (&start_race_barrier)) != 0) {
		printf ("Barrier problem!\n");
	}

	while (remaining_bikers > 0) {

		while (1) {

			/* Corre em intervalos de 72ms */
			int run_result = run_72ms (*biker.id);

			if ( run_result == LAPS_FINISHED ) {

				/* Pega o resultado dessa volta para o ciclista */
				int lap_result = lap_tracker (LAPS_FINISHED, ++biker.lap);

				if (lap_result == LAPS_ELIMINATED) {
					laps[biker.lap].lastone = *biker.id;
					print_lap_rank (biker.lap);
					pthread_exit (NULL);	/* Remove o ciclista que chegou por último */
				}
				else if (lap_result == LAPS_PENULTIMATE) {
					laps[biker.lap].penultimate = *biker.id;
				}
				else if (lap_result == LAPS_ANTE_PENULTIMATE) {
					laps[biker.lap].ante_penultimate = *biker.id;
				}

				/* Sai do while (1) */
				break;
			}
		}
	}

	return biker.id;
}

static void set_biker_in_track (int id)
{
	int i;

	sem_wait (&ids_semaphore);
	for (i = 0; i < track_size; i++) {
		if (!track[i].bikers[0]) {
			track[i].bikers[0] = id;
			track[i].total_occupied++;
			i = track_size;
		}
		else if (!track[i].bikers[1]) {
			track[i].bikers[0] = id;
			track[i].total_occupied++;
			i = track_size;
		}
		else if (!track[i].bikers[2]) {
			track[i].bikers[0] = id;
			track[i].total_occupied++;
			i = track_size;
		}
		else if (!track[i].bikers[3]) {
			track[i].bikers[0] = id;
			track[i].total_occupied++;
			i = track_size;
		}
	}
	sem_post (&ids_semaphore);
}

static int get_speed (void)
{
	if (speed_change == SPEED_RANDOM) {

		srand ( time (NULL) );
		int r = rand ();

		if (r%2)
			return SPEED_FAST;
		else {
			return SPEED_FASTER;
		}
	}
	else {
		return SPEED_FASTER;
	}
}

static struct Track *get_track (void)
{
	struct Track *new_track = malloc ( track_size*sizeof (struct Track) );
	int i;

	/* Inicializa a pista vazia */
	for (i = 0; i < track_size; i++) {

		new_track->track_position = i;

		new_track[i].bikers[0] = 0;
		new_track[i].bikers[1] = 0;
		new_track[i].bikers[2] = 0;
		new_track[i].bikers[3] = 0;
		new_track[i].total_occupied = 0;
	}

	return new_track;
}

static struct Lap *get_laps (void)
{
	int i;

	laps = malloc (bikers_num*sizeof(struct Lap));

	lap_tracker (LAPS_CONFIG, bikers_num);

	for (i = 0; i < bikers_num; i++) {
		laps[i].finished = 0;
		laps[i].lastone = 0;
		laps[i].penultimate = 0;
		laps[i].ante_penultimate = 0;
	}

	return laps;
}

static int get_biker_position (const int biker_id)
{
	int biker_position = 0;
	int i, j;

	/* Busca em cada metro da pista */
	for (i = 0; i < track_size; i++) {

		/* Se tiver alguem alí*/
		if (track[i].total_occupied > 0) {

			/* Busca em cada uma das quatro filas da pista */
			for (j = 0; j < 4; j++) {

				/* Se encontrar */
				if (track[i].bikers[j] == biker_id) {
					biker_position = i;

					/* Encerra os laços da busca */
					j = 4;
					i = track_size;
				}
			}
		}
	}
	
	return biker_position;
}

static int free_way (const int track_position)
{
	int answer;

	if (track_position+1 < track_size) {	/* Caso em que o ciclista não está para cruzar a linha de chegada */
		if (track[track_position+1].total_occupied < 4) {
			answer = YES;
		}
		else {
			answer = NO;
		}
	}
	else {					/* Caso em que o ciclista está para cruzar a linha de chegada */
		if (track[0].total_occupied < 4) {
			answer = YES;
		}
		else {
			answer = NO;
		}
	}

	return answer;
}

static int go_foward (const int biker_id, const int track_position)
{
	int i;
	int biker_row;
	for (i = 0; i < 4; i++) {
		if (track[track_position].bikers[i] == biker_id) {
			biker_row = i;
			i = 4;
		}
	}

	if (track_position+1 < track_size) {	/* Caso em que o ciclista não está para cruzar a linha de chegada */
		for (i = 0; i < 4; i++) {
			if (track[track_position+1].bikers[i] == 0) {
				track[track_position+1].bikers[i] = biker_id;
				track[track_position].bikers[biker_row] = 0;

				track[track_position+1].total_occupied++;
				track[track_position].total_occupied--;

				i = 4;
			}
		}

		return LAPS_NOT_FINISHED;
	}
	else {					/* Caso em que o ciclista está para cruzar a linha de chegada */
		for (i = 0; i < 4; i++) {
			if (track[0].bikers[i] == 0) {
				track[0].bikers[i] = biker_id;
				track[track_position].bikers[biker_row] = 0;

				track[0].total_occupied++;
				track[track_position].total_occupied--;

				i = 4;
			}
		}

		return LAPS_FINISHED;
	}
}

static void print_lap_rank (const int lap) {
	printf ("Volta %d completa!\n", lap);
	printf ("Eliminado: %d\n", laps[lap].lastone);
	printf ("Penultimo: %d\n", laps[lap].penultimate);
	printf ("Ante penultimo: %d\n", laps[lap].ante_penultimate);
}
