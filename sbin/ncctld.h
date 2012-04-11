#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

// command session mode
#define INIT 0   // init stage
#define INTERACTIVE 1 // interactive mode for "set, unset" command
#define ANY 2 // cmd for any mode

#define CREATE(result, type, number)  do {\
    if (!((result) = (type *) calloc ((number), sizeof(type))))\
    { perror("malloc failure"); abort(); } } while(0)

#define CLOSE_SOCKET(sock) close(sock);

#define REMOVE_FROM_LIST(item, head, next)      \
   if ((item) == (head))                \
      head = (item)->next;              \
   else {                               \
      temp = head;                      \
      while (temp && (temp->next != (item))) \
         temp = temp->next;             \
      if (temp)                         \
         temp->next = (item)->next;     \
   }                                    \


#define ACMD(name)  \
   void (name)( char *argument, int subcmd)

#define CMD_NAME (cmd_info[cmd].command)

#define VERSION "0.1"

#define MAXSTRINGLEN 1024
#define PREP_STR(str) str[strlen(str)-2]='\0'

#define PROMPT "node"

#define MAXARGLEN 1024
#define UTIL_ARG_PREF "--help" // execution postfix when 1st execution external tools
#define NODESIGNATURE "node"  //first char when script with --help executing


struct command_info {
    char *command;
    void (*command_pointer)
    (char * argument, int subcmd);
    int minimum_level;
    int subcmd;
};

// cmd with(out) arguments structure
struct arg_data {
    char arg[128];
    struct arg_data *prev, *next;
};

// external tools argument strucure
struct subarg_data {
    char arg[128];  //argname
    char data[1024]; //data
    struct subarg_data *next, *prev;
};

// allowrd path for external cmd
struct syspath_data {
    char *path;
};



