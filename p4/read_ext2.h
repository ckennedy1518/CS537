#ifndef READ_EXT2
#define READ_EXT2
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "ext2_fs.h"

#define BASE_OFFSET 1024    /* locates beginning of the super block (first group) */
#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1)*block_size)

extern unsigned int block_size;		/* default 1kB block size */
extern unsigned int inodes_per_block;			/* number of inodes per block */
extern unsigned int itable_blocks;				/* size in blocks of the inode table */
extern unsigned int blocks_per_group;		/* number of blocks per block group */
extern unsigned int num_groups;				/* number of block groups in the image */
extern unsigned int inodes_per_group;

extern int debug;		//turn on/off debug prints

/* Read the first super block to initialize common variables */
void ext2_read_init( int fd );

/* Read the specified super block
 * fd - the disk image file descriptor
 * ngroup - which block group to access
 * super - where to put the super block   
 */
int read_super_block( int fd, int ngroup, struct ext2_super_block *super );

/* Read the group-descriptor in the specified block group
 * fd - the disk image file descriptor
 * ngroup - which block group to access
 * group - where to put the group-descriptor   
 */
void read_group_desc( int fd, int ngroup, struct ext2_group_desc *group );

/* Calculate the start address of the inode table in the specified group
 * ngroup - which block group to access
 * group - the first group-descriptor
 */
off_t locate_inode_table( int ngroup, const struct ext2_group_desc *group );

/* Calculate the start address of the data blocks in the specified group
 * ngroup - which block group to access
 * group - the first group-descriptor
 */
off_t locate_data_blocks(int ngroup, const struct ext2_group_desc *group);

/* Read an inode with specified inode number and group number
 * fd - the disk image file descriptor
 * ngroup - which block group to access
 * offest - offset to the start of the inode table
 * inode_no - the inode # to read
 * inode - where to put the inode
 */
void read_inode( int fd, int ngroup, off_t offset, int inode_no, struct ext2_inode *inode );  
#endif