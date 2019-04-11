//file.c
/* 
 * Disk parameters:
	Size of a block: 512 bytes * 8 bits per byte = 4096 bits in a block
	Number of blocks on disk: 4096
	Name of file simulating disk: “vdisk” in current directory
	Blocks are numbered from 0 to 4095
 * 
 * 
 * 
 * Block 0 – superblock
· Contains information about the filesystem implementation, such as
· first 4 bytes: magic number
· next 4 bytes: number of blocks on disk
· next 4 bytes: number of inodes for disk
Block 1 – free block vector
· With 512 bytes in this block, and 8 bits per byte, our free-block vector may hold 4096 bits.
· First ten blocks (0 through 9) are not available for data.
· To indicate an available block, bit must be set to 1.
Other Blocks
· You can reserve other blocks for other persistent data structures in LLFS
· One thing to consider is how you are going to keep track of there all the inodes in the
filesystem.
· Each inode has a unique id number.
· Each inode is 32 bytes long.
· An inode has to be allocated to represent information for the root directory.
 * 
 * 
 * 
 * Inode format:
· Each inode is 32 bytes long
· First 4 bytes: size of file in bytes; an integer
· Next 4 bytes: flags – i.e., type of file (flat or directory); an integer
· Next 2 bytes, multiplied by 10: block numbers for file’s first ten blocks
· Next 2 bytes: single-indirect block
· Last 2 bytes: double-indirect block
* 
Directory format:
· Each directory block contains 16 entries.
· Each entry is 32 bytes long
· First byte indicates the inode (value of 0 means no entry)
· Next 31 bytes are for the filename, terminated with a “null” character.
 * */
#include "file.h"
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
const size_t BYTES_PER_BLOCK=512;
const size_t BITS_PER_BLOCK=4096;
const size_t MAX_BLOCK_INDEX=4095;
const size_t FREE_BLOCK_VECTOR_OFFSET=1;
const size_t DATA_SECTION_OFFSET = 16;
const size_t INODE_BYTES=33;
const size_t INODE_SIZE_OFFSET=0;
const size_t INODE_TYPE_OFFSET=4;
const size_t INODE_DIRECT_OFFSET=8;
const size_t INODE_SINGLEIND_OFFSET=28;
const size_t INODE_DOUBLEIND_OFFSET=30;
const size_t INODE_ID_OFFSET=32;
const size_t INODE_MAX_NUM=256;
const size_t INODE_ID_SIZE = 2;
const size_t INODE_MAP_OFFSET = 2;


const size_t DIRECTORY_BYTES = 512;
const size_t DIRECTORY_ELEMENT_SIZE=32;
const size_t DIRECTORY_INODE_OFFSET = 0;
const size_t DIRECTORY_ENTRY_OFFSET=1;



//////////////PROTOTYPING

void write_block(FILE* fp, int block_num, void* data,int size_of_data_in_bytes);
void read_block(FILE* fp, int block_num, char* buffer);
void read_block_value(FILE*  fp, int block_num, char* buffer, int byte_offset, size_t length_of_value);


unsigned short get_inode_address(FILE* fp, char directory_inode_id);
unsigned short check_fbv_for_available_block(FILE* fp);
void set_fbv_bit(FILE* fp, unsigned short block_number);
void reset_fbv_bit(FILE* fp, unsigned int block_number);

void* create_inode(FILE* fp, int inode_number, int size, int type,int id);
unsigned char find_next_free_inode_id(FILE* fp);

unsigned short add_element_to_directory(FILE* fp, unsigned char directory_inode_id, unsigned char element_inode_id, char* element_file_name);
unsigned short create_directory_block(FILE* fp, unsigned char parent_inode_id, unsigned char inode_id);
unsigned short create_directory_from_inode(FILE* fp, unsigned char parent_inode_id, char* new_directory_name);
unsigned char create_file_in_directory(FILE* fp, unsigned char parent_inode_id,char* file_name, FILE* fpin);

void assign_location_to_inode_map(FILE* fp, unsigned short inode_address, unsigned char inode_id);
void init_vdisk(FILE* fp);
FILE* download_file(FILE* fp, char* target_filename, char* new_filename);
void delete_file(FILE* fp, unsigned char filename);
void delete_inode(FILE* fp, unsigned char inode_id);
void clear_single_indirection_block(FILE* fp, unsigned short indirection_block_num);

unsigned char find_file_inode_id(FILE* fp, char* absolute_file_path);
void delete_filepath(FILE* fp, char* filename);
void delete_directory(FILE* fp, unsigned char directory_inode_id);
void delete_file(FILE* fp, unsigned char file_inode_id);
//////////////BASIC VDISK OPERATIONS

void write_block(FILE* fp, int block_num, void* data,int size_of_data_in_bytes){
	
	off_t total_offset = fseek(fp, (off_t)block_num*BYTES_PER_BLOCK, SEEK_SET);
	
	
	fwrite(data, 1,size_of_data_in_bytes,fp);
	//printf("%ld\n",ftell(fp));
	
	return;

}

void read_block(FILE* fp, int block_num, char* buffer){
	off_t total_offset = fseek(fp, (off_t)block_num*BYTES_PER_BLOCK, SEEK_SET);
	fread(buffer,  BYTES_PER_BLOCK, 1,fp);
	return;
	
}

//////////////////////////// BLOCK DATA MANIPULATION

