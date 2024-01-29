GITHOST?=https://github.com

help all:
	@echo "Makefile help:"
	@echo
	@if test -d $(CURDIR)/.git; \
	then \
	echo "    \"git-clean\"             \"remove .git dir\""; \
	echo "    \"profiles-update\"       \"Update vm-config files from the net\""; \
	else \
	echo "    \"profiles-create\"         \"fetch initial vm-config files from the net\""; \
	fi
	@echo

git-clean:
	@/bin/test ! -d ${.CURDIR}/.git || \
		(cd ${.CURDIR} && git ls-files | xargs rm -f; rm -rf ${.CURDIR}/.git; /usr/bin/true)

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
