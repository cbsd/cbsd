#!/bin/sh
# cixinit and cixinit -j. No jail required.
#   sh tests/internal/cixinit.sh
pgm="${0##*/}"
progdir="${0%/*}"

set -e
. "${progdir}/../config.conf"
set +e

# $1 is the script body. The remaining arguments are its positional parameters.
# cixinit reads those parameters; -j is a flag of cixinit itself.
cbsd_stdout()
{
	_body=$1
	shift
	${CIX_BIN} -c "${_body}" cixinit "$@" 2>/dev/null | grep -v '^Warning:' || true
}

testPlainAssign()
{
	body='CIXARG="jname"
CIXOPTARG="ip4_addr"
cixinit
printf "jname=[%s]\n" "$jname"
printf "ip4=[%s]\n" "$ip4_addr"
printf "ojname=[%s]\n" "$ojname"
printf "foo=[%s]\n" "$foo"
printf "other=[%s]\n" "$CIX_OTHER_ARGS"
printf "json=[%s]\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}" jname=test ip4_addr=192.168.1.1 foo=bar plain)
	assertEquals "declared names, other args, no JSON" \
"jname=[test]
ip4=[192.168.1.1]
ojname=[test]
foo=[]
other=[foo=bar plain]
json=[]" "${out}"
}

testLastWins()
{
	body='CIXARG="jname"
cixinit
printf "%s\n" "$jname"'
	out=$(cbsd_stdout "${body}" jname=one jname=two)
	assertEquals "scalar keeps the last value" "two" "${out}"
}

testMandatory()
{
	body='CIXARG="jname"
cixinit
printf "%s\n" "$jname"'
	err=$(${CIX_BIN} -c "${body}" cixinit 2>&1 >/dev/null)
	rc=$?
	assertFalse "missing jname fails" "${rc}"
	assertContains "mandatory message" "${err}" "param jname is mandatory"
}

testSkipRepeated()
{
	body='CIXARG="jname"
CIXOPTARG="environment"
CIX_INIT_SKIP="environment"
cixinit
printf "jname=[%s]\n" "$jname"
printf "environment=[%s]\n" "$environment"
printf "other=[%s]\n" "$CIX_OTHER_ARGS"'
	out=$(cbsd_stdout "${body}" jname=test environment=PATH=/bin environment=HOME=/home/user)
	assertEquals "skipped name stays out of the shell variable" \
"jname=[test]
environment=[]
other=[environment=PATH=/bin environment=HOME=/home/user]" "${out}"
}

testDashDash()
{
	body='CIXARG="jname"
CIXOPTARG="user"
cixinit
printf "jname=[%s]\n" "$jname"
printf "user=[%s]\n" "$user"
printf "other=[%s]\n" "$CIX_OTHER_ARGS"'
	out=$(cbsd_stdout "${body}" jname=test -- user=nobody /bin/sh)
	assertEquals "-- keeps the rest positional" \
"jname=[test]
user=[]
other=[-- user=nobody /bin/sh]" "${out}"
}

testWhere()
{
	body='CIXARG="jname"
cixinit
printf "jname=[%s]\n" "$jname"
printf "other=[%s]\n" "$CIX_OTHER_ARGS"
printf "cond=[%s]\n" "$CIXINIT_SQL_CONDITION"'
	out=$(cbsd_stdout "${body}" jname=test extra WHERE status=1)
	assertEquals "WHERE starts the SQL condition" \
"jname=[test]
other=[extra]
cond=[status=1]" "${out}"
}

testQuoteJoin()
{
	body='CIXOPTARG="sysrc runasap"
cixinit
printf "sysrc=[%s]\n" "$sysrc"
printf "runasap=[%s]\n" "$runasap"'
	out=$(cbsd_stdout "${body}" 'sysrc="hello' 'world"' runasap=1)
	assertEquals "quoted value is joined until the closing quote" \
"sysrc=[hello world]
runasap=[1]" "${out}"
}

testJsonDocument()
{
	body='CIXARG="jname"
CIXOPTARG="ip4_addr environment"
CIXMULTIARG="environment"
cixinit -j
printf "jname=[%s]\n" "$jname"
printf "environment=[%s]\n" "$environment"
printf "%s\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}" \
		jname=test \
		ip4_addr=192.168.1.1 \
		environment=PATH=/bin \
		environment=HOME=/home/user \
		-- /bin/sh -c 'echo abcd')
	assertEquals "kw, multi array, argv" \
