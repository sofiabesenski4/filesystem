#include "file.h"
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>





	
int main(int argc,char* argv[] )
{	
	
	printf("Testing app 2: Running tests using the file system: stage %d\n",argc);
	void* buffer;
	if (argc ==1)
	{
		FILE* fp =  fopen("../vdisk", "rb+");
		//printf("opened the file system. now attempting to download the small test file\n");
		download_file(fp, "/testdir1/smalltestfile","downloadedsmalltestfile");
		
	
	}
	else if (argc==2)
	{
		FILE* fp =  fopen("../vdisk", "rb+");
		//printf("opened the file system. now downloading the large test file\n");
		download_file(fp, "/testdir1/largetestfile","downloadedlargetestfile");
		}
	
	else if (argc==3)
	{
		FILE* fp = fopen("../vdisk","rb+");
		printf("removing the small test file\n");
		delete_filepath(fp, "/testdir1/smalltestfile");
		
		
	}
	else if (argc==4)
	{
		FILE* fp=fopen("../vdisk","rb+");
		printf("removing the large test file\n");
		delete_filepath(fp, "/testdir1/largetestfile");
		
		
		
	}
	
	else if (argc==5)
	{
		FILE* fp=fopen("../vdisk","rb+");
		printf("removing the directory /testdir1/ \n");
		delete_filepath(fp, "/testdir1");
		
		
		
	}
	//FILE* fpin = fopen("testin","rb");
	
	//create_directory(fp,"/","firstlevel");
	//upload_file(fp, "/firstlevel/","file1.txt",fpin);
	//delete_filepath(fp,"/firstlevel/file1.txt");
	//delete_filepath(fp,"/firstlevel/");
	
	return 0;
	
}


	
