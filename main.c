#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

#define SPEED_CHANGE_CONST	0x00
#define SPEED_CHANGE_RANDOM 	0x01

struct Biker {
	int round;
};

struct Track {
	struct Biker bikers [4];
};

static void start_race (struct Track *, struct Biker *);

static void *init_thread (void *);

int main (int argc, char **argv) {

	int option;
	int speed_change = 0;
	int track_size = 0;
	int bikers_num = 0;
	struct Track *track;
	struct Biker *bikers;
	pthread_t threads;

	while ( (option = getopt (argc, argv, "d:n:vu") ) != -1 ) {

		switch (option) {

			case 'd':
				track_size = atoi (optarg);
				break;
			case 'n':
				bikers_num = atoi (optarg);
				break;
			case 'v':
				speed_change = 1;
				break;
			case 'u':
				speed_change = 0;
				break;
			default:
				break;
		}

	}


	track = malloc (track_size*sizeof (struct Track) );
	bikers = malloc (bikers_num*sizeof (struct Biker) );

	if ( pthread_create (&threads, NULL, init_thread, NULL) ) {
		printf ("error\n");
		abort ();
	}

	if ( pthread_join (threads, NULL) ) {
		printf ("error joining\n");
	}
	else {
		printf ("encerrando");
	}

	return 0;
}

static void *init_thread (void *args)
{
	printf ("Criando thread!\n");
}
