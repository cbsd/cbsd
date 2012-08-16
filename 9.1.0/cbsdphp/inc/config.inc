<?php
/*
	config.inc
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard (olivier@freenas.org).
	All rights reserved.

	Portions of m0n0wall (http://m0n0.ch/wall).
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met: 

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer. 
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies, 
	either expressed or implied, of the NAS4Free Project.
*/
require_once("globals.inc");
require_once("util.inc");
require_once("xmlparse.inc");
require_once("system.inc");
require_once("array.inc");
require_once("rc.inc");

/* Get product information */
$productname = get_product_name();

/* read platform */
if (file_exists("{$g['etc_path']}/platform")) {
	$g['fullplatform'] = strtolower(chop(file_get_contents("{$g['etc_path']}/platform")));
	$data = explode("-", $g['fullplatform']);
	$g['arch'] = $data[0];
	$g['platform'] = $data[1];
} else {
	$g['arch'] = "unknown";
	$g['platform'] = "unknown";
	$g['fullplatform'] = "unknown";
}

if (is_booting()) {
	// Find the device where config.xml resides and write out an fstab
	unset($cfgdevice);

	// Check if there's already an fstab ('full' release booting?)
	if (!file_exists("{$g['etc_path']}/fstab")) {
		// Display platform type.
		echo "Booting platform: {$g['platform']}\n";

		echo "Looking for configuration file:";

		// Get list of kernel known disks
		$disks = explode(" ", trim(preg_replace("/kern.disks: /", "", exec("/sbin/sysctl kern.disks"))));

		if (stristr($g['platform'], "livecd")) {
			// Config is on floppy disk/USB stick for LiveCD version per default
			$cfgdevice = $cfgpartition = "fd0";
			$cfgfstype = "msdos";

			// Display progress...
			echo " fd0";

			// Check if there is any USB device that can be used
			foreach ($disks as $disk) {
				$found = false;

				// Skip non USB (SCSI) device. The filesystem type must be MSDOS
				if (FALSE === ereg("da[0-9]*", $disk))
					continue;

				// Display progress...
				echo " {$disk}";

				// Try to mount device using file system MSDOS (FAT16/32)
				if (0 == mwexec("/sbin/mount -r -t msdos /dev/{$disk}s1 {$g['cf_path']}")) {
					$cfgdevice = $disk;
					$cfgpartition = "{$cfgdevice}s1";
					$found = true;
				} else if (0 == mwexec("/sbin/mount -r -t msdos /dev/{$disk} {$g['cf_path']}")) {
					$cfgdevice = $disk;
					$cfgpartition = $cfgdevice;
					$found = true;
				}

				// Unmount device if necessary and exit foreach loop
				if (true == $found) {
					mwexec("/sbin/umount -f {$g['cf_path']}");
					break;
				}
			}

			// Display device information used to store the configuration
			echo "\n";
			echo "Using device=/dev/{$cfgpartition} fstype={$cfgfstype} to store configuration.\n";
		} else {
			// Probe kernel known disks until we find one with config.xml
			foreach ($disks as $disk) {
				// Skip mfs-mounted filesystems
				if (TRUE === ereg("md[0-9]*", $disk))
					continue;

				// Display progress...
				echo " {$disk}";

				// Try to mount device using UFS
				if (0 == mwexec("/sbin/mount -r /dev/{$disk}s1a {$g['cf_path']}")) { // Search on slice 1
					if (file_exists("{$g['cf_conf_path']}/config.xml")) {
						$cfgdevice = $disk;
						$cfgpartition = "{$cfgdevice}s1a";
						$cfgfstype = "ufs";
					}
					// Unmount device
					mwexec("/sbin/umount -f {$g['cf_path']}");
				} else if (0 == mwexec("/sbin/mount -r /dev/{$disk}a {$g['cf_path']}")) { // Search on partition 'a'
					if (file_exists("{$g['cf_conf_path']}/config.xml")) {
						$cfgdevice = $disk;
						$cfgpartition = "{$cfgdevice}a";
						$cfgfstype = "ufs";
					}
					// Unmount device
					mwexec("/sbin/umount -f {$g['cf_path']}");
				}

				// Device found, exit
				if ($cfgdevice) {
					break;
				}
			}

			if (!$cfgdevice) {
				// Check if there is any USB device that can be used
				foreach ($disks as $disk) {
					// Skip non USB (SCSI) device. The filesystem type must be MSDOS
					if (FALSE === ereg("da[0-9]*", $disk))
						continue;

					// Display progress...
					echo " {$disk}";

					// Try to mount device using file system MSDOS (FAT16/32)
					if (0 == mwexec("/sbin/mount -r -t msdos /dev/{$disk}s1 {$g['cf_path']}")) {
						if (file_exists("{$g['cf_conf_path']}/config.xml")) {
							$cfgdevice = $disk;
							$cfgpartition = "{$cfgdevice}s1";
							$cfgfstype = "msdos";
						}
						// Unmount device
						mwexec("/sbin/umount -f {$g['cf_path']}");
					} else if (0 == mwexec("/sbin/mount -r -t msdos /dev/{$disk} {$g['cf_path']}")) {
						if (file_exists("{$g['cf_conf_path']}/config.xml")) {
							$cfgdevice = $disk;
							$cfgpartition = $cfgdevice;
							$cfgfstype = "msdos";
						}
						// Unmount device
						mwexec("/sbin/umount -f {$g['cf_path']}");
					}

					// Device found, exit
					if ($cfgdevice) {
						break;
					}
				}
			}

			// Display device information where configuration is found
			echo "\n";
			if ($cfgdevice) {
				echo "Found configuration on device=/dev/{$cfgpartition} fstype={$cfgfstype}.\n";
			}
		}

		if (!$cfgdevice) {
			// No device found, print an error and die
			echo <<<EOD

*******************************************************************************
FATAL ERROR!
The device that contains the configuration file (config.xml) could not be
found. {$productname} cannot continue booting.
*******************************************************************************

EOD;

			mwexec("/sbin/halt");
			exit;
		}

		// Write device name to a file (used by rc.firmware for example)
		file_put_contents("{$g['etc_path']}/cfdevice", $cfgpartition, FILE_TEXT);

		// Write /etc/fstab
		$data = "/dev/{$cfgpartition} {$g['cf_path']} {$cfgfstype} ro 1 1\n";
		file_put_contents("{$g['etc_path']}/fstab", $data, FILE_TEXT);
	}

	// Append if not found
	if (mwexec("/usr/bin/grep '^proc ' {$g['etc_path']}/fstab") != 0) {
		// Append process file system to /etc/fstab
		$data = "proc /proc procfs rw 0 0\n";
		file_put_contents("{$g['etc_path']}/fstab", $data, FILE_TEXT | FILE_APPEND);
	}

	// Mount all filesystems
	mwexec("/sbin/mount -a");
}

//$noparseconfig=true;
/* parse configuration */
if (!$noparseconfig) {
	config_lock();

	/* see if there's a newer cache file */
	if (file_exists("{$g['tmp_path']}/config.cache") &&
		(filemtime("{$g['tmp_path']}/config.cache") >=
		 filemtime("{$g['conf_path']}/config.xml")) &&
		!is_booting()) {
		/* read cache */
		$config = unserialize(file_get_contents("{$g['tmp_path']}/config.cache"));
	} else {
		if (!file_exists("{$g['conf_path']}/config.xml")) {
			if (is_booting()) {
				if (stristr($g['platform'], "liveCD")) {
					// Try copying the default config.xml to the floppy disk/USB stick
					config_unlock();
					reset_factory_defaults();
					config_lock();

					echo "\n";
					echo "*******************************************************************************\n";
					echo "No XML configuration file found - using factory defaults.\n";
					echo "Make sure that the configuration floppy disk or USB stick (MSDOS FAT formatted)\n";
					echo "with the config.xml file is inserted. If it isn't, your configuration changes\n";
					echo "will be lost on reboot.\n";
					echo "*******************************************************************************\n";
					echo "\n";
				} else {
					echo "\n";
					echo "*******************************************************************************\n";
					echo "XML configuration file not found. {$productname} cannot continue booting.\n";
					echo "*******************************************************************************\n";
					echo "\n";
//					mwexec("/sbin/halt");
					exit;
				}
			} else {
				config_unlock();
				exit(0);
			}
		}
		/* Load configuration file into array. */
		$config = parse_xml_config("{$g['conf_path']}/config.xml", $g['xml_rootobj']);
		if (false === $config) {
			echo "Error: Failed to load '{$g['conf_path']}/config.xml' file. {$productname} cannot continue booting.\n";
			mwexec("/sbin/halt");
			exit;
		}

		if ((float)$config['version'] > (float)$g['latest_config']) {
			if (is_booting()) {
				echo <<<EOD

*******************************************************************************
WARNING!
The current configuration has been created with a newer version of
{$productname} than this one! This can lead to serious misbehaviour and
even security holes! You are urged to either upgrade to a newer version of
{$productname} or revert to the default configuration immediately!
*******************************************************************************

EOD;
			}
		}

		// Write configuration cache file
		config_write_cache($config);
	}

	config_unlock();
}

/* mount flash card read/write */
function conf_mount_rw() {
	global $g;

	if ($g['platform'] != "full") {
		/* don't use mount -u anymore
		(doesn't sync the files properly and /bin/sync won't help either) */
		mwexec("/sbin/umount -f {$g['cf_path']}");
		mwexec("/sbin/mount -w -o noatime {$g['cf_path']}");
	}
}

/* mount flash card read only */
function conf_mount_ro() {
	global $g;

	if ($g['platform'] != "full") {
		mwexec("/sbin/umount -f {$g['cf_path']}");
		mwexec("/sbin/mount -r {$g['cf_path']}");
	}
}

/**
 * Save the system configuration
 */
function write_config() {
	global $config, $g;

	config_lock();
	conf_mount_rw();

	// Make sure the clock settings is plausible
	if(function_exists("date_default_timezone_set") and function_exists("date_default_timezone_get"))
        date_default_timezone_set(date_default_timezone_get());
	if (time() > mktime(0, 0, 0, 9, 1, 2004)) {
		$config['lastchange'] = time();
	}

	// Dump configuration as XML data
	$xmlconfig = dump_xml_config($config, $g['xml_rootobj'],
		system_get_language_codeset());

	// Save current configuration file
	$filename = "{$g['cf_conf_path']}/config.xml";
	if (file_exists($filename)) {
		$filename_bk2 = preg_replace("/.xml$/", ".bk2", $filename);
		$filename_bk1 = preg_replace("/.xml$/", ".bk1", $filename);
		$filename_bk0 = preg_replace("/.xml$/", ".bk0", $filename);
		if (file_exists($filename_bk2) && unlink($filename_bk2) === FALSE) {
			die("Error: Failed to delete configuration file '{$filename_bk2}'");
		}
		if (file_exists($filename_bk1) && rename($filename_bk1, $filename_bk2) === FALSE) {
			die("Error: Failed to rename configuration file '{$filename_bk1}'");
		}
		if (file_exists($filename_bk0) && rename($filename_bk0, $filename_bk1) === FALSE) {
			die("Error: Failed to rename configuration file '{$filename_bk0}'");
		}
		if (rename($filename, $filename_bk0) === FALSE) {
			die("Error: Failed to rename configuration file '{$filename}'");
		}
	}

	// Write configuration file
	if (!file_put_contents_safe($filename, $xmlconfig)) {
		die("Error: Failed to write configuration file '{$filename}'");
	}

	// Modify file permissions
	chmod("{$g['cf_conf_path']}/config.xml", 0600);

	conf_mount_ro();

	// Re-read configuration
	$config = parse_xml_config("{$g['conf_path']}/config.xml", $g['xml_rootobj']);

	// Write config cache (to prevent parsing on every access)
	config_write_cache($config);

	// Force completion of pending disk writes
	mwexec("/bin/sync");

	config_unlock();
}