void read_block_value(FILE*  fp, int block_num, char* buffer, int byte_offset, size_t length_of_value)
{
	char* block_buffer = (char*)malloc(BYTES_PER_BLOCK);
	read_block(fp, block_num, block_buffer);
//	printf("read_block :%s\n", (char*)block_buffer);
	//not check within that buffer to get the values we wanted
	int index;
	for(index=0; index<length_of_value;index++)
	{
		buffer[index] = block_buffer[index+byte_offset];
		
	}
	
	free(block_buffer);
	return;
}
unsigned short get_inode_address(FILE* fp, char directory_inode_id){

	unsigned short address;
	read_block_value(fp, 2,(char*)&address,directory_inode_id*2,2);
	return address;
}
////////////////////////////PRIVATE FILE SYSTEM FUNCTIONS
//note type=1 when the inode is a directory file, 2 when anything other type of file
//block_type_needed can be either 'i' for index or 'd' for data. This will determine where in the free block vector to begin searching
unsigned short check_fbv_for_available_block(FILE* fp)
{
	
		
	char* free_block_vector = (char*) malloc(BITS_PER_BLOCK);
	read_block(fp,FREE_BLOCK_VECTOR_OFFSET,free_block_vector);
	unsigned int tester = 1;
	unsigned short i;
	unsigned short byte_pos= 2;
	for(byte_pos; byte_pos<DATA_SECTION_OFFSET+MAX_BLOCK_INDEX; byte_pos++)
	{
		for( i =0; i< 8;i++)
		{
			//printf("%u\n",tester);
			
			if (tester & free_block_vector[byte_pos])
			{
//	printf("check_fbv_for_available_block: found an available block to write in at bit %u in byte %u \n",i, byte_pos);
				free(free_block_vector);
				return(byte_pos*8 + i);
			
			}
			tester=tester<<1;
		}
		tester = 1;
	}

	
	printf("no blocks are free!\n");
	return 0;
}

void set_fbv_bit(FILE* fp, unsigned short block_number)
{
	char* vector = malloc(BITS_PER_BLOCK); 
	read_block(fp,FREE_BLOCK_VECTOR_OFFSET,vector);
	int byte_num = block_number / 8;
	int bit_pos = block_number%8;
	char target = 1;
	int i;
	for (i=0;i<bit_pos;i++)
	{
		target = target<<1;
		
	}

	vector[byte_num] = vector[byte_num]|target;
	write_block(fp,FREE_BLOCK_VECTOR_OFFSET,vector,BYTES_PER_BLOCK);
	return;
	
}

//void  reset_fbv_bit
void reset_fbv_bit(FILE* fp, unsigned int block_number)
{
	char* vector = malloc(BYTES_PER_BLOCK); 
	read_block(fp,FREE_BLOCK_VECTOR_OFFSET,vector);
	int byte_num = block_number / 8;
	int bit_pos = block_number%8;
	char target = 1;
	int i;
	for (i=0;i<bit_pos;i++)
	{
		target = target<<1;
		
	}
	vector[byte_num] = vector[byte_num]^target;
//	printf("reset_fbv_bit: reset bit in position %d, in block number %d\n", (int)i,(int)block_number);
	write_block(fp,FREE_BLOCK_VECTOR_OFFSET,vector,BYTES_PER_BLOCK);
	free(vector);
	return;
	
}
unsigned char find_next_free_inode_id(FILE* fp){
	
	unsigned short* inode_map = (unsigned short*) malloc(BYTES_PER_BLOCK);
	read_block(fp, INODE_MAP_OFFSET,(char*)inode_map);
	int i ;
	for ( i=0; i< INODE_MAX_NUM; i++)
	{// checking through the inode map block to determine which has a free address we can use
		if (inode_map[i]==0)
		{
//			printf("find_next_free_inode_id: found an empty inode space in inode id %d\n",(unsigned short)i);
			free(inode_map);
			return (unsigned char)i;
			
		}
		
	}
	fprintf(stderr,"no available inodes left");
	exit(1);
	return -1;
}


// TWO FILE TYPES: "f" and "d" for file and directory file, respectively
unsigned short create_empty_inode(FILE* fp, int inode_number, int size, int type)
{
	
	void* inode_block = malloc(INODE_BYTES);
	memset(inode_block,0,INODE_BYTES);
	((unsigned int*)inode_block)[0] = (unsigned int)size;
	((unsigned int*)inode_block)[1] = (unsigned int)type;
	((char*)inode_block)[32] = inode_number;
//	printf("Create_empty_inode: looking for empty block for inode id %d\n",(int)inode_number);
	unsigned short available_block = check_fbv_for_available_block(fp);
	write_block(fp, available_block, inode_block,INODE_BYTES);
//	printf("Create_empty_inode: writing  inode block to  location  %d\n", (short)available_block);
	
	reset_fbv_bit(fp,available_block);
	free(inode_block);
	//returns the absolute block address where the empty inode was created
	return available_block;
}
unsigned short create_and_write_data_block_from_file(FILE* fp, size_t number_of_bytes,FILE* infile)
{
	
	void* buffer = malloc(BYTES_PER_BLOCK);
	memset(buffer,0,BYTES_PER_BLOCK);
	//find a free block
	unsigned short available_block =  check_fbv_for_available_block(fp);
	//read block worth of data to a buffer
	
	fread(buffer,1,number_of_bytes,infile);
	//write buffer out to block
	
	write_block(fp, available_block,buffer,number_of_bytes);
	reset_fbv_bit(fp, available_block);
	//updte the fbv to fill that block
	free(buffer);	
	return available_block;
	
	}
	
unsigned short create_indirection_block(FILE* fp, unsigned char parent_inode_id)
{
	unsigned char* block_buffer = (unsigned char*)malloc(BYTES_PER_BLOCK);
	memset(block_buffer,0,BYTES_PER_BLOCK);
	unsigned short available_block_address = check_fbv_for_available_block(fp);
	write_block(fp, available_block_address, block_buffer,BYTES_PER_BLOCK);
	reset_fbv_bit(fp, available_block_address);
	free(block_buffer);
	return available_block_address;
	

}




