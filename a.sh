find . -type f -print | while read f; do

	head -n1 ${f} |grep -q /bin/sh > /dev/null 2>&1
	ret=$?
	[ $ret -eq 0 ] && echo $f

done

