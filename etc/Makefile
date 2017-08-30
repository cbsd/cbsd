GITHOST?=https://github.com
LPATT="   make %-23s- %s\n"

help all:
	@echo "Makefile help:"
	@echo
.if exists(${.CURDIR}/.git)
	@printf ${LPATT} "profiles-update"         "Update vm-config files from the net"
.else
	@printf ${LPATT} "profiles-create"         "fetch initial vm-config files from the net"
.endif
	@echo

git-check:
	@/usr/bin/which -s git || \
		(echo "git must be present on the system to use this option."; /usr/bin/false)

profiles-check: git-check
	@/bin/test ! -d ${.CURDIR}/tmp-Makefile-config || \
		/bin/rm -rf ${.CURDIR}/tmp-Makefile-config
	@/bin/test ! -d ${.CURDIR}/.git || \
		(echo "Error: ${.CURDIR}/.git already exists"; /usr/bin/false)

profiles-create-repo: profiles-check
	@echo "If problems occur you may have to rm -rf .git and try again."
	@echo
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

profiles-update:
	git reset --hard
	git pull