//returns the block addre
unsigned short fill_single_indirection_block(FILE* fp,unsigned short single_indirection_block_num, unsigned short* num_blocks_remaining_to_write, long int size,unsigned short temp_data_block_address, FILE* fpin)
{
					
//	printf("fill_single_indirection_block: block num %d, blocks remaining %d, \n",single_indirection_block_num,*num_blocks_remaining_to_write);
	unsigned short* single_indirection_block_buffer = (unsigned short*) malloc(BYTES_PER_BLOCK);
	read_block(fp,single_indirection_block_num,(char*)single_indirection_block_buffer);
	
	
	int k;
	for (k=0;k<BYTES_PER_BLOCK/2;k++)
	{
		//write another file block and allocate it to the next position in the single indirection block
		if (*num_blocks_remaining_to_write ==1 && size%BYTES_PER_BLOCK)
		{
//			printf("create_file_in_directory: one block left to write\n");
			temp_data_block_address = create_and_write_data_block_from_file(fp, size%BYTES_PER_BLOCK,fpin);
		}
		
		
		else
		{
//			printf("create_file_in_directory: there are %d blocks left to write\n",*num_blocks_remaining_to_write);
			temp_data_block_address = create_and_write_data_block_from_file(fp,BYTES_PER_BLOCK,fpin);
			
		}
		
		single_indirection_block_buffer[k]=temp_data_block_address;
//		printf("create_file_in_directory: writing in the %d position of the single indirect pointer position, writingthe address %d\n", k,temp_data_block_address);
		(*num_blocks_remaining_to_write)--;
//		printf("fill_single_indirection_block: num blocks remaining	 to write %d\n",*num_blocks_remaining_to_write);
		if (*num_blocks_remaining_to_write == 0)
		{
//			printf("create_file_in_directory: assigning the single indirect block to the inode, and writing it out \n");
			
			write_block(fp,single_indirection_block_num,single_indirection_block_buffer,BYTES_PER_BLOCK);
			free(single_indirection_block_buffer);
			return single_indirection_block_num;
			//there are no more blocks to write out and we can finish up the function
		}		
		
	}	
	
	
}

void delete_directory_entry(FILE* fp, unsigned char directory_inode_id, char* removal_filename)
{
	unsigned short directory_inode_address = get_inode_address(fp,directory_inode_id);
	unsigned short* directory_inode_block = malloc(BYTES_PER_BLOCK);
	read_block(fp,directory_inode_address,(char*)directory_inode_block);
	
	unsigned short directory_data_block_address =directory_inode_block[4];
	char* directory_data_block_buffer = (char*)malloc(BYTES_PER_BLOCK);
	read_block(fp,directory_data_block_address,directory_data_block_buffer);
	
	int i;
	for (i=2;i<16;i++)
	{
//		printf("Looking at directory entry number %d, filename: %s",i,&(directory_data_block_buffer[1+i*32]));
		if (!strncmp(&(directory_data_block_buffer[1+i*32]),removal_filename,31))
		{
			
			//found the correct file to remove from the directory block
			memset(directory_data_block_buffer+i*32,0,32);
			write_block(fp, directory_data_block_address,directory_data_block_buffer,(i+1)*32);
		}
	}
	free(directory_inode_block);
	free(directory_data_block_buffer); 
}

