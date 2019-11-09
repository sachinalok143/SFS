
/**************************************
* sachin kumar
* 15CS30025
***************************************/

#include "sfs.h"
#include "disk.h"
#include <stdio.h>

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
#define MAGICNUMBER 12345
#define DIR 1
#define FILE 0

int create_file_or_dir=DIR;
vector<string> inode_bitmap;
vector<string> data_block_bitmap;
int fd=-1;
bool isMounted=false;


string ESCAPE = "ESCAPE";
int root_dir=0;
int create_root_dir(){
	string dirname="root";
	cout<<" root dir created"<<endl;
	int ind=create_file();
	root_dir=ind;
	string dir=to_string(1)+ ","+ //valid bit
			to_string(DIR)+","+  //type file or directory
			dirname+","+ //dir name 
			to_string(dirname.size())+","+ // length of dir name 
			to_string(ind)+"|"; // inumber of file or directory
	dir=dir+dir;
	char * block_data = (char*)malloc((dir.size()+1)*sizeof(char));
	strcpy(block_data,dir.c_str());
	// cout<<block_data<<"---"<<endl;
	write_i(ind,block_data,strlen(block_data),0);
	return ind;
}


super_block* sb=(super_block*)malloc(1*sizeof(super_block));

void prin_super_block(super_block* spr_block){
	cout<< spr_block->magic_number <<","<<
	spr_block->blocks <<","<<
	spr_block->inode_blocks <<","<<
	spr_block->inodes <<","<<
	spr_block->inode_bitmap_block_idx <<","<<
	spr_block->inode_block_idx <<","<<
	spr_block->data_block_bitmap_idx <<","<<
	spr_block->data_block_idx <<","<<
	spr_block->data_blocks <<endl;
}

int format(int disk){
	cout<<"formating disk ...."<<endl;

	super_block *spr_block=(super_block*)malloc(1*sizeof(super_block));
	disk_stat* ds= get_disk_stat(disk);
	int N=ds->blocks;
	int M=N-1; // blocks except super blocks 
	int I=floor(0.1*M); //inode blocks
	int IB=ceil(((double)I*128)/(double)(8*BLOCKSIZE)); //inode bitmap blocks	
	int R=M-I-IB; 
	int DBB =ceil((double) R / (double)(8 * 4096));//datablock bitmap blocks
	int DB=R-DBB; //datablocks 
	
	spr_block->magic_number = 12345;
	spr_block->blocks = M;
	spr_block->inode_blocks = I;
	spr_block->inodes = I * 128;
	spr_block->inode_bitmap_block_idx = 1;
	spr_block->inode_block_idx = 1 + IB + DBB;
	spr_block->data_block_bitmap_idx = 1 + IB;
	spr_block->data_block_idx = 1 + IB + DBB + I;
	spr_block->data_blocks = DB;

	string line=to_string( spr_block->magic_number)+','+
		to_string( spr_block->blocks)+','+
		to_string( spr_block->inode_blocks)+','+
		to_string( spr_block->inodes)+','+
		to_string( spr_block->inode_bitmap_block_idx)+','+
		to_string( spr_block->inode_block_idx)+','+
		to_string( spr_block->data_block_bitmap_idx)+','+
		to_string( spr_block->data_block_idx)+','+
		to_string( spr_block->data_blocks)+'\n';
	int res=0;
	int blocknr=0;
	char *block_data=(char*)malloc((line.size()+1)*sizeof(char*));
	strcpy(block_data, line.c_str());
	res=write_block(disk, blocknr, block_data);
	if(res==ERR)return ERR;

	/*
	writting inodes bitmap
	*/
	// cout<<"spr_block->inode_bitmap_block_idx:"<<spr_block->inode_bitmap_block_idx<<endl;
	// cout<<"spr_block-> data_block_bitmap_idx:"<<spr_block-> data_block_bitmap_idx<<endl;
	char ib[BLOCKSIZE*8]={'0'};
	std::fill(std::begin(ib), std::end(ib), '0');
	ib[BLOCKSIZE*8-1]='\0';
	// string bitmap(ib);
	for(int i =spr_block->inode_bitmap_block_idx;i< spr_block-> data_block_bitmap_idx;i++){
		// cout<<i<<"\t";
		res=write_block(disk, i, ib);
		if(res==ERR)return ERR;
	}

	/*
	writing block bitmap
	*/

	for(int i=0;i<spr_block->data_block_idx;i++)ib[i]='1';
	// memset(ib,'1',sb->data_block_idx);
	// cout<<ib<<endl;
	// cout<<
	for(int i =spr_block->data_block_bitmap_idx; i<spr_block-> inode_block_idx; i++){
		res=write_block(disk, i, ib);
		if(res==ERR)return ERR;
	}

	/*
	
	*/

	/*typedef struct inode {
	uint32_t valid; // 0 if invalid
	uint32_t size; // logical size of the file
	uint32_t direct[5]; // direct data block pointer
	uint32_t indirect; // indirect pointer
	} inode;*/
	inode* ind=(inode*)malloc(1*sizeof(inode));
	ind->valid=0;
	ind->size=0;
	for(int i =0;i<5;i++){
		ind->direct[i]=0; //default value
	}
	ind->indirect=0;//default value
	// cout<<"to_string(ind->indirect):"<<to_string(ind->indirect)<<endl;
	string idn_string=to_string(ind->valid)+','+
					to_string(ind->size)+','+
					to_string(ind->direct[0])+','+
					to_string(ind->direct[1])+','+
					to_string(ind->direct[2])+','+
					to_string(ind->direct[3])+','+
					to_string(ind->direct[4])+','+
					to_string(ind->indirect);
	
	//128 inode for a block
	// int k=1;
	for(int i =0; i<7;i++){
		// k=k+k;
		idn_string=idn_string+"|"+idn_string;
	}

	// printf("%d\n",k);
	/*
	writing inods 
	*/
	block_data=(char*)malloc((idn_string.size()+1)*sizeof(char*));
	strcpy(block_data, idn_string.c_str());
	for(int i =spr_block->inode_block_idx; i<spr_block-> data_block_idx; i++){
		res=write_block(disk, i, block_data);
		if(res==ERR)return ERR;
	}
	mount(disk);
	create_root_dir();
	cout<<"Finished."<<endl;
	return SUCCESS;

	free(spr_block);
	free(block_data);
}

