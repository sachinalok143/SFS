/**************************************
* sachin kumar
* 15CS30025
***************************************/
#include "disk.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <cmath>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

using namespace std;

#define STATBLOCKSIZE 16
#define SUCCESS 0
#define ERR -1
#define MAXSIZE 999999



std::vector<string>  get_data_from_file_descriptor(int fd){
	char path[1024];
    char result[1024];
	sprintf(path, "/proc/self/fd/%d", fd);
    memset(result, 0, sizeof(result));
    readlink(path, result, sizeof(result)-1);
   	char *token = strtok(result, "/"); 
    char prevtoken[100] ;
    while (token != NULL) 
    { 
        // printf("%s\n", token); 
        strcpy(prevtoken,token);
        token = strtok(NULL, "/"); 
    } 
    // cout<<prevtoken<<"+++++++/\n";
    // cout<<fd<<"sachin2: "<<prevtoken<<endl;
  	ifstream myfile(prevtoken);
  	// cout<<"sachin3: "<<prevtoken<<endl;
    
    string line;
    std::vector<string> output;
    
  	if (myfile.is_open())
  	{
    	while ( getline (myfile,line) )
    	{
    		// output=output+line;
    		// cout<<line<<endl;
    		output.push_back(line);
    	}
    	myfile.close();
  	}
  	/*for (int i = 0; i < output.size(); ++i)
  	{
  		cout<<output[i]<<endl;
  	}
*/
    return output;
}
string get_line_from_file_descriptor(int fd){
  	std::vector<string> v=get_data_from_file_descriptor(fd);
  	// cout<<"sachin1"<<endl;
    return v[0];
}
char*  get_filename_from_fd(int fd,char *prevtoken){
	char path[1024];
    char result[1024];
	sprintf(path, "/proc/self/fd/%d", fd);
    memset(result, 0, sizeof(result));
    readlink(path, result, sizeof(result)-1);
   	char *token = strtok(result, "/"); 
    // char prevtoken[100] ;
    prevtoken=(char*)malloc(100*sizeof(char));
    while (token != NULL) 
    { 
        // printf("%s\n", token); 
        strcpy(prevtoken,token);
        token = strtok(NULL, "/"); 
    } 
    return prevtoken;
}
void over_write_data_of_fd(int fd,string data){
	// cout<<"do you rea"
	char *prevtoken=(char*)malloc(100*sizeof(char));
	strcpy(prevtoken, get_filename_from_fd(fd,prevtoken));
    // cout<<prevtoken<<"+++++++/\n";
  	fstream myfile ;
  	myfile.open(prevtoken, ios::out|ios::trunc);
    
    string line;
    std::vector<string> output;
    
  	if (myfile.is_open())
  	{
    	myfile<<data<<endl;
    	myfile.close();
  	}
}

void append_line_to_fd(int fd,string data){
	char *prevtoken=(char*)malloc(100*sizeof(char));
	
	strcpy(prevtoken, get_filename_from_fd(fd,prevtoken));
    // cout<<prevtoken<<"+++++++/\n";
  	fstream myfile ;
  	myfile.open(prevtoken, std::ios_base::app | std::ios_base::out);
    
    string line;
    std::vector<string> output;
    
  	if (myfile.is_open())
  	{
    	myfile<<data;
    	if (data[data.size()-1]!='\n')
    	{
    		myfile<<endl;
    	}
    	myfile.close();
  	}
}
bool fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}





int create_disk(char *filename, int nbytes){
	// cout<< "creating new disk with name \""<<filename<<"\" ...."<<endl;
	FILE *fptr;
	fptr = fopen(filename, "rb+");
	if(fptr == NULL) //if file does not exist, create it
	{
		fptr = fopen(filename, "wb");
		fclose(fptr);
	}else
	 {
	 	cout<< "disk with name \"" <<filename<<"\" is alread present.open disk and start using.\n";
	 	return ERR;
	 }
	disk_stat* new_disk=(disk_stat*)malloc(sizeof(disk_stat));
	new_disk->size=nbytes;
	new_disk->blocks= floor((nbytes-STATBLOCKSIZE)/BLOCKSIZE) ; 
	new_disk->reads=0; // number of block reads performed
	new_disk->writes=0;

	string line;
	line=to_string(new_disk->size)+','+to_string(new_disk->blocks)+','+to_string(new_disk->reads)+','+to_string(new_disk->writes)+'\n';
	ofstream os(filename);  
	if (!os) { 
		cerr<<"Error writing to ..."<<std::endl; 
		return ERR;
	} else {  
	  os << line;  
	}  
	os.close();
	// cout<< "disk created ...."<<endl;

	return SUCCESS;
}

int open_disk(char *filename){
	int sz,fd =open(filename,O_RDWR);
	if (fd==ERR)
	{
		cerr<<"[ERROR]:"<<errno<<endl;
		return ERR;
	}
	string getdata= get_line_from_file_descriptor(fd);
	char cstr[getdata.size() + 1];
	getdata.copy(cstr, getdata.size() + 1);
	cstr[getdata.size()] = '\0';


	char *token = strtok(cstr, ",");
    std::vector<int>v;
    while (token != NULL) 
    { 
        v.push_back(atoi(token));
        // cout<<token<<endl;
        token = strtok(NULL, ","); 
    }
    if (floor((v[0]-16)/BLOCKSIZE)==v[1] && v.size()==4)
    {
    	cout<<"disk opened ..."<<endl;
    	return fd;
    }else
		return ERR;// fd =-1 when error occured
}

