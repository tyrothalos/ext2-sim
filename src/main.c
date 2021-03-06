#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "project.h"

/* INITIALIZATION FUNCTIONS */

void init()
{
	for (int i = 0; i < NPROC; i++) {
		proc[i].uid = i;
		proc[i].gid = i;
		proc[i].pid = i + 1;
		proc[i].cwd = 0;
	}

	for (int i = 0; i < NMINODES; i++) {
		m_inodes[i].ref_count = 0;
	}

	root = 0;
}

int mount_root(char *rootname)
{
	int fd = open(rootname, O_RDWR);
	if (fd < 0) {
		printf("open '%s' failed\n", rootname);
		return -1;
	}

	// get super block and check for EXT2 FS
	SUPER s = get_super(fd);
	if (s.s_magic != SUPER_MAGIC) {
		printf("NOT an EXT2 FS\n");
		printf("s_magic = %x\n", s.s_magic);
		close(fd);
		return -2;
	}

	GROUP g = get_group(fd);

	// initialize the root and process working directories
	root = iget(fd, ROOT_INODE);
	for (int i = 0; i < NPROC; i++) {
		proc[i].cwd = iget(fd, ROOT_INODE);
	}
	// set running for easier reading
	running = &proc[0];

	// allocate mount table entry
	MOUNT *m = &mounttab[0];
	m->mounted_inode = root;
	m->dev = fd;
	m->nblocks = s.s_blocks_count;
	m->ninodes = s.s_inodes_count;
	m->bmap = g.bg_block_bitmap;
	m->imap = g.bg_inode_bitmap;
	m->iblk = g.bg_inode_table;
	strcpy(m->name, rootname);
	strcpy(m->mount_name, "/");

	root->mounted = 0;
	root->mountptr = m;

	printf("SUPER magic: %x\n", s.s_magic);
	printf("bmap: %d, imap: %d, iblk: %d\n",
			m->bmap, m->imap, m->iblk);
	printf("nblocks: %d, ninodes: %d\n",
			m->nblocks, m->ninodes);
	return 0;
}

int main(int argc, char *argv[], char *env[])
{
	int r = 0;
	if (argc < 2) {
		printf("usage: simulator <device>\n");
	} else if (init(), mount_root(argv[1]) < 0) {
		r = -1;
	} else {
		// begin main loop; takes user input
		char input[256];
		do {
			printf("proc uid: %d $ ", running->uid);
			fgets(input, 256, stdin);
			input[strcspn(input, "\r\n")] = 0;
		} while (execute(input));
		printf("Quitting filesystem.\n");
	}
	return r;
}