int mount(int disk){
	char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
	int res=read_block(disk,0,block_data);
    cout<<block_data<<endl;
	// string spr_block_data(block_data);
	super_block *spr_block=(super_block*)malloc(1*sizeof(super_block));

	char *token = strtok(block_data, ",");
    std::vector<int>v;
    while (token != NULL) 
    { 
        v.push_back(atoi(token));
        // cout<<token<<endl;
        token = strtok(NULL, ","); 
    }


	spr_block->magic_number = v[0];
	spr_block->blocks =v[1] ;
	spr_block->inode_blocks = v[2];
	spr_block->inodes = v[3];
	spr_block->inode_bitmap_block_idx = v[4];
	spr_block->inode_block_idx = v[5];
	spr_block->data_block_bitmap_idx = v[6];
	spr_block->data_block_idx = v[7];
	spr_block->data_blocks = v[8];
	// prin_super_block(spr_block);
	if (spr_block->magic_number!=MAGICNUMBER)
	{
		cout<<"Magic number miss match."<<endl;
		return ERR;
	}


	/*
	reading bitmap
	*/
	// int res;
	char *ib=(char*)malloc(MAXSIZE*sizeof(char));
	for(int i =spr_block->inode_bitmap_block_idx;i< spr_block-> data_block_bitmap_idx;i++){
		// cout<<i<<"\t";
		res=read_block(disk, i, ib);
		if(res==ERR)return ERR;
		
		string s(ib);
		inode_bitmap.push_back(s);
		// inode_bitmap=inode_bitmap+s;
	}

	/*
	writing block bitmap
	*/

	for(int i =spr_block->data_block_bitmap_idx; i<spr_block-> inode_block_idx; i++){
		res=read_block(disk, i, ib);
		if(res==ERR)return ERR;
		
		string s(ib);
		data_block_bitmap.push_back(s);
		// data_block_bitmap=data_block_bitmap+s;
	}

	// cout<<inode_bitmap.size()<<endl;
	// cout<<data_block_bitmap.size()<<endl;
	sb=spr_block;
	isMounted=true;
	fd=disk;
	// cout<<"disk:"<<disk<<endl;
	return SUCCESS;
}
string get_inode_as_string(int inode_idx){
	int blocknr=sb->inode_block_idx+(inode_idx)/128;
	int blockOffset=(inode_idx)%128;
	char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
	int res=read_block(fd,blocknr,block_data);
	if (res==ERR)
	{
		return NULL;
	}
	// cout<<block_data<<endl;
	char *token = strtok(block_data, "|");
    std::vector<string>v;
    while (token != NULL) 
    { 
        v.push_back(token);
        token = strtok(NULL, "|"); 
    }
    return v[blockOffset];
}
int save_inode(string inode_str,int inode_idx){
	int blocknr=sb->inode_block_idx+(inode_idx)/128;
	int blockOffset=(inode_idx)%128;
	char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
	int res=read_block(fd,blocknr,block_data);
	if (res==ERR)
	{
		return ERR;
	}
	// cout<<block_data<<endl;
	char *token = strtok(block_data, "|");
    std::vector<string>v;
    while (token != NULL) 
    { 
        v.push_back(token);
        token = strtok(NULL, "|"); 
    }
	v[blockOffset]=inode_str;
	string data_string=v[0];
    for (int i = 1; i < v.size(); ++i)
    {
    	data_string=data_string+"|"+v[i];
    }

    // cout<<endl<<data_string<<endl;
    block_data=(char*)malloc((data_string.size()+1)*sizeof(char));
    strcpy(block_data,data_string.c_str());
    res=write_block(fd,blocknr,block_data);
	if(res==ERR)return ERR;
    // cout<<"jhghjfghhgjgjhg"<<endl;
	// cout<<"oiuewiu"<<endl;
	return SUCCESS;
}
int create_file(){
	// cout<<fd<<endl;
	if (fd<0 || !isMounted)
	{
		cout<<"File system not mounted."<<endl;
		return ERR;
	}
	int res;
	for (int i = 0; i < inode_bitmap.size(); ++i)
	{	int f=0;
		for (int j = 0; j < inode_bitmap[i].size(); ++j)
		{
			if (inode_bitmap[i][j]=='0')
			{
				cout<<"free inode  found ..."<<endl;
				inode_bitmap[i][j]='1';
				
				inode* ind=(inode*)malloc(1*sizeof(inode));
				ind->valid=1;
				ind->size=0;
				for(int i =0;i<5;i++){
					ind->direct[i]=0; //default value
				}
				ind->indirect=0;

				string idn_string=to_string(ind->valid)+','+
					to_string(ind->size)+','+
					to_string(ind->direct[0])+','+
					to_string(ind->direct[1])+','+
					to_string(ind->direct[2])+','+
					to_string(ind->direct[3])+','+
					to_string(ind->direct[4])+','+
					to_string(ind->indirect);

				int inode_idx=(i*BLOCKSIZE*8)+j;
				int blocknr=sb->inode_block_idx+(inode_idx)/128;
				int blockOffset=(inode_idx)%128;
				char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
				res=read_block(fd,blocknr,block_data);
				if (res==ERR)
				{
					return ERR;
				}
				// cout<<block_data<<endl;
				char *token = strtok(block_data, "|");
			    std::vector<string>v;
			    while (token != NULL) 
			    { 
			        v.push_back(token);
			        token = strtok(NULL, "|"); 
			    }

			    string inode_str=v[blockOffset];
			    // string inode_str=get_inode_as_string(inode_idx);

			    char* inode_char=(char*)malloc((idn_string.size()+1)*sizeof(char));
			    strcpy(inode_char,inode_str.c_str());
			    token = strtok(inode_char, ",");

			    std::vector<int> v1;
			    while (token != NULL) 
			    { 
			        v1.push_back(atoi(token));
			        token = strtok(NULL, ","); 
			    }
			    if (v1.size()<8)
			    {
			    	cerr<<"inode is not correct"<<endl;
			    	return ERR;
			    }
			    v1[0]=1;//v1[0] is valid bit
			    inode_str=to_string(v1[0]);
			    for (int i = 1; i < v1.size(); ++i)
			    {	
			    	inode_str=inode_str+","+to_string(v1[i]);
			    }
			    v[blockOffset]=inode_str;
			    string data_string=v[0];
			    for (int i = 1; i < v.size(); ++i)
			    {
			    	data_string=data_string+"|"+v[i];
			    }

			    // cout<<endl<<data_string<<endl;
			    block_data=(char*)malloc((data_string.size()+1)*sizeof(char));
			    strcpy(block_data,data_string.c_str());
			    res=write_block(fd,blocknr,block_data);
				if(res==ERR)return ERR;

			    char* tmp=(char*)malloc((inode_bitmap[i].size()+1)*sizeof(char));
				strcpy(tmp,inode_bitmap[i].c_str());
				res=write_block(fd,i+sb->inode_bitmap_block_idx,tmp);
				if(res==ERR)return ERR;
				free(ind);
				free(block_data);
				free(inode_char);
				return inode_idx;

				f=1;
				break;
			}
		}
		if (f==1)break;
	}
	
}


