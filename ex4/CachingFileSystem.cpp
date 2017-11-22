/*
 * CachingFileSystem.cpp
 * 
 */

#define FUSE_USE_VERSION 26

#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <fuse.h>
#include <time.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ctime>
#include <list>
#include <vector>
#include <exception>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "Cache.h"
using namespace std;
//_______________________Errors__________________________//
#define LOGFILE "/.filesystem.log"
#define LOGFILE_OPEN_FAILED "couldn't open .filesystem.log file\n"
#define LOGFILE_WRITE_FAILED "couldn't write to .filesystem.log file\n"
#define LOGFILE_CLOSE_FAILED "couldn't close .filesystem.log file\n"
#define USAGE_ERROR "usage: CachingFileSystem rootdir mountdir numberOfBlocks blockSize\n"
#define INIT_BUFFER_FAILED "System Error: can't initialized buffer\n"
#define MEMORY_ALLOC_FAILED "System Error: memory allocation failed\n"
#define REAL_PATH_FAILED "System Error: couldn't get a real path from rootdir\n"
#define STRCPY_FAILED "System Error: strcpy failed\n"

#define FS_CONTEXT ((fs_context *) fuse_get_context()->private_data)

struct fuse_operations caching_oper;

/*
 * File system context, will store all the "global" data we need for the managment
 */
typedef struct fs_context 
{
	FILE* logFile;
	Cache* theCache;
	char* rootdirPath;
	int blockSize;
	int numberOfBlock;
} fs_context;

//_______________________our functions__________________________//
/*
 * Function that convert the path to the right type
 */
static void get_real_path(char real_path[PATH_MAX], const char *path) 
{
	strcpy(real_path, FS_CONTEXT->rootdirPath);
	strncat(real_path, path, PATH_MAX);
}

/*
 * Function to write a massage on the log file
 */
static int log_msg(const char *format, ...) 
{
	if (FS_CONTEXT->logFile == NULL) 
	{
		return -errno;
	}
	int retstat = 0;
	time_t timeToPrint = time(NULL);
	stringstream ss;
	ss << timeToPrint;
	string ts = ss.str();
	string massage = ts + " " + (string) format;
	retstat = fprintf(FS_CONTEXT->logFile, "%s\n", massage.c_str());
	if (retstat < 0) 
	{
		return -errno;
	}
	return retstat;
}

/*
 * Function to write a block info on the log file, for the ioctl func
 */
static int log_ioctl(Block* currentBlk) 
{
	if (FS_CONTEXT->logFile == NULL) 
	{
		return -errno;
	}
	int retstat = 0;
	stringstream msgBuff;
	msgBuff << currentBlk->filePath << " " << currentBlk->fileContentNum << " "
			<< currentBlk->counter;
	string msg = msgBuff.str();
	msg = msg.substr(1, msg.size() - 1);
	retstat = fprintf(FS_CONTEXT->logFile, "%s\n", msg.c_str()); //remove the starting '/' from the path
	if (retstat < 0) 
	{
		perror(LOGFILE_WRITE_FAILED);
		return -errno;
	}
	return retstat;
}