void delete_filepath(FILE* fp, char* filename)
{	
	/**PSEUDO
	 * use filepath to find inode id for the file specified
	 * look at inode block & check if it is file or directory
	 * if file: delete_file
	 * if dir: delete_directory 
	 * remove entry from directory filelist where it is found
	 * 
	 * 
	 */
	
	
	
	unsigned char file_inode_id = find_file_inode_id(fp, filename);
	unsigned short file_block_address = get_inode_address(fp, file_inode_id);
	char* file_inode_block = (char*)malloc(BYTES_PER_BLOCK);
//	printf("deleet_filepath: file_inode_id=%d, file_block_address=%d\n",(int)file_inode_id,file_block_address);
	
	//check filetype
	read_block(fp,file_block_address,file_inode_block);
	int file_type = ((int*)file_inode_block)[1];
//	printf("filetype=%c before tokenizing stuff\n",(char)file_type);
	
   
   
	char* delim = "/";
   char *filename_token;
   char *current_parent_filename = (char*)malloc(150);
   memset(current_parent_filename,0,150);
   current_parent_filename[0]='/';
   /* get the first token */
   char* working_filename = (char*)malloc(150);
   memset(working_filename,0,150);
   memcpy(working_filename,filename,strlen(filename));
   filename_token = strtok(working_filename, delim);
   
   char* temp;
   /* walk through other tokens */
   while( filename_token != NULL ) {
//      printf( "token %s\n", filename_token );
	temp = filename_token;
      filename_token = strtok(NULL, delim);
	if (filename_token) {
		strcat(current_parent_filename,temp);
		strcat(current_parent_filename,"/");
		}
   }
 //  if (current_parent_filename)
//	printf("parent filename: %s\n",current_parent_filename);
	unsigned char parent_inode_id;
	
	if (!strcmp(current_parent_filename,"/")) parent_inode_id=0;
	
	else parent_inode_id = find_file_inode_id(fp, current_parent_filename);
	
   delete_directory_entry(fp,parent_inode_id,temp);
   
	
	//now time to delete the entry from this directory
//	printf("temp= %s\n parent inode id = %d\n",temp, (int)parent_inode_id);
	
	
	
//	printf("filetype: %c\n",(char)file_type);
	
	if ((char)file_type=='d')
	{
		
		delete_directory(fp, file_inode_id);
		
		}
	else if((char)file_type=='f')
	{
		delete_file(fp,file_inode_id);
		}
	else
	{
		
		printf("inode corrupted! incorrect inode filetype specifier\n");}
	//now deleting the filename from the directory it is a part of 
	//find the parent directory id
	
	
	return;
}
void delete_directory(FILE* fp, unsigned char directory_inode_id)
{
	/*PSEUDO
	 * check if directory is empty ie: all of the directory entries from 2-15 are empty
	 * if not: print error message and returfn
	 * else:
	 * clear the directory's data block in inode's direct pointer position "0"
	 * set the directory's data block fbv bit
	 * set this inodes fbv bit to 1 meaning the block is free for use
	 * set the inode map address for this inode's id to 00, meaning it is free for use
	 * clear this inode's data
	 * return
	 * */
	 
	unsigned char* directory_inode_buffer=malloc(BYTES_PER_BLOCK);
	memset(directory_inode_buffer,0,BYTES_PER_BLOCK);
	unsigned short directory_inode_block_address = get_inode_address(fp,directory_inode_id);
	read_block(fp, get_inode_address(fp,directory_inode_id),(char*)directory_inode_buffer);
	//checking emptiness
	unsigned short directory_data_block_address = ((unsigned short*)directory_inode_buffer)[4];
//	printf("directory data block adress = %d\n",directory_data_block_address);
	set_fbv_bit(fp,directory_data_block_address);
	set_fbv_bit(fp,directory_inode_block_address);
	unsigned char* directory_data_block_buffer = malloc(BYTES_PER_BLOCK);
	memset(directory_data_block_buffer,0,BYTES_PER_BLOCK);
	
	read_block(fp, directory_data_block_address,(char*)directory_data_block_buffer);
	int i;
//	printf("looking at directory in block address %d\n",directory_data_block_address);
	for(i=2;i<16;i++)
	{//	printf("slot %d: inode id in slot %d\n",i,(int)directory_data_block_buffer[i*32]);
		if (directory_data_block_buffer[i*32])
		{
	//		printf("delete directory: directory of inode id %d not empty, therefore cannot delete directory\n",directory_inode_id);
			return;
			}
		
	}
	//made it this far, then the directory is empty and we can clear it
	unsigned short* inode_map=malloc(BYTES_PER_BLOCK);
	memset(inode_map,0,BYTES_PER_BLOCK);
	read_block(fp,2,(char*)inode_map);
	memset((char*)inode_map+directory_inode_id,0,2);
	write_block(fp,2,inode_map,(directory_inode_id+1)*2);
	free(inode_map);
	
	memset(directory_data_block_buffer,0,BYTES_PER_BLOCK);
	write_block(fp, directory_inode_block_address,directory_data_block_buffer,BYTES_PER_BLOCK);
//	printf("trying to overwrite in data block address %d",(int)directory_data_block_address);
	write_block(fp, directory_data_block_address, directory_data_block_buffer,100);
	
	
	return;
}
void delete_file(FILE* fp, unsigned char file_inode_id)
{
	/*PSEUDO
	 *for each direct pointer:
	 * 	clear the block in the address of the pointer
	 * 	set the fbv bit for the block
	 *if single_ind pointer != 00
	 * 	clear single_ind_block in the address stored
	 *  set the fbv bit for that address to be free/1\clear data in the single ind block
	 *if double_ind_pointer !==0:
	 * load the double indirection block	
	 * for each pointer!=00:
	 * 		clear single ind block for each pointer
	 * 		set the fbv bit for that single ind block
	 * clear the dbl ind block
	 * set the fbv bit for double ind block =0
	 *
	 * 
	 *
	 *set inode fbv bit to 1/free
	 *set the inode_map[id] = 00
	 *clear the file's inode block
	 */
	 char* empty_block_buffer = malloc(BYTES_PER_BLOCK);
	 memset(empty_block_buffer,0,BYTES_PER_BLOCK);
	 unsigned short file_inode_block_address = get_inode_address(fp,file_inode_id);
//	 printf("file inode block adddress = %d\n",file_inode_block_address);
	 unsigned short* file_inode_buffer = (unsigned short*)malloc(BYTES_PER_BLOCK);
	 read_block(fp,file_inode_block_address,(char*)file_inode_buffer);
	 //now we need to start clearing the blocks in the direct pointers
	 int i;
	 for(i=4;i<14;i++)
	 {//each one of these is a direct pointer to potentiall an occupied space in memory
//		printf("checking the %d direct pointer spot in the inode id = %d\n",i-4,file_inode_id);
		
		if (!file_inode_buffer[i])
		 {//no remaining blocks to wipe
//			printf("no remainging to wipe\n");
			 break;	 
		}
		 write_block(fp,file_inode_buffer[i],empty_block_buffer,BYTES_PER_BLOCK);
		 set_fbv_bit(fp,file_inode_buffer[i]);
		 
		 
	  }
	
	
	if (file_inode_buffer[14])
	{//then there is a single indirection block we need to clear!
		clear_single_indirection_block(fp,file_inode_buffer[14]);
		set_fbv_bit(fp,file_inode_buffer[14]);
		write_block(fp,file_inode_buffer[14],(char*)empty_block_buffer,BYTES_PER_BLOCK);
		
		
	}
	
	
//	printf("now setting the inode_map[%d] to be 0",file_inode_id);
	unsigned short* inode_map=malloc(BYTES_PER_BLOCK);
	memset(inode_map,0,BYTES_PER_BLOCK);
	read_block(fp,2,(char*)inode_map);
	//memset(((char*)inode_map)+file_inode_id,0,2);
	inode_map[file_inode_id]=0;
	
	write_block(fp,2,inode_map,BYTES_PER_BLOCK);
	free(inode_map);
	
	write_block(fp,file_inode_block_address,empty_block_buffer,BYTES_PER_BLOCK);
	free(empty_block_buffer);
	set_fbv_bit(fp, file_inode_block_address);
	return;
	
}
void clear_single_indirection_block(FILE* fp, unsigned short indirection_block_address)
{	
	unsigned char* empty_block_buffer = malloc(BYTES_PER_BLOCK);
	memset(empty_block_buffer,0,BYTES_PER_BLOCK);
//	printf("clearing indirection block\n");
	unsigned short* indirection_block_buffer = malloc(BYTES_PER_BLOCK);
	unsigned short i;
	read_block(fp,indirection_block_address,(char*)indirection_block_buffer);
	for(i=0;i<256;i++)
	{//for each pointer in the single indirection block
		if(indirection_block_buffer[i])
		{
//		printf("clear_single_indirection_block: clearing the data block %d  ",i);
		write_block(fp,indirection_block_buffer[i],empty_block_buffer,BYTES_PER_BLOCK);
		set_fbv_bit(fp,indirection_block_buffer[i]);
		}
	}
	
	
	return;
}