int remove_file(int inumber){
	if (fd<0 || !isMounted)
	{
		cout<<"File system not mounted."<<endl;
		return ERR;
	}
	int inode_idx=inumber;
	if (inode_bitmap[inumber/(8*BLOCKSIZE)][inumber%(8*BLOCKSIZE)]=='0')
	{
		cout<<"invalid inode number."<<endl;
		return ERR;
	}


	int blocknr=sb->inode_block_idx+(inode_idx)/128;
	int blockOffset=(inode_idx)%128;
	char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
	int res=read_block(fd,blocknr,block_data);
	if (res==ERR)
	{
		return ERR;
	}
	// cout<<block_data<<endl;
	char *token = strtok(block_data, "|");
    std::vector<string>v;
    while (token != NULL) 
    { 
        v.push_back(token);
        token = strtok(NULL, "|"); 
    }

	string inode_str=v[blockOffset];

	char* inode_char=(char*)malloc((inode_str.size()+1)*sizeof(char));
    strcpy(inode_char,inode_str.c_str());
    token = strtok(inode_char, ",");

    std::vector<int> v1;
    while (token != NULL) 
    { 
        v1.push_back(atoi(token));
        token = strtok(NULL, ","); 
    }
    free(inode_char);
    if (v1.size()<8)
    {
    	cerr<<"currupted inode"<<endl;
    	return ERR;
    }
    if (v1[0]==0)
    {
    	cout<<"invalid inode."<<endl;
    	return ERR;
    }
    v1[0]=0;//v1[0] is valid bit
    // v1[1]=0;//size of file is zero

    inode_str=to_string(v1[0]);
    for (int i = 1; i < v1.size(); ++i)
    {	
    	inode_str=inode_str+","+to_string(0);//all inode attributes are zeros
    }
   /* inode_str=to_string(v1[0]);
	for (int i = 1; i < v1.size(); ++i)
	{	
		inode_str=inode_str+","+to_string(v1[i]);
	}*/
	v[blockOffset]=inode_str;
	string data_string=v[0];
	for (int i = 1; i < v.size(); ++i)
	{
		data_string=data_string+"|"+v[i];
	}
	free(block_data);
	// cout<<endl<<data_string<<endl;
	block_data=(char*)malloc((data_string.size()+1)*sizeof(char));
	strcpy(block_data,data_string.c_str());
	res=write_block(fd,blocknr,block_data);
	if(res==ERR)return ERR;

    /*
    
    	updating data block bitmap

    */
    int x=inumber/(8*BLOCKSIZE);
    int y=inumber%(8*BLOCKSIZE);
    if (y>=128)return ERR;

    // cout<<inumber<<","<< x<<","<<y<<",hgbfghfghfgh"<<endl;
    inode_bitmap[x][y]='0';

    for (int i = 2; i < v1.size(); ++i)
    {	
    	// cout<<"v1[i]:"<<v1[i]<<endl;
    	if (v1[i]!=0)
    	{
		    data_block_bitmap[v1[i]/(8*BLOCKSIZE)][v1[i]%(8*BLOCKSIZE)]='0';		
    	}
    }
    
   	if(v1[7]!=0){

   		char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
		int res=read_block(fd,v1[7],block_data);
		if (res==ERR)return ERR;
		

		token = strtok(block_data, ",");

	    std::vector<int> indirect_block_idx;
	    while (token != NULL) 
	    { 
	        indirect_block_idx.push_back(atoi(token));
	        token = strtok(NULL, ","); 
	    }

	    free(block_data);
	    for (int i = 0; i < indirect_block_idx.size(); ++i)
	    {
	    	data_block_bitmap[indirect_block_idx[i]/(8*BLOCKSIZE)][indirect_block_idx[i]%(8*BLOCKSIZE)]='0';
	    }
   	}
    
    /*
    * save inode bitmap
    */
     char* tmp=(char*)malloc((inode_bitmap[x].size()+1)*sizeof(char));
	strcpy(tmp,inode_bitmap[x].c_str());
	res=write_block(fd,x+sb->inode_bitmap_block_idx,tmp);
	if(res==ERR)return ERR;
	free(tmp);
    /*
    * save inode
    */
   	// cout<<"bvcvbcv"<<endl;
    // res=save_inode(inode_str,inumber);
    if (res==ERR)return ERR; //save inode


	/*
	* save  data block bitmap
	*/
	for (int i = 0; i < data_block_bitmap.size(); ++i)
	{
		int idx=sb->data_block_bitmap_idx+i;
		char* tmp=(char*)malloc((data_block_bitmap[i].size()+1)*sizeof(char));
		strcpy(tmp,data_block_bitmap[i].c_str());
		int res=write_block(fd,idx,tmp);
		free(tmp);
    // cout<<"::::"<<endl;
		if(res==ERR)return ERR;
	}
	/*int blocknr=sb->data_block_bitmap_idx+indirect_block_idx[i]/data_block_bitmap.size();

	    	char* tmp=(char*)malloc(data_block_bitmap[indirect_block_idx[i]/data_block_bitmap.size()].size()*sizeof(char));
			strcpy(tmp,data_block_bitmap[indirect_block_idx[i]/data_block_bitmap.size()].c_str());
			int res=write_block(fd,blocknr,tmp);
			if(res==ERR)return ERR;*/
	return SUCCESS;


}