function reset_factory_defaults() {
	global $g;

	config_lock();
	conf_mount_rw();

	/* create conf directory, if necessary */
	if (!file_exists("{$g['cf_conf_path']}"))
		mkdir("{$g['cf_conf_path']}");

	/* clear out /conf */
	$dh = opendir($g['conf_path']);
	while ($filename = readdir($dh)) {
		if (($filename != ".") && ($filename != "..")) {
			unlink($g['conf_path'] . "/" . $filename);
		}
	}
	closedir($dh);

	/* copy default configuration */
	copy("{$g['conf_default_path']}/config.xml", "{$g['conf_path']}/config.xml");

	// Modify file permissions
	chmod("{$g['cf_conf_path']}/config.xml", 0600);

	conf_mount_ro();
	config_unlock();

	return 0;
}

/**
 * Install the given configuration file.
 * @param[in] conffile The path to the file to be installed.
 * @return Returns 0 on success or 1 on failure.
 */
function config_install($conffile) {
	global $config, $g;

	if (!file_exists($conffile))
		return 1;

	config_lock();
	conf_mount_rw();

	$result = copy($conffile, "{$g['conf_path']}/config.xml");

	conf_mount_ro();
	config_unlock();

	return (TRUE === $result) ? 0 : 1;
}

/* Lock configuration file, decide that the lock file is stale after 10 seconds */
function config_lock() {
	global $g;

	$lockfile = "{$g['varrun_path']}/config.lock";
	$n = 0;
	while ($n < 15) {
		/* open the lock file in append mode to avoid race condition */
		if ($fd = fopen($lockfile, "x")) {
			/* succeeded */
			fclose($fd);
			return;
		} else {
			/* file locked, wait and try again */
			sleep(1);
			$n++;
		}
	}
	mwexec("/usr/bin/logger 'Lock error {$lockfile}'");
	while (1) {
		/* can't continue without lock */
		sleep(1);
	}
	exit;
}

/* Unlock configuration file */
function config_unlock() {
	global $g;

	$lockfile = "{$g['varrun_path']}/config.lock";

	if (file_exists($lockfile))
		unlink($lockfile);
}

/**
 * Write configuration cache file.
 * @param[in] data	The data to write.
 * @return True if successful, otherwise false.
 */
function config_write_cache($data) {
	global $g;

	$filename = "{$g['tmp_path']}/config.cache";

	// Ensure configuration cache file is written
	if (!file_put_contents_safe($filename, serialize($data), TRUE)) {
		write_log("Error: Failed to write configuration cache file '{$filename}'.");
		return FALSE;
	}

	// Modify file permissions
	chmod($filename, 0600);

	return TRUE;
}

