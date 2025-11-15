#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs.h"
#include "fs_util.h"
#include "disk.h"

char inodeMap[MAX_INODE / 8];
char blockMap[MAX_BLOCK / 8];
Inode inode[MAX_INODE];
SuperBlock superBlock;
Dentry curDir;
int curDirBlock;

int fs_mount(char *name)
{
		int numInodeBlock =  (sizeof(Inode)*MAX_INODE)/ BLOCK_SIZE;
		int i, index, inode_index = 0;


		// load superblock, inodeMap, blockMap and inodes into the memory
		if(disk_mount(name) == 1) {
				disk_read(0, (char*) &superBlock);
				if(superBlock.magicNumber != MAGIC_NUMBER) {
						printf("Invalid disk!\n");
						exit(0);
				}
				disk_read(1, inodeMap);
				disk_read(2, blockMap);
				for(i = 0; i < numInodeBlock; i++)
				{
						index = i+3;
						disk_read(index, (char*) (inode+inode_index));
						inode_index += (BLOCK_SIZE / sizeof(Inode));
				}
				// root directory
				curDirBlock = inode[0].directBlock[0];
				disk_read(curDirBlock, (char*)&curDir);

		} else {
				// Init file system superblock, inodeMap and blockMap
				superBlock.magicNumber = MAGIC_NUMBER;
				superBlock.freeBlockCount = MAX_BLOCK - (1+1+1+numInodeBlock);
				superBlock.freeInodeCount = MAX_INODE;

				//Init inodeMap
				for(i = 0; i < MAX_INODE / 8; i++)
				{
						set_bit(inodeMap, i, 0);
				}
				//Init blockMap
				for(i = 0; i < MAX_BLOCK / 8; i++)
				{
						if(i < (1+1+1+numInodeBlock)) set_bit(blockMap, i, 1);
						else set_bit(blockMap, i, 0);
				}
				//Init root dir
				int rootInode = get_free_inode();
				curDirBlock = get_free_block();

				inode[rootInode].type = directory;
				inode[rootInode].owner = 0;
				inode[rootInode].group = 0;
				gettimeofday(&(inode[rootInode].created), NULL);
				gettimeofday(&(inode[rootInode].lastAccess), NULL);
				inode[rootInode].size = 1;
				inode[rootInode].blockCount = 1;
				inode[rootInode].directBlock[0] = curDirBlock;

				curDir.numEntry = 1;
				strncpy(curDir.dentry[0].name, ".", 1);
				curDir.dentry[0].name[1] = '\0';
				curDir.dentry[0].inode = rootInode;
				disk_write(curDirBlock, (char*)&curDir);
		}
		return 0;
}

int fs_umount(char *name)
{
		int numInodeBlock =  (sizeof(Inode)*MAX_INODE )/ BLOCK_SIZE;
		int i, index, inode_index = 0;
		disk_write(0, (char*) &superBlock);
		disk_write(1, inodeMap);
		disk_write(2, blockMap);

		for(i = 0; i < numInodeBlock; i++)
		{
				index = i+3;
				disk_write(index, (char*) (inode+inode_index));
				inode_index += (BLOCK_SIZE / sizeof(Inode));
		}
		// current directory
		disk_write(curDirBlock, (char*)&curDir);

		disk_umount(name);	
		
}

int search_cur_dir(char *name)
{
		// return inode. If not exist, return -1
		int i;

		for(i = 0; i < curDir.numEntry; i++)
		{
				if (strcmp(name, curDir.dentry[i].name) == 0) return curDir.dentry[i].inode;
		}
		return -1;
}

