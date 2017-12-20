#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "util.h"

/* Generate the Redis "Run ID", a SHA1-sized random number that identifies a
 * given execution of Redis, so that if you are talking with an instance
 * having run_id == A, and you reconnect and it has run_id == B, you can be
 * sure that it is either a different instance or it was restarted. */
void getRandomHexChars(char *p, unsigned int len) {
    FILE *fp = fopen("/dev/urandom","r");
    char *charset = "0123456789abcdef";
    unsigned int j;
    
    if (fp == NULL || fread(p,len,1,fp) == 0) {
        /* If we can't read from /dev/urandom, do some reasonable effort
         * in order to create some entropy, since this function is used to
         * generate run_id and cluster instance IDs */
        char *x = p;
        unsigned int l = len;
        struct timeval tv;
        pid_t pid = getpid();

        /* Use time and PID to fill the initial array. */
        gettimeofday(&tv,NULL);
        if (l >= sizeof(tv.tv_usec)) {
            memcpy(x,&tv.tv_usec,sizeof(tv.tv_usec));
            l -= sizeof(tv.tv_usec);
            x += sizeof(tv.tv_usec);
        }
        if (l >= sizeof(tv.tv_sec)) {
            memcpy(x,&tv.tv_sec,sizeof(tv.tv_sec));
            l -= sizeof(tv.tv_sec);
            x += sizeof(tv.tv_sec);
        }
        if (l >= sizeof(pid)) {
            memcpy(x,&pid,sizeof(pid));
            l -= sizeof(pid);
            x += sizeof(pid);
        }
        /* Finally xor it with rand() output, that was already seeded with
         * time() at startup. */
        for (j = 0; j < len; j++)
            p[j] ^= rand();
    }
    /* Turn it into hex digits taking just 4 bits out of 8 for every byte. */
    for (j = 0; j < len; j++)
        p[j] = charset[p[j] & 0x0F];
    if (fp) fclose(fp);
}

/* Given the filename, return the absolute path as an SDS string, or NULL
 * if it fails for some reason. Note that "filename" may be an absolute path
 * already, this will be detected and handled correctly.
 *
 * The function does not try to normalize everything, but only the obvious
 * case of one or more "../" appearning at the start of "filename"
 * relative path. */
sds getAbsolutePath(char *filename) {
    char cwd[1024];
    sds abspath;
    sds relpath = sdsnew(filename);

    relpath = sdstrim(relpath," \r\n\t");
    if (relpath[0] == '/') return relpath; /* Path is already absolute. */

    /* If path is relative, join cwd and relative path. */
    if (getcwd(cwd,sizeof(cwd)) == NULL) {
        sdsfree(relpath);
        return NULL;
    }
    abspath = sdsnew(cwd);
    if (sdslen(abspath) && abspath[sdslen(abspath)-1] != '/')
        abspath = sdscat(abspath,"/");

    /* At this point we have the current path always ending with "/", and
     * the trimmed relative path. Try to normalize the obvious case of
     * trailing ../ elements at the start of the path.
     *
     * For every "../" we find in the filename, we remove it and also remove
     * the last element of the cwd, unless the current cwd is "/". */
    while (sdslen(relpath) >= 3 &&
           relpath[0] == '.' && relpath[1] == '.' && relpath[2] == '/')
    {
        sdsrange(relpath,3,-1);
        if (sdslen(abspath) > 1) {
            char *p = abspath + sdslen(abspath)-2;
            int trimlen = 1;

            while(*p != '/') {
                p--;
                trimlen++;
            }
            sdsrange(abspath,0,-(trimlen+1));
        }
    }

    /* Finally glue the two parts together. */
    abspath = sdscatsds(abspath,relpath);
    sdsfree(relpath);
    return abspath;
}