-- -jail.h - orig 2017 - 06 - 06 23 : 05 : 15.757232000 + 0300 ++ +jail.h 2017 -
    07 - 04 01 : 10 : 55.671478000 + 0300 @ @-231,
    7 + 231,
    9 @ @
#define PR_ALLOW_MOUNT_LINPROCFS 0x2000
#define PR_ALLOW_MOUNT_LINSYSFS 0x4000
#define PR_ALLOW_RESERVED_PORTS 0x8000
    -#define PR_ALLOW_ALL 0xffff +
    #define PR_ALLOW_DEV_IO_ACCESS 0x10000 +
    #define PR_ALLOW_DEV_DRI_ACCESS 0x20000 +
    #define PR_ALLOW_ALL 0x3ffff
 
 /*
  * OSD methods
