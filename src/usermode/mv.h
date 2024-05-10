#ifndef _MV_H_
#define _MV_H_

/*	if arg is a valid directory, move file to that directory
	else if arg is an invalid directory (i.e. has "/" in arg but folder not found), cancel mv
	else rename file to arg
*/	
void mv(char* arg);
void rename(char* file_name, char* new_file_name, int parent_cluster_number);
void move(char* file_name, int old_parent_cluster_number, int new_parent_cluster_number);

#endif