int file_create(char *name, int size)
{
		int i;

		if(size > SMALL_FILE) {
				printf("Do not support files larger than %d bytes.\n", SMALL_FILE);
				return -1;
		}

		if(size < 0){
				printf("File create failed: cannot have negative size\n");
				return -1;
		}

		int inodeNum = search_cur_dir(name); 
		if(inodeNum >= 0) {
				printf("File create failed:  %s exist.\n", name);
				return -1;
		}

		if(curDir.numEntry + 1 > MAX_DIR_ENTRY) {
				printf("File create failed: directory is full!\n");
				return -1;
		}

		int numBlock = size / BLOCK_SIZE;
		if(size % BLOCK_SIZE > 0) numBlock++;

		if(numBlock > superBlock.freeBlockCount) {
				printf("File create failed: data block is full!\n");
				return -1;
		}

		if(superBlock.freeInodeCount < 1) {
				printf("File create failed: inode is full!\n");
				return -1;
		}

		char *tmp = (char*) malloc(sizeof(int) * size + 1);

		rand_string(tmp, size);
		printf("New File: %s\n", tmp);

		// get inode and fill it
		inodeNum = get_free_inode();
		if(inodeNum < 0) {
				printf("File_create error: not enough inode.\n");
				return -1;
		}

		inode[inodeNum].type = file;
		inode[inodeNum].owner = 1;  // pre-defined
		inode[inodeNum].group = 2;  // pre-defined
		gettimeofday(&(inode[inodeNum].created), NULL);
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);
		inode[inodeNum].size = size;
		inode[inodeNum].blockCount = numBlock;
		inode[inodeNum].link_count = 1;

		// add a new file into the current directory entry
		strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
		curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		printf("curdir %s, name %s\n", curDir.dentry[curDir.numEntry].name, name);
		curDir.numEntry++;

		// get data blocks
		for(i = 0; i < numBlock; i++)
		{
				int block = get_free_block();
				if(block == -1) {
						printf("File_create error: get_free_block failed\n");
						return -1;
				}
				//set direct block
				inode[inodeNum].directBlock[i] = block;

				disk_write(block, tmp+(i*BLOCK_SIZE));
		}

		//update last access of current directory
		gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);		

		printf("file created: %s, inode %d, size %d\n", name, inodeNum, size);

		free(tmp);
		return 0;
}

int file_cat(char *name)
{
		int inodeNum, i, size;
		char str_buffer[512];
		char * str;

		//get inode
		inodeNum = search_cur_dir(name);
		size = inode[inodeNum].size;

		//check if valid input
		if(inodeNum < 0)
		{
				printf("cat error: file not found\n");
				return -1;
		}
		if(inode[inodeNum].type == directory)
		{
				printf("cat error: cannot read directory\n");
				return -1;
		}

		//allocate str
		str = (char *) malloc( sizeof(char) * (size+1) );
		str[ size ] = '\0';

		for( i = 0; i < inode[inodeNum].blockCount; i++ ){
				int block;
				block = inode[inodeNum].directBlock[i];

				disk_read( block, str_buffer );

				if( size >= BLOCK_SIZE )
				{
						memcpy( str+i*BLOCK_SIZE, str_buffer, BLOCK_SIZE );
						size -= BLOCK_SIZE;
				}
				else
				{
						memcpy( str+i*BLOCK_SIZE, str_buffer, size );
				}
		}
		printf("%s\n", str);

		//update lastAccess
		gettimeofday( &(inode[inodeNum].lastAccess), NULL );

		free(str);

		//return success
		return 0;
}

