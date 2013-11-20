#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOGFILE "./log.txt"
#define KEYFILE "./client.c"
#define LOG_FILE_ID 3
#define HELLO 1
#define WORKINFO 2
#define LEFTMESSAGE 3
#define RIGHTMESSAGE 4
#define RESULTMESSAGE 5
#define BYE 6
#define MSGBUFFSIZE 20

int count_of_located_pointers = 0;
void ** array_of_located_pointers = NULL;

char * client_signature = NULL;

void write_log (char* message, int sem_id, int log_file_id);

void my_error (char* message);

int getnumber (char* string);

void add_pointer (void* new_pointer);

void print_matrix (int*array, int dim);

int get_element_matrix (int* array_left, int* array_right, int dim, int column, int string);

void free_all_pointers ();

int min_two (int first, int second);

void increase_sem (int sem_id);
	
void decrease_sem (int sem_id);

void send_array_res (int* array, int numb, int msg_id , long type);

void get_array_msg (int* array, int dim, int msg_id , long type);

int main (int argc, char** argv) {
	int sem_id , msg_id, dimension , i;
	key_t sem_key, msg_key;
	int this_client_number;
	int * first_array = NULL, *second_array = NULL, *result_array_part = NULL;
	int start_position , total_elements ; 

	struct first_msgbuf {
		long mtype;
		char some_string [1];
	} first_msg;
	
	struct second_msgbuf {
		long mtype;
		struct infostr {
			int dimen;
			int start_pos;
			int dest_size;
		} info;
	} second_msg;

	first_msg.mtype = HELLO;
	
	umask (0);
	
	client_signature = (char*) malloc (sizeof(char) * (strlen ("Client number \0") + strlen (argv[1]) + 1) );
	strcpy (client_signature , "Client number \0");
	strcat (client_signature , argv[1]);
	strcat (client_signature , ":\0");
	add_pointer (client_signature);
	
	this_client_number = getnumber (argv[1]);
	msg_key = ftok (KEYFILE , (char) (this_client_number));
	if (msg_key < 0)
		my_error ("ERROR WHEN CALL FTOK IN CLIENT!\n\0");
	
	msg_id = msgget (msg_key, IPC_CREAT | 0666);
	if (msg_id < 0)
		my_error ("ERROR WHEN CALL MSGGET IN CLIENT!\n\0");

	sem_key = ftok (KEYFILE , 1);
	if (sem_key < 0)
		my_error ("FATALL ERROR! GENERAL FAILURE GETING KEY FOR SEMAFOR!\n\0");

	sem_id = semget (sem_key , 1 , 0666);
	if (sem_id < 0)
		my_error ("Error in client when semget!\n\0");
	

	write_log ("Client is ready to work!\n\0" , sem_id, LOG_FILE_ID);
	
	if (msgsnd (msg_id, &first_msg , 0, 0) < 0) {
		msgctl (msg_id , IPC_RMID , NULL );
		my_error ("Can not send a message!\n\0");
	}
	
	if ( msgrcv (msg_id , &second_msg, sizeof (struct infostr), WORKINFO, 0) < 0)
		my_error ("Can't recieve a message!\n\0");
	
	write_log ("Message WORKINFO has been recieved!\n\0" , sem_id, LOG_FILE_ID);
	
	dimension = second_msg.info.dimen ; 
	start_position = second_msg.info.start_pos;
	total_elements = second_msg.info.dest_size;
	
	
	first_array = (int*)malloc(dimension*dimension*sizeof(int));
	second_array = (int*)malloc(dimension*dimension*sizeof(int));
	result_array_part = (int*) malloc( (total_elements) * sizeof(int));
	add_pointer (first_array);
	add_pointer (second_array);
	add_pointer (result_array_part);
	
	get_array_msg (first_array , dimension , msg_id , LEFTMESSAGE);
	write_log ("First array has been recieved!\n\0" , sem_id, LOG_FILE_ID);
	get_array_msg (second_array , dimension , msg_id , RIGHTMESSAGE);
	write_log ("Second array has been recieved!\n\0" , sem_id, LOG_FILE_ID);

	for (i = start_position ; i < (start_position + total_elements) ; ++i)
			result_array_part [i - start_position ] = get_element_matrix (first_array , second_array , dimension , i%dimension,i/dimension);
	
	
	write_log ("Result array has been calculated!\n\0" , sem_id, LOG_FILE_ID);
	
	send_array_res (result_array_part , total_elements , msg_id , RESULTMESSAGE);
	
	if ( msgrcv (msg_id , &first_msg, 0 , BYE , 0) < 0)
		my_error ("Can't recieve a message!\n\0");	
	
	free_all_pointers();
	return 0;
}

