GITHOST?=https://github.com
LPATT="   make %-23s- %s\n"

help all:
	@echo "Makefile help:"
	@echo
.if exists(${.CURDIR}/.git)
	@printf ${LPATT} "git-clean"		"remove .git dir"
	@printf ${LPATT} "profiles-update"	"Update vm-config files from the net"
.else
	@printf ${LPATT} "profiles-create"         "fetch initial vm-config files from the net"
.endif
	@echo

git-clean:
	@/bin/test ! -d ${.CURDIR}/.git || \
		(rm -rf ${.CURDIR}/.git; /usr/bin/true)

git-check:
	@/usr/bin/which -s git || \
		(echo "git must be present on the system to use this option."; /usr/bin/false)

profiles-check: git-check
	@/bin/test ! -d ${.CURDIR}/tmp-Makefile-config || \
		/bin/rm -rf ${.CURDIR}/tmp-Makefile-config
	@/bin/test ! -d ${.CURDIR}/.git || \
		(echo "Error: ${.CURDIR}/.git already exists"; /usr/bin/false)

profiles-create-repo: profiles-check
	@mkdir -p ${.CURDIR}/tmp-Makefile-config
	@cd ${.CURDIR}/tmp-Makefile-config && git init
	@cd ${.CURDIR}/tmp-Makefile-config && \
		git remote add origin ${GITHOST}/cbsd/cbsd-vmprofiles.git
	@mv ${.CURDIR}/tmp-Makefile-config/.git ${.CURDIR}/.git
	/bin/rm -rf ${.CURDIR}/tmp-Makefile-config
	git fetch origin
	git branch master remotes/origin/master
	git reset --hard
	git pull

profiles-create: profiles-create-repo

remove-dup-profiles:
	@/usr/bin/find ${.CURDIR} -type f -depth 1 -maxdepth 1 -name vm-\* |while read _d; do \
		basename=$$(basename $${_d}) ; \
		[ -f ${.CURDIR}/defaults/$$basename ] || continue ; \
		cmp ${.CURDIR}/$$basename ${.CURDIR}/defaults/$$basename && rm -f ${.CURDIR}/$$basename; \
	done

git-pull:
	git reset --hard
	git pull

profiles-update: git-pull remove-dup-profiles

profiles-upgrade: git-clean profiles-create remove-dup-profiles