//RETURNS the inode id which belongs to this new files inode 


unsigned char upload_file(FILE* fp, char* path_to_parent_dir, char* file_name, FILE* fpin)
{
	fseek(fp,0,SEEK_SET);
	unsigned char parent_inode_id = find_file_inode_id(fp,path_to_parent_dir);
	create_file_in_directory(fp,parent_inode_id,file_name,fpin);
	
	
}

unsigned char create_file_in_directory(FILE* fp, unsigned char parent_inode_id, char* file_name, FILE* fpin)
{
//	printf("create_file_in_directory: starting file creation\n");
	//find out size of file
	long int size = 0;
	fseek(fpin,0,SEEK_END);
	size=ftell(fpin);
//	printf("create_file_in_directory: size = %ld bytes\n",size);
	
	
	//reposition the fp to the beginnign of the file
	fseek(fpin, 0,SEEK_SET);
	
	
	
	unsigned char inode_num = find_next_free_inode_id(fp);
//	printf("create_file_in_directory: next free inode %d\n",(int)inode_num);
	
	unsigned short num_blocks_remaining_to_write = size/BYTES_PER_BLOCK;
//	printf("create_file_in_directory: total num of blocks needed= %d\n",num_blocks_remaining_to_write);
	if (size%BYTES_PER_BLOCK) num_blocks_remaining_to_write++;
//	 printf("create_file_in_directory: num blocks to write %d\n",(int)num_blocks_remaining_to_write);
	//create inode with file type and size
	unsigned short inode_data_block_address = create_empty_inode(fp, inode_num,size,'f');
//	printf("create_file_in_directory: inode data block address = %d\n", (int)inode_data_block_address);
	unsigned short* inode_buffer = (unsigned short*)malloc(BYTES_PER_BLOCK);
	
	read_block(fp,inode_data_block_address,(char*)inode_buffer);
	assign_location_to_inode_map(fp, inode_data_block_address, inode_num);
	unsigned short temp_data_block_address;
	int i =0;
	//the first 10 blocks will be written to direct pointers
	for (i=0;i<10;i++)
	{	
		if (num_blocks_remaining_to_write ==1 && size%BYTES_PER_BLOCK)
		{
//			printf("create_file_in_directory: one block left to write\n");
			temp_data_block_address = create_and_write_data_block_from_file(fp, size%BYTES_PER_BLOCK,fpin);
		}
		
		
		else
		{
//			printf("create_file_in_directory: there are %d blocks left to write\n",num_blocks_remaining_to_write);
			temp_data_block_address = create_and_write_data_block_from_file(fp,BYTES_PER_BLOCK,fpin);
			
		}	
		
		inode_buffer[INODE_DIRECT_OFFSET/2+i]=temp_data_block_address;
//		printf("create_file_in_directory: writing in the %d position of the inode direct pointers\n", i);
		num_blocks_remaining_to_write--;
		if (num_blocks_remaining_to_write == 0)
		{
//			printf("create_file_in_directory: adding the element to the parent directory\n");
			add_element_to_directory(fp,parent_inode_id,inode_num,file_name);
			
			write_block(fp,inode_data_block_address,inode_buffer,INODE_BYTES);
			return inode_num;
			//there are no more blocks to write out and we can finish up the function
		}
	}
	
	//if execution has made it this far, then there are blocks to be written which have not been written out yet
	unsigned short single_indirection_block_num = create_indirection_block(fp,parent_inode_id);
	fill_single_indirection_block(fp,single_indirection_block_num,&num_blocks_remaining_to_write, size,temp_data_block_address,fpin);
	inode_buffer[14]=single_indirection_block_num;
	
	int k;
	if (num_blocks_remaining_to_write!=0)
	{
		unsigned short double_indirection_block_num = create_indirection_block(fp,parent_inode_id);
		unsigned short* double_indirection_block_buffer = (unsigned short*)malloc(BYTES_PER_BLOCK);
		read_block(fp,double_indirection_block_num, (char*)double_indirection_block_buffer);
		memset(double_indirection_block_buffer,0,BYTES_PER_BLOCK);
//		printf("creating double indirection block. to be stored in block space %d\n",double_indirection_block_num);
		for (k=0;k<256;k++)
		{
//			printf("creating a new single indirection block within the dbl , number %d",k);
			single_indirection_block_num = create_indirection_block(fp,parent_inode_id);
			fill_single_indirection_block(fp,single_indirection_block_num,&num_blocks_remaining_to_write, size,temp_data_block_address,fpin);
			double_indirection_block_buffer[k]=single_indirection_block_num;
			if (num_blocks_remaining_to_write==0)
			{
				//finished writing out the file
				inode_buffer[15]=double_indirection_block_num;
				break;
			}
		
			
		
		
		}
	}	
	add_element_to_directory(fp,parent_inode_id,inode_num,file_name);
			
	write_block(fp,inode_data_block_address,inode_buffer, INODE_BYTES);
	return inode_num;
	//update the single indirection pointer in the inode
	
	
}
//returns the blocks remaining to write
unsigned short read_from_single_indirection_block(FILE* fp, unsigned short single_indirection_block_num, 
										unsigned short num_blocks_remaining_to_read,long int size,
										FILE* fpout)
{
	/*
	 * if num==1&&&&remainder
	 * 	read block into buff
	 * 
	 * 	write block out 
	 * 
	 * else
	 * 	do same
	 * dec num remaining
	 * 
	 * 
	 * */
//	 printf("read_from_single_ind_block: num blocks to read = %d\n",num_blocks_remaining_to_read);
	char* temp_block = (char*)malloc(BYTES_PER_BLOCK);
	unsigned short * indirection_block_buff = (unsigned short*)malloc(BYTES_PER_BLOCK);
	memset(indirection_block_buff,0,BYTES_PER_BLOCK);
	read_block(fp, single_indirection_block_num, (char*)indirection_block_buff);
	int i;
	
	for (i=0; i< BYTES_PER_BLOCK/2; i++)
	{
//		printf("writin to block %d in newfile from address number %d in indirection block. block addres stord is %d\n",10+i,i,indirection_block_buff[i]);
		if (num_blocks_remaining_to_read==1)
		{
//			printf("reading the last block which is number %d in the indirection block and contains block address: %d\n",i,indirection_block_buff[i]);
			read_block(fp, indirection_block_buff[i],(char*)temp_block);
			write_block(fpout, 10+i,temp_block, size%BYTES_PER_BLOCK);
//			printf("temp_block: %s\n",(char*)temp_block);
			
			
		}
		else
		{
//			printf("reading another block\n");
			read_block(fp, indirection_block_buff[i],temp_block);
			write_block(fpout, 10+i, temp_block,BYTES_PER_BLOCK);
			
			
		}
		num_blocks_remaining_to_read--;
		if (num_blocks_remaining_to_read==0)
		{
//			printf("block remaining counter is 0 so exiting loop\n");
			break;
		}
	}
	free(indirection_block_buff);
	free(temp_block);
	return num_blocks_remaining_to_read;
	
	
}
///////////////////////////////////////////////////////////////////////////////////////////Dirctly copied from above. need to use the same principles to read from an inode id
FILE* download_file_from_inode_id(FILE* fp, unsigned char inode_id, char* new_filename)
{
//	printf("create_file_in_directory: starting file creation\n");
	//find out size of file
	unsigned int size = 0;
	
	unsigned short* inode_map = (unsigned short*)malloc(BYTES_PER_BLOCK);
	memset(inode_map,0,BYTES_PER_BLOCK);
	read_block(fp,2,(char*)inode_map);
	
	
	unsigned short* inode_buffer = (unsigned short*)malloc(BYTES_PER_BLOCK);
	
	read_block(fp,inode_map[inode_id],(char*)inode_buffer);
	
	size = (int)inode_buffer[0];
	
	
	
	FILE* outfile = fopen(new_filename,"wb");
	
	unsigned short num_blocks_needed_in_total = size/BYTES_PER_BLOCK;
	if (size%BYTES_PER_BLOCK) num_blocks_needed_in_total++;
	unsigned short num_blocks_remaining_to_write = size/BYTES_PER_BLOCK;
	if (size%BYTES_PER_BLOCK) num_blocks_remaining_to_write++;
//	 printf("create_file_in_directory: num blocks to write %d\n",(int)num_blocks_remaining_to_write);
	//create inode with file type and size
	unsigned short temp_data_block_address;
	int i =0;
	
	unsigned char* temp_data_block_buffer = malloc(BYTES_PER_BLOCK);
	//the first 10 blocks will be written to direct pointers
	for (i=0;i<10;i++)
	{	//triggered if there is one block left to write out which does not have enough characters to fill a whole block
		if (num_blocks_remaining_to_write ==1 && size%BYTES_PER_BLOCK)
		{	
			read_block(fp, inode_buffer[4+i],(char*)temp_data_block_buffer);
//				printf("create_file_in_directory: one block left to write\n");
			write_block(outfile,i,temp_data_block_buffer, size%BYTES_PER_BLOCK);
//			printf("last block: %s\n",temp_data_block_buffer);
		}
		
		
		else
		{
//			printf("create_file_in_directory: there are %d blocks left to write\n",num_blocks_remaining_to_write);
			read_block(fp, inode_buffer[4+i],(char*)temp_data_block_buffer);
//			printf("create_file_in_directory: one block left to write\n");
			write_block(outfile,i,temp_data_block_buffer, BYTES_PER_BLOCK);
			
		}	
		
		num_blocks_remaining_to_write--;
		if (num_blocks_remaining_to_write == 0)
		{
//			printf("create_file_in_directory: adding the element to the parent directory\n");
			
			return outfile;
			//there are no more blocks to write out and we can finish up the function
		}
	}
	
	//if execution has made it this far, then there are blocks to be written which have not been written out yet
	if (num_blocks_remaining_to_write!=0)
		
		
		num_blocks_remaining_to_write = read_from_single_indirection_block(fp,inode_buffer[14],num_blocks_remaining_to_write,size,outfile);
		
	
	int k;
	if (num_blocks_remaining_to_write!=0)
	{
		int i=1;
	}
//	fputc('\n',outfile);
	return outfile;
	//update the single indirection pointer in the inode
	
	
}

