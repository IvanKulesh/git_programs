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
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>

#define TMPFILE "./tmpmatrix.txt"
#define RESULTPATH "./result.txt"
#define BUFFSIZE 20
#define LOGFILE "./log.txt"
#define KEYFILE "./client.c"
#define LOG_FILE_ID 3
#define MSGBUFFSIZE 20
#define HELLO 1
#define WORKINFO 2
#define LEFTMESSAGE 3
#define RIGHTMESSAGE 4
#define RESULTMESSAGE 5
#define BYE 6

int count_of_alocated_pointers = 0;
void ** array_of_alocated_pointers = NULL;

typedef struct struct_for_thread {
	int start_position;
	int destination_size;
	int *l_array;
	int *r_array;
	int *ans_array;
	int dim;
	int message_id;
} thread_arguments;

void my_error (char* message);

void print_matrix (int*array, int dim);

void add_pointer (void* new_pointer);

void free_all_pointers ();

void get_array (int* array, int dim, int file_id);

int getnumber (char* string);

int min_two (int first, int second);

void* thread_function (void* arg);

int get_element_matrix (int* array_left, int* array_right, int dim, int column, int string);

void mywrite_final (int file_id, int* result_array, int dimension);

void mywrite_int (int file_id, int number);

int get_number_symbols (int x);

void send_array (int* array, int dim, int msg_id , long type);

void get_array_msg_res (int* array, int dim, int msg_id , long type);