int stat(int inumber){
	if (fd<0 || !isMounted)
	{
		cout<<"File system not mounted."<<endl;
		return ERR;
	}
	int inode_idx=inumber;
	if (inode_bitmap[inumber/(8*BLOCKSIZE)][inumber%(8*BLOCKSIZE)]=='0')
	{
		cout<<"invalid inode number."<<endl;
		return ERR;
	}


	int blocknr=sb->inode_block_idx+(inode_idx)/128;
	int blockOffset=(inode_idx)%128;
	char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
	int res=read_block(fd,blocknr,block_data);
	if (res==ERR)
	{
		return ERR;
	}
	// cout<<block_data<<endl;
	char *token = strtok(block_data, "|");
    std::vector<string>v;
    while (token != NULL) 
    { 
        v.push_back(token);
        token = strtok(NULL, "|"); 
    }

	string inode_str=v[blockOffset];

	char* inode_char=(char*)malloc((inode_str.size()+1)*sizeof(char));
    strcpy(inode_char,inode_str.c_str());
    token = strtok(inode_char, ",");

    std::vector<int> v1;
    while (token != NULL) 
    { 
        v1.push_back(atoi(token));
        token = strtok(NULL, ","); 
    }
    free(inode_char);
    if (v1.size()<8)
    {
    	cerr<<"currupted inode"<<endl;
    	return ERR;
    }
    if (v1[0]==0)
    {
    	cout<<"invalid inode."<<endl;
    	return ERR;
    }


    int counter=0;

    cout<<"++++++++++++++ Inode data ++++++++++++" <<endl;
    cout<<"size:"<<v1[1]<< "bytes"<<endl;
    cout<<"direct block pointers : ";
    for (int i = 2; i <v1.size()-1 ; ++i)
    {
    	if (v1[i]!=0)
    	{
    		cout<<v1[i]<<",";
    		counter++;
    	}
    }
    cout<<endl;
    cout<<"indirect block pointers : ";
    if(v1[7]!=0){
   		char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
		int res=read_block(fd,v1[7],block_data);
		if (res==ERR)return ERR;
		

		token = strtok(block_data, ",");

	    std::vector<int> indirect_block_idx;
	    while (token != NULL) 
	    { 
	        indirect_block_idx.push_back(atoi(token));
	        token = strtok(NULL, ","); 
	    }
	    for (int i = 0; i < indirect_block_idx.size(); ++i)
	    {
	    	if (indirect_block_idx[i]!=0)
	    	{
	    		cout<<indirect_block_idx[i]<<",";
	    		counter++;
	    	}
	    }
	    cout<<endl;

	}
	cout<< "number of the blocks in use: "<<counter<<endl;

    return SUCCESS;
}



bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

/*void replace_ESCAPE(string &s){
	replaceAll()
}*/

int read_i(int inumber, char *data, int length, int offset){
	if (fd<0 || !isMounted)
	{
		cout<<"File system not mounted."<<endl;
		return ERR;
	}

	int inode_idx=inumber;
	if (inode_bitmap[inumber/(8*BLOCKSIZE)][inumber%(8*BLOCKSIZE)]=='0')
	{
		cout<<"invalid inode number."<<endl;
		return ERR;
	}


	int blocknr=sb->inode_block_idx+(inode_idx)/128;
	int blockOffset=(inode_idx)%128;
	char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
	int res=read_block(fd,blocknr,block_data);
	if (res==ERR)
	{
		return ERR;
	}
	// cout<<block_data<<endl;
	char *token = strtok(block_data, "|");
    std::vector<string>v;
    while (token != NULL) 
    { 
        v.push_back(token);
        token = strtok(NULL, "|"); 
    }

	string inode_str=v[blockOffset];

	char* inode_char=(char*)malloc((inode_str.size()+1)*sizeof(char));
    strcpy(inode_char,inode_str.c_str());
    token = strtok(inode_char, ",");

    std::vector<int> v1;
    while (token != NULL) 
    { 
        v1.push_back(atoi(token));
        token = strtok(NULL, ","); 
    }
    free(inode_char);
    if (v1.size()<8)
    {
    	cerr<<"currupted inode"<<endl;
    	return ERR;
    }
    if (v1[0]==0)
    {
    	cout<<"invalid inode."<<endl;
    	return ERR;
    }	

    
    int size=v1[1];
	/*inode *ind=(inode*)malloc(1*sizeof(inode));
	ind->valid=v1[0];
	ind->size=v1[1];*/
	// disk_stat*  ds=get_disk_stat(fd);
	// int* db=(int*)malloc(ds->blocks*sizeof(int)); 	
	std::vector<int> dbs;
	int count=0;
	for (int i = 2; i <v1.size()-1 ; ++i)
    {
    	if (v1[i]!=0)
    	{
    		dbs.push_back(v1[i]);
    	}
    }
   /* cout<<endl;
    cout<<"indirect block pointers : ";*/
    if(v1[7]!=0){
   		char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
		int res=read_block(fd,v1[7],block_data);
		if (res==ERR)return ERR;
		

		token = strtok(block_data, ",");

	    std::vector<int> indirect_block_idx;
	    while (token != NULL) 
	    { 
	        indirect_block_idx.push_back(atoi(token));
	        token = strtok(NULL, ","); 
	    }
	    for (int i = 0; i < indirect_block_idx.size(); ++i)
	    {
	    	if (indirect_block_idx[i]!=0)
	    	{
	    		dbs.push_back(indirect_block_idx[i]);
	    		/*cout<<indirect_block_idx[i]<<",";
	    		counter++;*/
	    	}
	    }
	}
	string file_data("");
	for (int i =0;i<dbs.size();i++)
	{
		char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
		int res=read_block(fd,dbs[i],block_data);
		if(res==ERR)return ERR;
		string s(block_data);

		file_data=file_data+s;
	}

	// cout<<file_data<<endl;
	replaceAll(file_data,ESCAPE,"\n");
	// cout<<"AFTER: "<<file_data<<endl;
	size=file_data.size();
	if (offset>=size)
    {
    	cout<<"[ERROR] offset out of range"<<endl;
    	return ERR;
    }
	string tmp;
	int retlen=length;
	if (size< offset+length)
	{
		// retlen=size-offset-1;
		tmp=file_data.substr(offset,size- offset);
		// data =(char*)malloc(length*sizeof(char));
		strcpy(data,tmp.c_str());
		// cout<<"1: "<<data<<endl;
		return size-offset;
		
	}else{
		// retlen=length;
		tmp=file_data.substr(offset,length);
		// data =(char*)malloc(length*sizeof(char));
		strcpy(data,tmp.c_str());
		// cout<<"2:"<<data<<endl;
	}
	return SUCCESS;
}

