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

#define BUFFSIZE 20
#define LENGTH_CPU 8
#define TMPFILE "./tmpmatrix.txt"
#define RESULTPATH "./result.txt"


typedef struct struct_for_thread {
	int start_position;
	int destination_size;
	int *l_array;
	int *r_array;
	int *ans_array;
	int dim;
} thread_arguments;

void myclose (int file_id);

int getnumber (char* string);

int get_cpu_number ();

void file_check();

int min_two (int first, int second);

void* thread_function (void* arg);

void get_array (int* array, int dim, int file_id);

void print_matrix (int*array, int dim);

int get_element_matrix (int* array_left, int* array_right, int dim, int column, int string);

void mywrite_final (int file_id, int* result_array, int dimension);

void mywrite_int (int file_id, int number);

int main (int argc, char** argv) {
	int number_of_threads, cpu_number, dimension;
	char c;
	int file_id, result_file_id;
	int * first_array=NULL, *second_array=NULL, *result_array=NULL;
	int i,j;
	int each_thread_numbers, last_thread_numbers;
	time_t current_time, execution_time;
	thread_arguments *array_of_arguments=NULL;
	pthread_t *threads_id=NULL;
	
	if (argc<=1) {
		printf ("Enter a number of threads ->");
		scanf("%i",&number_of_threads);
	} else
		number_of_threads=getnumber(argv[1]);

	if (number_of_threads<=0) {
		printf("The number should be positive!\n");
		return -1;
	}
	
	cpu_number=get_cpu_number();

	if (cpu_number<number_of_threads) {
		printf("Your machine can work only with %i threads at the same time.",cpu_number);
		printf("This means that there is no sense to create %i threads.\n",number_of_threads);
		number_of_threads=cpu_number;
	}
	
	file_check();
	
	if ((file_id=open (TMPFILE,O_RDONLY))<0) {
		printf("Error when open file!\n");
		exit(-1);
	}
	
	if (read(file_id,&dimension,sizeof(int))!=sizeof(int)) {
		printf("Error when reading!\n");
		exit(-1);
	}
	
	if ( (dimension*dimension)<number_of_threads)
		number_of_threads=dimension*dimension;

	printf ("%i threads will be created. Continue? (Y/n)\n",number_of_threads);
	while (1) {
		c=getchar();
		if ( (c=='Y')||(c=='y'))
			break;
		if ( (c=='N')||(c=='n'))
			return 0;
		if (c!='\n')
			printf ("Enter, please, 'y' or 'n' -> ");
	}
	
	threads_id=(pthread_t *)malloc(number_of_threads*sizeof(pthread_t));
	array_of_arguments=(thread_arguments*)malloc (number_of_threads*sizeof(thread_arguments));
	
	each_thread_numbers=(dimension*dimension)/number_of_threads;
	last_thread_numbers=(dimension*dimension)%number_of_threads + each_thread_numbers;

	first_array=(int*)malloc(dimension*dimension*sizeof(int));
	second_array=(int*)malloc(dimension*dimension*sizeof(int));
	result_array=(int*)malloc(dimension*dimension*sizeof(int));
	
	get_array (first_array,dimension,file_id);
	get_array (second_array,dimension,file_id);
	
	myclose(file_id);	
	
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
	
	current_time=time(NULL);		
	
	for (j=0;j<number_of_threads-1;++j)
		pthread_create (threads_id+j, NULL, &thread_function, array_of_arguments + j);
	 thread_function (array_of_arguments+number_of_threads-1);

	for (j=0;j<number_of_threads-1;++j)
		pthread_join (threads_id[j],NULL);

	execution_time=time(NULL);
	execution_time-=current_time;	
	
	//print_matrix(result_array,dimension);

	free (array_of_arguments);
	free(first_array);
	free(second_array);
	free (threads_id);
	
	result_file_id=open(RESULTPATH,O_RDWR|O_CREAT|O_TRUNC,0666);
	if (result_file_id<0) {
		printf ("Can not create file!\n");
		return -1;
		free (result_array);
	}

	mywrite_final (result_file_id, result_array, dimension);
	
	free (result_array);
	printf("\nDone!\n");
	printf("execution time is %i seconds\n",(int)execution_time);
	return 0;
}

