#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include "ext2_fs.h"
#include "read_ext2.h"

typedef struct jpgStore {
    uint inodeNumber;
    struct ext2_inode *iptr;
} jpgStore;

/**
 * @brief Returns whether this buffer contains the 
 * magic nums for a jpg
 * @param buffer - buffer to check 
 * @return Returns ? 1 : 0
 */
int isjpg(char buffer[block_size]) {
    return (buffer[0] == (char)0xff &&
    buffer[1] == (char)0xd8 &&
    buffer[2] == (char)0xff &&
    (buffer[3] == (char)0xe0 ||
    buffer[3] == (char)0xe1 ||
    buffer[3] == (char)0xe8));
}

// refactor to not have to take in inode_num? i.e. basic file creation?
void createfile(int inode_num, char* dir, char create_me[512]) {
    char filename[16];  // create file name to dump data blocks to
    if (dir[strlen(dir) - 1] != '/') strcat(dir, "/"); // add slash if needed
    snprintf(filename, sizeof(filename), "file-%d.jpg", inode_num);
    strcat(create_me, dir);
    strcat(create_me, filename);
}

void createfile2(char* name, char* dir, char create_me[512]) {
    if (dir[strlen(dir) - 1] != '/') strcat(dir, "/"); // add slash if needed
    strcat(create_me, dir);
    strcat(create_me, name);
}