// Convert the config file.
// Return 0 if successful, otherwise 1
function config_upgrade() {
	global $config, $g;

	if ($config['version'] == $g['latest_config'])
		return 0;		/* already at latest version */

	if (is_booting())
		echo "Converting configuration. ";

	/* convert 1 -> 1.1 */
	/* changing partition name, and initializing old bad user parameters */
	if ($config['version'] === "1") {
		/* change the partitions name "1" to "s1" */
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++)
		{
			if (isset($config['mounts']['mount'][$i]['partition']))
			{
				switch ($config['mounts']['mount'][$i]['partition'])
				{
					case "1":
					   $config['mounts']['mount'][$i]['partition']="s1";
					   break;
					case "2":
					   $config['mounts']['mount'][$i]['partition']="s2";
					   break;
					case "3":
					   $config['mounts']['mount'][$i]['partition']="s3";
					   break;
					case "4":
					   $config['mounts']['mount'][$i]['partition']="s4";
					   break;
				}
			}
		}

		if (is_array($config['nfs']['network'])) 	{
			$config['nfs']['nfsnetwork'] = $config['nfs']['network'];
			unset($config['nfs']['network']);
		}

		$config['access']['groupid']=1002;
		$config['access']['userid']=1002;
		unset($config['access']['group']);
		unset($config['access']['user']);

		$config['version'] = "1.1";
	}

	/* convert 1.1 -> 1.2 */
	/* change the gvinum mount point type 'sraid' to 'gvinum' */
	if ($config['version'] === "1.1") {
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			if (isset($config['mounts']['mount'][$i]['partition'])) {
				if (strcmp($config['mounts']['mount'][$i]['partition'],"sraid") == 0)
					$config['mounts']['mount'][$i]['partition'] = "gvinum";
			}
		}

		$config['version'] = "1.2";
	}

	/* convert 1.2 -> 1.3 */
	/* change the FTP anonymous and local user authentication YES/NO value to right/false type */
	if ($config['version'] === "1.2") {
		if ($config['ftp']['anonymous'] === "YES")
			$config['ftp']['anonymous'] = true;
		else
			$config['ftp']['anonymous'] = false;

		if ($config['ftp']['localuser'] === "YES")
			$config['ftp']['localuser'] = true;
		else
			$config['ftp']['localuser'] = false;

		$config['version'] = "1.3";
	}

	/* convert 1.3 -> 1.4 */
	/* change the gvinum disk fstype attribute 'raid' to 'gvinum' */
	/* change <raid> to <gvinum> */
	if ($config['version'] === "1.3") {
		for ($i = 0; isset($config['disks']['disk'][$i]); $i++) {
			if (0==strcmp($config['disks']['disk'][$i]['fstype'],"raid")) {
				$config['disks']['disk'][$i]['fstype'] = "gvinum";
			}
		}

    if(is_array($config['raid'])) {
      $config['gvinum'] = $config['raid'];
    }
    unset($config['raid']);

		$config['version'] = "1.4";
	}

	/* convert 1.4 -> 1.5 */
	/* Add [fullname] information on mount, disk, gmirror, gvinum and graid5 */
	if ($config['version'] === "1.4") {
		/* Add fullname (/dev/ad0) under disk */
		for ($i = 0; isset($config['disks']['disk'][$i]); $i++) {
			if (isset($config['disks']['disk'][$i]['name'])) 	{
				$config['disks']['disk'][$i]['fullname']="/dev/{$config['disks']['disk'][$i]['name']}";
			}
		}

		/* Add fullname (/dev/ad0s1) under mount point */
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			switch ($config['mounts']['mount'][$i]['partition']) {
				case "gvinum":
					$config['mounts']['mount'][$i]['fullname'] = "/dev/gvinum/{$config['mounts']['mount'][$i]['mdisk']}";
					break;
				case "gmirror":
					$config['mounts']['mount'][$i]['fullname'] = "/dev/mirror/{$config['mounts']['mount'][$i]['mdisk']}";
					break;
				case "gconcat":
					$config['mounts']['mount'][$i]['fullname'] = "/dev/concat/{$config['mounts']['mount'][$i]['mdisk']}";
					break;
				case "gstripe":
					$config['mounts']['mount'][$i]['fullname'] = "/dev/stripe/{$config['mounts']['mount'][$i]['mdisk']}";
					break;
				case "graid5":
					$config['mounts']['mount'][$i]['fullname'] = "/dev/raid5/{$config['mounts']['mount'][$i]['mdisk']}";
					break;
				default:
					$config['mounts']['mount'][$i]['fullname'] = "/dev/{$config['mounts']['mount'][$i]['mdisk']}{$config['mounts']['mount'][$i]['partition']}";
			}
		}

		// Add fullname (/dev/mirror/toto) under gmirror
		for ($i = 0; isset($config['gmirror']['vdisk'][$i]); $i++) {
			if (isset($config['gmirror']['vdisk'][$i]['name'])) 	{
				$config['gmirror']['vdisk'][$i]['fullname']="/dev/mirror/{$config['gmirror']['vdisk'][$i]['name']}";
			}
		}

		// Add fullname (/dev/gvinum/toto) under gvinum
		for ($i = 0; isset($config['gvinum']['vdisk'][$i]); $i++) {
			if (isset($config['gvinum']['vdisk'][$i]['name'])) 	{
				$config['gvinum']['vdisk'][$i]['fullname']="/dev/gvinum/{$config['gvinum']['vdisk'][$i]['name']}";
			}
		}

		// Add fullname (/dev/raid5/toto) under graid5
		for ($i = 0; isset($config['graid5']['vdisk'][$i]); $i++) {
			if (isset($config['graid5']['vdisk'][$i]['name'])) 	{
				$config['graid5']['vdisk'][$i]['fullname']="/dev/raid5/{$config['graid5']['vdisk'][$i]['name']}";
			}
		}

		$config['version'] = "1.5";
	}

	/* convert 1.5 -> 1.6 */
	/* Upgrade iSCSI and NFS configurations
	Upgrade RSYNC client configuration */
	if ($config['version'] === "1.5") {
		if (isset($config['nfs']['enable'])) 	{
			$config['nfs']['nfsnetworks']=array();
			$config['nfs']['nfsnetworks'][]=$config['nfs']['nfsnetwork'];
			unset($config['nfs']['nfsnetwork']);
		}

		if (isset($config['iscsi']['enable'])) 	{
			$iscsivdisk = array();
			$iscsivdisk['targetname'] = $config['iscsi']['targetname'];
			$iscsivdisk['targetaddress'] = $config['iscsi']['targetaddress'];
			$iscsivdisk['name'] = "target0";
			$iscsivdisk['initiatorname'] = "iqn.2005-01.il.ac.huji.cs";
			$config['iscsiinit']['vdisk'] = array();
			$config['iscsiinit']['vdisk'][] = $iscsivdisk;

			unset($config['iscsi']['enable']);
			unset($config['iscsi']['targetaddress']);
			unset($config['iscsi']['targetname']);
		}

		/* Convert 'rsyncclient' part
		Special note: rsyncclient is now an array, so the xml parse will create an array.
		We must use an array
		*/
		if (isset($config['rsyncclient'][0]['enable'])) {
			if (!is_array($config['rsync'])) {
				$config['rsync'] = array();
				if (!is_array($config['rsync']['rsyncclient'])) {
					$config['rsync']['rsyncclient'] = array();
				}
			} else if (!is_array($config['rsync']['rsyncclient'])) {
				$config['rsync']['rsyncclient'] = array();
			}

			foreach ($config['rsyncclient'][0]['sharetosync'] as $sharek => $sharev) 	{
				$rsyncclient = array();
				$rsyncclient['opt_delete'] = isset($config['rsyncclient'][0]['opt_delete']);
				$rsyncclient['rsyncserverip'] = $config['rsyncclient'][0]['rsyncserverip'];
				$rsyncclient['localshare'] = $sharev;
				$rsyncclient['remoteshare'] = $sharev;
				$rsyncclient['minute'] = $config['rsyncclient'][0]['minute'];
				$rsyncclient['hour'] = $config['rsyncclient'][0]['hour'];
				$rsyncclient['day'] = $config['rsyncclient'][0]['day'];
				$rsyncclient['month'] = $config['rsyncclient'][0]['month'];
				$rsyncclient['weekday'] = $config['rsyncclient'][0]['weekday'];
				$rsyncclient['all_mins'] = $config['rsyncclient'][0]['all_mins'];
				$rsyncclient['all_hours'] = $config['rsyncclient'][0]['all_hours'];
				$rsyncclient['all_days'] = $config['rsyncclient'][0]['all_days'];
				$rsyncclient['all_months'] = $config['rsyncclient'][0]['all_months'];
				$rsyncclient['all_weekdays'] = $config['rsyncclient'][0]['all_weekdays'];

				$config['rsync']['rsyncclient'][] = $rsyncclient;

				unset($rsyncclient);
			}

		unset ($config['rsyncclient']);

		} else if (is_array($config['rsyncclient'])) {
			// Clean old unused code
			unset ($config['rsyncclient']);
		}
		if (isset($config['rsync_local']['enable'])) {
			if (!is_array($config['rsync'])) {
				$config['rsync'] = array();
				if (!is_array($config['rsync']['rsynclocal'])) {
					$config['rsync']['rsynclocal'] = array();
				}
			} else if (!is_array($config['rsync']['rsynclocal'])) {
				$config['rsync']['rsynclocal'] = array();
			}

			$rsynclocal['opt_delete'] = isset($config['rsync_local']['opt_delete']);
			$rsynclocal['source'] = $config['rsync_local']['source'];
			$rsynclocal['destination'] = $config['rsync_local']['destination'];
			$rsynclocal['minute'] = $config['rsync_local']['minute'];
			$rsynclocal['hour'] = $config['rsync_local']['hour'];
			$rsynclocal['day'] = $config['rsync_local']['day'];
			$rsynclocal['month'] = $config['rsync_local']['month'];
			$rsynclocal['weekday'] = $config['rsync_local']['weekday'];
			$rsynclocal['all_mins'] = $config['rsync_local']['all_mins'];
			$rsynclocal['all_hours'] = $config['rsync_local']['all_hours'];
			$rsynclocal['all_days'] = $config['rsync_local']['all_days'];
			$rsynclocal['all_months'] = $config['rsync_local']['all_months'];
			$rsynclocal['all_weekdays'] = $config['rsync_local']['all_weekdays'];

			$config['rsync']['rsynclocal'][] = $rsynclocal;

			unset ($config['rsync_local']);
		}

		$config['version'] = "1.6";
	}

	/* Convert 1.6 -> 1.7 */
	/* - Change ['system']['howl_disable'] to ['system']['zeroconf']. */
	/* - Delete ['system']['polling']. */
	/* - Delete ['filter']['bypassstaticroutes']. */
	/* - Delete ['bridge'], ['bridge']['filteringbridge']. */
	if ($config['version'] === "1.6") {
		if (isset($config['system']['howl_disable'])) {
			unset($config['system']['howl_disable']);
		} else {
			$config['system']['zeroconf'] = true;
		}

		if (isset($config['system']['polling'])) {
			unset($config['system']['polling']);
		}

		if (isset($config['filter']['bypassstaticroutes'])) {
			unset($config['filter']['bypassstaticroutes']);
		}

		if (isset($config['bridge'])) {
			unset($config['bridge']);
		}

		$config['version'] = "1.7";
	}

	/* Convert 1.7 -> 1.8 */
	/* Bug Fix: Forget to convert old fstype value with new one */
	/* Create bug "You must add disks first" with older configured disk */
	/* - Change ['fstype']='gmirror' to 'softraid' */
	/* - Change ['fstype']='gconcat' to 'softraid' */
	/* - Change ['fstype']='gstripe' to 'softraid' */
	/* - Change ['fstype']='graid5' to 'softraid' */
	if ($config['version'] === "1.7") {
		for ($i = 0; isset($config['disks']['disk'][$i]); $i++) {
			if (isset($config['disks']['disk'][$i]['name'])) 	{
				switch ($config['disks']['disk'][$i]['fstype']) {
				case "gmirror":
					$config['disks']['disk'][$i]['fstype']="softraid";
					break;
				case "gconcat":
					$config['disks']['disk'][$i]['fstype']="softraid";
					break;
				case "gstripe":
					$config['disks']['disk'][$i]['fstype']="softraid";
					break;
				case "graid5":
					$config['disks']['disk'][$i]['fstype']="softraid";
					break;
				}
			}
		}

		$config['version'] = "1.8";
	}

	/* Convert 1.8 -> 1.9 */
	/* Add type "disk" to existing mountpoint */
	/* Add fullname (/dev/ad0s1) under mount point */
	if ($config['version'] === "1.8") {
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			$config['mounts']['mount'][$i]['type']= "disk";
		}

		$config['version'] = "1.9";
	}

	/* Convert 1.9 -> 2.0 */
	/* Add IPv6 address type */
	if ($config['version'] === "1.9") {
		$config['interfaces']['lan']['ipv6addr']="auto";
		$config['interfaces']['lan']['ipv6subnet']=64;

		for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++) {
			$optcfg = $config['interfaces']['opt' . $i];
			if (isset($optcfg['enable'])) {
				$config['interfaces']['opt' . $i]['ipv6addr']="auto";
				$config['interfaces']['opt' . $i]['ipv6subnet']=64;
			}
		}

		$config['version'] = "2.0";
	}

	/* Convert 2.0 -> 2.1 */
	/* Remove //system/harddiskstandby */
	if ($config['version'] === "2.0") {
		if (isset($config['system']['harddiskstandby'])) {
			unset($config['system']['harddiskstandby']);
		}

		$config['version'] = "2.1";
	}

	/* Convert 2.1 -> 2.2 */
	/* Add //samba/share and remove useless //samba/xxx attributes. */
	if ($config['version'] === "2.1") {
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			$share = array();
			$share['name'] = $config['mounts']['mount'][$i]['sharename'];
			$share['path'] = "/mnt/{$config['mounts']['mount'][$i]['sharename']}";
			$share['comment'] = $config['mounts']['mount'][$i]['desc'];
			$share['browseable'] = true;
			if (is_array($config['samba']['hidemount'])) {
				if (in_array($config['mounts']['mount'][$i]['sharename'],$config['samba']['hidemount']))
					$share['browseable'] = false;
			}
			$share['inheritpermissions'] = true;
			$share['recyclebin'] = isset($config['samba']['recyclebin']);

			$config['samba']['share'][] = $share;
		}

		unset($config['samba']['hidemount']);
		unset($config['samba']['recyclebin']);

		$config['version'] = "2.2";
	}

	/* Convert 2.2 -> 2.3 */
	/* Set default UPnP profile //upnp/profile. */
	if ($config['version'] === "2.2") {
		$config['upnp']['profile'] = "default";

		$config['version'] = "2.3";
	}

	/* Convert 2.3 -> 2.4 */
	/* Refactor //syslog. */
	if ($config['version'] === "2.3") {
		$config['syslogd']['reverse'] = isset($config['syslog']['reverse']);
		$config['syslogd']['nentries'] = $config['syslog']['nentries'];
		$config['syslogd']['resolve'] = isset($config['syslog']['resolve']);
		$config['syslogd']['remote']['enable'] = isset($config['syslog']['enable']);
		$config['syslogd']['remote']['ipaddr'] = $config['syslog']['remoteserver'];
		$config['syslogd']['remote']['daemon'] = isset($config['syslog']['daemon']);
		$config['syslogd']['remote']['ftp'] = isset($config['syslog']['ftp']);
		$config['syslogd']['remote']['rsyncd'] = isset($config['syslog']['rsyncd']);
		$config['syslogd']['remote']['smartd'] = isset($config['syslog']['smartd']);
		$config['syslogd']['remote']['sshd'] = isset($config['syslog']['sshd']);
		$config['syslogd']['remote']['system'] = isset($config['syslog']['system']);

		unset($config['syslog']);

		$config['version'] = "2.4";
	}

	/* Convert 2.4 -> 2.5 */
	/* Refactor //iscsitarget. */
	if ($config['version'] === "2.4") {
		for ($i = 0; isset($config['iscsitarget']['vdisk'][$i]); $i++) {
			$extent = array();
			$extent['name'] = "extent{$i}";
			$extent['path'] = "/mnt/" . $config['iscsitarget']['vdisk'][$i]['sharename'];
			$extent['size'] = $config['iscsitarget']['vdisk'][$i]['size'];

			$network = explode("/", $config['iscsitarget']['vdisk'][$i]['network']);

			$target = array();
			$target['name'] = "target{$i}";
			$target['storage'] = "extent{$i}";
			$target['flags'] = "rw";
			$target['ipaddr'] = $network[0];
			$target['subnet'] = $network[1];

			$config['iscsitarget']['extent'][] = $extent;
			$config['iscsitarget']['target'][] = $target;

			unset($config['iscsitarget']['vdisk'][$i]);
		}

		$config['version'] = "2.5";
	}

	/* Convert 2.5 -> 2.6 */
	/* Remove useless //access/userid and //access/groupid. */
	/* Remove useless //access/user/usergroup. */
	/* Rename //access/user/usergroupid to //access/user/group. */
	if ($config['version'] === "2.5") {
		for ($i = 0; isset($config['access']['user'][$i]); $i++) {
			$config['access']['user'][$i]['primarygroup'] = $config['access']['user'][$i]['usergroupid'];

			unset($config['access']['user'][$i]['usergroupid']);
			unset($config['access']['user'][$i]['usergroup']);
		}

		unset($config['access']['userid']);
		unset($config['access']['groupid']);

		$config['version'] = "2.6";
	}

	/* Convert 2.6 -> 2.7 */
	/* Convert language setting. */
	if ($config['version'] === "2.6") {
		$language = array(
			"Bulgarian" => "bg",
			"Dutch" => "nl",
			"English" => "en_US",
			"French" => "fr",
			"German" => "de",
			"Greek" => "el",
			"Italian" => "it",
			"Japanese" => "ja",
			"Romanian" => "ro",
			"Russian" => "ru",
			"SimplifiedChinese" => "zh_CN",
			"Spanish" => "es",
			"TraditionalChinese" => "zh_TW"
		);

		if (array_key_exists($config['system']['language'], $language)) {
			$config['system']['language'] = $language[$config['system']['language']];
		} else {
			$config['system']['language'] = "en_US"; // Set default language.
		}

		$config['version'] = "2.7";
	}

	/* Convert 2.7 -> 2.8 */
	/* Add new default sshd settings. */
	/* Remove useless //sshd/readonly. */
	if ($config['version'] === "2.7") {
		$config['sshd']['passwordauthentication'] = true;
		$config['sshd']['pubkeyauthentication'] = true;

		unset($config['sshd']['readonly']);

		$config['version'] = "2.8";
	}

	/* Convert 2.8 -> 2.9 */
	/* Add //samba/share/hostallow and //samba/share/hostdeny. */
	/* Add //ftp/chrooteveryone. */
	/* Check if //syslogd/nentries is set. */
	if ($config['version'] === "2.8") {
		for ($i = 0; isset($config['samba']['share'][$i]); $i++) {
			$config['samba']['share'][$i]['hostsallow'] = "ALL";
			$config['samba']['share'][$i]['hostsdeny'] = "ALL";
		}

		$config['ftp']['chrooteveryone'] = true;

		if (!$config['syslogd']['nentries']) {
			$config['syslogd']['nentries'] = 50;
		}

		$config['version'] = "2.9";
	}

	/* Convert 2.9 -> 3.0 */
	/* Rename //system/earlyshellcmd -> //rc/preinit/cmd */
	/* Rename //system/shellcmd -> //rc/postinit/cmd */
	/* Rename //snmpd/rocommunity -> //snmpd/read */
	if ($config['version'] === "2.9") {
		for ($i = 0; isset($config['system']['earlyshellcmd'][$i]); $i++) {
			$config['rc']['preinit']['cmd'][] = $config['system']['earlyshellcmd'][$i];
			unset($config['system']['earlyshellcmd'][$i]);
		}

		for ($i = 0; isset($config['system']['shellcmd'][$i]); $i++) {
			$config['rc']['postinit']['cmd'][] = $config['system']['shellcmd'][$i];
			unset($config['system']['shellcmd'][$i]);
		}

		$config['snmpd']['mibii'] = true;
		$config['snmpd']['netgraph'] = true;
		$config['snmpd']['hostres'] = true;
		$config['snmpd']['read'] = $config['snmpd']['rocommunity'];
		unset($config['snmpd']['rocommunity']);

		$config['version'] = "3.0";
	}

	/* Convert 3.0 -> 3.1 */
	/* Append file name to path. */
	if ($config['version'] === "3.0") {
		for ($i = 0; isset($config['iscsitarget']['extent'][$i]); $i++) {
			$extent = &$config['iscsitarget']['extent'][$i];
			$extent['path'] = $extent['path'] . "/" . $extent['name'];
		}

		$config['version'] = "3.1";
	}

	/* Convert 3.1 -> 3.2 */
	/* Modify NTP settings. */
	if ($config['version'] === "3.1") {
		$config['system']['ntp']['timeservers'] = $config['system']['timeservers'];
		if ("0" !== $config['system']['time-update-interval']) {
			$config['system']['ntp']['enable'] = true;
			$config['system']['ntp']['updateinterval'] = $config['system']['time-update-interval'];
		}	else {
			$config['system']['ntp']['updateinterval'] = 300;
		}

		unset($config['system']['time-update-interval']);
		unset($config['system']['timeservers']);

		$config['version'] = "3.2";
	}

	/* Convert 3.2 -> 3.3 */
	/* Rename attribute //mounts/mount/fullname to devicespecialfile. */
	/* Rename attribute //disks/disk/fullname and //gxxx/vdisk/fullname to devicespecialfile. */
	/* Rename attribute //gxxx/vdisk/diskr to device. */
	/* Modify attribute //geli/vdisk/name content. */
	/* Add attribute //geli/vdisk/device. */
	if ($config['version'] === "3.2") {
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			$config['mounts']['mount'][$i]['devicespecialfile'] = $config['mounts']['mount'][$i]['fullname'];
			unset($config['mounts']['mount'][$i]['fullname']);
		}

		for ($i = 0; isset($config['disks']['disk'][$i]); $i++) {
			if (isset($config['disks']['disk'][$i]['fullname'])) {
				$config['disks']['disk'][$i]['devicespecialfile'] = $config['disks']['disk'][$i]['fullname'];
				unset($config['disks']['disk'][$i]['fullname']);
			}
		}

		foreach (explode(" ", "gconcat gmirror graid5 gstripe gvinum geli") as $class) {
			for ($i = 0; isset($config[$class]['vdisk'][$i]); $i++) {
				if (isset($config[$class]['vdisk'][$i]['fullname'])) {
					$config[$class]['vdisk'][$i]['devicespecialfile'] = $config[$class]['vdisk'][$i]['fullname'];
					unset($config[$class]['vdisk'][$i]['fullname']);
				}

				if (isset($config[$class]['vdisk'][$i]['diskr'])) {
					$config[$class]['vdisk'][$i]['device'] = $config[$class]['vdisk'][$i]['diskr'];
					unset($config[$class]['vdisk'][$i]['diskr']);
				}

				// GEli special handling.
				if ("geli" === $class) {
					$config[$class]['vdisk'][$i]['device'] = $config[$class]['vdisk'][$i]['name'];
					$config[$class]['vdisk'][$i]['name'] = str_replace("/dev/", "", $config[$class]['vdisk'][$i]['name']);
				}
			}
		}

		$config['version'] = "3.3";
	}

	/* Convert 3.3 -> 3.4 */
	/* Add attribute //samba/storedosattributes. */
	if ($config['version'] === "3.3") {
		$config['samba']['storedosattributes'] = true;

		$config['version'] = "3.4";
	}

	/* Convert 3.4 -> 3.5 */
	/* Add attribute //iscsitarget/enable if //iscsitarget/target's exists. */
	if ($config['version'] === "3.4") {
		if (isset($config['iscsitarget']['target']))
			$config['iscsitarget']['enable'] = true;

		$config['version'] = "3.5";
	}

	/* Convert 3.5 -> 3.6 */
	/* Configure current mountpoints as rsyncd shares. */
	/* Convert attribute //rsync/rsyncclient/localshare. */
	/* Convert attribute //rsync/rsynclocal/source and //rsync/rsynclocal/destination. */
	if ($config['version'] === "3.5") {
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			$share = array();
			$share['name'] = $config['mounts']['mount'][$i]['sharename'];
			$share['path'] = "/mnt/" . $config['mounts']['mount'][$i]['sharename'];
			$share['comment'] = $config['mounts']['mount'][$i]['desc'];
			$share['browseable'] = true;
			$share['rwmode'] = "ro";
			$share['maxconnections'] = "10";
			$share['hostsallow'] = "ALL";
			$share['hostsdeny'] = "ALL";

			$config['rsyncd']['share'][] = $share;
		}

		for ($i = 0; isset($config['rsync']['rsyncclient'][$i]); $i++) {
			$config['rsync']['rsyncclient'][$i]['localshare'] = "/mnt/" . $config['rsync']['rsyncclient'][$i]['localshare'];
		}

		for ($i = 0; isset($config['rsync']['rsynclocal'][$i]); $i++) {
			$config['rsync']['rsynclocal'][$i]['source'] = "/mnt/" . $config['rsync']['rsynclocal'][$i]['source'];
			$config['rsync']['rsynclocal'][$i]['destination'] = "/mnt/" . $config['rsync']['rsynclocal'][$i]['destination'];
		}

		$config['version'] = "3.6";
	}

	/* Convert 3.6 -> 3.7 */
	/* Remove attribute //upnp/profile. */
	if ($config['version'] === "3.6") {
		if (isset($config['upnp']['profile']))
			unset($config['upnp']['profile']);

		$config['version'] = "3.7";
	}

	/* Convert 3.7 -> 3.8 */
	/* Remove attribute //access/user/admin and add //access/user/group['1000'] if set. */
	/* Remove attribute //ftp/anonymous. */
	/* Remove attribute //ftp/localuser. */
	/* Remove attribute //rsyncd/maxcon. */
	/* Remove attribute //rsyncd/readonly. */
	if ($config['version'] === "3.7") {
		for ($i = 0; isset($config['access']['user'][$i]); $i++) {
			if (isset($config['access']['user'][$i]['admin'])) {
				if (false === in_array("1000", $config['access']['user'][$i]['group'])) {
					$config['access']['user'][$i]['group'][] = "1000";
				}
				unset($config['access']['user'][$i]['admin']);
			}
		}

		if (isset($config['ftp']['anonymous']))
			unset($config['ftp']['anonymous']);
		if (isset($config['ftp']['localuser']))
			unset($config['ftp']['localuser']);

		if (isset($config['rsyncd']['maxcon']))
			unset($config['rsyncd']['maxcon']);
		if (isset($config['rsyncd']['readonly']))
			unset($config['rsyncd']['readonly']);

		$config['version'] = "3.8";
	}

	/* Convert 3.8 -> 3.9 */
	/* Rename attribute //disks/disk/udma -> //disks/disk/transfermode. */
	if ($config['version'] === "3.8") {
		for ($i = 0; isset($config['disks']['disk'][$i]); $i++) {
			if (isset($config['disks']['disk'][$i]['udma'])) {
				$config['disks']['disk'][$i]['transfermode'] = $config['disks']['disk'][$i]['udma'];
				unset($config['disks']['disk'][$i]['udma']);
			}
		}

		$config['version'] = "3.9";
	}

	/* Convert 3.9 -> 4.0 */
	/* Add attribute //samba/guestaccount. */
	/* Add attribute //statusreport/security. */
	/* Rename attributes //rsyncd/share to //rsyncd/module. */
	/* Rename attributes //rsyncd/share/browseable to //rsyncd/module/list. */
	if ($config['version'] === "3.9") {
		$config['samba']['guestaccount'] = "ftp";
		$config['statusreport']['security'] = "none";

		for ($i = 0; isset($config['rsyncd']['share'][$i]); $i++) {
			$module = array();
			$module['name'] = $config['rsyncd']['share'][$i]['name'];
			$module['path'] = $config['rsyncd']['share'][$i]['path'];
			$module['comment'] = $config['rsyncd']['share'][$i]['comment'];
			$module['rwmode'] = $config['rsyncd']['share'][$i]['rwmode'];
			$module['maxconnections'] = $config['rsyncd']['share'][$i]['maxconnections'];
			$module['hostsallow'] = $config['rsyncd']['share'][$i]['hostsallow'];
			$module['hostsdeny'] = $config['rsyncd']['share'][$i]['hostsdeny'];
			$module['auxparam'] = $config['rsyncd']['share'][$i]['auxparam'];
			if (isset($config['rsyncd']['share'][$i]['browseable']))
				$module['list'] = true;

			$config['rsyncd']['module'][] = $module;

			unset($config['rsyncd']['share'][$i]);
		}

		$config['version'] = "4.0";
	}

	/* Convert 4.0 -> 4.1 */
	/* Add attribute //upnp/profile. */
	/* Add attribute //ftp/tls. */
	/* Add attribute //ftp/certificate. */
	/* Add attribute //ftp/privatekey. */
	if ($config['version'] === "4.0") {
		$config['upnp']['profile'] = "default";
		$config['ftp']['tls'] = "0";
		$config['ftp']['certificate'] = "";
		$config['ftp']['privatekey'] = "";

		$config['version'] = "4.1";
	}

	/* Convert 4.1 -> 4.2 */
	/* Move attribute //afp/share/mswindows to //afp/share/options/mswindows. */
	/* Move attribute //afp/share/noadouble to //afp/share/options/noadouble. */
	if ($config['version'] === "4.1") {
		for ($i = 0; isset($config['afp']['share'][$i]); $i++) {
			if (isset($config['afp']['share'][$i]['mswindows'])) {
				$config['afp']['share'][$i]['options']['mswindows'] = true;
				unset($config['afp']['share'][$i]['mswindows']);
			}
			if (isset($config['afp']['share'][$i]['noadouble'])) {
				$config['afp']['share'][$i]['options']['noadouble'] = true;
				unset($config['afp']['share'][$i]['noadouble']);
			}
		}

		$config['version'] = "4.2";
	}

	/* Convert 4.2 -> 4.3 */
	/* Rename attribute //ftp to //ftpd. */
	/* Convert //nfs configuration to new share architecture. */
	if ($config['version'] === "4.2") {
		$config['ftpd'] = $config['ftp'];
		unset($config['ftp']);

		$config['nfsd']['enable'] = isset($config['nfs']['enable']);

		for ($i = 0; isset($config['nfs']['nfsnetworks'][$i]); $i++) {
			for ($k = 0; isset($config['mounts']['mount'][$k]); $k++) {
				$share = array();
				$share['path'] = "/mnt/" . $config['mounts']['mount'][$k]['sharename'];
				$share['mapall'] = $config['nfs']['mapall'];
				$share['network'] = $config['nfs']['nfsnetworks'][$i];
				$share['comment'] = $config['mounts']['mount'][$k]['desc'];
				$share['options']['alldirs'] = true;

				$config['nfsd']['share'][] = $share;
			}
		}

		unset($config['nfs']);

		$config['version'] = "4.3";
	}

	/* Convert 4.3 -> 4.4 */
	/* Add attribute //nfsd/numproc. */
	if ($config['version'] === "4.3") {
		$config['nfsd']['numproc'] = "4";

		$config['version'] = "4.4";
	}

	/* Convert 4.4 -> 4.5 */
	/* Add attribute //interfaces/lan/media. */
	if ($config['version'] === "4.4") {
		if (empty($config['interfaces']['lan']['media']))
			$config['interfaces']['lan']['media'] = "autoselect";

		$config['version'] = "4.5";
	}

	/* Convert 4.5 -> 4.6 */
	/* Add attribute //samba/share/hidedotfiles. */
	/* Create new //smartd section. */
	/* Convert email report settings. */
	if ($config['version'] === "4.5") {
		for ($i = 0; isset($config['samba']['share'][$i]); $i++) {
			$config['samba']['share'][$i]['hidedotfiles'] = true;
		}

		$config['smartd']['enable'] = isset($config['system']['smart']);
		$config['smartd']['interval'] = "1800";
		$config['smartd']['powermode'] = "never";
		$config['smartd']['temp']['diff'] = "0";
		$config['smartd']['temp']['info'] = "0";
		$config['smartd']['temp']['crit'] = "0";
		$config['smartd']['email']['to'] = "";
		$config['smartd']['email']['subject'] = "S.M.A.R.T. report";
		unset($config['system']['smart']);

		$config['system']['email']['server'] = $config['statusreport']['server'];
		$config['system']['email']['port'] = $config['statusreport']['port'];
		$config['system']['email']['security'] = $config['statusreport']['security'];
		if (isset($config['statusreport']['auth']))
			$config['system']['email']['auth'] = true;
		$config['system']['email']['username'] = $config['statusreport']['username'];
		$config['system']['email']['password'] = $config['statusreport']['password'];
		$config['system']['email']['from'] = $config['statusreport']['from'];
		unset($config['statusreport']['server']);
		unset($config['statusreport']['port']);
		unset($config['statusreport']['security']);
		unset($config['statusreport']['auth']);
		unset($config['statusreport']['username']);
		unset($config['statusreport']['password']);
		unset($config['statusreport']['from']);

		$config['version'] = "4.6";
	}

	/* Convert 4.6 -> 4.7 */
	/* Add WebGUI certificate and private key. */
	if ($config['version'] === "4.6") {
		if (!isset($config['system']['webgui']['certificate']))
			$config['system']['webgui']['certificate'] = "LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tDQpNSUlEVlRDQ0FyNmdBd0lCQWdJSkFKS3FUNzk2K0ZHSE1BMEdDU3FHU0liM0RRRUJCUVVBTUhzeEN6QUpCZ05WDQpCQVlUQWtaU01STXdFUVlEVlFRSUV3cFRiMjFsTFZOMFlYUmxNUTh3RFFZRFZRUUhFd1pPWVc1MFpYTXhFREFPDQpCZ05WQkFvVEIwWnlaV1ZPUVZNeEVEQU9CZ05WQkFNVEIwWnlaV1ZPUVZNeElqQWdCZ2txaGtpRzl3MEJDUUVXDQpFMjlzYVhacFpYSkFabkpsWlc1aGN5NXZjbWN3SGhjTk1EY3dOakk0TVRjeU9EQTVXaGNOTXpReE1URXlNVGN5DQpPREE1V2pCN01Rc3dDUVlEVlFRR0V3SkdVakVUTUJFR0ExVUVDQk1LVTI5dFpTMVRkR0YwWlRFUE1BMEdBMVVFDQpCeE1HVG1GdWRHVnpNUkF3RGdZRFZRUUtFd2RHY21WbFRrRlRNUkF3RGdZRFZRUURFd2RHY21WbFRrRlRNU0l3DQpJQVlKS29aSWh2Y05BUWtCRmhOdmJHbDJhV1Z5UUdaeVpXVnVZWE11YjNKbk1JR2ZNQTBHQ1NxR1NJYjNEUUVCDQpBUVVBQTRHTkFEQ0JpUUtCZ1FDaktxR0FrMk9Lbm9JWkhQYjloaFVaeThwRkV0UHZtUjBJZkU3VkFtc1RZVHpWDQp2bmhmNllXbkNRSVlQZXJPbStXckxQcWZlcXBqamlOUnBjUFgxTENpT1BOU1FzU09WK2lhVWRDdnJHVnRvbHJrDQpsaGJTUTg2MGdRRlZaS2tNc2xudnJka1VZOFRMdEw1bVhhTmo3ek1oNFVjTnVnbWtrRzhlRVJHa1BEdXhxUUlEDQpBUUFCbzRIZ01JSGRNQjBHQTFVZERnUVdCQlEyYUJrYXdiRDcrdXBpcVQ5bm1TdmlDOC9BUFRDQnJRWURWUjBqDQpCSUdsTUlHaWdCUTJhQmthd2JENyt1cGlxVDlubVN2aUM4L0FQYUYvcEgwd2V6RUxNQWtHQTFVRUJoTUNSbEl4DQpFekFSQmdOVkJBZ1RDbE52YldVdFUzUmhkR1V4RHpBTkJnTlZCQWNUQms1aGJuUmxjekVRTUE0R0ExVUVDaE1IDQpSbkpsWlU1QlV6RVFNQTRHQTFVRUF4TUhSbkpsWlU1QlV6RWlNQ0FHQ1NxR1NJYjNEUUVKQVJZVGIyeHBkbWxsDQpja0JtY21WbGJtRnpMbTl5WjRJSkFKS3FUNzk2K0ZHSE1Bd0dBMVVkRXdRRk1BTUJBZjh3RFFZSktvWklodmNODQpBUUVGQlFBRGdZRUFXRnZtMXgyY3JCNW5UalcwbnA4WWF5Q1hnVHZKTjV6SGdJTFIrODRmS2ZzZU1sb2dUK1QvDQpnQWt4Vy9yR1JOc2Y2VUVVR1M4bW92NlMyeTIxbGRwaDA1dGp0NVdlZ0d2VDRPQUFscGx0Wmg0UUgxejNBclpGDQo3M3hYOVdiUEkyTGtLZi9STzFDczQ0MmFTUmtiQ1oxdFpSeU5tWUVQWVp0LzViNitaZ1hLbEFFPQ0KLS0tLS1FTkQgQ0VSVElGSUNBVEUtLS0tLQ==";
		if (!isset($config['system']['webgui']['private-key'])) {
			$config['system']['webgui']['privatekey'] = "LS0tLS1CRUdJTiBSU0EgUFJJVkFURSBLRVktLS0tLQ0KTUlJQ1d3SUJBQUtCZ1FDaktxR0FrMk9Lbm9JWkhQYjloaFVaeThwRkV0UHZtUjBJZkU3VkFtc1RZVHpWdm5oZg0KNllXbkNRSVlQZXJPbStXckxQcWZlcXBqamlOUnBjUFgxTENpT1BOU1FzU09WK2lhVWRDdnJHVnRvbHJrbGhiUw0KUTg2MGdRRlZaS2tNc2xudnJka1VZOFRMdEw1bVhhTmo3ek1oNFVjTnVnbWtrRzhlRVJHa1BEdXhxUUlEQVFBQg0KQW9HQUxGWVpMR2JnVlhwY3Y1WXZSeFI0QjFwb3kvd2h1cWxRU2szODNjU2FjOW1vUGl4VGFUeDdKTGc5NlhocA0KcFZxWFBid3JIY3FuSWdZdGNySW9WUzFtRVQ5VlBVUUV4eWg4RG10RGdRM0l3dmc4c3ptYmRwekVWbmRyVm1SbA0KWkdoTUlqeGRNbVd1UXJST3lDa3oxRVhHUFowQlBhUm9LOVhiYWxpeVl1Q1dLMEVDUVFEV2RTM0ZTNEhpclNaVA0KajQ0SUY3T2VnN01hL0o2cnV2SWtNMUdPdWREK3I1dDM2OC8rc0oxNlF5cmNyeWxtQzdDcmdvVDRONldyeFRKKw0KMzErL2lPUHZBa0VBd3NYMFNRQmRBQnlJVzljbWRCYzBJcXFqQTc4ZFpSWjVEUGMzMGxDaFppVUFtR0Q2d0NjbQ0KbWVucTRzUDJpNGgxRXRLa1J1bHovd0t6WThnUkp3Rkw1d0pBR21OUXZuWU1CVmhJM3Y5SytyUHY1aEZPZUlPZA0KQW9SK1J2WUprd1pLTVF4cWxOYnZRT2Q2ZVJES3RWellPelkxQnVXSWR3Smg0RURqZGJrR2p3ckpBUUpBYVg3Zw0KM3IyRW9lL1NaN1EwRHNNV3FRUTFnV3o5bGVoRExxT2twNVNtcHZXNXljOFZZZ2lTbnVFVktUZ3RETXlmWk0rVQ0KelJmYmpTOUFUUFpyVys0VzF3SkFXZ0RDTjh0dDF5S2lJZTFFTVdzTmZSVjc1dWJvZ0g0ODlETzM1dWQrUDhzcw0KRUc0STFrMjk5Smh1Ym1rQjJGanZnRkM0QlRBN0M3NW5CTHBzWTZja1lBPT0NCi0tLS0tRU5EIFJTQSBQUklWQVRFIEtFWS0tLS0t";
		} else {
			$config['system']['webgui']['privatekey'] = $config['system']['webgui']['private-key'];
			unset($config['system']['webgui']['private-key']);
		}

		$config['version'] = "4.7";
	}

	/* Convert 4.7 -> 4.8 */
	/* Refactor //ad configuration. */
	if ($config['version'] === "4.7") {
		$config['ad']['domaincontrollername'] = $config['ad']['ad_srv_name'];
		$config['ad']['domainname_dns'] = "";
		$config['ad']['domainname_netbios'] = "";
		$config['ad']['username'] = $config['ad']['admin_name'];
		if (empty($config['ad']['username']))
			$config['ad']['username'] = "Administrator";
		$config['ad']['password'] = $config['ad']['admin_pass'];
		unset($config['ad']['ad_srv_name']);
		unset($config['ad']['admin_name']);
		unset($config['ad']['admin_pass']);
		unset($config['ad']['ad_srv_ip']);

		$config['version'] = "4.8";
	}

	/* Convert 4.8 -> 4.9 */
	/* Move //rsync/rsynclocal/opt_delete to //rsync/rsynclocal/options/delete. */
	/* Move //rsync/rsyncclient/opt_delete to //rsync/rsyncclient/options/delete. */
	/* Refactor //websrv/authentication. */
	if ($config['version'] === "4.8") {
		for ($i = 0; isset($config['rsync']['rsyncclient'][$i]); $i++) {
			if (isset($config['rsync']['rsyncclient'][$i]['opt_delete'])) {
				$config['rsync']['rsyncclient'][$i]['options']['delete'] = true;
				unset($config['rsync']['rsyncclient'][$i]['opt_delete']);
			}
		}

		for ($i = 0; isset($config['rsync']['rsynclocal'][$i]); $i++) {
			if (isset($config['rsync']['rsynclocal'][$i]['opt_delete'])) {
				$config['rsync']['rsynclocal'][$i]['options']['delete'] = true;
				unset($config['rsync']['rsynclocal'][$i]['opt_delete']);
			}
		}

		if (isset($config['websrv']['authentication'])) {
			unset($config['websrv']['authentication']); // Remove entry first.
			$config['websrv']['authentication']['enable'] = true;
		}

		$url = array();
		$url['path'] = "/";
		$url['realm'] = "Webserver";
		$config['websrv']['authentication']['url'][] = $url;

		$config['version'] = "4.9";
	}

	/* Convert 4.9 -> 5.0 */
	/* Set default //upnp/port. */
	if ($config['version'] === "4.9") {
		if (!isset($config['upnp']['port']) || empty($config['upnp']['port']))
			$config['upnp']['port'] = "49152";

		$config['version'] = "5.0";
	}

	/* Convert 5.0 -> 5.1 */
	/* Add //interfaces/lan/enable. */
	if ($config['version'] === "5.0") {
		$config['interfaces']['lan']['enable'] = true;

		$config['version'] = "5.1";
	}

	/* Convert 5.1 -> 5.2 */
	/* Add //mounts/mount/accessrestrictions/xxx attributes. */
	if ($config['version'] === "5.1") {
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			$config['mounts']['mount'][$i]['accessrestrictions']['owner'] = "root";
			$config['mounts']['mount'][$i]['accessrestrictions']['group'] = "wheel";
			$config['mounts']['mount'][$i]['accessrestrictions']['mode'] = "0777";
		}

		$config['version'] = "5.2";
	}

	/* Convert 5.2 -> 5.3 */
	/* Add UUID's to physical devices and virtual GEOM devices. */
	if ($config['version'] === "5.2") {
		for ($i = 0; isset($config['disks']['disk'][$i]); $i++) {
			$config['disks']['disk'][$i]['uuid'] = uuid();
		}

		foreach (explode(" ", "gconcat gmirror graid5 gstripe gvinum geli") as $class) {
			for ($i = 0; isset($config[$class]['vdisk'][$i]); $i++) {
				$config[$class]['vdisk'][$i]['uuid'] = uuid();
			}
		}

		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			$config['mounts']['mount'][$i]['uuid'] = uuid();
		}

		$config['version'] = "5.3";
	}

	/* Convert 5.3 -> 5.4 */
	/* Add //rsync/rsyncclient/options/recursive. */
	/* Add //rsync/rsyncclient/options/times. */
	/* Add //rsync/rsyncclient/options/compress. */
	/* Add //rsync/rsyncclient/options/delete_algorithm. */
	/* Add //rsync/rsynclocal/options/archive. */
	/* Add //rsync/rsynclocal/options/delete_algorithm. */
	if ($config['version'] === "5.3") {
		for ($i = 0; isset($config['rsync']['rsyncclient'][$i]); $i++) {
			$config['rsync']['rsyncclient'][$i]['options']['recursive'] = true;
			$config['rsync']['rsyncclient'][$i]['options']['times'] = true;
			$config['rsync']['rsyncclient'][$i]['options']['compress'] = true;
			if (isset($config['rsync']['rsyncclient'][$i]['options']['delete'])) {
				$config['rsync']['rsyncclient'][$i]['options']['delete_algorithm'] = "after";
			}
		}

		for ($i = 0; isset($config['rsync']['rsynclocal'][$i]); $i++) {
			$config['rsync']['rsynclocal'][$i]['options']['archive'] = true;
			if (isset($config['rsync']['rsynclocal'][$i]['options']['delete'])) {
				$config['rsync']['rsynclocal'][$i]['options']['delete_algorithm'] = "after";
			}
		}

		$config['version'] = "5.4";
	}

	/* Convert 5.4 -> 5.5 */
	/* Add UUID's to //rsyncd/module, //rsync/rsyncclient and //rsync/rsynclocal. */
	if ($config['version'] === "5.4") {
		for ($i = 0; isset($config['rsyncd']['module'][$i]); $i++) {
			$config['rsyncd']['module'][$i]['uuid'] = uuid();
		}

		foreach (explode(" ", "rsyncclient rsynclocal") as $attr) {
			for ($i = 0; isset($config['rsync'][$attr][$i]); $i++) {
				$config['rsync'][$attr][$i]['uuid'] = uuid();
			}
		}

		$config['version'] = "5.5";
	}

	/* Convert 5.5 -> 5.6 */
	/* Add UUID's to //samba/share, //staticroutes/route, //afp/share, //nfsd/share, */
	/* //cron/job, //system/hosts, //access/user, //access/group, */
	/* //smartd/selftest, //vinterfaces/vlan, //vinterfaces/lagg, */
	/* //iscsitarget/extent, //iscsitarget/device and //iscsitarget/target. */
	if ($config['version'] === "5.5") {
		for ($i = 0; isset($config['staticroutes']['route'][$i]); $i++) {
			$config['staticroutes']['route'][$i]['uuid'] = uuid();
		}

		foreach (explode(" ", "samba afp nfsd") as $attr) {
			for ($i = 0; isset($config[$attr]['share'][$i]); $i++) {
				$config[$attr]['share'][$i]['uuid'] = uuid();
			}
		}

		for ($i = 0; isset($config['cron']['job'][$i]); $i++) {
			$config['cron']['job'][$i]['uuid'] = uuid();
		}

		for ($i = 0; isset($config['system']['hosts'][$i]); $i++) {
			$config['system']['hosts'][$i]['uuid'] = uuid();
		}

		foreach (explode(" ", "user group") as $attr) {
			for ($i = 0; isset($config['access'][$attr][$i]); $i++) {
				$config['access'][$attr][$i]['uuid'] = uuid();
			}
		}

		for ($i = 0; isset($config['smartd']['selftest'][$i]); $i++) {
			$config['smartd']['selftest'][$i]['uuid'] = uuid();
		}

		foreach (explode(" ", "vlan lagg") as $attr) {
			for ($i = 0; isset($config['vinterfaces'][$attr][$i]); $i++) {
				$config['vinterfaces'][$attr][$i]['uuid'] = uuid();
			}
		}

		foreach (explode(" ", "extent device target") as $attr) {
			for ($i = 0; isset($config['iscsitarget'][$attr][$i]); $i++) {
				$config['iscsitarget'][$attr][$i]['uuid'] = uuid();
			}
		}

		$config['version'] = "5.6";
	}

	/* Convert 5.6 -> 5.7 */
	/* Add UUID's to //system/rcconf/param, //system/sysctl/param and //iscsiinit/vdisk. */
	if ($config['version'] === "5.6") {
		foreach (explode(" ", "rcconf sysctl") as $attr) {
			for ($i = 0; isset($config['system'][$attr]['param'][$i]); $i++) {
				$config['system'][$attr]['param'][$i]['uuid'] = uuid();
			}
		}

		for ($i = 0; isset($config['iscsiinit']['vdisk'][$i]); $i++) {
			$config['iscsiinit']['vdisk'][$i]['uuid'] = uuid();
		}

		$config['version'] = "5.7";
	}

	/* Convert 5.7 -> 5.8 */
	/* Add //rsync/rsynclocal/enable and //rsync/rsyncclient/enable. */
	if ($config['version'] === "5.7") {
		foreach (explode(" ", "rsyncclient rsynclocal") as $attr) {
			for ($i = 0; isset($config['rsync'][$attr][$i]); $i++) {
				$config['rsync'][$attr][$i]['enable'] = true;
			}
		}

		$config['version'] = "5.8";
	}

	/* Convert 5.8 -> 5.9 */
	/* Add hw.ata.to=15 to //system/sysctl/param. */
	if ($config['version'] === "5.8") {
		if (false === array_search_ex("hw.ata.to", $config['system']['sysctl']['param'], "name")) {
			$config['system']['sysctl']['param'][] = array(
				"uuid" => uuid(),
				"name" => "hw.ata.to",
				"value" => "15",
				"comment" => "ATA disk timeout vis-a-vis power-saving");
		}

		$config['version'] = "5.9";
	}

	/* Convert 5.9 -> 6.0 */
	/* Add //rsync/rsynclocal/who and //rsync/rsyncclient/who. */
	if ($config['version'] === "5.9") {
		foreach (explode(" ", "rsyncclient rsynclocal") as $attr) {
			for ($i = 0; isset($config['rsync'][$attr][$i]); $i++) {
				$config['rsync'][$attr][$i]['who'] = "root";
			}
		}

		$config['version'] = "6.0";
	}

	/* Convert 6.0 -> 6.1 */
	/* Reset password to default. */
	if ($config['version'] === "6.0") {
		$config['system']['password'] = $g['default_passwd'];

		$config['version'] = "6.1";
	}

	/* Convert 6.1 -> 6.2 */
	/* Decode email password. */
	if ($config['version'] === "6.1") {
		$config['system']['email']['password'] = base64_decode($config['system']['email']['password']);

		$config['version'] = "6.2";
	}

	/* Convert 6.2 -> 6.3 */
	/* Add system group 'admin' and 'transmission' to //system/usermanagement/group. */
	/* Add system user 'transmission' to //system/usermanagement/user. */
	if ($config['version'] === "6.2") {
		$config['system']['usermanagement']['group'][] = array(
			"uuid" => "e504881b-c8dc-43d7-9ca4-74552f9b405b",
			"id" => "1000",
			"name" => "admin");

		$config['system']['usermanagement']['group'][] = array(
			"uuid" => "e8cc78b8-7c3d-46de-a34c-11b1c260a37e",
			"id" => "999",
			"name" => "transmission");

		$config['system']['usermanagement']['user'][] = array(
			"uuid" => "60c4bfb1-7c3c-4535-800c-2b17eeae9369",
			"id" => "999",
			"name" => "transmission",
			"primarygroup" => "999",
			"group" => "50",
			"extraoptions" => "-m -w none");

		$config['version'] = "6.3";
	}

	/* Convert 6.3 -> 6.4 */
	/* Set default email authentication method. */
	if ($config['version'] === "6.3") {
		$config['system']['email']['authmethod'] = "login";

		$config['version'] = "6.4";
	}

	/* Convert 6.4 -> 6.5 */
	/* Set UPS shutdown mode //ups/shutdownmode and timer //ups/shutdowntimer. */
	if ($config['version'] === "6.4") {
		$config['ups']['shutdownmode'] = "onbatt";
		$config['ups']['shutdowntimer'] = "30";

		$config['version'] = "6.5";
	}

	/* Convert 6.5 -> 6.6 */
	/* Add //samba/usesendfile. */
	if ($config['version'] === "6.5") {
		$config['samba']['usesendfile'] = true;

		$config['version'] = "6.6";
	}

	/* Convert 6.6 -> 6.7 */
	/* Add misc attributes to //ldap. */
	if ($config['version'] === "6.6") {
		$config['ldap']['anonymousbind'] = true;
		$config['ldap']['auxparam'][] = "ldap_version 3";
		$config['ldap']['auxparam'][] = "timelimit 30";
		$config['ldap']['auxparam'][] = "bind_timelimit 30";
		$config['ldap']['auxparam'][] = "bind_policy soft";
		$config['ldap']['auxparam'][] = "pam_ldap_attribute uid";

		$config['version'] = "6.7";
	}

	/* Convert 6.7 -> 6.8 */
	/* Convert //ftpd/timeout from minutes to seconds. */
	/* Convert //ftpd/tls to boolean. */
	/* Remove //ftpd/keepallfiles and //ftpd/natmode. */
	if ($config['version'] === "6.7") {
		if (isset($config['ftpd']['timeout']) && !empty($config['ftpd']['timeout']))
			$config['ftpd']['timeout'] = $config['ftpd']['timeout'] * 60;

		if (isset($config['ftpd']['tls']) && ("0" !== ($config['ftpd']['tls'])))
			$config['ftpd']['tls'] = true;

		unset($config['ftpd']['keepallfiles']);
		unset($config['ftpd']['natmode']);

		$config['version'] = "6.8";
	}

	/* Convert 6.8 -> 6.9 */
	/* Add //system/hostsacl/rule host access control rules. */
	if ($config['version'] === "6.8") {
		$config['system']['hostsacl']['rule'][] = "#ftpd  : xxx.xxx.xxx.xxx : deny";
		$config['system']['hostsacl']['rule'][] = "#sshd : .example.com : deny";
		$config['system']['hostsacl']['rule'][] = "#in.tftpd : xxx.xxx.xxx.xxx : deny";
		$config['system']['hostsacl']['rule'][] = "ALL : ALL : allow";

		$config['version'] = "6.9";
	}

	/* Convert 6.9 -> 7.0 */
	/* Convert swap configuration. */
	if ($config['version'] === "6.9") {
		$config['system']['swap']['type'] = "file";

		if (isset($config['system']['swap_enable'])) {
			$index = array_search_ex($config['system']['swap_mountname'], $config['mounts']['mount'], "sharename");

			$config['system']['swap']['enable'] = true;
			$config['system']['swap']['mountpoint'] = $config['mounts']['mount'][$index]['uuid'];
			$config['system']['swap']['size'] = $config['system']['swap_size'];

			unset($config['system']['swap_enable']);
			unset($config['system']['swap_mountname']);
			unset($config['system']['swap_size']);
		}

		$config['version'] = "7.0";
	}

	/* Convert 7.0 -> 7.1 */
	/* Add //bittorrent/authrequired flag. */
	if ($config['version'] === "7.0") {
		$config['bittorrent']['authrequired'] = true;

		$config['version'] = "7.1";
	}

	/* Convert 7.1 -> 7.2 */
	/* Convert //ups/remotemonitor to boolean. Add upsd TCP wrapper example. */
	if ($config['version'] === "7.1") {
		$config['ups']['remotemonitor'] = !empty($config['ups']['remotemonitor']);
		$config['system']['hostsacl']['rule'][] = "#upsd : xxx.xxx.xxx.xxx : deny";

		$config['version'] = "7.2";
	}

	/* Convert 7.2 -> 7.3 */
	/* Add sysctl variable to disable core dumps and power button. */
	/* Enable all already existing sysctl variables. */
	/* Add bsnmpd TCP wrapper example. */
	if ($config['version'] === "7.2") {
		for ($i = 0; isset($config['system']['sysctl']['param'][$i]); $i++) {
			$config['system']['sysctl']['param'][$i]['enable'] = true;
		}

		$config['system']['sysctl']['param'][] = array(
			"enable" => true,
			"uuid" => uuid(),
			"name" => "kern.coredump",
			"value" => "0",
			"comment" => "Disable core dump");

		$config['system']['sysctl']['param'][] = array(
			"enable" => false,
			"uuid" => uuid(),
			"name" => "hw.acpi.power_button_state",
			"value" => "NONE",
			"comment" => "Disable power button");

		$config['system']['hostsacl']['rule'][] = "#bsnmpd : xxx.xxx.xxx.xxx : deny";

		$config['version'] = "7.3";
	}

	/* Convert 7.3 -> 7.4 */
	/* Convert //daap/content from single string to array. */
	if ($config['version'] === "7.3") {
		if (isset($config['daap']['content'])) {
			$config['daap']['content'] = explode(",", $config['daap']['content'][0]);
		}

		$config['version'] = "7.4";
	}

	/* Convert 7.4 -> 7.5 */
	/* Set default username //bittorrent/username. */
	/* Set default peer port //bittorrent/peerport. */
	/* Enable port forwarding //bittorrent/portforwarding. */
	/* Enable port peer exchange //bittorrent/pex. */
	/* Set default encryption mode //bittorrent/encryption. */
	if ($config['version'] === "7.4") {
		$config['bittorrent']['username'] = "admin";
		$config['bittorrent']['peerport'] = "51413";
		$config['bittorrent']['portforwarding'] = true;
		$config['bittorrent']['pex'] = true;
		$config['bittorrent']['encryption'] = "1";

		$config['version'] = "7.5";
	}

	/* Convert 7.5 -> 7.6 */
	/* Set default console screensaver blanktime //system/sysconsaver/blanktime. */
	if ($config['version'] === "7.5") {
		$config['system']['sysconsaver']['blanktime'] = "300";

		$config['version'] = "7.6";
	}

	/* Convert 7.7 -> 7.7 */
	/* Enable all existing //system/rcconf/param entries. */
	/* Add powerd_flags to //system/rcconf. */
	if ($config['version'] === "7.6") {
		for ($i = 0; isset($config['system']['rcconf']['param'][$i]); $i++) {
			$config['system']['rcconf']['param'][$i]['enable'] = true;
		}

		if (false === array_search_ex("powerd_flags", $config['system']['rcconf']['param'], "name")) {
			$config['system']['rcconf']['param'][] = array(
				"enable" => true,
				"uuid" => uuid(),
				"name" => "powerd_flags",
				"value" => "-a adaptive -b adaptive",
				"comment" => "System power control options");
		}

		$config['version'] = "7.7";
	}

	/* Convert 7.7 -> 7.8 */
	/* Set uuid's for //websrv/authentication/url. */
	if ($config['version'] === "7.7") {
		for ($i = 0; isset($config['websrv']['authentication']['url'][$i]); $i++) {
			$config['websrv']['authentication']['url'][$i]['uuid'] = uuid();
		}

		$config['version'] = "7.8";
	}

	/* Convert 7.8 -> 7.9 */
	/* Set default iSCSI target (istgt) values. */
	if ($config['version'] === "7.8") {
		$config['iscsitarget']['nodebase'] = "iqn.2007-09.jp.ne.peach.istgt";
		$config['iscsitarget']['discoveryauthmethod'] = "Auto";
		$config['iscsitarget']['discoveryauthgroup'] = "0";
		$config['iscsitarget']['timeout'] = "30";
		$config['iscsitarget']['nopininterval'] = "20";
		$config['iscsitarget']['maxsessions'] = "32";
		$config['iscsitarget']['maxconnections'] = "8";
		$config['iscsitarget']['firstburstlength'] = "65536";
		$config['iscsitarget']['maxburstlength'] = "262144";
		$config['iscsitarget']['maxrecvdatasegmentlength'] = "262144";

		$config['version'] = "7.9";
	}

	/* Convert 7.9 -> 8.0 */
	/* Modify 'extraoptions' attribute for user 'transmission'. */
	/* Convert 'fullshell' attribute to its 'shell' equivalent. */
	if ($config['version'] === "7.9") {
		$index = array_search_ex("60c4bfb1-7c3c-4535-800c-2b17eeae9369", $config['system']['usermanagement']['user'], "uuid");
		if (false !== $index) {
			$config['system']['usermanagement']['user'][$index]['extraoptions'] = "-m -w random";
		}

		for ($i = 0; isset($config['access']['user'][$i]); $i++) {
			if (isset($config['access']['user'][$i]['fullshell'])) {
				$config['access']['user'][$i]['shell'] = "tcsh";
				unset($config['access']['user'][$i]['fullshell']);
			} else {
				$config['access']['user'][$i]['shell'] = "scponly";
			}
		}

		$config['version'] = "8.0";
	}

	/* Convert 8.0 -> 8.1 */
	/* Convert unison configuration. */
	if ($config['version'] === "8.0") {
		$config['unison']['mkdir'] = $config['unison']['makedir'];
		$config['unison']['workdir'] = "{$g['media_path']}/{$config['unison']['share'][0]}/{$config['unison']['workdir']}";
		unset($config['unison']['makedir']);
		unset($config['unison']['share']);

		$config['version'] = "8.1";
	}

	/* Convert 8.1 -> 8.2 */
	/* Convert S.M.A.R.T. configuration. */
	if ($config['version'] === "8.1") {
		for ($i = 0; isset($config['disks']['disk'][$i]); $i++) {
			$enable = false;
			if (isset($config['disks']['disk'][$i]['smart'])) {
				unset($config['disks']['disk'][$i]['smart']);
				$enable = true;
			}
			$config['disks']['disk'][$i]['smart']['enable'] = $enable;
			$config['disks']['disk'][$i]['smart']['extraoptions'] = "";
		}

		$config['version'] = "8.2";
	}

	/* Convert 8.2 -> 8.3 */
	/* Set default //ftpd/maxloginattempts. */
	if ($config['version'] === "8.2") {
		$config['ftpd']['maxloginattempts'] = 1;

		$config['version'] = "8.3";
	}

	/* Convert 8.3 -> 8.4 */
	/* Set default custom script. */
	/* Set default AIO values. */
	if ($config['version'] === "8.3") {
		if (!array_key_exists('report_scriptname', $config['statusreport'])) {
			$config['statusreport']['report_scriptname'] = '';
		}
		$config['samba']['aio'] = false;
		$config['samba']['aiorsize'] = 1;
		$config['samba']['aiowsize'] = 1;
		$config['samba']['aiowbehind'] = '';

		$config['version'] = "8.4";
	}

	/* Convert 8.4 -> 8.5 */
	/* Disable fsck on MBR/UFS. */
	if ($config['version'] === "8.4") {
		for ($i = 0; isset($config['mounts']['mount'][$i]); $i++) {
			if ($config['mounts']['mount'][$i]['fstype'] == "ufs" && preg_match("/s\d+$/", $config['mounts']['mount'][$i]['partition'])) {
				$config['mounts']['mount'][$i]['fsck'] = false;
			}
		}

		$config['version'] = "8.5";
	}

	/* Convert 8.5 -> 8.6 */
	/* Set enable/blocklen on iSCSI target */
	if ($config['version'] === "8.5") {
		for ($i = 0; isset($config['iscsitarget']['target'][$i]); $i++) {
			$config['iscsitarget']['target'][$i]['enable'] = true;
			if ($config['iscsitarget']['target'][$i]['type'] === "Disk") {
				$config['iscsitarget']['target'][$i]['blocklen'] = 512;
			} else {
				$config['iscsitarget']['target'][$i]['blocklen'] = 0;
			}
		}

		$config['version'] = "8.6";
	}

	/* Convert 8.6 -> 8.7 */
	/* Change locale name */
	if ($config['version'] === "8.6") {
		if ($config['system']['language'] === "no_NB") {
			$config['system']['language'] = "nb";
		}

		$config['version'] = "8.7";
	}

	/* Convert 8.7 -> 8.8 */
	/* Set iSCSI parameter */
	if ($config['version'] === "8.7") {
		if ($config['iscsitarget']['maxsessions'] === "32") {
			$config['iscsitarget']['maxsessions'] = "16";
		}
		if ($config['iscsitarget']['maxconnections'] === "8") {
			$config['iscsitarget']['maxconnections'] = "4";
		}
		if ($config['iscsitarget']['firstburstlength'] === "65536") {
			$config['iscsitarget']['firstburstlength'] = "262144";
		}
		if ($config['iscsitarget']['maxburstlength'] === "262144") {
			$config['iscsitarget']['maxburstlength'] = "1048576";
		}
		$config['iscsitarget']['maxr2t'] = "32";
		$config['iscsitarget']['maxoutstandingr2t'] = "16";
		$config['iscsitarget']['defaulttime2wait'] = "2";
		$config['iscsitarget']['defaulttime2retain'] = "60";

		$config['version'] = "8.8";
	}

	/* Convert 8.8 -> 8.9 */
	if ($config['version'] === "8.8") {
		if ($config['samba']['security'] === "domain") {
			$config['samba']['security'] = "ads";
		}

		$config['version'] = "8.9";
	}

	/* Convert 8.9 -> 9.0 */
	if ($config['version'] === "8.9") {
		/*
		unset($config['disks']);
		unset($config['mounts']);
		unset($config['gvinum']);
		unset($config['gmirror']);
		unset($config['gconcat']);
		unset($config['gstripe']);
		unset($config['graid5']);
		unset($config['geli']);
		*/
		// convert all ad devices to ada, and clear ZFS settings.
		disks_convert_to_ada();

		// this was converted from 0.7.2.
		$config['after072'] = true;

		$config['version'] = "9.0";
	}

	/* Convert 9.0 -> 9.1 */
	system_language_load();
	if ($config['version'] === "9.0") {
		if (!isset($config['system']['tune'])) {
			$config['system']['tune'] = true;
		}
		$a_mib = array(
			"net.inet.tcp.delayed_ack" => 0,
			"net.inet.tcp.rfc1323" => 1,
			"net.inet.tcp.sendspace" => 262144,
			"net.inet.tcp.recvspace" => 262144,
			"net.inet.tcp.sendbuf_max" => 4194304,
			"net.inet.tcp.sendbuf_inc" => 262144,
			"net.inet.tcp.sendbuf_auto" => 1,
			"net.inet.tcp.recvbuf_max" => 4194304,
			"net.inet.tcp.recvbuf_inc" => 262144,
			"net.inet.tcp.recvbuf_auto" => 1,
			"net.inet.udp.recvspace" => 65536,
			"net.inet.udp.maxdgram" => 57344,
			"net.local.stream.recvspace" => 65536,
			"net.local.stream.sendspace" => 65536,
			"kern.ipc.maxsockbuf" => 8388608,
			"kern.ipc.somaxconn" => 8192,
			"kern.ipc.nmbclusters" => 262144,
			"kern.ipc.nmbjumbop" => 262144,
			"kern.ipc.nmbjumbo9" => 65536,
			"kern.ipc.nmbjumbo16" => 32768,
			"kern.maxfiles" => 65536,
			"kern.maxfilesperproc" => 32768,
			"net.inet.icmp.icmplim" => 300,
			"net.inet.icmp.icmplim_output" => 1,
			//"net.inet.tcp.inflight.enable" => 0,
			"net.inet.tcp.path_mtu_discovery" => 0,
			"hw.intr_storm_threshold" => 9000,
		);
		foreach ($a_mib as $key => $val) {
			$index = array_search_ex($key, $config['system']['sysctl']['param'], "name");
			if ($index === false) {
				$config['system']['sysctl']['param'][] = array(
					"uuid" => uuid(),
					"name" => $key,
					"value" => $val,
					"comment" => gettext("System tuning"),
					"enable" => true );
			} else {
				$config['system']['sysctl']['param'][$index]['value'] = $val;
				$config['system']['sysctl']['param'][$index]['comment'] = gettext("System tuning");
				$config['system']['sysctl']['param'][$index]['enable'] = true;
			}
		}
		$index = array_search_ex("net.inet.tcp.inflight.enable", $config['system']['sysctl']['param'], "name");
		if ($index !== false) {
			unset($config['system']['sysctl']['param'][$index]);
		}

		$config['version'] = "9.1";
	}

	/* Convert 9.1 -> 9.2 */
	if ($config['version'] === "9.1") {
		if ($config['samba']['security'] === "share") {
			$config['samba']['maxprotocol'] = "NT1";
		} else {
			$config['samba']['maxprotocol'] = "SMB2";
		}
		$config['samba']['maptoguest'] = "Never";
		unset($config['samba']['usesendfile']);

		$config['version'] = "9.2";
	}

	/* Convert 9.2 -> 9.3 */
	if ($config['version'] === "9.2") {
		$index = array_search_ex("varsize", $config['system']['rcconf']['param'], "name");
		if ($index === false) {
			$config['system']['rcconf']['param'][] = array(
				"uuid" => uuid(),
				"name" => "varsize",
				"value" => "32m",
				"comment" => "Default /var size",
				"enable" => true );
		}
		$a_log = array(
			"fuppes_logfile" => "/var/log/fuppes.log",
			"mtdaapd_logfile" => "/var/log/mt-daapd.log",
			"rsync_client_logfile" => "/var/log/rsync_client.log",
			"rsync_local_logfile" => "/var/log/rsync_local.log",
		);
		foreach ($a_log as $key => $val) {
			$index = array_search_ex($key, $config['system']['rcconf']['param'], "name");
			if ($index === false) {
				$config['system']['rcconf']['param'][] = array(
					"uuid" => uuid(),
					"name" => $key,
					"value" => $val,
					"comment" => "Default log file",
					"enable" => true );
			}
		}

		$config['version'] = "9.3";
	}

	/* Convert 9.3 -> 9.4 */
	if ($config['version'] === "9.3") {
		$index = array_search_ex("kern.ipc.maxsockbuf", $config['system']['sysctl']['param'], "name");
		if ($index !== false) {
			$config['system']['sysctl']['param'][$index]['value'] = 16777216;
		}
		$index = array_search_ex("kern.ipc.nmbjumbo9", $config['system']['sysctl']['param'], "name");
		if ($index !== false) {
			$config['system']['sysctl']['param'][$index]['value'] = 131072;
		}
		$index = array_search_ex("kern.ipc.nmbjumbo16", $config['system']['sysctl']['param'], "name");
		if ($index !== false) {
			$config['system']['sysctl']['param'][$index]['value'] = 65536;
		}
		$config['lcdproc'] = array(
			"driver" => "hd44780",
			"port" => 13666,
			"waittime" => 10,
		);	
		$config['lcdproc']['lcdproc'] = array();
		$a1 = array(
			"# USB connected FT245R based LCD",
			"ConnectionType=ftdi",
			"ftdi_mode=4",
			"ftdi_line_RS=0x40",
			"ftdi_line_RW=0x20",
			"ftdi_line_EN=0x10",
			"#Port=0x378",
			"#Device=/dev/ttyu0",
			"#Speed=0",
			"Keypad=no",
			"Contrast=0",
			"#Brightness=1000",
			"#OffBrightness=0",
			"Backlight=no",
			"OutputPort=no",
			"#Lastline=yes",
			"Size=20x4",
			"#vspan=2,2",
			"#ExtendedMode=yes",
			"#LineAddress=0x10",
			"CharMap=hd44780_default",
			"#DelayMult=2",
			"#KeepAliveDisplay=0",
			"#RefreshDisplay=5",
			"DelayBus=true",
			"KeyMatrix_4_1=Enter",
			"KeyMatrix_4_2=Up",
			"KeyMatrix_4_3=Down",
			"KeyMatrix_4_4=Escape",
		);
		$a2 = array(
			"[CPU]",
			"Active=True",
			"OnTime=1",
			"OffTime=2",
			"ShowInvisible=false",
			"[Iface]",
			"Active=True",
			"Interface0=re0",
			"Alias0=LAN",
			"#Interface1=em0",
			"#Alias1=WAN",
			"[Iface]",
			"Active=True",
			"Interface0=re0",
			"Alias0=LAN",
			"#Interface1=em0",
			"#Alias1=WAN",
			"#Interface2=em1",
			"#Alias2=MGMT",
			"unit=bit",
			"#transfer=TRUE",
			"[Memory]",
			"Active=True",
			"[Load]",
			"Active=True",
			"LowLoad=0.05",
			"HighLoad=1.3",
			"[TimeDate]",
			"Active=True",
			"TimeFormat=\"%H:%M:%S\"",
			"DateFormat=\"%x\"",
			"[About]",
			"Active=false",
			"[SMP-CPU]",
			"Active=false",
			"[OldTime]",
			"Active=false",
			"TimeFormat=\"%H:%M:%S\"",
			"DateFormat=\"%x\"",
			"[BigClock]",
			"Active=false",
			"[Uptime]",
			"Active=false",
			"[Battery]",
			"Active=false",
			"[CPUGraph]",
			"Active=false",
			"[ProcSize]",
			"Active=false",
			"[Disk]",
			"Active=false",
			"[MiniClock]",
			"Active=false",
			"TimeFormat=\"%H:%M\"",
		);

		unset($config['lcdproc']['param']);
		foreach ($a1 as $param) {
			$config['lcdproc']['param'][] = $param;
		}
		unset($config['lcdproc']['lcdproc']['param']);
		foreach ($a2 as $param) {
			$config['lcdproc']['lcdproc']['param'][] = $param;
		}

		$config['version'] = "9.4";
	}

	/* Convert 9.4 -> 9.5 */
	if ($config['version'] === "9.4") {
		// remove entities
		if (is_array($config['lcdproc']['lcdproc']['param'])) {
			$tmp = array();
			foreach ($config['lcdproc']['lcdproc']['param'] as $param) {
				$param = str_replace('&quot;', '"', $param);
				$tmp[] = $param;
			}
			unset($config['lcdproc']['lcdproc']['param']);
			$config['lcdproc']['lcdproc']['param'] = $tmp;
		}
		// enable lcdproc
		if (isset($config['lcdproc']['enable'])) {
			$config['lcdproc']['lcdproc']['enable'] = true;
		}
		// disk serial
		if (isset($config['disks']) && isset($config['disks']['disk'])
		    && is_array($config['disks']['disk'])) {
			foreach ($config['disks']['disk'] as &$a) {
				$dev = $a['devicespecialfile'];
				if (file_exists($dev)) {
					$serial = system_get_volume_serial($dev);
				} else {
					$serial = "";
				}
				if (($serial == "n/a") || ($serial == gettext("n/a"))) {
					$serial = "";
				}
				$a['serial'] = $serial;
			}
			unset($a);
		}

		$config['version'] = "9.5";
	}

	/* Convert 9.5 -> 9.6 */
	if ($config['version'] === "9.5") {
		// modify device type IDE to SCSI
		if (isset($config['disks']) && isset($config['disks']['disk'])
		    && is_array($config['disks']['disk'])) {
			foreach ($config['disks']['disk'] as &$a) {
				if ($a['type'] == 'IDE') {
					$a['type'] = 'SCSI';
				}
			}
			unset($a);
		}
		// guest access
		if (isset($config['samba']) && isset($config['samba']['share'])
		    && is_array($config['samba']['share'])) {
			foreach ($config['samba']['share'] as &$a) {
				$a['guest'] = true;
			}
			unset($a);
		}
		// remove polling
		unset($config['interfaces']['lan']['polling']);
		for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++) {
			unset($config['interfaces']['opt' . $i]['polling']);
		}

		$config['version'] = "9.6";
	}

	write_config();

	if (is_booting())
		echo "done\n";

	return 0;
}

// hook for preinit
function config_preinit() {
	global $config, $g;

	$need_write_config = false;

	// temporary disable AIO
	if ($config['samba']['maxprotocol'] == "SMB2") {
		unset($config['samba']['aio']);
	}

	if ($need_write_config) {
		echo "save config\n";
		write_config();
	}
}

// hook for postinit
function config_postinit() {
	global $config, $g;

	$need_write_config = false;

	if (isset($config['after072'])) {
		// remove database
		if (isset($config['upnp']['enable'])) {
			rc_stop_service("fuppes");
		}
		if (!empty($config['upnp']['home'])) {
			$file = $config['upnp']['home']."/fuppes.db";
			if (file_exists($file)) {
				echo "remove old $file\n";
				unlink($file);
			}
		}
		if (isset($config['upnp']['enable'])) {
			rc_start_service("fuppes");
		}
		unset($config['after072']);
		$need_write_config = true;
	}

	if ($need_write_config) {
		echo "save config\n";
		write_config();
	}
}

?>