"jname=[test]
environment=[HOME=/home/user]
{\"kw\":{\"jname\":\"test\",\"ip4_addr\":\"192.168.1.1\",\"environment\":[\"PATH=/bin\",\"HOME=/home/user\"]},\"argv\":[\"/bin/sh\",\"-c\",\"echo abcd\"]}" "${out}"
}

testJsonNoQuoteJoin()
{
	body='CIXOPTARG="sysrc runasap"
cixinit -j
printf "sysrc=[%s]\n" "$sysrc"
printf "runasap=[%s]\n" "$runasap"
printf "%s\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}" 'sysrc="hello' 'world"' runasap=1)
	assertEquals "-j keeps each parameter whole" \
"sysrc=[\"hello]
runasap=[1]
{\"kw\":{\"sysrc\":\"\\\"hello\",\"runasap\":\"1\"},\"argv\":[\"world\\\"\"]}" "${out}"
}

testJsonSpecialChars()
{
	body='CIXARG="jname"
cixinit -j
printf "%s\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}" 'jname=a"b\c')
	assertEquals "serializer escapes quotes and backslashes" \
'{"kw":{"jname":"a\"b\\c"},"argv":[]}' "${out}"
}

testJsonDashDash()
{
	body='CIXARG="jname"
CIXOPTARG="user"
cixinit -j
printf "user=[%s]\n" "$user"
printf "%s\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}" jname=test -- user=nobody /bin/sh)
	assertEquals "words after -- are argv elements" \
"user=[]
{\"kw\":{\"jname\":\"test\"},\"argv\":[\"user=nobody\",\"/bin/sh\"]}" "${out}"
}

testJsonWhere()
{
	body='CIXARG="jname"
cixinit -j
printf "cond=[%s]\n" "$CIXINIT_SQL_CONDITION"
printf "%s\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}" jname=test extra WHERE status=1)
	assertEquals "WHERE is neither kw nor argv" \
"cond=[status=1]
{\"kw\":{\"jname\":\"test\"},\"argv\":[\"extra\"]}" "${out}"
}

testJsonEmpty()
{
	body='cixinit -j
printf "%s\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}")
	assertEquals "no arguments" '{"kw":{},"argv":[]}' "${out}"
}

testJsonOverlay()
{
	body='CIXARG="jname"
CIXOPTARG="ip4_addr"
CIXMULTIARG="environment"
cixinit -j
printf "%s\n" "$CIX_JSON_ARGS"
CIXARG="jname"
CIXOPTARG="ip4_addr"
set -- jname=jail1
cixinit -j
printf "jname=[%s]\n" "$jname"
printf "ip4=[%s]\n" "$ip4_addr"
printf "%s\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}" \
		'jname=*' \
		ip4_addr=192.168.1.1 \
		environment=PATH=/bin \
		-- /bin/sh -c 'echo abcd')
	assertEquals "later jname replaces the restored one" \
'{"kw":{"jname":"*","ip4_addr":"192.168.1.1","environment":["PATH=/bin"]},"argv":["/bin/sh","-c","echo abcd"]}
jname=[jail1]
ip4=[192.168.1.1]
{"kw":{"jname":"jail1","ip4_addr":"192.168.1.1","environment":["PATH=/bin"]},"argv":["/bin/sh","-c","echo abcd"]}' "${out}"
}

testJsonOverlayAppend()
{
	body='CIXARG="jname"
CIXMULTIARG="environment"
cixinit -j
set -- environment=HOME=/home/user /extra
cixinit -j
printf "%s\n" "$CIX_JSON_ARGS"'
	out=$(cbsd_stdout "${body}" \
		'jname=*' \
		environment=PATH=/bin \
		-- /bin/sh)
	assertEquals "multi value and positional argument are appended" \
'{"kw":{"jname":"*","environment":["PATH=/bin","HOME=/home/user"]},"argv":["/bin/sh","/extra"]}' "${out}"
}

. "${progdir}/../shunit2"