FILE* download_file(FILE* fp, char* target_filename, char* new_filename)
{
	
	unsigned char inode_id = find_file_inode_id(fp,target_filename);
	FILE* fpout =download_file_from_inode_id(fp,inode_id,new_filename);
	fclose(fpout);
	
}

//will return the free block number to which this directory was written to
unsigned short create_directory_block(FILE* fp, unsigned char parent_inode_id, unsigned char inode_id){
	unsigned short data_block_num = check_fbv_for_available_block(fp);
	
	//16 entries * 32 bytes each
	//1st byte is the inode id
	char* this_directory_name = ".";
	char* parent_directory_name = "..";
	
	char* directory_block = (char*)malloc(BYTES_PER_BLOCK);
	memset(directory_block,0,BYTES_PER_BLOCK);
	directory_block[32] = (char)parent_inode_id;
	
	memcpy((directory_block+1),this_directory_name,1);
	memcpy((directory_block+33),parent_directory_name,2);
	
	directory_block[0]=(char)inode_id;
	
	write_block(fp, data_block_num, (char *)directory_block, DIRECTORY_BYTES);
	free(directory_block);
	//reset_fbv_bit(fp, data_block_num);
	
//	printf("create_directory_block: creating directory data block  in %u\n",data_block_num);
	
	/*
	((unsigned char*)directory_block)[DIRECTORY_INODE_OFFSET]=(unsigned char)id;
	strncpy(((char**)directory_block)[DIRECTORY_ENTRY_OFFSET], this_directory_name, 1);
	((unsigned char*)directory_block)[DIRECTORY_ELEMENT_SIZE+DIRECTORY_INODE_OFFSET] = (unsigned char)parent_id;
	
	strncpy(((char**)directory_block)[DIRECTORY_ENTRY_OFFSET+DIRECTORY_ELEMENT_SIZE], parent_directory_name, 2);
	write_block(fp, block_num,directory_block,BYTES_PER_BLOCK);
	*/
	return data_block_num;
}