void writeimgtofile(FILE* fp, struct ext2_inode *inode, int fd){
    int num_ind_blocks = 256;
    char buffer[block_size];
    lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
    read(fd, buffer, block_size);

    int partial_block = (int)(inode->i_size) % block_size;  // partial block at the end
    int full_blocks = ((int)(inode->i_size) / block_size); // full blocks of img

    // dump file contents into created file:
    int j;
    for (j = 0; (j < EXT2_NDIR_BLOCKS) && (j < full_blocks); j++) {
        lseek(fd, BLOCK_OFFSET(inode->i_block[j]), SEEK_SET);
        read(fd, buffer, block_size);
        fwrite(buffer, 1, block_size, fp);
    }
    if (j < EXT2_NDIR_BLOCKS) {
        if (partial_block > 0) {
            lseek(fd, BLOCK_OFFSET(inode->i_block[j]), SEEK_SET);
            read(fd, buffer, partial_block);
            fwrite(buffer, 1, partial_block, fp);
        }
    } else {  // more full blocks to iterate over
        int ind_blocks[num_ind_blocks];
        lseek(fd, BLOCK_OFFSET(inode->i_block[EXT2_IND_BLOCK]), SEEK_SET);
        read(fd, ind_blocks, block_size);  // ind_blocks now holds all the pointers to the data blocks
        int iterations = (int)(inode->i_size) - (EXT2_NDIR_BLOCKS * block_size);
        iterations /= block_size;
        for (j = 0; (j < num_ind_blocks) && (j < iterations); j++) {  // iterate over ind_blocks or until all data has been written
            lseek(fd, BLOCK_OFFSET(ind_blocks[j]), SEEK_SET);
            read(fd, buffer, block_size);
            fwrite(buffer, 1, block_size, fp);
        }
        if (j < num_ind_blocks) {
            if (partial_block) {
                lseek(fd, BLOCK_OFFSET(ind_blocks[j]), SEEK_SET);
                read(fd, buffer, partial_block);
                fwrite(buffer, 1, partial_block, fp);
            }
        } else {
            // do same as above but with doubly indirect blocks
            lseek(fd, BLOCK_OFFSET(inode->i_block[EXT2_DIND_BLOCK]), SEEK_SET);
            read(fd, ind_blocks, block_size);
            int done = 0;
            int num_blocks = (int)(inode->i_size) / block_size;
            num_blocks -= (num_ind_blocks + EXT2_NDIR_BLOCKS);
            for (j = 0; j < num_ind_blocks; j++) {
                int dind_blocks[256];
                lseek(fd, BLOCK_OFFSET(ind_blocks[j]), SEEK_SET);
                read(fd, dind_blocks, block_size);
                for (int k = 0; k < num_ind_blocks; k++) {
                    if (k == num_blocks) {
                        done = k;
                        break;
                    }
                    lseek(fd, BLOCK_OFFSET(dind_blocks[k]), SEEK_SET);
                    read(fd, buffer, block_size);
                    fwrite(buffer, 1, block_size, fp);
                }
                num_blocks -= num_ind_blocks;
                if (done) {
                    if (partial_block) {
                        lseek(fd, BLOCK_OFFSET(dind_blocks[done]), SEEK_SET);
                        read(fd, buffer, partial_block);
                        fwrite(buffer, 1, partial_block, fp);
                    }
                    break;
                }
            }
        }
    }
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("[Error] expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}
	
	int fd;
	fd = open(argv[1], O_RDONLY);    /* open disk image */

	if (opendir(argv[2])) {  // ensure directory passed in doesn't exist
		printf("[Warning] %s already exsists.\n[Hint] try> make clean; make\n", argv[2]);
		exit(1);
	} else {  // make the directory
		int ret = mkdir(argv[2], S_IRWXU);
		if (ret == -1) {
			printf("[Error] Could not make directory\n");
			exit(1);
		}
	}
	
	ext2_read_init(fd);
	struct ext2_super_block super;
	struct ext2_group_desc group;

    jpgStore jpgInodes[num_groups * inodes_per_group];
    int jpgInodesSize = 0;
    int group_num = 0;  // only look at first group_num
    read_super_block(fd, group_num, &super);
    read_group_desc(fd, group_num, &group);
    off_t start_inode_table = locate_inode_table(group_num, &group);
    uint total_blocks = inodes_per_group / inodes_per_block;

    for (unsigned int inode_num = 0; inode_num < total_blocks * inodes_per_block; inode_num++) { // iter over inodes in group
        // printf("[Debug] inode %u\n", inode_num);
        struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
        read_inode(fd, group_num, start_inode_table, inode_num, inode);

        int shouldFree = 1;
        // check if reg file
        if (S_ISREG(inode->i_mode)) {
            // printf("[Debug] checking inode %d is jpg\n", inode_num);
            char buffer[block_size];
            lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
            read(fd, buffer, block_size);

            if (isjpg(buffer)) {
                shouldFree = 0;
                jpgInodes[jpgInodesSize].inodeNumber = (uint)(inode_num);
                jpgInodes[jpgInodesSize++].iptr = inode;
                // printf("\t[Debug] found a jpg!\n"); 
                char pathname[512] = "";
                createfile(inode_num, argv[2], pathname);
                FILE* fp = fopen(pathname, "a");
                if (fp == 0) {
                    printf("Error opening file\n");
                    exit(1);
                }
                writeimgtofile(fp, inode, fd);

                fclose(fp);
            }
        }
        if (shouldFree)
            free(inode);
    }

    for (unsigned int inode_num = 0; inode_num < total_blocks * inodes_per_block; inode_num++) { // iter over inodes in group
        // printf("[Debug] inode %u\n", inode_num);
        struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
        read_inode(fd, group_num, start_inode_table, inode_num, inode);

        // check if dir file
        if (S_ISDIR(inode->i_mode)) {
            char dirIter[block_size];
            lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);  // all directory information stored in first i_block
            read(fd, dirIter, block_size);

            int bytesTraversed = 0;
            struct ext2_dir_entry *cur;
            while (bytesTraversed < (int)block_size) {
                cur = (struct ext2_dir_entry*) & (dirIter[bytesTraversed]);
                for (int i = 0; i < jpgInodesSize; i++) {
                    // printf("Checking for %u\n", (int)((cur->inode) & 0xFF));
                    if (cur->inode == jpgInodes[i].inodeNumber) {
                        char filename[512] = "";
                        char name [EXT2_NAME_LEN];
                        memset(name, 0, EXT2_NAME_LEN * 2);
                        int nameLen = cur->name_len & 0xFF;
                        strcpy(name, cur->name);
                        name[nameLen] = '\0';
                        createfile2(name, argv[2], filename);

                        FILE *fp = fopen(filename, "a");
                        if (!fp) {
                            printf("Error opening file\n");
                            exit(1);
                        }

                        writeimgtofile(fp, jpgInodes[i].iptr, fd);
                    }
                }
                
                int nameLen = cur->name_len & 0xFF;
                nameLen += 8 + (4 - (nameLen % 4)) % 4;
                bytesTraversed += nameLen;
            }
        }
        free(inode);
    }

    for (int i = 0; i < jpgInodesSize; i++) {
        free(jpgInodes[i].iptr);
    }
	close(fd);
}
