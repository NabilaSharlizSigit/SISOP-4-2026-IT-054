#define FUSE_USE_VERSION 28
#include <time.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>

char source_dir[1024];

static int xmp_getattr(const char *path, struct stat *stbuf){
	int res;
	char fpath[1024];

	if(strcmp(path, "/tujuan.txt") == 0){
		memset(stbuf, 0, sizeof(struct stat));
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = 66;
		return 0;
	}

	sprintf(fpath, "%s%s", source_dir, path);

	res = lstat(fpath, stbuf);

	if(res == -1) return -errno;
	return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){


    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;
    char fpath[1024];

    sprintf(fpath, "%s%s", source_dir, path);

    dp = opendir(fpath);

    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;

        memset(&st, 0, sizeof(st));

        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        if(filler(buf, de->d_name, &st, 0)) break;
    }
    filler(buf, "tujuan.txt", NULL, 0);
    closedir(dp);
    return 0;
}


static int xmp_open(const char *path, struct fuse_file_info *fi){
        int res;
        char fpath[1024];

	if(strcmp(path, "/tujuan.txt") == 0) return 0;
        sprintf(fpath, "%s%s", source_dir, path);

        res = open(fpath, fi->flags);

        if(res == -1) return -errno;
	close(res);
        return 0;
}


static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;
    (void) fi;

    if(strcmp(path, "/tujuan.txt") == 0){
	static char result[4096];
	char line[256];

	strcpy(result, "Tujuan Mas Amba: ");
	for(int i = 1; i <= 7; i++){
		char filepath[4096];
		snprintf(filepath, sizeof(filepath), "%s/%d.txt", source_dir, i);
		FILE *fp = fopen(filepath, "r");
		if(fp==NULL) continue;
		while(fgets(line, sizeof(line), fp)){
			char *p = strstr(line, "KOORD:");
			if(p != NULL){
			p += strlen("KOORD:");
            		while(*p == ' ') p++;
			p[strcspn(p, "\n")] = 0;
			strcat(result, p);
        		}
		}

		fclose(fp);
	}

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char waktu[16];

    strftime(waktu, sizeof(waktu), " %H:%M", t);
    strcat(result, waktu);
    int len = strlen(result);

    if(offset<len){
	if(offset + size > len) size = len - offset;
	memcpy(buf, result + offset, size);
	} else{
		size = 0;
	}
	return size;
    }

    char fpath[1024];

    sprintf(fpath, "%s%s", source_dir, path);

    fd = open(fpath, O_RDONLY);

    if (fd == -1) return -errno;

    res = pread(fd, buf, size, offset);

    if (res == -1) res = -errno;

    close(fd);

    return res;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_directory> <mount_directory>\n", argv[0]);
        return 1;
    }

    realpath(argv[1], source_dir);

    argv[1] = argv[2];
    argc--;

    return fuse_main(argc, argv, &xmp_oper, NULL);
}