int get_cpu_number() {
	int fork_result, fd_for_pipe[2];
	int resultdup;
	int answer;
	char* mybash=getenv("SHELL"); //don't change mybash!
	
	if ( pipe(fd_for_pipe)<0) {
		printf("Error! Can not create a pipe!\n");
		exit(-1);
	}
	
	fork_result=fork();
	if (fork_result<0) {
		printf("Error! Can not creare a child process!\n");
		exit(-1);
	} else if (fork_result==0) {
		resultdup=dup2(fd_for_pipe[1],STDOUT_FILENO);
		if( ( (resultdup-STDOUT_FILENO)||close(fd_for_pipe[0])||close(fd_for_pipe[1]) ) !=0) {  
			char errormesage[]="Error occured when file descriptor table editing!\n";
			write (STDERR_FILENO,errormesage,strlen(errormesage));
			exit(-1);
		}
		if ( execl (mybash,mybash,"-c","nproc --all",NULL) <0 ) {
			char errormesage[]="Error occured! Can not run bash!\n";
			write (STDERR_FILENO,errormesage,strlen(errormesage));
			exit(-1);
		}
	} else {
		if (close (fd_for_pipe[1])!=0) {
			printf("Error when closing pipe for writing\n");
			exit(-1);
		}
		int readresult;
		char *buffstring=NULL;
		buffstring=(char*)calloc(LENGTH_CPU+1,sizeof(char));
		readresult=read (fd_for_pipe[0],buffstring,LENGTH_CPU);
		if (readresult<=0) {
			printf("Error when reading from pipe!\n");
			free(buffstring);
			exit(-1);
		}
		answer=getnumber(buffstring);
		free (buffstring);
		if (close (fd_for_pipe[0])!=0) {
			printf("Error when closing pipe for reading\n");
			exit(-1);
		}
		return answer;
	}
	return 0;
}

void myclose (int file_id) {
	if ( close(file_id)<0) {
		printf("Can not close file!\n");
		exit(-1);
	}
	return;
}

int min_two (int first, int second) {
	if (first<=second) 
		return first;
	return second;
}

int getnumber (char* string) {
	unsigned int i;
	int answer=0;
	char c,flag=1;
	for (i=0;i<sizeof(string);++i) {
		c=string[i];
		if ((flag==1)&&( (c>'9')||(c<'0') )) {
			printf("Not a number!\n");
			exit(-1);
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
		myclose(file_id);
		return;
	}
	while (1) {
		c=getchar();
		if ( (c=='Y')||(c=='y'))
			break;
		if ( (c=='N')||(c=='n'))
			exit(-1);
		if (c!='\n')
			printf ("Enter, please, 'y' or 'n' -> ");
	}
	fork_result=fork();
	if (fork_result<0) {
		printf("Error! Can not creare a child process!\n");
		exit(-1);
	} else if (fork_result==0) {
		execl("./matrix.out","./matrix.out",NULL);
		printf("Can not run the first application!\n");
		exit (-1);
	}
	w=waitpid (fork_result, NULL,0);
	if (w == -1) {
		printf("waitpid error!\n");
		exit(-1);
	}

	return;
}

void get_array (int* array, int dim, int file_id) {
	int remaind_var,buff_size,i,current_size;
	int *buff_array;

	remaind_var=dim*dim;
	
	if (BUFFSIZE<sizeof(char))
		buff_size=sizeof(char);
	else
		buff_size=BUFFSIZE;

	while (remaind_var>0) {
		current_size=min_two(buff_size,remaind_var*sizeof(int))/sizeof(int);
		buff_array=(int*)malloc(current_size*sizeof(int));

		if ( read(file_id,buff_array,current_size*sizeof(int)) < current_size*sizeof(int)) {
			printf("error when reading!\n");
			free(buff_array);
			free(array);
			exit(-1);
		}
		
		for (i=0; i<current_size;++i)
			array[dim*dim-remaind_var+i]=buff_array[i];
		free(buff_array);
		remaind_var-=current_size;
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

int get_element_matrix (int* array_left, int* array_right, int dim, int column, int string) {
	int result=0, i;
	for (i=0;i<dim;++i)
		result+=(array_left[string*dim+i]*array_right[i*dim+column]);
	return result;
}

void* thread_function (void* argument) {
	thread_arguments *arg;
	int i, dimension;
	
	arg = (thread_arguments *) argument;
	dimension=arg->dim;
	
	for (i=arg->start_position;i<(arg->start_position+arg->destination_size); ++i)
			(arg->ans_array)[i]=get_element_matrix(arg->l_array,arg->r_array,dimension,i%dimension,i/dimension);
	return NULL;
}

void mywrite_final (int file_id, int* result_array, int dimension) {
	int i,j;
	char tab=9;
	for (j=0;j<dimension;++j) {
		for (i=0;i<dimension;++i) {
			mywrite_int (file_id, result_array[dimension*j+i]);
			if (write (file_id,&tab,1) != 1) {
				printf ("FATALL ERROR!!!\n");
				exit (-1);
			}
		}
		if (write (file_id,"\n",1) != 1) {
			printf ("FATALL ERROR!!!\n");
			exit (-1);
		}
	}
	return;
}

void mywrite_int (int file_id, int number) {
	char* string=NULL, *revstr=NULL;
	int size=1, i;

	string = (char*) malloc (sizeof (char));

	string[0]=number%10+'0';
	number/=10;
	while (number>0) {
		size++;
		string=(char*)realloc(string,size*sizeof(char));
		string[size-1]=number%10+'0';
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