int get_empty_block_and_save_data(char* block_data){
	for (int i = 0; i < data_block_bitmap.size(); ++i)
	{
		for (int j = 0; j < data_block_bitmap[i].size(); ++j)
		{	
			if (i*8*BLOCKSIZE+j > sb->blocks-1)
			{
				cout<<"Memory Full."<<endl;
				return ERR;		
			}
			if (data_block_bitmap[i][j]=='0')
			{
				data_block_bitmap[i][j]='1';
				int bnr=i*8*BLOCKSIZE+j;
				int res=write_block(fd,bnr,block_data);
				if (res==ERR)return ERR;

				char* block_bitmap_data=(char*)malloc((data_block_bitmap[i].size()+1)*sizeof(char));
				strcpy(block_bitmap_data,data_block_bitmap[i].c_str());
				res=write_block(fd,i+sb->data_block_bitmap_idx,block_bitmap_data);
				if (res==ERR)return ERR;
				return bnr;
			}
			
		}
	}	
}

int write_i(int inumber, char *data, int length, int offset){
	if (fd<0 || !isMounted)
	{
		cout<<"File system not mounted."<<endl;
		return ERR;
	}

	int inode_idx=inumber;
	if (inode_bitmap[inumber/(8*BLOCKSIZE)][inumber%(8*BLOCKSIZE)]=='0')
	{
		cout<<"invalid inode number."<<endl;
		return ERR;
	}


	int blocknr=sb->inode_block_idx+(inode_idx)/128;
	int blockOffset=(inode_idx)%128;
	char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
	int res=read_block(fd,blocknr,block_data);
	if (res==ERR)
	{
		return ERR;
	}
	// cout<<block_data<<endl;
	char *token = strtok(block_data, "|");
    std::vector<string>v;
    while (token != NULL) 
    { 
        v.push_back(token);
        token = strtok(NULL, "|"); 
    }

	string inode_str=v[blockOffset];

	char* inode_char=(char*)malloc((inode_str.size()+1)*sizeof(char));
    strcpy(inode_char,inode_str.c_str());
    token = strtok(inode_char, ",");

    std::vector<int> v1;
    while (token != NULL) 
    { 
        v1.push_back(atoi(token));
        token = strtok(NULL, ","); 
    }
    free(inode_char);
    if (v1.size()<8)
    {
    	cerr<<"currupted inode"<<endl;
    	return ERR;
    }
    if (v1[0]==0)
    {
    	cout<<"invalid inode."<<endl;
    	return ERR;
    }	

    
    int size=v1[1];
	/*inode *ind=(inode*)malloc(1*sizeof(inode));
	ind->valid=v1[0];
	ind->size=v1[1];*/
	// disk_stat*  ds=get_disk_stat(fd);
	// int* db=(int*)malloc(ds->blocks*sizeof(int)); 	
	std::vector<int> dbs;
	int count=0;
	for (int i = 2; i <v1.size()-1 ; ++i)
    {
    	if (v1[i]!=0)
    	{
    		dbs.push_back(v1[i]);
    	}
    }
   /* cout<<endl;
    cout<<"indirect block pointers : ";*/
	std::vector<int> indirect_block_idx;
    if(v1[7]!=0){
   		char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
		int res=read_block(fd,v1[7],block_data);
		if (res==ERR)return ERR;
		

		token = strtok(block_data, ",");

	    while (token != NULL) 
	    { 
	        indirect_block_idx.push_back(atoi(token));
	        token = strtok(NULL, ","); 
	    }
	    for (int i = 0; i < indirect_block_idx.size(); ++i)
	    {
	    	if (indirect_block_idx[i]!=0)
	    	{
	    		dbs.push_back(indirect_block_idx[i]);
	    		/*cout<<indirect_block_idx[i]<<",";
	    		counter++;*/
	    	}
	    }
	}
	string file_data("");
	for (int i =0;i<dbs.size();i++)
	{
		char* block_data=(char*)malloc(MAXSIZE*sizeof(char));
		int res=read_block(fd,dbs[i],block_data);
		if(res==ERR)return ERR;
		string s(block_data);

		file_data=file_data+s;
	}

	// cout<<file_data<<endl;
	replaceAll(file_data,ESCAPE,"\n");
	// cout<<"AFTER: "<<file_data<<endl;
	size=file_data.size();
	if (offset>size)
	{
		cout<<"[ERROR] offset in greater than size of file."<<endl;
		return ERR;
	}
	string data_str(data);
	file_data.replace(offset,file_data.size()-offset,data_str.substr(0,length));
	// cout<<"file_data:"<<file_data<<endl;
	v1[1]=file_data.size();
	replaceAll(file_data,"\n",ESCAPE);
	int required_blocks=ceil((double)file_data.size()/(double)BLOCKSIZE);
	
	int init_pointer=0;
	int empty_db_idx=2;
	int indirect_empty_db_idx=0;
	while(init_pointer < file_data.size()){
		string tmp;
		if (init_pointer+BLOCKSIZE < file_data.size() )
		{
		 	tmp =file_data.substr(init_pointer,BLOCKSIZE);
			init_pointer+=BLOCKSIZE;
		}else{

			tmp=file_data.substr(init_pointer,file_data.size()- init_pointer);
			init_pointer+=BLOCKSIZE;
		}
		char *block_data=(char*)malloc((tmp.size()+1)*sizeof(char*));
		strcpy(block_data, tmp.c_str());
		// v1[1]+=tmp.size();
		if (empty_db_idx>=7) /* direct pointers are not empty*/
		{
			if (v1[7]==0)
			{
				int wr=get_empty_block_and_save_data(block_data);	
				if(wr==ERR)return ERR;
				// v1[empty_db_idx]=wr;
				string indirect_pointer_list=to_string(wr);
				char *indirect_pointer_list_char=(char*)malloc((indirect_pointer_list.size()+1)*sizeof(char*));

				wr=get_empty_block_and_save_data(indirect_pointer_list_char);	
				if(wr==ERR)return ERR;
				v1[7]=wr;
			}else{
				
				if (indirect_empty_db_idx < indirect_block_idx.size())
				{
					
					int t=indirect_block_idx[indirect_empty_db_idx++];
					res=write_block(fd,t,block_data);
					if(res==ERR)return ERR;
				}else{
					int t=get_empty_block_and_save_data(block_data);
					if(t==ERR)return ERR;
					indirect_block_idx.push_back(t);
				}
			}
		}else
			if (v1[empty_db_idx]==0)
			{
				int wr=get_empty_block_and_save_data(block_data);
				if(wr==ERR)return ERR;
				v1[empty_db_idx]=wr;	
				empty_db_idx++;
			}else{				
				res=write_block(fd, v1[empty_db_idx], block_data);
				if(res==ERR)return ERR;
				empty_db_idx++;
			}

	}
	string idn_string=to_string( v1[0])+ ","+
					to_string( v1[1])+ ","+
					to_string( v1[2])+ ","+
					to_string( v1[3])+ ","+
					to_string( v1[4])+ ","+
					to_string( v1[5])+ ","+
					to_string( v1[6])+ ","+
					to_string( v1[7]);
	if(save_inode(idn_string,inumber)==ERR)return ERR;


	if(indirect_block_idx.size()>0){
		string indirect_pointer_list=to_string(indirect_block_idx[0]);
		for (int i = 1; i < indirect_block_idx.size(); ++i)
		{
			indirect_pointer_list = indirect_pointer_list+","+to_string(indirect_block_idx[i]);
		}
		// indirect_pointer_list = indirect_pointer_list+","+to_string(res);
		char *indirect_pointer_list_char=(char*)malloc((indirect_pointer_list.size()+1)*sizeof(char*));
		strcpy(indirect_pointer_list_char,indirect_pointer_list.c_str());
		res=write_block(fd,v1[7],indirect_pointer_list_char);
		if(res==ERR)return ERR;
	}
	return SUCCESS;
}



