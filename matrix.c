#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAXDIMENSION 20000
#define BUFFSIZE 20
#define TMPFILE "./tmpmatrix.txt"
#define MAXINT 40
#define OFFSET 0

void myclose (int file_id);

void mywrite(int file_id,void *array,int size);

int getnumber (char* string);

int min_two (int first, int second);

int main (int argc, char** argv) {
	int dim, i ,remaind_var, *matrix_array=NULL,current_size, buff_size;
	char c;
	char* path = TMPFILE;
	int file_id;

	if (argc<=1) {
		printf ("Enter a number of string in matrix ->");
		scanf("%i",&dim);
	}
	else
		dim=getnumber(argv[1]);
	
	if (dim<=0) {
		printf("The number should be positive!\n");
		return -1;
	}
	
	if (dim>MAXDIMENSION) {
		dim = MAXDIMENSION;
		printf ("The number is too big. Maximum possible number is %i. Would you like to continue? (Y/n)\n", MAXDIMENSION);
		while (1) {
			c=getchar();
			if ( (c=='Y')||(c=='y'))
				break;
			if ( (c=='N')||(c=='n'))
				return 0;
			if (c!='\n')
				printf ("Enter, please, 'y' or 'n' -> ");
		}
	}
	
	file_id=open(path,O_RDWR|O_CREAT|O_TRUNC,0666);
	if (file_id<0) {
		printf ("Can not open file!\n");
		return -1;
	}
	
	if (write(file_id,&dim,sizeof(int))<sizeof(int)) {
		printf("error when writing!\n");
		myclose(file_id);
		return -1;
	}
	
	if (BUFFSIZE<sizeof(char))
		buff_size=sizeof(char);
	else
		buff_size=BUFFSIZE;
	remaind_var=dim*dim*2;

	while (remaind_var>0) {
		current_size=min_two(buff_size,remaind_var*sizeof(int))/sizeof(int);
		matrix_array=(int*)malloc( sizeof(int)*current_size );
		for (i=0;i<current_size;++i)
			matrix_array[i]=random()%MAXINT-OFFSET;
		mywrite(file_id,matrix_array,current_size);
		free(matrix_array);
		remaind_var-=current_size;
	}
			
			

	myclose(file_id);
	return 0;
}

int getnumber (char* string) {
	int i,answer=0;
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

void mywrite(int file_id,void *array,int size) {
	int write_result,error_size;
	char* repair_array=NULL;
	
	repair_array=(char*)calloc(size, sizeof(int));

	write_result=write(file_id,array,size*sizeof(int));
	
	if (write_result<0) {
		printf("Unexpected error occured!\n");
		myclose(file_id);	
		free(array);
		free (repair_array);
		exit(-1);
	}

	if (write_result<size*sizeof(int)) {
		printf("Warning!\n");
		error_size=size*sizeof(int)-write_result;
		write_result=write(file_id,repair_array,error_size);
		if (write_result!=error_size) {
			printf("Attemp to fix the problem failled!\n");
			free(array);
			free (repair_array);
			myclose(file_id);
			exit (-1);
		} else 
			printf ("The problem have been fixed succesfully!\n");
	}
	free(repair_array);
	return;
}
		