int file_read(char *name, int offset, int size)
{
		int inodeNum;
		char str_buffer[512];
		char *str;
		int i = offset / BLOCK_SIZE; // index of the first directBlock
		int o = offset % BLOCK_SIZE; // offset from the first block
		int bn = (size + offset) / BLOCK_SIZE; // index of the last block to read
		int s = size;
		


		// get inode
		inodeNum = search_cur_dir(name);

		//check if valid input
		if(inodeNum < 0)
		{
				printf("read error: file not found\n");
				return -1;
		}
		if(inode[inodeNum].type == directory)
		{
				printf("read error: cannot read directory\n");
				return -1;
		}
		if (offset > inode[inodeNum].size) {
				printf("read error: offset is larger than file size\n");
				return -1;
		}

		// allocate str
		str = (char*) malloc(sizeof(char) * (size + 1));
		str[size] = '\0';
		
		if (bn > inode[inodeNum].blockCount) bn = inode[inodeNum].blockCount;

		int block = inode[inodeNum].directBlock[i];
		disk_read(block, str_buffer);
		memcpy( str, str_buffer + o, BLOCK_SIZE - o );
		s -= (BLOCK_SIZE - o);
		i++;
		int a = 0;


		for (int j = i ; j <= bn; j++) {
			block = inode[inodeNum].directBlock[j];

			disk_read(block, str_buffer);
			if( s >= BLOCK_SIZE )
			{
					memcpy( str+a*BLOCK_SIZE + BLOCK_SIZE - o, str_buffer, BLOCK_SIZE );
					s -= BLOCK_SIZE;
					a++;
					
			}
			else
			{
					memcpy( str+a*BLOCK_SIZE + BLOCK_SIZE - o, str_buffer, s );
			}
			

			


		}
		printf("%s\n", str);

		//update lastAccess
		gettimeofday( &(inode[inodeNum].lastAccess), NULL );

		free(str);

		
		return 0;
}


int file_stat(char *name)
{
		char timebuf[28];
		int inodeNum = search_cur_dir(name);
		if(inodeNum < 0) {
				printf("file cat error: file is not exist.\n");
				return -1;
		}

		printf("Inode\t\t= %d\n", inodeNum);
		if(inode[inodeNum].type == file) printf("type\t\t= File\n");
		else printf("type\t\t= Directory\n");
		printf("owner\t\t= %d\n", inode[inodeNum].owner);
		printf("group\t\t= %d\n", inode[inodeNum].group);
		printf("size\t\t= %d\n", inode[inodeNum].size);
		printf("link_count\t= %d\n", inode[inodeNum].link_count);
		printf("num of block\t= %d\n", inode[inodeNum].blockCount);
		format_timeval(&(inode[inodeNum].created), timebuf, 28);
		printf("Created time\t= %s\n", timebuf);
		format_timeval(&(inode[inodeNum].lastAccess), timebuf, 28);
		printf("Last acc. time\t= %s\n", timebuf);
}

int file_remove(char *name)
{
		int inodeNum = search_cur_dir(name);

		if (inodeNum < 0 ) {
			printf("file rm error: %s does not exist!\n", name);
			return -1;
		}

		if (inode[inodeNum].type != file) {
			printf("file rm error: %s is not a file!\n", name);
			return -1;
		}

		// remove dentry
		for(int i = 0; i < curDir.numEntry; i++)
		{
				if (strcmp(name, curDir.dentry[i].name) == 0) {
					curDir.numEntry--;
					curDir.dentry[i] = curDir.dentry[curDir.numEntry];
					
					if (inode[inodeNum].link_count > 1) {
						inode[inodeNum].link_count--;
						printf("Hard link '%s' removed successfully.\n", name);

						return 0;
					}
				}
		}
		// update the super block
		superBlock.freeBlockCount += inode[inodeNum].blockCount;
		superBlock.freeInodeCount += 1;

		// free data blocks
		for (int i = 0; i < inode[inodeNum].blockCount; i++) {
			set_bit(blockMap, inode[inodeNum].directBlock[i], 0 );

		}

		// free inode block
		set_bit(inodeMap, inodeNum, 0);
		gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);		

		printf("File '%s' removed successfully.\n", name);


		return 0;
}