int get_inumber_from_path(char* filepath){
	char *token = strtok(filepath, "/"); 
    vector<string> tokens;
    while (token != NULL) 
    { 
        string s(token); 
        tokens.push_back(s) ;
        token = strtok(NULL, "/"); 
    } 
    int len,inumber=root_dir;
    for (int i = 0; i < tokens.size(); ++i)
    {
    	string data;
    	do{
	    	char * file_data = (char*)malloc((MAXSIZE+1)*sizeof(char));
			if(read_i(inumber,file_data,MAXSIZE,0)==ERR){
				return ERR;
			}
			string s(file_data);
			data=data+s;

			 len=strlen(file_data);
		}while(len==MAXSIZE);


		char* fdata=(char*)malloc((data.size()+1)*sizeof(char));
		strcpy(fdata,data.c_str());
		char *fdata_token = strtok(fdata, "|"); 
	    vector<string> fdata_tokens;
	    while (fdata_token != NULL) 
	    { 
	        string s(fdata_token); 
	        fdata_tokens.push_back(s) ;
	        fdata_token = strtok(NULL, "|"); 
	    } 
	    
	    if (fdata_tokens.size()<1)return ERR;

	    for (int j = 0; j < fdata_tokens.size(); ++j)
	    {

		    char* fmetadata=(char*)malloc((fdata_tokens[j].size()+1)*sizeof(char));
		    strcpy(fmetadata,fdata_tokens[j].c_str());

		    char* bit=strtok(fmetadata,",");
		    vector<string> fmetadata_str;
		    while(bit != NULL){
		    	string b(bit);
		    	fmetadata_str.push_back(b);
		    	bit=strtok(NULL,",");
		    }
		    if (fmetadata_str.size()<5)
		    {
		    	return ERR;
		    }

		    if(fmetadata_str[0]!="1") {
		    	cout<<" invalid file"<<endl;
		    	break;
		    }
		    if (fmetadata_str[2]==tokens[i])
		    {
		    	if (i==tokens.size()-1)
		    	{
		    		// cout<<tokens[i]<<" inumber:"<<stoi(fmetadata_str[4])<<endl;
		    		// remove_base_dir(inumber,j,fdata_tokens,stoi(fmetadata_str[4]),stoi(fmetadata_str[1]));
		    		// read_i(stoi(fmetadata_str[4]),data,length,offset);
		    		return stoi(fmetadata_str[4]);
		    	} else 
		    		if (fmetadata_str[1]=="0")
		    		{
		    			cout<< "\""<<tokens[i] <<"\" is not directory." <<endl; 
		    			return ERR;
		    		}

		    	inumber=stoi(fmetadata_str[4]);
		    	break;
		    }

		    if (j==fdata_tokens.size()-1)
		    {
		    	cout<<"directory\""<<tokens[i]<<"\" does not exist."<<endl;
		    }

	    }


    }
 return SUCCESS;
}


int read_file(char *filepath, char *data, int length, int offset){
	int inumber=get_inumber_from_path(filepath);
	if (inumber<=0)return ERR;
	if( read_i(inumber,data,length,offset)==ERR)return ERR;
	return SUCCESS;
}

int write_file(char *filepath, char *data, int length, int offset){
	create_file_or_dir=FILE;
	int inumber=create_dir(filepath);
	create_file_or_dir=DIR;
	if (inumber<=0)return ERR;
	if( write_i(inumber,data,length,offset)==ERR)return ERR;
	return SUCCESS;
}