void assign_location_to_inode_map(FILE* fp, unsigned short inode_address, unsigned char inode_id)
{
	char* inode_map = malloc(INODE_MAX_NUM*2);
	read_block(fp, INODE_MAP_OFFSET, inode_map);
	inode_map[inode_id*2] = inode_address;
	write_block(fp, INODE_MAP_OFFSET,inode_map,(INODE_MAX_NUM*2));
}


unsigned short add_element_to_directory(FILE* fp, unsigned char directory_inode_id, unsigned char element_inode_id, char* element_file_name)
{
//	printf("add_element_to_directory:entering function\n");
	unsigned short parent_directory_inode_block_address = get_inode_address(fp, directory_inode_id);
	//HARDCODING TO FIND THE DIRECTORY ADDRESS WITHIN THE INODE BECAUSE THERE IS ONLY EVER ONE DIRECTORY FILE ATTACHED TO A DIRECTORY INODE
	unsigned short* parent_directory_inode_contents = (unsigned short*)malloc(BYTES_PER_BLOCK);
	read_block(fp,parent_directory_inode_block_address,(char*)parent_directory_inode_contents);
	unsigned short directory_data_block_address = parent_directory_inode_contents[4];
	
//	printf("add_element_to_directory:directory block address %d\n",directory_data_block_address);
	char* directory_block_data = (char*)malloc(BYTES_PER_BLOCK);
	read_block(fp,directory_data_block_address,directory_block_data);
	//now we have a directory data block stored in directory_block_data
	
	int i=0;
//	printf("[i*32+1] = %d\n",i*32+1);
	while (directory_block_data[i*32+1])
	{
//		printf("i=%d\n",i*32+1);
		i++;
		if (i>BYTES_PER_BLOCK)
		{
			printf("directory full!!\n");
			return -1;
			
			}
		}
	 
	//now i points to the empty directory slot
//	printf("add_element_to_directory:assigned byte number %d to the element_inode_id %d\n",i,element_inode_id);
	directory_block_data[i*32]=element_inode_id;
	int j=0;
	
//	printf("add_element_to_directory:about to write the element/file name to byte %d \n",i*32+1+j);
	while(element_file_name[j])
	{
		directory_block_data[i*32+1+j] = element_file_name[j];
		if (j>31)
		{
			break;
			}
		j++;
	}
	write_block(fp, directory_data_block_address, directory_block_data, i*32+1+j);
	
	
}



unsigned short create_directory_from_inode(FILE* fp, unsigned char parent_inode_id,char* new_directory_name)
{
	
//	printf("creating directory\n");
	unsigned char inode_id  = find_next_free_inode_id(fp);
//	printf("creating directory: next free inode %d\n", (int)inode_id);
	unsigned short directory_block = create_directory_block(fp, parent_inode_id, inode_id);
//	printf("creating directory: assigning directory block to %d\n", (int)directory_block);
	reset_fbv_bit(fp, (unsigned int)directory_block);
//	printf("creating directory: reset fbv bit in %d\n",(int)directory_block);
	//unsigned short available_block_number = check_fbv_for_available_block(fp);
	unsigned short inode_block = create_empty_inode(fp,inode_id,DIRECTORY_BYTES,'d');
	
	//assign inode map id to point to this inode block
//	printf("creating directory: created inode in block %d\n", (int)inode_block);
	
	assign_location_to_inode_map(fp, inode_block,inode_id);
	//adding directory file to inode 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////must troubleshoot adding a pointer to the directory in the dir's inode itself
	unsigned short* dir_inode_block = (unsigned short*)malloc(BYTES_PER_BLOCK);
	read_block(fp,inode_block,(char*)dir_inode_block);
	dir_inode_block[4] = directory_block;
	write_block(fp, inode_block,dir_inode_block,10);
//	printf("create_directory: added the block address %d to inode id %d\n",directory_block, inode_block);
	add_element_to_directory(fp,parent_inode_id,inode_id,new_directory_name);
	
	//returning the block address to which the directory file was created
	return directory_block;
	
	
}

