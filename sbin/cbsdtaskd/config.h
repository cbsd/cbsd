#ifndef DEBUGGING
#define DEBUGGING 1	/* 1 or 0 -- do you want debugging code built in? */
#endif

			/*
			 * choose one of these mailer commands.  some use
			 * /bin/mail for speed; it makes biff bark but doesn't
			 * do aliasing.  sendmail does do aliasing but is
			 * a hog for short messages.  aliasing is not needed
			 * if you make use of the MAILTO= feature in crontabs.
			 * (hint: MAILTO= was added for this reason).
			 */

#define MAILFMT "%s -FCronDaemon -odi -oem -oi -t"
#define MAILARG _PATH_SENDMAIL
#define SYSLOG