/*int ls(char *filepath){
	bool isPathFound=false;
	char *token = strtok(filepath, "/"); 
    vector<string> tokens;
    while (token != NULL) 
    { 
        string s(token); 
        tokens.push_back(s) ;
        token = strtok(NULL, "/"); 
    } 
    int len,inumber=root_dir;
    for (int i = 0; i < tokens.size(); ++i)
    {
    	string data;
		int offset=0;
    	do{
	    	char * file_data = (char*)malloc((MAXSIZE+1)*sizeof(char));
			if(read_i(inumber,file_data,MAXSIZE,offset)==ERR){
				return ERR;
			}
			offset=offset+strlen(file_data);
			string s(file_data);
			data=data+s;

			 len=strlen(file_data);
		}while(len==MAXSIZE);

		cout<<data<<endl;

		char* fdata=(char*)malloc((data.size()+1)*sizeof(char));
		strcpy(fdata,data.c_str());
		char *fdata_token = strtok(fdata, "|"); 
	    vector<string> fdata_tokens;
	    while (fdata_token != NULL) 
	    { 
	        string s(fdata_token); 
	        fdata_tokens.push_back(s) ;
	        fdata_token = strtok(NULL, "|"); 
	    } 
	    
	    if (fdata_tokens.size()<1)return ERR;

	    for (int j = 0; j < fdata_tokens.size(); ++j)
	    {

		    char* fmetadata=(char*)malloc((fdata_tokens[j].size()+1)*sizeof(char));
		    strcpy(fmetadata,fdata_tokens[j].c_str());

		    char* bit=strtok(fmetadata,",");
		    vector<string> fmetadata_str;
		    while(bit != NULL){
		    	string b(bit);
		    	fmetadata_str.push_back(b);
		    	bit=strtok(NULL,",");
		    }
		    if (fmetadata_str.size()<5)
		    {
		    	return ERR;
		    }

		    if(fmetadata_str[0]!="1") {
		    	cout<<" invalid file"<<endl;
		    	break;
		    }

		    if (fmetadata_str[2]==tokens[i])
		    {
		    	if (i==tokens.size()-1)
		    	{
		    		isPathFound=true;	
		    		
		    	} else 
		    		if (fmetadata_str[1]=="0")
		    		{
		    			cout<< "\""<<tokens[i] <<"\" is not directory." <<endl; 
		    			return ERR;
		    		}

		    	inumber=stoi(fmetadata_str[4]);
		    	break;
		    }

		   

	    }


    }
    return SUCCESS;
}
*/
int create_base_file(string parent_mdate,string fdata, int inumber,char* fname,int type){
	string dirname(fname);
	// strcpy(dirname,fname);
	// cout<<"parent_mdate: "<<parent_mdate<<endl;
	int parent_dir=root_dir;
	
	int ind=create_file();
	string dir=to_string(1)+ ","+ //valid bit
			to_string(type)+","+  //type file or directory
			dirname+","+ //dir name 
			to_string(dirname.size())+","+ // length of dir name 
			to_string(ind)+"|"; // inumber of file or directory
	
	fdata=fdata+dir;
	char * file_data = (char*)malloc((fdata.size()+1)*sizeof(char));
	strcpy(file_data,fdata.c_str());
	write_i(inumber,file_data,strlen(file_data),0);
	
	dir=dir+parent_mdate+"|";
	char * block_data = (char*)malloc((dir.size()+1)*sizeof(char));
	strcpy(block_data,dir.c_str());
	write_i(ind,block_data,strlen(block_data),0);
	return ind;
}

int create_dir(char *filepath){
	char *token = strtok(filepath, "/"); 
    vector<string> tokens;
    while (token != NULL) 
    { 
        string s(token); 
        tokens.push_back(s) ;
        token = strtok(NULL, "/"); 
    } 
    int len,inumber=root_dir;
    for (int i = 0; i < tokens.size(); ++i)
    {
    	string data;
		int offset=0;
    	do{
	    	char * file_data = (char*)malloc((MAXSIZE+1)*sizeof(char));
			if(read_i(inumber,file_data,MAXSIZE,offset)==ERR){
				return ERR;
			}
			offset=offset+strlen(file_data);
			string s(file_data);
			data=data+s;

			 len=strlen(file_data);
		}while(len==MAXSIZE);

		// cout<<data<<endl;

		char* fdata=(char*)malloc((data.size()+1)*sizeof(char));
		strcpy(fdata,data.c_str());
		char *fdata_token = strtok(fdata, "|"); 
	    vector<string> fdata_tokens;
	    while (fdata_token != NULL) 
	    { 
	        string s(fdata_token); 
	        fdata_tokens.push_back(s) ;
	        fdata_token = strtok(NULL, "|"); 
	    } 
	    
	    if (fdata_tokens.size()<1)return ERR;

	    for (int j = 0; j < fdata_tokens.size(); ++j)
	    {

		    char* fmetadata=(char*)malloc((fdata_tokens[j].size()+1)*sizeof(char));
		    strcpy(fmetadata,fdata_tokens[j].c_str());

		    char* bit=strtok(fmetadata,",");
		    vector<string> fmetadata_str;
		    while(bit != NULL){
		    	string b(bit);
		    	fmetadata_str.push_back(b);
		    	bit=strtok(NULL,",");
		    }
		    if (fmetadata_str.size()<5)
		    {
		    	return ERR;
		    }

		    if(fmetadata_str[0]!="1") {
		    	cout<<" invalid file"<<endl;
		    	break;
		    }
		    if (fmetadata_str[2]==tokens[i])
		    {
		    	if (i==tokens.size()-1)
		    	{
		    		if (create_file_or_dir==FILE)
		    		{
		    			return stoi(fmetadata_str[4]);
		    		}
		    		cout<<"directory/file already exist."<<endl;
		    		return ERR;
		    	} else 
		    		if (fmetadata_str[1]=="0")
		    		{
		    			cout<< "\""<<tokens[i] <<"\" is not directory." <<endl; 
		    			return ERR;
		    		}

		    	inumber=stoi(fmetadata_str[4]);
		    	break;
		    }

		    if (j==fdata_tokens.size()-1)
		    {
		    	char* fname=(char*)malloc((tokens[i].size()+1)*sizeof(char));
		    	strcpy(fname,tokens[i].c_str());
		    	// cout<<data<<endl;
		    	if((i==tokens.size()-1) && create_file_or_dir==FILE )
		    	{
		    		inumber=create_base_file(fdata_tokens[0],data,inumber,fname,FILE);
		    		return inumber;
		    	}else
		    		inumber=create_base_file(fdata_tokens[0],data,inumber,fname,DIR);


		    }

	    }


    }
    return SUCCESS;

}