void create_directory(FILE* fp, char* parent_directory_name, char* new_directory_name)
{
	unsigned char parent_inode_id = find_file_inode_id(fp,parent_directory_name);
	create_directory_from_inode(fp,parent_inode_id,new_directory_name);
	
	}
//	RETURNS AN INODE ID

/*
unsigned char* create_file(FILE* fp, FILE* new_file, unsigned char directory_inode_id)
{
	printf("create_file: \n");
	unsigned char inode_id = find_next_free_inode_id(fp);
	printf("create_file: next free inode %d\n", (int)inode_id);
	
	
	return 0;
}
*/


unsigned char find_file_inode_id(FILE* fp, char* absolute_file_path)
{
	
	unsigned short* inode_map = (unsigned short*)malloc(BYTES_PER_BLOCK);
	memset(inode_map,0,BYTES_PER_BLOCK);
	read_block(fp,2,(char*)inode_map);
	char* temp_directory_data_block = (char*)malloc(BYTES_PER_BLOCK);
	memset(temp_directory_data_block,0,BYTES_PER_BLOCK);
	char* temp_inode_data_block = (char*)malloc(BYTES_PER_BLOCK);
	memset(temp_inode_data_block,0,BYTES_PER_BLOCK);
	
	//working file path can be at most 4 directory names at once, each one being a max of 31 chars, so the total filepath can be 124+1 for null char
	char* working_file_path= (char*)malloc(125);
	memset(working_file_path,0,125);
	strncpy(working_file_path,absolute_file_path,strnlen(absolute_file_path,125));
	char* delimiter="/";
	//Use strtok to break up the filepath into directory names:
	char* token;
	token= strtok(working_file_path,delimiter);
	//current inode id will be initialized to 0 which is the root directory
	unsigned short directory_data_block_num;
	unsigned char current_inode_id=0;
	while(token!=NULL)
	{
//		printf("find_file_inode_id:dir name requested: %s\n",token);
//		printf("find_file_inode_id:looking through current directory with inode id %d\n", current_inode_id);
		int i;
		
		read_block(fp, inode_map[current_inode_id],(char*)temp_inode_data_block);
		//now to read the directory data in from the first direct pointer in the inode data block
		read_block(fp,temp_inode_data_block[8],temp_directory_data_block);
//		printf("copying directory data block from block num %d\n",inode_map[current_inode_id]);
		for (i=2;i<16;i++)
		{
//			printf("looking in slot # %d, \n",i);
			
//				printf("printing test: %s\n",(char*)temp_directory_data_block+i*32+1);
			if (!strncmp(token,temp_directory_data_block+i*32+1,31))
			{
//				printf("find_file_inode_id: found a match! with inode id %d\n", temp_directory_data_block[i*32]);
				current_inode_id = temp_directory_data_block[i*32];
				token=strtok(NULL,delimiter);
				break;
			////////////////////////////////////////////////////////////////////
			}
		
			if (i==15)
			{
//			printf("find_file_inode_id: could not find the file requested!\n");
			return 0;
			}
		}
		
		
	}
	
	free(inode_map);
	free(temp_directory_data_block);
	free(temp_inode_data_block);
	free(working_file_path);
	return current_inode_id;
	//store current directory inode id, initialized to 0 ie the root
	//while token!= NULL
		//look through the directory for the inode_id corresponding to the token
		//if not found, return file not found error
		//set current directory inode id to the matching string's inode id 
		//set current directory inode id to the next token
	
	
	return 0;
}


void init_vdisk(FILE* fp){
	//FIRSTLY CLEARING ALL THE DATA FROM THE vdisk file
	void* buffer = malloc(BYTES_PER_BLOCK);
	memset(buffer,0,BYTES_PER_BLOCK);
	if (!buffer)
	{printf("FAILED TO ALLOCATE BUFFER IN init_vdisk\n");exit(1);}
	int index;
	for(index=0; index<=MAX_BLOCK_INDEX; index++)
	{
		write_block(fp, index, buffer,BYTES_PER_BLOCK);
	}
	
	//FREE BLOCK VECTOR: BLOCK #1
	memset(buffer, 255, BYTES_PER_BLOCK);
	
	//SETTING THE first 16 blocks as unavailable because of superblock, FBV, and reserved spaces
	memset(buffer,0,2);
	
	write_block(fp, FREE_BLOCK_VECTOR_OFFSET, buffer,BYTES_PER_BLOCK);
	free(buffer);
	//printf("init_vdisk: creating the root directory\n");
	create_directory_from_inode(fp,-1,"");
	
}
/*
int main()
{
	void* buffer;
	
	FILE* fp =  fopen("./vdisk", "rb+");
	if (fp==NULL)
	{
		fp = fopen("./vdisk","wb+");
		init_vdisk(fp);
		
	}
	FILE* fpin = fopen("testin","rb");
	
	
//	create_directory(fp,"/","firstlevel");
//	upload_file(fp, "/firstlevel/","file1.txt",fpin);
//	delete_filepath(fp,"/firstlevel/file1.txt");
//	delete_filepath(fp,"/firstlevel/");
	if (fp==NULL)
	{
		printf("file pointer is null\n");
		
		
	}
//	fclose(fpout);
	int error = fclose(fp);
	printf("error from fclose %d",error);
	return 0;
	
}

*/