//_______________________original functions__________________________//

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *path, struct stat *statbuf) 
{
	int retVal = log_msg("getattr");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	int retstat = 0;
	char real_path[PATH_MAX];
	get_real_path(real_path, path);
	retstat = lstat(real_path, statbuf);
	if (retstat < 0)
		return -errno;
	return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int caching_fgetattr(const char *path, struct stat *statbuf,
		struct fuse_file_info *fi) 
{
	int retVal = log_msg("fgetattr");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	int retstat = 0;
	retstat = fstat(fi->fh, statbuf);
	if (retstat < 0)
		return -errno;
	return retstat;
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int caching_access(const char *path, int mask) 
{
	int retVal = log_msg("access");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) {
		return -ENONET;
	}
	int retstat = 0;
	char real_path[PATH_MAX];
	get_real_path(real_path, path);
	retstat = access(real_path, mask);
	if (retstat < 0)
		return -errno;
	return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.

 * pay attention that the max allowed path is PATH_MAX (in limits.h).
 * if the path is longer, return error.

 * Changed in version 2.2
 */
int caching_open(const char *path, struct fuse_file_info *fi) 
{
	int retVal = log_msg("open");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	int retstat = 0;
	int fd;
	char real_path[PATH_MAX];
	get_real_path(real_path, path);

	if ((fi->flags & 3) != (O_RDONLY)) 
	{
		return -errno;
	}
	fd = open(real_path, fi->flags);
	if (fd < 0) 
	{
		return -errno;
	}
	fi->fh = fd;
	return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.
 *
 * Changed in version 2.2
 */
int caching_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) 
{
	int retVal = log_msg("read");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	int retstat = 0;
	char real_path[PATH_MAX];
	get_real_path(real_path, path);
	buf[0] = '\0';
	// finds the size of file
	int seek_curr = lseek(fi->fh, 0, SEEK_CUR);
	if (seek_curr < 0) 
	{
		return -errno;
	}
	int sizeOfFile = lseek(fi->fh, 0, SEEK_END);
	if (sizeOfFile < 0) 
	{
		return -errno;
	}
	int check = lseek(fi->fh, seek_curr, SEEK_SET);
	if (check < 0) 
	{
		return -errno;
	}
	size = min(size, (size_t) sizeOfFile);
	if (size == 0)
		return retstat; //case the file is empty or nothing to read
	if ((unsigned) offset > size) 
	{
		return retstat;
	}
	unsigned int finalOffset = min(offset + size, (size_t) sizeOfFile);
	BlocksVec tempVec;
	BlocksVec empty;
	//find the blocks that already in cache
	std::map<string, BlocksVec>::iterator mapIter;
	mapIter = FS_CONTEXT->theCache->fileBlocks.find(path);
	if (mapIter == FS_CONTEXT->theCache->fileBlocks.end()) //path not found on map
	{
		tempVec = empty; //creating new vector to insert to the map
	} else //path found on map means the vector exist
	{
		tempVec = (*mapIter).second;
	}
	bool foundOnCache = false;
	Block* currentBlock = NULL;
	unsigned int currentOffset = offset;
	BlocksVec::iterator mapVecIter;
	int blockNum = (currentOffset / FS_CONTEXT->blockSize) + 1;
	int firstBlockNum = blockNum;
	int blockStartOffset = (firstBlockNum - 1) * FS_CONTEXT->blockSize;
	int curStartOffset = 0;
	int sizeToReadNow = 0;
	int lastBlockNum = (finalOffset / FS_CONTEXT->blockSize) + 1;
	while (currentOffset < finalOffset) 
	{
		//calculate how much we need to read from this block and the start offset from this block
		if (blockNum == firstBlockNum) 
		{
			curStartOffset = currentOffset - blockStartOffset;
			sizeToReadNow = FS_CONTEXT->blockSize - curStartOffset;
		} 
		else if (blockNum == lastBlockNum) 
		{
			curStartOffset = 0;
			sizeToReadNow = finalOffset - currentOffset;
		} 
		else 
		{
			curStartOffset = 0;
			sizeToReadNow = FS_CONTEXT->blockSize;
		}
		//check if this block already exist so we dont need to use pread and we have the data
		if (tempVec.size() != 0) 
		{
			for (mapVecIter = tempVec.begin();
					mapVecIter != tempVec.end() && !foundOnCache;
					++mapVecIter) 
			{
				if ((*mapVecIter)->fileContentNum == blockNum) 
				{
					foundOnCache = true;
					currentBlock = (*mapVecIter);
				}
			}
		}
		//check if there is no block for this data
		if (!foundOnCache) 
		{
			currentBlock = FS_CONTEXT->theCache->addNewBlock(path, blockNum,
					fi->fh, FS_CONTEXT->blockSize); //create new block
			//read to it's data the relevant data to read from file
			int retVal = pread(currentBlock->fd, currentBlock->data,
			FS_CONTEXT->blockSize, blockStartOffset);
			if (retVal < 0) 
			{
				return -errno;
			}
		}
		//add the relevant data of this block to the buffer
		strncat(buf, currentBlock->data + curStartOffset, sizeToReadNow);
		//prepering to the next iteration
		curStartOffset = 0;
		retstat += sizeToReadNow;
		currentBlock->counter++;
		foundOnCache = false;
		blockStartOffset += FS_CONTEXT->blockSize;
		currentOffset += FS_CONTEXT->blockSize;
		blockNum += 1;
	}
	FS_CONTEXT->theCache->sortList();
	return retstat;

}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int caching_flush(const char *path, struct fuse_file_info *fi) 
{
	int retVal = log_msg("flush");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	return EXIT_SUCCESS;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int caching_release(const char *path, struct fuse_file_info *fi) 
{
	int retVal = log_msg("release");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	int retstat = 0;
	retstat = close(fi->fh);
	if (retstat < 0) 
	{
		return -errno;
	}
	return retstat;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi) 
{
	int retVal = log_msg("opendir");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	DIR *dp;
	int retstat = 0;
	char real_path[PATH_MAX];
	get_real_path(real_path, path);
	dp = opendir(real_path);
	if (dp == NULL) 
	{
		return -errno;
	}
	fi->fh = (intptr_t) dp;
	return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * Introduced in version 2.3
 */
int caching_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) 
{
	int retVal = log_msg("readdir");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	int retstat = 0;
	DIR *dp;
	struct dirent *de;

	dp = (DIR *) (uintptr_t) fi->fh;
	de = readdir(dp);
	if (de == 0) 
	{
		return -errno;
	}
	do {
		if (filler(buf, de->d_name, NULL, 0) != 0) 
		{
			return -errno;
		}
	} while ((de = readdir(dp)) != NULL);
	return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi) 
{
	int retVal = log_msg("releasedir");
	if (retVal < 0)
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	int retstat = 0;
	retstat = closedir((DIR *) (uintptr_t) fi->fh);
	if (retstat < 0) 
	{
		return -errno;
	}
	return retstat;
}

/** Rename a file */
int caching_rename(const char *path, const char *newpath) 
{
	int retVal = log_msg("rename");
	if (retVal < 0) 
	{
		return retVal;
	}
	if (path == (string) LOGFILE) 
	{
		return -ENONET;
	}
	int retstat = 0;
	char real_path[PATH_MAX];
	char real_new_path[PATH_MAX];
	get_real_path(real_path, path);
	get_real_path(real_new_path, newpath);
	retstat = rename(real_path, real_new_path);
	if (retstat < 0) 
	{
		return -errno;
	}
	// updates the file_path in the cache, also handle the case of rename directory.
	vector<Block*> fileVec = FS_CONTEXT->theCache->fileBlocks[path];
	vector<Block*> newVec;
	vector<Block*>::iterator it;
	Block* currentBlock;
	for (it = fileVec.begin(); it != fileVec.end(); ++it) 
	{
		currentBlock = (*it);
		currentBlock->filePath = newpath;
		newVec.push_back(currentBlock); //for the future map update
	}
	//update the map to work with our new path
	fileVec.clear();
	FS_CONTEXT->theCache->fileBlocks.erase(
	FS_CONTEXT->theCache->fileBlocks.find(path));
	FS_CONTEXT->theCache->fileBlocks[newpath] = newVec;
	return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *caching_init(struct fuse_conn_info *conn) 
{
	int retVal = log_msg("init");
	if (retVal < 0) 
	{
		return NULL;
	}
	return FS_CONTEXT;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void caching_destroy(void *userdata) 
{
	int retVal = log_msg("destroy");
	if (retVal < 0) 
	{
		return;
	}
	FS_CONTEXT->theCache->freeCache();
	delete (FS_CONTEXT->theCache);
	fclose(FS_CONTEXT->logFile);
	free(FS_CONTEXT);
}

/**
 * Ioctl from the FUSE sepc:
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * However, in our case, this function only needs to print cache table to the log file .
 *
 * Introduced in version 2.8
 */
int caching_ioctl(const char *, int cmd, void *arg, struct fuse_file_info *,
		unsigned int flags, void *data) 
{
	int retVal = log_msg("ioctl");
	if (retVal < 0) 
	{
		return retVal;
	}
	int retstat = 0;
	list<Block*>::iterator listIter;
	Block* currentBlock;
	for (listIter = FS_CONTEXT->theCache->cacheList.begin();
			listIter != FS_CONTEXT->theCache->cacheList.end(); ++listIter) 
	{

		currentBlock = (*listIter);
		retstat = log_ioctl(currentBlock);
		if (retstat < 0) 
		{
			return retstat;
		}
	}
	return retstat;
}

// Initialise the operations.
// You are not supposed to change this function.caching_releasedir
void init_caching_oper() 
{
	caching_oper.getattr = caching_getattr;
	caching_oper.access = caching_access;
	caching_oper.open = caching_open;
	caching_oper.read = caching_read;
	caching_oper.flush = caching_flush;
	caching_oper.release = caching_release;
	caching_oper.opendir = caching_opendir;
	caching_oper.readdir = caching_readdir;
	caching_oper.releasedir = caching_releasedir;
	caching_oper.rename = caching_rename;
	caching_oper.init = caching_init;
	caching_oper.destroy = caching_destroy;
	caching_oper.ioctl = caching_ioctl;
	caching_oper.fgetattr = caching_fgetattr;

	caching_oper.readlink = NULL;
	caching_oper.getdir = NULL;
	caching_oper.mknod = NULL;
	caching_oper.mkdir = NULL;
	caching_oper.unlink = NULL;
	caching_oper.rmdir = NULL;
	caching_oper.symlink = NULL;
	caching_oper.link = NULL;
	caching_oper.chmod = NULL;
	caching_oper.chown = NULL;
	caching_oper.truncate = NULL;
	caching_oper.utime = NULL;
	caching_oper.write = NULL;
	caching_oper.statfs = NULL;
	caching_oper.fsync = NULL;
	caching_oper.setxattr = NULL;
	caching_oper.getxattr = NULL;
	caching_oper.listxattr = NULL;
	caching_oper.removexattr = NULL;
	caching_oper.fsyncdir = NULL;
	caching_oper.create = NULL;
	caching_oper.ftruncate = NULL;
}

int main(int argc, char* argv[]) 
{

	int argv_3;
	int argv_4;
	try //check validation of num params
	{
		argv_3 = atoi(argv[3]);
		argv_4 = atoi(argv[4]);
	} catch (exception& e) {
		fprintf(stdout, USAGE_ERROR);
		exit(EXIT_SUCCESS);
	}
	//check number of params
	if (argc != 5 || !(argv_3 > 0) || !(argv_4 > 0)) 
	{
		fprintf(stdout, USAGE_ERROR);
		exit(EXIT_SUCCESS);
	}

	//check if the dir params are acually dir
	struct stat st;
	stat(argv[1], &st);
	if (!S_ISDIR(st.st_mode)) 
	{
		fprintf(stdout, USAGE_ERROR);
		exit(EXIT_SUCCESS);
	}
	stat(argv[2], &st);
	if (!S_ISDIR(st.st_mode)) 
	{
		fprintf(stdout, USAGE_ERROR);
		exit(EXIT_SUCCESS);
	}

	fs_context* context = (fs_context*) malloc(sizeof(fs_context));
	if (context == NULL) 
	{
		fprintf(stderr, MEMORY_ALLOC_FAILED);
		exit(EXIT_SUCCESS);
	}

	try {
		context->theCache = new Cache(argv_4, argv_3);
	} catch (exception& e) {
		fprintf(stderr, MEMORY_ALLOC_FAILED);
		free(context);
		exit(EXIT_SUCCESS);
	}

	context->rootdirPath = realpath(argv[1], NULL);
	if (context->rootdirPath == NULL) 
	{
		fprintf(stderr, REAL_PATH_FAILED);
		context->theCache->freeCache();
		delete (context->theCache);
		free(context);
		exit(EXIT_SUCCESS);
	}
	context->numberOfBlock = argv_3;
	context->blockSize = argv_4;
	char real_path[PATH_MAX];
	string temp = (string) context->rootdirPath + (string) LOGFILE;
	try {
		strcpy(real_path, temp.c_str());
	} catch (exception& e) {
		fprintf(stderr, STRCPY_FAILED);
		context->theCache->freeCache();
		delete (context->theCache);
		free(context);
		exit(EXIT_SUCCESS);
	}

	context->logFile = fopen(real_path, "a");
	if (context->logFile == NULL) {
		fprintf(stderr, LOGFILE_OPEN_FAILED);
		context->theCache->freeCache();
		delete (context->theCache);
		free(context);
		exit(EXIT_SUCCESS);
	}
	try {
		setvbuf(context->logFile, NULL, _IOLBF, 0);
	} catch (exception& e) {
		fprintf(stderr, INIT_BUFFER_FAILED);
		context->theCache->freeCache();
		delete (context->theCache);
		free(context);
		exit(EXIT_SUCCESS);
	}

	init_caching_oper();
	argv[1] = argv[2];
	for (int i = 2; i < (argc - 1); i++) 
	{
		argv[i] = NULL;
	}
	argv[2] = (char*) "-s";
	argc = 3;
	int fuse_stat = fuse_main(argc, argv, &caching_oper, context);
	free(context);
	return fuse_stat;
}

