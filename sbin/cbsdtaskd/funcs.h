/* Notes:
 *	This file has to be included by cron.h after data structure defs.
 *	We should reorg this into sections by module.
 */

void		set_cron_uid(void),
		set_cron_cwd(void),
		load_database(cron_db *),
		open_logfile(void),
		sigpipe_func(void),
		job_add(entry *, user *),
		do_command(entry *, user *),
		link_user(cron_db *, user *),
		unlink_user(cron_db *, user *),
		free_user(user *),
		env_free(char **),
		unget_char(int, FILE *),
		free_entry(entry *),
		acquire_daemonlock(int),
		skip_comments(FILE *),
		log_it(const char *, int, const char *, const char *),
		log_close(void);

int		job_runqueue(void),
		set_debug_flags(const char *),
		get_char(FILE *),
		get_string(char *, int, FILE *, char *),
		swap_uids(void),
		swap_uids_back(void),
		load_env(char *, FILE *),
		cron_pclose(FILE *),
		glue_strings(char *, size_t, const char *, const char *, char),
		strcmp_until(const char *, const char *, char),
		allowed(const char * ,const char * ,const char *),
		strdtb(char *);

size_t		strlens(const char *, ...);

char		*env_get(char *, char **),
		*arpadate(time_t *),
		*mkprints(unsigned char *, unsigned int),
		*first_word(char *, char *),
		**env_init(void),
		**env_copy(char **),
		**env_set(char **, char *);

user		*load_user(int, struct passwd *, const char *),
		*find_user(cron_db *, const char *);

entry		*load_entry(FILE *, void (*)(), struct passwd *, char **);

FILE		*cron_popen(char *, char *, struct passwd *);

struct passwd	*pw_dup(const struct passwd *);

#ifndef HAVE_TM_GMTOFF
long		get_gmtoff(time_t *, struct tm *);
#endif


