char *cbsdpidfile();
char *cbsdcrondir();
char *cbsdspooldir();

char *cbsdpidfile()
{
char *dst;

    dst=malloc(strlen(workdir)+strlen(PIDDIR)+strlen(PIDFILE)+1);
    strcpy(dst,workdir);
    strcat(dst,PIDDIR);
    strcat(dst,"/");
    strcat(dst,PIDFILE);
    return dst;
}

char *cbsdcrondir()
{
char *dst;

    dst=malloc(strlen(workdir)+strlen(CRONDIR)+1);
    strcpy(dst,workdir);
    strcat(dst,CRONDIR);

    return dst;
}

char *cbsdspooldir()
{
char *dst;

    dst=malloc(strlen(workdir)+strlen(CRONDIR)+strlen(SPOOL_DIR)+1);
    strcpy(dst,workdir);
    strcat(dst,CRONDIR);
    strcat(dst,"/");
    strcat(dst,SPOOL_DIR);

    return dst;
}

#define CBSD_DEFPATH "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin"