int dir_make(char* name)
{
		int inodeNum = search_cur_dir(name); 
		if(inodeNum >= 0) {
				printf("Dir_make failed:  %s exists.\n", name);
				return -1;
		}

		if(curDir.numEntry + 1 > MAX_DIR_ENTRY) {
				printf("Dir_make failed: directory is full!\n");
				return -1;
		}
		if(superBlock.freeBlockCount < 1) {
				printf("Dir_make failed: data block is full!\n");
				return -1;
		}

		if(superBlock.freeInodeCount < 1) {
				printf("Dir_make failed: inode is full!\n");
				return -1;
		}

		//get new inode block
		inodeNum = get_free_inode();
		
		if(inodeNum < 0) {
				printf("Dir_make error: not enough inode.\n");
				return -1;
		}
		//create new inode 
		inode[inodeNum].type = directory;
		inode[inodeNum].owner = 0;
		inode[inodeNum].group = 0;
		gettimeofday(&(inode[inodeNum].created), NULL);
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);
		inode[inodeNum].size = 1;
		inode[inodeNum].blockCount = 1;
		inode[inodeNum].directBlock[0] = get_free_block();

		// add new dentry to the parent dir
		strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
		curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		curDir.numEntry++;

		// make new dir and write it to disk
		Dentry newD;
		newD.numEntry = 2;
		// "." dir
		strncpy(newD.dentry[0].name, ".", 1);
		newD.dentry[0].name[1] = '\0';
		newD.dentry[0].inode = inodeNum;
		// ".." dir
		strncpy(newD.dentry[1].name, "..", strlen(".."));
		newD.dentry[1].name[strlen("..")] = '\0';
		newD.dentry[1].inode = curDir.dentry[0].inode;
		gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);		

		disk_write(inode[inodeNum].directBlock[0], (char*)&newD);
		printf("dir created: %s, inode %d\n", name, inodeNum);


		return 0;
}
int dir_change(char* name)
{
		int inodeNum, i;

		//get inode number
		inodeNum = search_cur_dir(name);
		if (inodeNum < 0) 
		{
				printf("cd error: %s does not exist\n", name);
				return -1;
		}
		if (inode[inodeNum].type != directory)
		{
				printf("cd error: %s is not a directory\n", name);
				return -1;
		}

		//write parent directory (curDir) to disk
		disk_write(curDirBlock, (char*)&curDir);

		//read new directory from disk into curDir
		curDirBlock = inode[inodeNum].directBlock[0];
		disk_read(curDirBlock, (char*)&curDir);

		//update last access of directory we are changing to
		gettimeofday(&(inode[inodeNum].lastAccess), NULL);		

		return 0;
}

int dir_remove(char *name)
{
    
    int inodeNum = search_cur_dir(name);

    if (inodeNum < 0) {
        printf("dir rm error: %s does not exist!\n", name);
        return -1;
    }

    if (inode[inodeNum].type == file) {
        printf("dir rm error: %s is not a directory!\n", name);
        return -1;
    }

    //Load directory block ---
    Dentry d;
    disk_read(inode[inodeNum].directBlock[0], (char *)&d);

    //Recursively delete subdirectories ---
    for (int i = 0; i < d.numEntry; i++) {

        char *childName = d.dentry[i].name;
        int childInode = d.dentry[i].inode;

        // skip . and ..
        if (strcmp(childName, ".") == 0 || strcmp(childName, "..") == 0)
            continue;

        // If child is a file, we refuse
        if (inode[childInode].type == file) {
            printf("dir rm error: directory (%s) contains file (%s). Delete files first.\n",
                   name, childName);
            return -1;
        }

        // Child is another directory → recursive delete
        dir_change(name);                // into parent/name
        if (dir_remove(childName) < 0) { // recursively delete child
            dir_change("..");
            return -1;
        }
        dir_change("..");                // back to parent
    }

    //Free inode + block for *this* directory ---
    int blockToFree = inode[inodeNum].directBlock[0];

    set_bit(blockMap, blockToFree, 0);  // free data block
    set_bit(inodeMap, inodeNum, 0);     // free inode

    superBlock.freeBlockCount++;
    superBlock.freeInodeCount++;

    //Remove this directory entry from its parent ---
    for (int i = 0; i < curDir.numEntry; i++) {
        if (strcmp(curDir.dentry[i].name, name) == 0) {
            // overwrite deleted entry with last entry
            curDir.dentry[i] = curDir.dentry[curDir.numEntry - 1];
            curDir.numEntry--;
            break;
        }
    }
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);		

    printf("Directory '%s' removed successfully.\n", name);
    return 0;
}




