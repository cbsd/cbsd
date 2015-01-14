--- kern_jail.c.bak	2015-01-15 01:59:49.425840561 +0300
+++ kern_jail.c	2015-01-15 02:04:32.201821554 +0300
@@ -208,6 +208,8 @@
 	"allow.mount.zfs",
 	"allow.mount.procfs",
 	"allow.mount.tmpfs",
+	"allow.dev_io_access",
+	"allow.dev_dri_access",
 };
 const size_t pr_allow_names_size = sizeof(pr_allow_names);
 
@@ -224,6 +226,8 @@
 	"allow.mount.nozfs",
 	"allow.mount.noprocfs",
 	"allow.mount.notmpfs",
+	"allow.nodev_io_access",
+	"allow.nodev_dri_access",
 };
 const size_t pr_allow_nonames_size = sizeof(pr_allow_nonames);
 
@@ -3934,6 +3938,27 @@
 		return (0);
 
 		/*
+		* Allow access to /dev/io in a jail if the non-jailed admin
+		* requests this and if /dev/io exists in the jail. This
+		* allows Xorg to probe a card.
+		*/
+	case PRIV_IO:
+	case PRIV_KMEM_WRITE:
+		if (cred->cr_prison->pr_allow & PR_ALLOW_DEV_IO_ACCESS)
+			return (0);
+		else
+			return (EPERM);
+
+		/*
+		* Allow low level access to DRI. This allows Xorgs to use DRI.
+		*/
+	case PRIV_DRI_DRIVER:
+		if (cred->cr_prison->pr_allow & PR_ALLOW_DEV_DRI_ACCESS)
+			return (0);
+		else
+			return (EPERM);
+
+		/*
 		 * Allow jailed root to set loginclass.
 		 */
 	case PRIV_PROC_SETLOGINCLASS:
@@ -4229,6 +4254,14 @@
     CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_MPSAFE,
     NULL, PR_ALLOW_MOUNT_ZFS, sysctl_jail_default_allow, "I",
     "Processes in jail can mount the zfs file system");
+SYSCTL_PROC(_security_jail, OID_AUTO, dev_io_access,
+    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_MPSAFE,
+    NULL, PR_ALLOW_DEV_IO_ACCESS, sysctl_jail_default_allow, "I",
+    "Processes in jail can access /dev/io if it exists");
+SYSCTL_PROC(_security_jail, OID_AUTO, dev_dri_access,
+    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_MPSAFE,
+    NULL, PR_ALLOW_DEV_DRI_ACCESS, sysctl_jail_default_allow, "I",
+    "Processes in jail can access /dev/dri if it exists");
 
 static int
 sysctl_jail_default_level(SYSCTL_HANDLER_ARGS)
@@ -4367,6 +4400,10 @@
     "B", "Jail may set file quotas");
 SYSCTL_JAIL_PARAM(_allow, socket_af, CTLTYPE_INT | CTLFLAG_RW,
     "B", "Jail may create sockets other than just UNIX/IPv4/IPv6/route");
+SYSCTL_JAIL_PARAM(_allow, dev_io_access, CTLTYPE_INT | CTLFLAG_RW,
+    "B", "Jail may access /dev/io if it exists");
+SYSCTL_JAIL_PARAM(_allow, dev_dri_access, CTLTYPE_INT | CTLFLAG_RW,
+    "B", "Jail may access /dev/dri if it exists");
 
 SYSCTL_JAIL_PARAM_SUBNODE(allow, mount, "Jail mount/unmount permission flags");
 SYSCTL_JAIL_PARAM(_allow_mount, , CTLTYPE_INT | CTLFLAG_RW,
