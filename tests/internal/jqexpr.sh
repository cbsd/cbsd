#!/bin/sh
# ${var@jq} in quotes splits like "$@". No jail required.
#   sh tests/internal/jqexpr.sh
pgm="${0##*/}"
progdir="${0%/*}"

set -e
. "${progdir}/../config.conf"
set +e

cbsd_stdout()
{
	_body=$1
	shift
	${CIX_BIN} -c "${_body}" jqexpr "$@" 2>/dev/null | grep -v '^Warning:' || true
}

testQuotedElements()
{
	body='x="[\"a\", \"b c\", \"d e f\"]"
set -- "${x@.[]}"
printf "%s\n" "$1//$2//$3"
printf "%s\n" "$#"'
	out=$(cbsd_stdout "${body}")
	assertEquals "each jq output is one parameter" \
"a//b c//d e f
3" "${out}"
}

testQuotedEmpty()
{
	body='x="[]"
set -- "${x@.[]}"
printf "%s\n" "$#"'
	out=$(cbsd_stdout "${body}")
	assertEquals "no outputs leave no parameters" "0" "${out}"
}

testQuotedOne()
{
	body='x="[\"b c\"]"
set -- "${x@.[]}"
printf "%s\n" "$#"
printf "%s\n" "[$1]"'
	out=$(cbsd_stdout "${body}")
	assertEquals "one output stays one parameter" \
"1
[b c]" "${out}"
}

# Real call sites pass a single jq value, often through // empty.
# Assignments are not field-split. Quoted export/eval arguments must stay one word.

testAssignEmpty()
{
	body='row="{\"nic_parent\":null}"
nic_parent="${row@.nic_parent // empty}"
printf "[%s]\n" "$nic_parent"'
	out=$(cbsd_stdout "${body}")
	assertEquals "missing field assigns an empty string" "[]" "${out}"
}

testAssignValue()
{
	body='row="{\"nic_parent\":\"bridge 0\"}"
nic_parent="${row@.nic_parent // empty}"
printf "[%s]\n" "$nic_parent"'
	out=$(cbsd_stdout "${body}")
	assertEquals "assignment keeps spaces in one value" "[bridge 0]" "${out}"
}

testExportEmpty()
{
	body='row="{\"nic_hwaddr\":null}"
export "nic_hwaddr0=${row@.nic_hwaddr // empty}"
printf "[%s]\n" "$nic_hwaddr0"'
	out=$(cbsd_stdout "${body}")
	assertEquals "export of an empty field stays one argument" "[]" "${out}"
}

testExportValue()
{
	body='row="{\"nic_hwaddr\":\"aa:bb:cc\"}"
export "nic_hwaddr0=${row@.nic_hwaddr // empty}"
printf "[%s]\n" "$nic_hwaddr0"'
	out=$(cbsd_stdout "${body}")
	assertEquals "export keeps a single mac address" "[aa:bb:cc]" "${out}"
}

testEvalQuoted()
{
	body='q="{\"jname\":\"a b\",\"ip4_addr\":\"1.2.3.4\"}"
eval "jname0=\"${q@.jname}\""
eval "ips0=\"${q@.ip4_addr}\""
printf "[%s][%s]\n" "$jname0" "$ips0"'
	out=$(cbsd_stdout "${body}")
	assertEquals "eval assignment stays one parameter" "[a b][1.2.3.4]" "${out}"
}

testUnquotedAssign()
{
	body='q="{\"nodename\":\"n1\",\"ip\":\"10.0.0.1\"}"
nodename=${q@.nodename}
ip=${q@.ip}
printf "[%s][%s]\n" "$nodename" "$ip"'
	out=$(cbsd_stdout "${body}")
	assertEquals "unquoted assignment is unchanged" "[n1][10.0.0.1]" "${out}"
}

testQuotedAround()
{
	body='x="[\"a\", \"b c\", \"d e f\"]"
set -- "pre${x@.[]}post"
printf "%s\n" "$1"
printf "%s\n" "$2"
printf "%s\n" "$3"'
	out=$(cbsd_stdout "${body}")
	assertEquals "text around the expansion joins the outer fields" \
"prea
b c
d e fpost" "${out}"
}

. "${progdir}/../shunit2"
