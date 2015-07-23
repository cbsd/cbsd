product="CBSD"
myversion="10.2.0a"

if not workdir then
	print ( "no workdir" )
	os.exit(1)
end

distdir="/usr/local/cbsd"
subr= workdir .. "/nc.lua"
nodenamefile= workdir .. "/nodename"
settingstui= workdir .. "/settings-tui.subr"
dialog= workdir .. "/dialog.subr"
tools= workdir .. "/tools.subr"
mdtools= workdir .. "/mdtools.subr"
zfstool= workdir .. "/zfs.subr"
jfs= workdir .. "/jfs.subr"
color= workdir .. "/ansiicolor.subr"
nodes= workdir .. "/nodes.subr"
jrcconf= workdir .. "/rcconf.subr"
vimageconf= workdir .. "/vnet.subr"
vimagetui= workdir .. "/vnet-tui.subr"
inventory= workdir .. "/nc.inventory"
nodedescr= workdir .. "/node.desc"
initenv= workdir .. "/initenv.subr"
system= workdir .. "/system.subr"
buildconf= workdir .. "/build.subr"
distccacheconf= workdir .. "/distccache.subr"
base2pkgconf= workdir .. "/base2pkg.subr"
mailconf= workdir .. "/mailtools.subr"
strings= workdir .. "/strings.subr"
miscdir= workdir .. "/misc"
jobdir= workdir .. "/job"
srcdir= workdir .. "/src"
tmpdir= workdir .. "/tmp"
ftmpdir= workdir .. "/ftmp"
importdir= workdir .. "/import"
exportdir= workdir .. "/export"
basejaildir= workdir .. "/basejail"
basejailpref="base"
jaildatadir= workdir .. "/jails-data"
jailfstabdir= workdir .. "/jails-fstab"
jailrcconfdir= workdir .. "/jails-rcconf"
jailfstabpref="fstab."
jaildir= workdir .. "/jails"
jaildatapref="data"
jailsysdir= workdir .. "/jails-system"
bindir= workdir .. "/bin"
etcdir= workdir .. "/etc"
jailctldir= workdir .. "/jailctl"
nodectl= workdir .. "/nodectl"
rcddir= workdir .. "/rc.d"
sbindir= workdir .. "/sbin"
systemdir= workdir .. "/system"
moduledir= workdir .. "/modules"
toolsdir= workdir .. "/tools"
upgradedir= workdir .. "/upgrade"
vardir= workdir .. "/var"
spooldir= vardir .. "/spool"
taskdir= spooldir .. "/task"
rundir= vardir .. "/run"
maildir= vardir .. "/mail"
logdir= vardir .. "/log"
sharedir= workdir .. "/share"
dbdir= vardir .. "/db"
localcbsdconffile= "/cbsd.lua"
localcbsdconf= workdir .. localcbsdconffile
sudoexecdir= workdir .. "/sudoexec"
systemsudoexecdir= distdir .. "/sudoexec"
cbsduser="cbsd"
production="0"
rsshdir= workdir .. "/.rssh"
sshdir= workdir .. "/.ssh"
templateldir= workdir .. "/template"
fwcount_st="99"
fwcount_end="2000"
rsync_flags="arlHpEAXogt8 --delete"
greeting="node"
jailmapdb= dbdir .. "/jmap.txt"
-- external source for online doc
cbsddocsrc="http://www.bsdstore.ru/en" .. myversion .. "/"

f=io.open(nodenamefile,"r")
if f ~= nil then
	nodename = f:read("*l")
	io.close(f)
end

-- if [ -n "${NOCOLOR}" ]; then
--    ECHO="echo"
-- else
--    ECHO="echo -e"
--    [ -f "${color}" ] && . $color
-- fi

