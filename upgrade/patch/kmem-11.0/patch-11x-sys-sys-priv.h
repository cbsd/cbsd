-- -priv.h - orig 2016 - 10 - 15 12 : 59 : 09.282547000 + 0300 ++ +priv.h 2016 -
    10 - 15 12 : 58 : 52.000000000 + 0300 @ @-504,
    11 + 504,
    12 @ @ * /
#define PRIV_KMEM_READ 680  /* Open mem/kmem for reading. */
#define PRIV_KMEM_WRITE 681 /* Open mem/kmem for writing. */
	+#define PRIV_DRI_DRIVER 682

    /*
     * Track end of privilege list.
     */
    - #define _PRIV_HIGHEST 682 +
    #define _PRIV_HIGHEST 683
 
 /*
  * Validate that a named privilege is known by the privilege system.  Invalid