int ls()
{
		int i;
		for(i = 0; i < curDir.numEntry; i++)
		{
				int n = curDir.dentry[i].inode;
				if(inode[n].type == file) printf("type: file, ");
				else printf("type: dir, ");
				printf("name \"%s\", inode %d, size %d byte\n", curDir.dentry[i].name, curDir.dentry[i].inode, inode[n].size);
		}

		return 0;
}

int fs_stat()
{
		printf("File System Status: \n");
		printf("# of free blocks: %d (%d bytes), # of free inodes: %d\n", superBlock.freeBlockCount, superBlock.freeBlockCount*512, superBlock.freeInodeCount);
}

int hard_link(char *src, char *dest)
{
		int inodeNum = search_cur_dir(dest); 
		if(inodeNum >= 0) {
				printf("File create failed:  %s exist.\n", dest);
				return -1;
		}
		inodeNum = search_cur_dir(src); 
		if(inodeNum < 0) {
				printf("File create failed:  %s does not exist.\n", src);
				return -1;
		}

		if(curDir.numEntry + 1 > MAX_DIR_ENTRY) {
				printf("File create failed: directory is full!\n");
				return -1;
		}
		inode[inodeNum].link_count++;

		// add a new file into the current directory entry
		strncpy(curDir.dentry[curDir.numEntry].name, dest, strlen(dest));
		curDir.dentry[curDir.numEntry].name[strlen(dest)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		curDir.numEntry++;

		//update last access of current directory
		gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);
		printf("hard link created: %s, inode %d\n", dest, inodeNum);
		return 0;
}

int execute_command(char *comm, char *arg1, char *arg2, char *arg3, char *arg4, int numArg)
{

    printf ("\n");
	if(strcmp(comm, "df") == 0) {
				return fs_stat();

    // file command start    
    } else if(strcmp(comm, "create") == 0) {
        if(numArg < 2) {
            printf("error: create <filename> <size>\n");
            return -1;
        }
		return file_create(arg1, atoi(arg2)); // (filename, size)

	} else if(strcmp(comm, "stat") == 0) {
		if(numArg < 1) {
			printf("error: stat <filename>\n");
			return -1;
		}
		return file_stat(arg1); //(filename)

	} else if(strcmp(comm, "cat") == 0) {
		if(numArg < 1) {
			printf("error: cat <filename>\n");
			return -1;
		}
		return file_cat(arg1); // file_cat(filename)

	} else if(strcmp(comm, "read") == 0) {
		if(numArg < 3) {
			printf("error: read <filename> <offset> <size>\n");
			return -1;
		}
		return file_read(arg1, atoi(arg2), atoi(arg3)); // file_read(filename, offset, size);

	} else if(strcmp(comm, "rm") == 0) {
		if(numArg < 1) {
			printf("error: rm <filename>\n");
			return -1;
		}
		return file_remove(arg1); //(filename)

	} else if(strcmp(comm, "ln") == 0) {
		return hard_link(arg1, arg2); // hard link. arg1: src file or dir, arg2: destination file or dir

    // directory command start
	} else if(strcmp(comm, "ls") == 0)  {
		return ls();

	} else if(strcmp(comm, "mkdir") == 0) {
		if(numArg < 1) {
			printf("error: mkdir <dirname>\n");
			return -1;
		}
		return dir_make(arg1); // (dirname)

	} else if(strcmp(comm, "rmdir") == 0) {
		if(numArg < 1) {
			printf("error: rmdir <dirname>\n");
			return -1;
		}
		return dir_remove(arg1); // (dirname)

	} else if(strcmp(comm, "cd") == 0) {
		if(numArg < 1) {
			printf("error: cd <dirname>\n");
			return -1;
		}
		return dir_change(arg1); // (dirname)

	} else {
		fprintf(stderr, "%s: command not found.\n", comm);
		return -1;
	}
	return 0;
}

