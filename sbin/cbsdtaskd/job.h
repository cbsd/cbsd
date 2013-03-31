typedef	struct _job {
	struct _job	*next;
	entry		*e;
	user		*u;
} job;

static job	*jhead = NULL, *jtail = NULL;

void
job_add(entry *e, user *u) {
	job *j;

	/* if already on queue, keep going */
	for (j = jhead; j != NULL; j = j->next)
		if (j->e == e && j->u == u)
			return;

	/* build a job queue element */
	if ((j = (job *)malloc(sizeof(job))) == NULL)
		return;
	j->next = NULL;
	j->e = e;
	j->u = u;

	/* add it to the tail */
	if (jhead == NULL)
		jhead = j;
	else
		jtail->next = j;
	jtail = j;
}

int
job_runqueue(void) {
	job *j, *jn;
	int run = 0;

	for (j = jhead; j; j = jn) {
		do_command(j->e, j->u);
		jn = j->next;
		free(j);
		run++;
	}
	jhead = jtail = NULL;
	return (run);
}