int remove_base_dir(int parent_inumber,int pos,vector<string>parent_dir ,int inumber,int type){
	    // cout<<"remove1:"<<inumber<<endl;

	if (type==FILE)
	{
		remove_file(inumber);
		// parent_dir.erase(parent_dir.begin()+pos);
		string parent_str;
		for (int i = 0; i < parent_dir.size(); ++i)
		 {
		 	if (i!=pos)
		 	{
			 	parent_str=parent_str+parent_dir[i]+"|";
		 	}
		 } 
		 char *parent_char=(char *)malloc((parent_str.size()+1)*sizeof(char));
		 strcpy(parent_char,parent_str.c_str());
		 write_i(parent_inumber,parent_char,strlen(parent_char),0);
		return SUCCESS;
	}
	string data;
	int len=MAXSIZE;
	do{
		char * file_data = (char*)malloc((MAXSIZE+1)*sizeof(char));
		if(read_i(inumber,file_data,MAXSIZE,0)==ERR){
			return ERR;
		}
		string s(file_data);
		data=data+s;

		 len=strlen(file_data);
	}while(len==MAXSIZE);
	char* fdata=(char*)malloc((data.size()+1)*sizeof(char));
	strcpy(fdata,data.c_str());
	char *fdata_token = strtok(fdata, "|"); 
    vector<string> fdata_tokens;
    while (fdata_token != NULL) 
    { 
        string s(fdata_token); 
        fdata_tokens.push_back(s) ;
        fdata_token = strtok(NULL, "|"); 
    } 
    
    if (fdata_tokens.size()<1)return ERR;
    if (fdata_tokens.size()==2){
    	// parent_dir.erase(parent_dir.begin()+pos);
		string parent_str;
		for (int i = 0; i < parent_dir.size(); ++i)
		 {
		 	if (i!=pos)
		 	{
			 	parent_str=parent_str+parent_dir[i]+"|";
		 	}
		 } 
		cout<<"parent_str:"<<parent_str<<endl;
		char *parent_char=(char *)malloc((parent_str.size()+1)*sizeof(char));
		strcpy(parent_char,parent_str.c_str());
		write_i(parent_inumber,parent_char,strlen(parent_char),0);
    	remove_file(inumber);
    	return SUCCESS;
    }
    // cout<<fdata_tokens.size()<<endl;
    for (int j = 2; j < fdata_tokens.size(); ++j)
    {

	    char* fmetadata=(char*)malloc((fdata_tokens[j].size()+1)*sizeof(char));
	    strcpy(fmetadata,fdata_tokens[j].c_str());

	    char* bit=strtok(fmetadata,",");
	    vector<string> fmetadata_str;
	    while(bit != NULL){
	    	string b(bit);
	    	fmetadata_str.push_back(b);
	    	bit=strtok(NULL,",");
	    }
	    if (fmetadata_str.size()<5)
	    {
	    	return ERR;
	    }

	    if(fmetadata_str[0]!="1") {
	    	cout<<" invalid file"<<endl;
	    	break;
	    }
	    remove_base_dir(inumber,j,fdata_tokens,stoi(fmetadata_str[4]),stoi(fmetadata_str[1]));

    }
	remove_file(inumber);
    // parent_dir.erase(parent_dir.begin()+pos);
	string parent_str;
	for (int i = 0; i < parent_dir.size(); ++i)
	 {
	 	if (i!=pos)
	 	{
		 	parent_str=parent_str+parent_dir[i]+"|";
	 	}
	 } 
	char *parent_char=(char *)malloc((parent_str.size()+1)*sizeof(char));
	strcpy(parent_char,parent_str.c_str());
	write_i(parent_inumber,parent_char,strlen(parent_char),0);
	cout<<"directory removed"<<endl;
    return SUCCESS;
}

 

int remove_dir(char *filepath){
	char *token = strtok(filepath, "/"); 
    vector<string> tokens;
    while (token != NULL) 
    { 
        string s(token); 
        tokens.push_back(s) ;
        token = strtok(NULL, "/"); 
    } 
    int len,inumber=root_dir;
    for (int i = 0; i < tokens.size(); ++i)
    {
    	string data;
    	do{
	    	char * file_data = (char*)malloc((MAXSIZE+1)*sizeof(char));
			if(read_i(inumber,file_data,MAXSIZE,0)==ERR){
				return ERR;
			}
			string s(file_data);
			data=data+s;

			 len=strlen(file_data);
		}while(len==MAXSIZE);


		char* fdata=(char*)malloc((data.size()+1)*sizeof(char));
		strcpy(fdata,data.c_str());
		char *fdata_token = strtok(fdata, "|"); 
	    vector<string> fdata_tokens;
	    while (fdata_token != NULL) 
	    { 
	        string s(fdata_token); 
	        fdata_tokens.push_back(s) ;
	        fdata_token = strtok(NULL, "|"); 
	    } 
	    
	    if (fdata_tokens.size()<1)return ERR;

	    for (int j = 0; j < fdata_tokens.size(); ++j)
	    {

		    char* fmetadata=(char*)malloc((fdata_tokens[j].size()+1)*sizeof(char));
		    strcpy(fmetadata,fdata_tokens[j].c_str());

		    char* bit=strtok(fmetadata,",");
		    vector<string> fmetadata_str;
		    while(bit != NULL){
		    	string b(bit);
		    	fmetadata_str.push_back(b);
		    	bit=strtok(NULL,",");
		    }
		    if (fmetadata_str.size()<5)
		    {
		    	return ERR;
		    }

		    if(fmetadata_str[0]!="1") {
		    	cout<<" invalid file"<<endl;
		    	break;
		    }
		    if (fmetadata_str[2]==tokens[i])
		    {
		    	if (i==tokens.size()-1)
		    	{
		    		// cout<<tokens[i]<<" inumber:"<<stoi(fmetadata_str[4])<<endl;
		    		remove_base_dir(inumber,j,fdata_tokens,stoi(fmetadata_str[4]),stoi(fmetadata_str[1]));
		    		return SUCCESS;
		    	} else 
		    		if (fmetadata_str[1]=="0")
		    		{
		    			cout<< "\""<<tokens[i] <<"\" is not directory." <<endl; 
		    			return ERR;
		    		}

		    	inumber=stoi(fmetadata_str[4]);
		    	break;
		    }

		    if (j==fdata_tokens.size()-1)
		    {
		    	cout<<"directory\""<<tokens[i]<<"\" does not exist."<<endl;
		    }

	    }


    }
 return SUCCESS;
}