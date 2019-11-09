
/**************************************
* sachin kumar
* 15CS30025
***************************************/
#include "fuse.h"
#include "sfs.h"
#include "disk.h"


#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
/*#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>*/

#define SUCCESS 0
#define ERR -1
// #include <iostream>
// using namespace std;

int main(int argc, char const *argv[])
{
	char filename[]="sachin";
	int fd=open_disk(filename);
	if (fd!=ERR)
		{	
			// int p=format(fd);
			if(mount(fd)==SUCCESS)
			{
				// return fuse_main( argc, (char**)argv, &operations, NULL );
			}
		}	
	return 0;
}