int main(int argc, char** argv) {
	int total_cleints, file_id, dimension, sem_id, number_of_threads, result_file_id, tmp_msg_id;
	key_t sem_key , tmp_msg_key;
	pthread_t * threads_id = NULL;
	thread_arguments *array_of_arguments = NULL;
	int each_thread_numbers, last_thread_numbers;
	int * first_array = NULL, *second_array = NULL, *result_array = NULL;
	time_t current_time, execution_time;
	int i,j;
	
	
	umask(0);
	
	total_cleints = getnumber (argv[1]);	
	
	sem_key = ftok (KEYFILE , 1);
	if (sem_key < 0)
		my_error ("FATALL ERROR! GENERAL FAILURE GETING KEY FOR SEMAFOR!\n\0");

	sem_id = semget (sem_key , 1 , 0666);
	if (sem_id < 0)
		my_error ("Error in client when semget!\n\0");
	
	if ((file_id=open (TMPFILE,O_RDONLY))<0)
		my_error ("Error when open file!\n\0");

	if (read(file_id,&dimension,sizeof(int))!=sizeof(int))
		my_error ("error when reading from file!\n\0");

	total_cleints = getnumber (argv[1]);
	if ( total_cleints > dimension * dimension )
		total_cleints = dimension * dimension;
	number_of_threads = total_cleints;
	
	threads_id=(pthread_t *)malloc(total_cleints*sizeof(pthread_t));
	array_of_arguments=(thread_arguments*)malloc (total_cleints*sizeof(thread_arguments));	
	add_pointer (threads_id);
	add_pointer (array_of_arguments);

	each_thread_numbers=(dimension*dimension)/number_of_threads;
	last_thread_numbers=(dimension*dimension)%number_of_threads + each_thread_numbers;

	first_array=(int*)malloc(dimension*dimension*sizeof(int));
	second_array=(int*)malloc(dimension*dimension*sizeof(int));
	result_array=(int*)malloc(dimension*dimension*sizeof(int));
	add_pointer (first_array);
	add_pointer (second_array);
	add_pointer (result_array);
	
	get_array (first_array,dimension,file_id);
	get_array (second_array,dimension,file_id);

	/*print_matrix (first_array,dimension);
	printf ("\n-----------------\n");
	print_matrix (second_array,dimension);*/
	
	if (close(file_id) < 0)
		my_error ("Error when closing file!\n\0");
	
	for (i=0;i<number_of_threads-1;++i) {
		(array_of_arguments[i]).start_position=each_thread_numbers*i;
		(array_of_arguments[i]).destination_size=each_thread_numbers;
		(array_of_arguments[i]).l_array=first_array;
		(array_of_arguments[i]).r_array=second_array;
		(array_of_arguments[i]).ans_array=result_array;
		(array_of_arguments[i]).dim=dimension;
	}

	(array_of_arguments[number_of_threads-1]).start_position=each_thread_numbers*(number_of_threads-1);
	(array_of_arguments[number_of_threads-1]).destination_size=last_thread_numbers;
	(array_of_arguments[number_of_threads-1]).l_array=first_array;
	(array_of_arguments[number_of_threads-1]).r_array=second_array;
	(array_of_arguments[number_of_threads-1]).ans_array=result_array;
	(array_of_arguments[number_of_threads-1]).dim=dimension;
	
	for (i=0; i <  total_cleints; ++i) {
		tmp_msg_key = ftok (KEYFILE , (char) (i+1));
		if (tmp_msg_key < 0)
			my_error ("ERROR WHEN CALL FTOK!\n\0");
		tmp_msg_id = msgget (tmp_msg_key, IPC_CREAT | 0666);
		if (tmp_msg_id < 0)
			my_error ("ERROR WHEN CALL MSGGET!\n\0");
		(array_of_arguments[i]).message_id = tmp_msg_id;
	}

	current_time=time(NULL);		
	
	for (j=0 ; j<number_of_threads-1 ; ++j)
		if ( pthread_create (threads_id + j, NULL, &thread_function, array_of_arguments + j) < 0)
			my_error ("FATAL ERROR! CAN NOT CREATE NEW THREAD!\n\0");
	
	thread_function (array_of_arguments+number_of_threads-1);

	for (j=0 ; j < number_of_threads - 1 ; ++j)
		if (pthread_join (threads_id[j],NULL) != 0)
			my_error ("ERROR WHEN JOING THREADS!\n\0");

	execution_time = time(NULL);
	execution_time -= current_time;	
	
	if (semctl (sem_id , 0 ,  IPC_RMID , 0) < 0)
		my_error ("Holly shit! Can not delete the semefor!\n\0");

	if (close (LOG_FILE_ID) < 0)
		my_error ("FATAL ERROR! BIOS WILL BE DELETED!\n\0");
	
	printf ("Total calculating time is %i seconds\n", (int) execution_time);	
	
	result_file_id = open(RESULTPATH,O_RDWR|O_CREAT|O_TRUNC,0666);
	if (result_file_id<0)
		my_error ("Can not create file!\n\0");
	
	mywrite_final (result_file_id, result_array, dimension);
	
	
	if (close (result_file_id) < 0)
		my_error ("Error when closing file!\n\0");
	
	free_all_pointers();

	printf ("\nDone!\n");
	return 0;
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

void mywrite_final (int file_id, int* result_array, int dimension) {
	int i,j;
	char tab=9;
	for (j=0;j<dimension;++j) {
		for (i=0;i<dimension;++i) {
			mywrite_int (file_id, result_array[dimension*j+i]);
			if (write (file_id,&tab,1) != 1) {
				my_error ("FATALL ERROR!!!\n\0");
			}
		}
		if (write (file_id,"\n",1) != 1) {
			my_error ("FATALL ERROR!!!\n\0");
		}
	}
	return;
}

void mywrite_int (int file_id, int number) {
	char* string=NULL, *revstr=NULL;
	int size, i;
	
	size = get_number_symbols (number);
	string = (char*) malloc (sizeof (char) * size);

	for (i=0; i<size; ++i) {
		string[i] = (number % 10) + '0';
		number/=10;
	}
	
	revstr=(char*)malloc(size*sizeof(char));
	for (i=0;i<size;++i)
		revstr[size-1-i]=string[i];
	free(string);
	if (write(file_id,revstr,size)<size) {
		printf("General failure write the file!\n");
		free (revstr);
		exit(-1);
	}
	free (revstr);
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

void print_matrix (int*array, int dim) {
	int i,j;
	for (j=0;j<dim;++j) {
		for (i=0;i<dim;++i)
			printf("%i ",array [j*dim+i]);
		printf ("\n");
	}
	return;
}

void get_array (int* array, int dim, int file_id) {
	int remaind_var , buff_size , i , current_size;
	int *buff_array;

	remaind_var=dim*dim;
	
	if (BUFFSIZE < sizeof(char))
		buff_size = sizeof(char);
	else
		buff_size = BUFFSIZE;

	while (remaind_var > 0) {
		current_size = min_two(buff_size, remaind_var * sizeof(int))/sizeof(int);
		buff_array = (int*)malloc(current_size*sizeof(int));

		if ( read(file_id,buff_array,current_size*sizeof(int)) < current_size*sizeof(int)) {
			free(buff_array);
			my_error ("error when reading!\n\0");
		}
		
		for (i=0; i<current_size;++i)
			array[dim*dim-remaind_var+i] = buff_array[i];
		free(buff_array);
		remaind_var -= current_size;
	}
	return;
}

void* thread_function (void* argument) {
	thread_arguments *arg;
	int i, dimension;
	int *result_array_part = NULL;	
	
	struct first_msgbuf {
		long mtype;
		char some_string [1];
	} msg_buf;
	
	struct second_msgbuf {
		long mtype;
		struct infostr {
			int dimen;
			int start_pos;
			int dest_size;
		} info;
	} msg_info;
	
	arg = (thread_arguments *) argument;
	dimension=arg->dim;
	
	if ( msgrcv (arg->message_id , &msg_buf, 0, HELLO, 0) < 0)
		my_error ("Can't recieve a message!\n\0");	
	
	msg_info.mtype = WORKINFO;
	msg_info.info.dimen = dimension;
	msg_info.info.start_pos = arg->start_position;
	msg_info.info.dest_size = arg->destination_size;

	result_array_part = (int*) malloc ( (arg->destination_size)*sizeof (int));
	add_pointer (result_array_part);

	if (msgsnd (arg->message_id, &msg_info , sizeof (struct infostr), 0) < 0) {
		msgctl (arg->message_id , IPC_RMID , NULL );
		my_error ("Can not send a message!\n\0");
	}
	
	send_array (arg -> l_array , dimension , arg->message_id , LEFTMESSAGE );
	send_array (arg -> r_array , dimension , arg->message_id , RIGHTMESSAGE );
	
	get_array_msg_res (result_array_part, (arg->destination_size) , arg->message_id , RESULTMESSAGE);
	
	for (i=arg->start_position;i < (arg->start_position + arg->destination_size); ++i)
			(arg->ans_array)[i] = result_array_part [i-arg->start_position];
	
	msg_buf.mtype = BYE;	
	
	if (msgsnd (arg->message_id, &msg_buf , 0,  0) < 0) {
		msgctl (arg->message_id , IPC_RMID , NULL );
		my_error ("Can not send a message!\n\0");
	}
	
	if ( msgctl ( arg->message_id , IPC_RMID , NULL ) < 0 )
		my_error ("Can not delete message!\n\0");
	return NULL;
}

int min_two (int first, int second) {
	if (first<=second) 
		return first;
	return second;
}

int get_element_matrix (int* array_left, int* array_right, int dim, int column, int string) {
	int result=0, i;
	for (i=0;i<dim;++i)
		result+=(array_left[string*dim+i]*array_right[i*dim+column]);
	return result;
}

void send_array (int* array, int dim, int msg_id , long type) {
	
	int remaind_var , buff_size , i , current_size;
	
	struct third_msgbuf {
		long mtype;
		int matrix_buf [MSGBUFFSIZE];
	} third_msg;

	remaind_var = dim * dim;
	
	third_msg.mtype = type;	
	
	while (remaind_var > 0) {
		current_size = min_two (MSGBUFFSIZE , remaind_var);
		
		for (i=0 ; i < current_size ; ++i) 
			(third_msg . matrix_buf) [i] = array [dim*dim - remaind_var + i] ;
			
		if (msgsnd (msg_id , &third_msg , current_size * sizeof (int) , 0) < 0) {
			msgctl (msg_id , IPC_RMID , NULL );
			my_error ("Can not send a message!\n\0");
		}
		
		remaind_var -= current_size;
	}
	return;
}

void get_array_msg_res (int* array, int dim, int msg_id, long type) {
	
	int remaind_var , buff_size , i , current_size;
	
	struct third_msgbuf {
		long mtype;
		int l_matrix_buf [MSGBUFFSIZE];
	} third_msg;

	remaind_var=dim;
	
	while (remaind_var > 0) {
		current_size = min_two (MSGBUFFSIZE , remaind_var);
	

		if ( msgrcv (msg_id , &third_msg , MSGBUFFSIZE * sizeof (int) , type , 0) <  current_size * sizeof (int))
			my_error ("error when getting a message with matrix!\n\0");

		
		for (i=0; i<current_size;++i)
			array [ dim - remaind_var + i] = third_msg . l_matrix_buf [i];
		
		
		remaind_var -= current_size;
	}
	return;
}
