#define DEFSQLDELIMER "|"


// NODES SQL default scheme
struct nodes_db {
	char           *rowname;
	char           *rowtype;
};

//fields for sqlite
// scheme and upgrade procedure
// "row name", "type of row", status(1 - actual, 0 - not)
const struct nodes_db nodesdb_info[] = {
	{"nodename", "TEXT UNIQUE PRIMARY KEY"},
	{"ip", "TEXT"},
	{"port", "INTEGER"},
	{"keyfile", "TEXT"},
	{"rootkeyfile", "TEXT"},
	{"status", "INTEGER"},
	{"invfile", "TEXT"},
	{"idle", "TIMESTAMP DATE DEFAULT (datetime('now','localtime'))"},
	{"\n", NULL}
}; //this must be last

// JAIL SQL default scheme
struct jail_db {
	char	*rowname;
	char	*rowtype;
};

//fields for sqlite
//scheme and upgrade procedure
// "row name", "type of row", status(1 - actual, 0 - not)
const struct jail_db jaildb_info[] = {
	{"jname", "text default null unique PRIMARY KEY"},
	{"jid", "integer default 0"},
	{"path", "text default null"},
	{"host_hostname", "text default null"},
	{"ip4_addr", "text default null"},
	{"mount_devfs", "boolean default 0"},
	{"allow_mount", "boolean default 0"},
	{"allow_devfs", "boolean default 0"},
	{"allow_nullfs", "boolean default 0"},
	{"mount_fstab", "boolean default 0"},
	{"mkhostsfile", "boolean default 0"},
	{"devfs_ruleset", "integer default 0"},
	{"interface", "text default null"},
	{"basename", "text default null"},
	{"slavenode", "text default null"},
	{"baserw", "boolean default 0"},
	{"mount_src", "boolean default 0"},
	{"mount_obj", "boolean default 0"},
	{"mount_kernel", "text default null"},
	{"mount_ports", "boolean default 0"},
	{"astart", "integer default 0"},
	{"data", "text default null"},
	{"vnet", "boolean default 0"},
	{"applytpl", "boolean default 0"},
	{"mdsize", "integer default 0"},
	{"rcconf", "text default null"},
	{"floatresolv", "boolean default 0"},
	{"ver", "text default null"},
	{"arch", "text default null"},
	{"exec_start", "text default null"},
	{"exec_stop", "text default null"},
	{"exec_poststart", "text default null"},
	{"exec_poststop", "text default null"},
	{"exec_prestart", "text default null"},
	{"exec_prestop", "text default null"},
	{"exec_master_poststart", "text default null"},
	{"exec_master_poststop", "text default null"},
	{"exec_master_prestart", "text default null"},
	{"exec_master_prestop", "text default null"},
	{"status", "integer default 0"},
	{"\n", NULL}
}; //this must be last


// INVENTORY SQL default scheme
struct inventory_db {
	char           *rowname;
	char           *rowtype;
	int		actual;
};

// fields for sqlite
// scheme and upgrade procedure
// "row name", "type of row", status(1 - actual, 0 - not)
const struct inventory_db inventorydb_info[] = {
	{"nodename", "text default null unique PRIMARY KEY", 1},
	{"hostname", "text default null", 1},
	{"nodeip", "text default null", 1},
	{"nodedescr", "text default null", 1},
	{"jnameserver", "text default null", 1},
	{"nodeippool", "text default null", 1},
	{"natip", "text default null", 1},
	{"nat_enable", "text default null", 1},
	{"fbsdrepo", "boolean default 1", 1},
	{"mdtmp", "integer default 0", 1},
	{"repo", "text default null", 1},
	{"workdir", "text default null", 1},
	{"ipfw_enable", "boolean default 0", 1},
	{"fs", "text default null", 1},
	{"zfsfeat", "boolean default 0", 1},
	{"jail_interface", "text default null", 1},
	{"ncpu", "integer default 0", 1},
	{"physmem", "integer default 0", 1},
	{"memtype", "text default null", 1},
	{"disks", "text default null", 1},
	{"cpumodel", "text default null", 1},
	{"cpufreq", "integer default 0", 1},
	{"kernhz", "integer default 0", 1},
	{"sched", "text default null", 1},
	{"eventtimer", "text default null", 1},
	{"nics", "text default null", 1},
	{"parallel", "integer default 5", 1},
	{"stable", "integer default 0", 1},
	{"osrelease", "text default null", 1},
	{"arch", "text default null", 1},
	{"\n", NULL, 0}
}; //this must be last