disk_stat* get_disk_stat(int disk){
	// cout<<"Reading disk statistics ....."<<endl;
	if (!fd_is_valid(disk))
	{
		return NULL;
	}
	string getdata= get_line_from_file_descriptor(disk);
	// cout<<"sahin"<<endl;

	char cstr[getdata.size() + 1];
	getdata.copy(cstr, getdata.size() + 1);
	cstr[getdata.size()] = '\0';
	char *token = strtok(cstr, ",");
    std::vector<int>v;
    while (token != NULL) 
    { 
        v.push_back(atoi(token));
        // cout<<token<<endl;
        token = strtok(NULL, ","); 
    }
    if (v.size()!=4)
    {
    	return NULL;
    }
	disk_stat* new_disk=(disk_stat*)malloc(sizeof(disk_stat));
	new_disk->size=v[0];
	new_disk->blocks= v[1] ; 
	new_disk->reads=v[2]; // number of block reads performed
	new_disk->writes=v[3];
	// cout<<"Reading disk statistics ..... Finish"<<endl;
	
	return new_disk;
}
int read_block(int disk, int blocknr, void *block_data){
	// cout<<"reading block..."<<endl;
	if (!fd_is_valid(disk))
	{
		return ERR;
	}

	disk_stat* ds=get_disk_stat(disk);
	if (disk<0 || blocknr >= ds->blocks)
	{
		return ERR;
	}
	std::vector<string> data;

	data=get_data_from_file_descriptor(disk);
	string output_data="";
	if (data.size()>blocknr)
	{
		output_data=data[blocknr+1];
	}
	// cout<<output_data<<"++++\n";
	char cstr[output_data.size() + 1];
	strcpy(cstr, output_data.c_str());
	// cout<<cstr<<endl;
	strcpy((char*)block_data,cstr);
	ds->reads=ds->reads+1;
	data[0]=to_string(ds->size)+','+to_string(ds->blocks)+','+to_string(ds->reads)+','+to_string(ds->writes)+'\n';
	
	std::ofstream ofs;
	char fm[100];
	strcpy(fm,get_filename_from_fd(disk,fm));
	ofs.open(fm, std::ofstream::out | std::ofstream::trunc);
	ofs.close();
	
	for(string s:data)append_line_to_fd(disk,s);
	// cout<<"Finished."<<endl;
	return SUCCESS;
}

int write_block(int disk, int blocknr, void *block_data){
	// cout<<"Writing data to block ..."<<endl;
	if (!fd_is_valid(disk))
	{
		return ERR;
	}

	// cout<<"sahin"<<endl;
	disk_stat* ds=get_disk_stat(disk);
	if (disk<0 || blocknr >= ds->blocks)
	{
		return ERR;
	}
	std::vector<string> data;

	data=get_data_from_file_descriptor(disk);

	char* inputput_data=(char*)block_data;
	string s(inputput_data);
	// cout<<"yes1"<<data.size()<<","<<blocknr<<endl;
	if (data.size()>blocknr+1)
	{
		// data[blocknr+1]=(char*)malloc(s.size()*sizeof(char));
		data[blocknr+1]=s;
		// data.insert(blocknr+1,s);
		// cout<<s<<","<<blocknr<<endl;
	}else{
		while(data.size()<=blocknr)data.push_back("\n");
		data.push_back(s);
	}
	// cout<<"yes2"<<endl;

	ds->writes=ds->writes+1;
	data[0]=to_string(ds->size)+','+to_string(ds->blocks)+','+to_string(ds->reads)+','+to_string(ds->writes)+'\n';
	string temp;
	// over_write_data_of_fd(disk,"");
	std::ofstream ofs;
	char fm[100];
	strcpy(fm,get_filename_from_fd(disk,fm));
	ofs.open(fm, std::ofstream::out | std::ofstream::trunc);
	ofs.close();
	for(string s:data)append_line_to_fd(disk,s);
	// over_write_data_of_fd(disk,temp);
	// cout<<"Finished"<<endl;
	return SUCCESS;
}

int close_disk(int disk){
	if (!fd_is_valid(disk))
	{
		cerr<<"Invalid file descriptor"<<endl;
		return ERR;
	}
	if(close(disk)==ERR)return ERR;
	cout<<"Disk closed"<<endl;
	return SUCCESS;
}

/*int main(int argc, char const *argv[])
{
	char filename[]="temp";
	int fd=open_disk(filename);
	// disk_stat* disk=get_disk_stat(fd);
	char s[]="alok";//=(char*)malloc(MAXSIZE*sizeof(char));
	// s[]="sachin";
	int t=read_block(fd,98,s);
	if (t==SUCCESS)
	{
		printf("%s\n",(char*)s );
	}else printf("INVALID\n");

	int o=close(fd);
	// cout<<o<<endl;
	return 0;
}*/