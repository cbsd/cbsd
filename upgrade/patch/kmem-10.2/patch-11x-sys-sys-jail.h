-- -jail.h - orig 2015 - 08 - 09 15 : 06 : 54.451544000 + 0300 ++ +jail.h 2015 -
    08 - 09 15 : 08 : 49.752879000 + 0300 @ @-232,
    7 + 232,
    9 @ @
#define PR_ALLOW_MOUNT_PROCFS 0x0400
#define PR_ALLOW_MOUNT_TMPFS 0x0800
#define PR_ALLOW_MOUNT_FDESCFS 0x1000
    -#define PR_ALLOW_ALL 0x1fff +
    #define PR_ALLOW_DEV_IO_ACCESS 0x2000 +
    #define PR_ALLOW_DEV_DRI_ACCESS 0x4000 +
    #define PR_ALLOW_ALL 0x7fff
 
 /*
  * OSD methods