void add_pointer (void* new_pointer) {
	count_of_located_pointers ++;
	if (array_of_located_pointers == NULL )
		array_of_located_pointers = (void**) malloc (sizeof (void*) );
	else 
		array_of_located_pointers = (void**) realloc (array_of_located_pointers , count_of_located_pointers * sizeof (void*) );
	array_of_located_pointers [count_of_located_pointers - 1] = new_pointer;
	return;
}

void my_error (char* message) {
	int i;
	write (STDERR_FILENO, message, strlen (message));
	for (i = 0; i < count_of_located_pointers; ++i)
		free (array_of_located_pointers [i]);
	free (array_of_located_pointers);
	exit (-1);
}

void write_log (char* message, int sem_id, int log_file_id) {
	decrease_sem (sem_id);
	
	if (write (log_file_id, client_signature, strlen (client_signature)) < 0)
		my_error ("Write in file: Something goes wrong! You will lose all your data!\n\0");
	
	if (write (log_file_id, message, strlen (message)) < 0)
		my_error ("Write in file: Something goes wrong! You will lose all your data!\n\0");
	
	increase_sem (sem_id);
	return;
}

void free_all_pointers () {
	int i;
	for (i = 0; i < count_of_located_pointers; ++i)
		free (array_of_located_pointers [i]);
	free (array_of_located_pointers);
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
	mybuf.sem_op = -1;
	mybuf.sem_flg = 0;
	mybuf.sem_num = 0;
	if (semop (sem_id, &mybuf, 1) < 0)
		my_error ("WTF?! Can not do an operation on semafor!\n\0");
	return;
}

int min_two (int first, int second) {
	if (first<=second) 
		return first;
	return second;
}

int getnumber (char* string) {
	int i,answer=0;
	char c,flag=1;
	for (i=0;i<sizeof(string);++i) {
		c=string[i];
		if ((flag==1)&&( (c>'9')||(c<'0') ))
			my_error ("Not a numer!\n\0");
		if ( (c>='0')&&(c<='9') ) {
			answer=answer*10+c-'0';
			flag=0;
		}
		if ((flag==0)&&( (c>'9')||(c<'0') ))
			break;
	}
	return answer;
}


int get_element_matrix (int* array_left, int* array_right, int dim, int column, int string) {
	int result=0, i;
	for (i=0;i<dim;++i)
		result+=(array_left[string*dim+i]*array_right[i*dim+column]);
	return result;
}

void get_array_msg (int* array, int dim, int msg_id, long type) {
	
	int remaind_var , buff_size , i , current_size;
	
	struct third_msgbuf {
		long mtype;
		int l_matrix_buf [MSGBUFFSIZE];
	} third_msg;

	remaind_var=dim*dim;
	
	while (remaind_var > 0) {
		current_size = min_two (MSGBUFFSIZE , remaind_var);
	

		if ( msgrcv (msg_id , &third_msg , MSGBUFFSIZE * sizeof (int) , type , 0) <  current_size * sizeof (int))
			my_error ("error when getting a message with matrix!\n\0");

		
		for (i=0; i<current_size;++i)
			array [ dim*dim - remaind_var + i] = third_msg . l_matrix_buf [i];
		
		
		remaind_var -= current_size;
	}
	return;
}

void print_matrix (int*array, int dim) {
	int i,j;
	for (j=0;j<dim;++j) {
		for (i=0;i<dim;++i)
			printf("%i ",array [j*dim+i]);
		printf ("\n");
	}
	return;
}

void send_array_res (int* array, int dim, int msg_id , long type) {
	
	int remaind_var , buff_size , i , current_size;
	
	struct third_msgbuf {
		long mtype;
		int matrix_buf [MSGBUFFSIZE];
	} third_msg;

	remaind_var =  dim;
	
	third_msg.mtype = type;	
	
	while (remaind_var > 0) {
		current_size = min_two (MSGBUFFSIZE , remaind_var);
		
		for (i=0 ; i < current_size ; ++i) 
			(third_msg . matrix_buf) [i] = array [dim - remaind_var + i] ;
			
		if (msgsnd (msg_id , &third_msg , current_size * sizeof (int) , 0) < 0) {
			msgctl (msg_id , IPC_RMID , NULL );
			my_error ("Can not send a message!\n\0");
		}
		
		remaind_var -= current_size;
	}
	return;
}
