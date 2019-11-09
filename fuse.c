

/**************************************
* sachin kumar
* 15CS30025
***************************************/



#include "sfs.h"
#include "disk.h"
#include "fuse.h"


#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define STATBLOCKSIZE 16
#define SUCCESS 0
#define ERR -1
#define MAXSIZE 999999
#define MAGICNUMBER 12345
#define DIR 1
#define FILE 0


struct f_struct
{
	char *name;
	struct f_struct *next;
};
typedef struct f_struct f_data_struct;
char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 256 ];
int curr_file_content_idx = -1;

void add_dir( const char *dir_name )
{
	curr_dir_idx++;
	strcpy( dir_list[ curr_dir_idx ], dir_name );
}

int is_dir( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

void add_file( const char *filename )
{
	curr_file_idx++;
	strcpy( files_list[ curr_file_idx ], filename );
	
	curr_file_content_idx++;
	// strcpy( files_content[ curr_file_content_idx ], "" );
}

int is_file( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

/*int get_file_index( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}*/
/*
void write_to_file( const char *path, const char *new_content )
{
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 ) // No such file
		return;
		
	strcpy( files_content[ file_idx ], new_content ); 
}*/

// ... //

static int do_getattr( const char *path, struct stat *st )
{
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if ( strcmp( path, "/" ) == 0 || is_dir( path ) == 1 )
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
	}
	else if ( is_file( path ) == 1 )
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}
	else
	{
		return -ENOENT;
	}
	
	return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{	char *data;
	int len=0,foffset=0;
	do{
    	char * file_data = (char*)malloc((MAXSIZE+1)*sizeof(char));
		if(read_file(path,file_data,MAXSIZE,foffset)==ERR){
			return ERR;
		}
		foffset=foffset+strlen(file_data);
		// string s(file_data);
		char *fdata=(char*)malloc((strlen(data)+strlen(file_data)+1)*sizeof(char));
		strcpy(fdata,data);
		strcat(fdata,file_data);
		len=strlen(file_data);
		data=(char*)malloc((strlen(fdata)+1)*sizeof(char));
		strcpy(data,fdata);
	}while(len==MAXSIZE);

	char* fdata=data;//(char*)malloc((data.size()+1)*sizeof(char));
	// strcpy(fdata,data.c_str());
	char *fdata_token = strtok(fdata, "|"); 
    // vector<string> fdata_tokens;
    f_data_struct *f_meta_data=(f_data_struct*)malloc(1*sizeof(f_data_struct));
    f_data_struct *f_meta_data_head=f_meta_data;
    while (fdata_token != NULL) 
    { 
        f_meta_data->name=(char*)malloc((strlen(fdata_token)+1)*sizeof(char));
        strcpy(f_meta_data->name,fdata_token);
        f_meta_data->next=(f_data_struct*)malloc(1*sizeof(f_data_struct));
        f_meta_data=f_meta_data->next;
        fdata_token = strtok(NULL, "|"); 
    } 
    f_meta_data=f_meta_data_head;
	if (f_meta_data==NULL)return ERR;    
    while(f_meta_data!=NULL){
 		char* fmetadata=(char*)malloc((strlen(f_meta_data->name)+1)*sizeof(char));
	    strcpy(fmetadata,f_meta_data->name);
	    int count=0;
	    char* bit=strtok(fmetadata,",");
	    count++;
	    // vector<string> fmetadata_str;
	    while(bit != NULL){
	    	// string b(bit);
	    	// fmetadata_str.push_back(b);
	    	if (count==3)//filename or dir name
	    	{
				filler( buffer, bit, NULL, 0 ); 
	    		break;
	    	}
	    	bit=strtok(NULL,",");
	    	count++;
	    }
	  /*  if (fmetadata_str.size()<5)
	    {
	    	return ERR;
	    }
*/
	    /*if(fmetadata_str[0]!="1") {
	    	printf("invalid file\n");
	    	break;
	    }*/
	    // fmetadata_str[2]
		// filler( buffer, fmetadata[2], NULL, 0 ); // Current Directory
    }

	/*filler( buffer, "..", NULL, 0 ); // Parent Directory
	
	if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
	{
		for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
			filler( buffer, dir_list[ curr_idx ], NULL, 0 );
	
		for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
			filler( buffer, files_list[ curr_idx ], NULL, 0 );
	}*/
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	
	buffer=(char*)malloc((size+1)*sizeof(char));
	read_file(path, buffer, (int )size, (int )offset);
	/*int file_idx = get_file_index( path );
	
	if ( file_idx == -1 )
		return -1;
	
	char *content = files_content[ file_idx ];
	
	memcpy( buffer, content + offset, size );*/
		
	return strlen( buffer );
}

static int do_mkdir( const char *path, mode_t mode )
{
	path++;
	add_dir(path);
	if(create_dir(path)==ERR)return ERR;
	return 0;
}

static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
	path++;
	add_file( path );
	char buffer[10];
	if(write_file(path,buffer,0,0)==ERR)return ERR;
	return 0;
}

static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	if(write_file(path, buffer, (int)size, (int)offset)==ERR)return ERR;
	
	return size;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
    .mkdir		= do_mkdir,
    .mknod		= do_mknod,
    .write		= do_write,
};
