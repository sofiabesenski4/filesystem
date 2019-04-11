#ifndef _file_h

#define _file_h
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>





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
void delete_filepath(FILE* fp, char* filename);
void delete_file(FILE* fp, unsigned char filename);
void delete_inode(FILE* fp, unsigned char inode_id);
void clear_single_indirection_block(FILE* fp, unsigned short indirection_block_num);
FILE* download_file(FILE* fp, char* target_filename, char* new_filename);
unsigned char find_file_inode_id(FILE* fp, char* absolute_file_path);
void create_directory(FILE* fp, char* parent_directory_name, char* new_directory_name);
void delete_directory(FILE* fp, unsigned char directory_inode_id);
void delete_file(FILE* fp, unsigned char file_inode_id);
unsigned char upload_file(FILE* fp, char* path_to_parent_dir, char* file_name, FILE* fpin);

#endif
