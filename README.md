LLFS CSC 360 Assignment 3:
Thomas Besenski
V00879481

PIRATE FILE SYSTEM: PFS
-idea: all data saved is wiped when you decide to delete the file. For people who like to illegally download
	media and are afraid of copywrite police!


TESTING:

to test, go into the apps folder, use the makefile



Vdisk contents:
Block 0: super block
Block 1: free block vector
Block 2: inode map
Block 3-15: checkpoint region
Block 16-2055: Inode section
Block 2056-4095: Data section

How to use!
Rules
-You cannot delete the root directory
-directories can contain up to 14 files (including other directories)
-there is a total of 255 files allowed in the whole file system. Limited by number of inode indices


void init_vdisk(FILE* fp)
	fp: file pointer to empty file which we want to make our vdisk

unsigned char upload_file(FILE* fp, char* path_to_parent_dir, char* file_name, FILE* fpin)
	Pass a file pointer to the vdisk in as fp,
	pass the parent directory which you would like to hold your file
	pass a name which you will give to the file in the directory's listing
	pass a file pointe rto the file yo uwish to upload


FILE* download_file(FILE* fp, char* target_filename, char* new_filename)
	fp: file pointer to vdisk
	target_filename: absolute path to the file which yo uwant to download from the vdisk/File system
	new_filename: what would yo ulike to save your file as, in your current working directory 
	
	
	
void create_directory(FILE* fp, char* parent_directory_name, char* new_directory_name)
	fp: filepointer to vdisk
	parent_directory_name; absolute path to the directory which you want to create the subdirectory inside
	new_directory_name: the name which you would like to give the new directory
	


void delete_filepath(FILE* fp, char* filename)
	fp: fiel pointer to vdisk
	filename: absolute file path/name to the file you wish to remove from the file system. Can be directory or regular file


