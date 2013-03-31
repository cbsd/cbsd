//#include <sys/param.h>

//#include <pwd.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>

//#include "config.h"

struct passwd *
pw_dup(const struct passwd *pw) {
	char		*cp;
	size_t		 nsize, psize, csize, gsize, dsize, ssize, total;
	struct passwd	*newpw;

	/* Allocate in one big chunk for easy freeing */
	total = sizeof(struct passwd);
	if (pw->pw_name) {
		nsize = strlen(pw->pw_name) + 1;
		total += nsize;
	}
	if (pw->pw_passwd) {
		psize = strlen(pw->pw_passwd) + 1;
		total += psize;
	}
#ifdef LOGIN_CAP
	if (pw->pw_class) {
		csize = strlen(pw->pw_class) + 1;
		total += csize;
	}
#endif /* LOGIN_CAP */
	if (pw->pw_gecos) {
		gsize = strlen(pw->pw_gecos) + 1;
		total += gsize;
	}
	if (pw->pw_dir) {
		dsize = strlen(pw->pw_dir) + 1;
		total += dsize;
	}
	if (pw->pw_shell) {
		ssize = strlen(pw->pw_shell) + 1;
		total += ssize;
	}
	if ((cp = malloc(total)) == NULL)
		return (NULL);
	newpw = (struct passwd *)cp;

	/*
	 * Copy in passwd contents and make strings relative to space
	 * at the end of the buffer.
	 */
	(void)memcpy(newpw, pw, sizeof(struct passwd));
	cp += sizeof(struct passwd);
	if (pw->pw_name) {
		(void)memcpy(cp, pw->pw_name, nsize);
		newpw->pw_name = cp;
		cp += nsize;
	}
	if (pw->pw_passwd) {
		(void)memcpy(cp, pw->pw_passwd, psize);
		newpw->pw_passwd = cp;
		cp += psize;
	}
#ifdef LOGIN_CAP
	if (pw->pw_class) {
		(void)memcpy(cp, pw->pw_class, csize);
		newpw->pw_class = cp;
		cp += csize;
	}
#endif /* LOGIN_CAP */
	if (pw->pw_gecos) {
		(void)memcpy(cp, pw->pw_gecos, gsize);
		newpw->pw_gecos = cp;
		cp += gsize;
	}
	if (pw->pw_dir) {
		(void)memcpy(cp, pw->pw_dir, dsize);
		newpw->pw_dir = cp;
		cp += dsize;
	}
	if (pw->pw_shell) {
		(void)memcpy(cp, pw->pw_shell, ssize);
		newpw->pw_shell = cp;
		cp += ssize;
	}

	return (newpw);
}

