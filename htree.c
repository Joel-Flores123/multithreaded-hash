#include <pthread.h>
#include <stdio.h>     
#include <stdlib.h>   
#include <stdint.h>  
#include <inttypes.h>  
#include <errno.h>     // for EINTR
#include <fcntl.h>     
#include <unistd.h>    
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

// Print out the usage of the program and exit.
void Usage(char*);

uint32_t jenkins_one_at_a_time_hash(uint8_t* , size_t ); //prototype for the hashing function

void *tree(void *arg); //prototype for our tree function

//finds the maxI 

int maxI(int height);

#define BSIZE 4096

int main(int argc, char** argv) 
{
	
	void* retval = NULL;	
	int32_t fd;
  	uint32_t nblocks;

  	// input checking 
  	if (argc != 3)
    	{	
		
		Usage(argv[0]);
	
	}
  
	// open input file
  
	fd = open(argv[1], O_RDWR);
  
	if (fd == -1) 
	{
 
	     	perror("open failed");
    		exit(EXIT_FAILURE);
  
	}
	
  	struct stat s;

  	// use fstat to get file size
  	// calculate nblocks 

  	if(fstat(fd, &s) != 0)
  	{
  	
   		 perror("fstat failed");
  	 	 exit(EXIT_FAILURE);
  
 	 }

  	nblocks = s.st_size / BSIZE; //get n blocks
 	printf(" no. of blocks = %u \n", nblocks);

 	// double start = GetTime();

 	// calculate hash of the input file
	
  	int32_t dat[4];
	dat[1] = atoi(argv[2]); //height
	dat[2] = 0;  //i
	dat[3] = maxI(dat[1] + 1) - 1; //get maxi aka max threads
	dat[0] = nblocks / (dat[3] + 1); //nblocks/threads
	dat[4] = fd; //get fd

	pthread_t p; //Make root thread	
	
	pthread_create(&p, NULL, tree, dat);
 	pthread_join(p, &retval);

 	//double end = GetTime();
  	printf("hash value = %s\n", (char*)retval);
  	//printf("time taken = %f \n", (end - start));
  	close(fd);
  
	return EXIT_SUCCESS;

}

uint32_t jenkins_one_at_a_time_hash(uint8_t* key, size_t length) 
{

	size_t i = 0;
	uint32_t hash = 0;
	
	while (i != length) 
	{
		
		hash += key[i++];
		hash += hash << 10;
		hash ^= hash >> 6;

	}

	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	
	return hash;

}

//n = blocks m = threads
//arg will be an array with 2 values the max height and the block number

void *tree(void *arg)
{

	char result [400] = ""; // where we will store the final hashed string
	int* oldDat  = (int*) arg; //cast array 
	int32_t dat[5]; //make new array so we dont mess up addresses
	
	//transfer data from old dat to new dat

        dat[0] = oldDat[0];
	dat[1] = oldDat[1];
	dat[2] = oldDat[2];
	dat[3] = oldDat[3];
       	dat[4] = oldDat[4];

	size_t  eof = lseek(dat[4], 0, SEEK_END); //get eof	
	size_t length = BSIZE * dat[0]; //get length

	pthread_t p,p2;
	unsigned char hash [BSIZE * dat[0]]; //we read this amount of characters
	int i;
	void* retval = NULL;
	uint32_t val = 0;

	if(dat[1] > 0)
	{
		
		size_t cur = lseek(dat[4], BSIZE * dat[0] * dat[2], SEEK_SET); //get cur which is i * consecutive blocks * block size
		
		if(cur > eof) //if beyond file
		{
				
			puts("error with current location");
			exit(EXIT_FAILURE);
		
		}

		if(read(dat[4], hash, BSIZE * dat[0]) != -1) //check if this works
		{
			
			dat[1] = dat[1] - 1; //change height
			
			val = jenkins_one_at_a_time_hash(hash, length); 
			sprintf(result,"%u", val); //throw val into result
		
			i = dat[2] * 2 + 2; //compute the i of right child 
			dat[2] = dat[2] * 2 + 1; //change dats i so we can toss it into the next thread
		
			pthread_create(&p, NULL, tree, dat);
			pthread_join(p, &retval);
			strcat(result, (char*) retval); //concatenate the left child with the internal node	

		
			dat[2] = i; 
			pthread_create(&p2, NULL, tree, dat);
			pthread_join(p2, &retval);
			strcat(result, (char*) retval); //concatenate the right child with the internal node	

		}
		else //read failed
		{
			
			puts("read failed");
    			exit(EXIT_FAILURE);

		}

	}
	else 
	{
	
		size_t cur = lseek(dat[4], BSIZE * dat[0] * dat[2], SEEK_SET); //get cur which is i * consecutive blocks * block size
	
		if(cur > eof) //if beyond file
		{
				
			puts("error with current location");
			exit(EXIT_FAILURE);
		
		}

		i = 2 * dat[2] + 1;
		
		if(i == dat[3]) //if i is equal to left most make a new thread
		{
			
			if(read(dat[4], hash, BSIZE * dat[0]) != -1) //check if this works
			{
				
				val = jenkins_one_at_a_time_hash(hash, length); 
				sprintf(result,"%u", val); //throw val into result
				
				dat[2] = i;	
				pthread_create(&p, NULL, tree, dat);
				pthread_join(p, &retval);
				strcat(result, (char*) retval); //concatenate the leftmost child with the internal node	

			}
			else //error out
			{
			
				puts("error in read");
				exit(EXIT_FAILURE);
			
			}
		
		}
		else
		{
		
			eof = eof - cur; //for our comparison
				
			if(eof < length) //so we do not go out of bounds
			{
					
				length = eof;
				
			}
			
			if(read(dat[4], hash, BSIZE * dat[0]) != -1) //check if this works
			{
				

				val = jenkins_one_at_a_time_hash(hash, length); 
				sprintf(result,"%u", val); //throw val into result
			
			}
			else //error out
			{
			
				puts("error in read");
				exit(EXIT_FAILURE);
			
			}
		
		}


	}
	
	pthread_exit((void*)result);

	return NULL;

}

//find maxI to know whats our leftMode node

int maxI(int height)
{
	
	int i = 1;

	//multiply i by 2 until height is 0

	while(height > 0)
	{
	
		i = i * 2;
		height--;
	
	}

	return i;

}

void Usage(char* s) 
{
  fprintf(stderr, "Usage: %s filename height \n", s);
  exit(EXIT_FAILURE);
}
