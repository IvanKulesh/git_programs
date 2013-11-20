#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define LOGFILE "./log.txt"
#define KEYFILE "./client.c"
#define TMPFILE "./tmpmatrix.txt"

int count_of_alocated_pointers = 0;
void ** array_of_alocated_pointers = NULL;

void my_error (char* message);

void add_pointer (void* new_pointer);

int getnumber (char* string);

void free_all_pointers ();

void file_check();

int get_number_symbols (int x);

void increase_sem (int sem_id);
	
void decrease_sem (int sem_id);

int main(int argc, char **argv) {
	int sem_id , log_file_id;
	key_t sem_key;
	int i, fork_result;
	char *client_number = NULL;
	int total_cleints;

	umask (0);	
	
	if (argc<2)
		my_error ("You should enter a number of clients!\n\0");	

	total_cleints = getnumber (argv[1]);

	printf (" total %i clients \n",total_cleints);

	file_check();
	
	/*fork_result=fork();
	if (fork_result < 0)
		my_error ("Can not run fork!\n\0");
	else if (fork_result == 0) {
		execl ("./matrix.out","./matrix.out",NULL);
		my_error ("Can not run matrix creating application!\n\0");
	}

	if ( waitpid (fork_result, NULL,0) < 0)
		my_error ("Problems with wait!\n\0");*/
	
	log_file_id = open (LOGFILE , O_RDWR | O_CREAT | O_TRUNC , 0666);
	if (log_file_id < 0)
		my_error ("FATAL ERROR! DISK C: WILL BE FORMATED!\n\0");
	
	sem_key = ftok (KEYFILE , 1);
	if (sem_key < 0)
		my_error ("FATALL ERROR! GENERAL FAILURE GETING KEY FOR SEMAFOR!\n\0");

	sem_id = semget (sem_key , 1 , IPC_CREAT | 0666);
	if (sem_id < 0)
		my_error ("Can not create semafore!\n\0");

	increase_sem (sem_id); 
	
	for (i=0; i<total_cleints; ++i) {
		fork_result=fork();
		if (fork_result < 0)
			my_error ("Can not run fork!\n\0");
		else if (fork_result == 0) {
			client_number = (char*) malloc (sizeof(char)*(get_number_symbols (i+1) + 1));
			snprintf (client_number, get_number_symbols (i+1)+1, "%i", i + 1);
			execl ("./client.out","./client.out" , client_number , NULL);
			my_error ("Can not run client application!\n\0");
		}
	}
		
	

	free_all_pointers();
	
	execl ("./server.out","./server.out", argv [1], NULL);

	my_error ("Can not run server!\n\0");
	
	return -1;
}

void add_pointer (void* new_pointer) {
	count_of_alocated_pointers ++;
	if (array_of_alocated_pointers == NULL )
		array_of_alocated_pointers = (void**) malloc (sizeof (void*) );
	else 
		array_of_alocated_pointers = (void**) realloc (array_of_alocated_pointers , count_of_alocated_pointers * sizeof (void*) );
	array_of_alocated_pointers [count_of_alocated_pointers - 1] = new_pointer;
	return;
}

void my_error (char* message) {
	int i;
	write (STDERR_FILENO, message, strlen (message));
	for (i = 0; i < count_of_alocated_pointers; ++i)
		free (array_of_alocated_pointers [i]);
	free (array_of_alocated_pointers);
	exit (-1);
}

void free_all_pointers () {
	int i;
	for (i = 0; i < count_of_alocated_pointers; ++i)
		free (array_of_alocated_pointers [i]);
	free (array_of_alocated_pointers);
	return;
}

void increase_sem (int sem_id) {
	struct sembuf mybuf;	
	mybuf.sem_op = 1;
	mybuf.sem_flg = 0;
	mybuf.sem_num = 0;
	if (semop (sem_id, &mybuf, 1) < 0)
		my_error ("WTF?! Can not do an operation on semafor!\n\0");
	return;
}
void decrease_sem (int sem_id) {
	struct sembuf mybuf;
	mybuf.sem_op = - 1;
	mybuf.sem_flg = 0;
	mybuf.sem_num = 0;
	if (semop (sem_id, &mybuf, 1) < 0)
		my_error ("WTF?! Can not do an operation on semafor!\n\0");
	return;
}

int get_number_symbols (int x) {
	int i = 1;
	x=x/10;
	while (x>0) {
		x=x/10;
		++i;
	}
	return i;
}

int getnumber (char* string) {
	int i,answer=0;
	char c,flag=1;
	for (i=0;i<sizeof(string);++i) {
		c=string[i];
		if ((flag==1)&&( (c>'9')||(c<'0') )) {
			my_error ("NOT A NUMBER!\n\0");
		}
		if ( (c>='0')&&(c<='9') ) {
			answer=answer*10+c-'0';
			flag=0;
		}
		if ((flag==0)&&( (c>'9')||(c<'0') ))
			break;
	}
	return answer;
}

void file_check() {
	int file_id;
	char c;
	pid_t fork_result,w;
	if ((file_id=open (TMPFILE,O_RDONLY))<0)
		printf("It seems that you haven't run the first application! Would you like to run it? (Y/n)\n");
	else {
		if (close(file_id) < 0)
			my_error ("Can not close file!\n\0");
		return;
	}
	while (1) {
		c=getchar();
		if ( (c=='Y')||(c=='y'))
			break;
		if ( (c=='N')||(c=='n')) {
			free_all_pointers ();
			exit(0);
		}
		if (c!='\n')
			printf ("Enter, please, 'y' or 'n' -> ");
	}
	fork_result=fork();
	if (fork_result<0)
		my_error("Error! Can not creare a child process!\n\0");
	else if (fork_result==0) {
		execl("./matrix.out","./matrix.out", NULL);
		my_error ("Can not run the first application!\n\0");
	}
	w=waitpid (fork_result , NULL , 0);
	if (w == -1) {
		my_error("waitpid error! Disk C: will be destroyed!\n\0");
	}

	return;
}
