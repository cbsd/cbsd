-- -jail.h - orig 2016 - 07 - 08 10 : 52 : 17.168839000 + 0300 ++ +jail.h 2016 -
    10 - 14 22 : 54 : 46.064958000 + 0300 @ @-230,
    7 + 230,
    9 @ @
#define PR_ALLOW_MOUNT_FDESCFS 0x1000
#define PR_ALLOW_MOUNT_LINPROCFS 0x2000
#define PR_ALLOW_MOUNT_LINSYSFS 0x4000
    -#define PR_ALLOW_ALL 0x7fff +
    #define PR_ALLOW_DEV_IO_ACCESS 0x8000 +
    #define PR_ALLOW_DEV_DRI_ACCESS 0x10000 +
    #define PR_ALLOW_ALL 0x7ffff
 
 /*
  * OSD methods
