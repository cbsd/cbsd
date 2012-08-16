<?php
/*
	disks.inc
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
require_once("functions.inc");
require_once("util.inc");

// Mount all configured disks.
// Return 0 if successful, 1 if error
function disks_mount_all()
{
	global $config, $g;

	// For each device configured:
	if (is_array($config['mounts']) && is_array($config['mounts']['mount'])) {
		// Mount real disks first
		foreach ($config['mounts']['mount'] as $mount) {
			if ("disk" !== $mount['type'])
				continue;

			// Advanced unmount filesystem if not in booting mode (mount edition).
			// Background: 'disks_umount' can't be used in the case the mount point
			// has been renamed by the user because it uses the 'sharename' (which is
			// now the new name of the share, but we need the old name) for unmounting.
			if (!is_booting())
				disks_umount_ex($mount);

			disks_mount($mount);
		}

		// Then mount ISOs
		foreach ($config['mounts']['mount'] as $mount) {
			if ("iso" !== $mount['type'])
				continue;

			// Advanced unmount filesystem if not in booting mode (mount edition)
			if (!is_booting())
				disks_umount($mount);

			disks_mount($mount);
		}
	}

	return 0;
}

// This function unmounts all configured mountpoints
// Return 0 if successful, otherwise 1
function disks_umount_all()
{
	global $config;

	// Sync disks first
	mwexec("/bin/sync");

	if (is_array($config['mounts'])) {
		foreach ($config['mounts']['mount'] as $mountent) {
			disks_umount($mountent);
		}
	}

	return 0;
}

// Mount using the configured mount given in parameter
// Return 0 if successful, 1 if error
function disks_mount($mount)
{
	global $config, $g;

	// Set mount name
	$mountname = "{$g['media_path']}/{$mount['sharename']}";

	// Create one directory for each device under /mnt
	if (!mkdir($mountname)) {
		write_log("Error: Failed to create directory {$mountname}");
	}

	/* Mount the filesystem */
	/* In booting mode, skip encrypted disk (need to enter the passphrase on the webgui) */
	if (is_booting()) {
		if (is_array($config['geli']) && is_array($config['geli']['vdisk'])) {
			if (array_search_ex($mount['mdisk'], $config['geli']['vdisk'], "devicespecialfile")) {
				write_log("Skip mounting '{$mount['sharename']}': Passphrase needed for '{$mountname}'.");
				return 0;
			}
		}
	}

	switch($mount['type']) {
		case "disk":
			$devicespecialfile = $mount['devicespecialfile'];
			break;

		case "iso":
			// Create memory disk. 'mdconfig' returns the ID of this disk
			exec("/sbin/mdconfig -a -t vnode -f '{$mount['filename']}'", $mdid, $result);
			if (0 !== $result) {
				write_log("Error: Failed to create memory disk for '{$mount['filename']}'");
				return 1;
			}
			$devicespecialfile = "/dev/{$mdid[0]}";
			break;
	}

	/* Check if filesystem should be mounted read-only */
	$readonly = (isset($mount['readonly'])) ? "-r" : "";

	/* Check the fileystem only if there is a problem. */
	/* This part is too stupid: I must read the FreeBSD start script for use the same intelligent method for checking hard drive */
	switch ($mount['fstype']) {
		case "ufs":
			$result = mwexec("/sbin/mount -t ufs -o acls {$readonly} {$devicespecialfile} '{$mountname}'", true);
			if (0 == $result) {
				write_log("Device {$devicespecialfile} mounted using UFS on '{$mountname}'");
				$result = 0;
			} else {
				rmdir($mountname);
				write_log("Error: Failed to mount {$devicespecialfile} using UFS");
				$result = 1;
			}
			break;

		case "msdosfs":
			$result = mwexec("/sbin/mount -t msdosfs -o large {$readonly} {$devicespecialfile} '{$mountname}'", true);
			if (0 == $result) {
				write_log("Device {$devicespecialfile} mounted using FAT on '{$mountname}'");
				$result = 0;
			} else {
				rmdir($mountname);
				write_log("Error: Failed to mount {$devicespecialfile} using FAT");
				$result = 1;
			}
			break;

		case "udf":
		case "cd9660":
			$result = mwexec("/sbin/mount -t {$mount['fstype']} {$devicespecialfile} '{$mountname}'", true);
			if (0 == $result) {
				write_log("Device {$devicespecialfile} mounted as CD/DVD on '{$mountname}'");
				$result = 0;
			} else {
				rmdir($mountname);
				write_log("Error: Failed to mount {$devicespecialfile} as CD/DVD (fstype={$mount['fstype']})");
				$result = 1;
			}
			break;

		case "ntfs":
			// Load the kernel FUSE module
			mwexec("/sbin/kldload /boot/kernel/fuse.ko");
			$result = mwexec("/sbin/mount -t ntfs {$readonly} {$devicespecialfile} '{$mountname}'", true); // default options: silent,allow_other
			if (0 == $result) {
				write_log("Disk {$devicespecialfile} mounted using NTFS on '{$mountname}'");
				$result = 0;
			} else {
				rmdir($mountname);
				write_log("Error: Failed to mount {$devicespecialfile} using NTFS");
				$result = 1;
			}
			break;

		case "ext2fs":
			$result = mwexec("/sbin/mount -t ext2fs {$readonly} {$devicespecialfile} '{$mountname}'", true);
			if (0 == $result) {
				write_log("Disk {$devicespecialfile} mounted using EXT2 on '{$mountname}'");
				$result = 0;
			} else {
				rmdir("{$mountname}");
				write_log("Error: Failed to mount {$devicespecialfile} using EXT2");
				$result = 1;
			}
			break;
	}

	// Set access restrictions. Use the following default values.
	$mode = "777";
	$owner = "root";
	$group = "wheel";

	if (is_array($mount['accessrestrictions'])) {
		$mode = $mount['accessrestrictions']['mode'];
		$owner = $mount['accessrestrictions']['owner'];
		$group = $mount['accessrestrictions']['group'][0];
	}

	// Check if target directory is writable, otherwise we will get an error message.
	// For ISO's this may be 'chmod: /mnt/xxx: Read-only file system)'
	if (file_exists($mountname) && is_writable($mountname)) {
		if (!chmod($mountname, octdec($mode)) || !chown($mountname, $owner) || !chgrp($mountname, $group)) {
			write_log("Error: Failed to set access restrictions on directory '{$mountname}'");
		}
	}

	return $result;
}

// Unmount the specified configured mount point.
// Return 0 is successfull, 1 if error
function disks_umount($mount)
{
	global $g;

	// Get memory disk unit for ISOs
	if ("iso" === $mount['type']) {
		// Get current mount list
		$amountlist = get_mounts_list();

		foreach ($amountlist as $amountk => $amountv) {
			if (0 == strcmp($amountv['sharename'], $mount['sharename'])) {
				// Extract memory disk unit id
				preg_match("/^\/dev\/md(\d+)$/", $amountv['devicespecialfile'], $matches);
				$mdunit = $matches[1];
				break;
			}
		}
	}

	// Unmount mount point
	$mountname = "{$g['media_path']}/{$mount['sharename']}";

	if (mwexec("/sbin/umount -f '{$mountname}'") == 0) {
		if(rmdir("{$mountname}")) {
			write_log("Successfully unmounted '{$mountname}'");
			$result = 0;
		} else {
			write_log("Error: Failed to unmount '{$mountname}'");
			$result = 1;
		}
	} else {
		write_log("Error: Failed to unmount '{$mountname}'");
		$result = 1;
	}

	// Detach memory disk for ISOs
	if (0 == $result && "iso" === $mount['type']) {
		$result = mwexec("/sbin/mdconfig -d -u {$mdunit}");
	}

	return $result;
}

// Advanced unmount the specified mount point without using the sharename,
// e.g. used when the 'sharename' has been modified/changed. Use the device
// special file (e.g. /dev/ad1p1) instead.
// Return 0 if successful, 1 if error
function disks_umount_ex($mount)
{
	// Get current mount list
	$amountlist = get_mounts_list();

	foreach ($amountlist as $amountk => $amountv) {
		if (0 == strcmp($amountv['devicespecialfile'], $mount['devicespecialfile'])) {
			$mountname = $amountv['mp'];
			break;
		}
	}

	if ($mountname) {
		if (mwexec("/sbin/umount -f '{$mountname}'") == 0) {
			if(rmdir("{$mountname}")) {
				write_log("Successfully unmounted '{$mountname}'");
				$result = 0;
			} else {
				write_log("Error: Failed to unmount '{$mountname}'");
				$result = 1;
			}
		} else {
			write_log("Error: Failed to unmount '{$mountname}'");
			$result = 1;
		}
	}

	return $result;
}

// Check if mount point is mounted.
// $mount - Mount point config array
// Return 0 if not mounted, otherwise 1
function disks_ismounted($mount)
{
	// MUST CHECK IF IT'S NTFS filesystem
	if (0 == strcmp($mount['fstype'],"ntfs")) {
		return disks_ismounted_ex($mount['sharename'], "sharename");
	} else {
		return disks_ismounted_ex($mount['devicespecialfile'], "devicespecialfile");
	}
}

// Check if mountpoint is mounted.
// $var - The attribute value to search for
// $key - The attribute name to search for
// e.g.:  disks_ismounted_ex("/dev/ad0s1","devicespecialfile");
//        disks_ismounted_ex("data","sharename");
//        disks_ismounted_ex("ad0","mdisk");
// Return 0 if not mounted, otherwise 1
function disks_ismounted_ex($var,$key)
{
	if (empty($var))
		return 0;

	$result = 0;

	// Get current mount list
	$amountlist = get_mounts_list();

	foreach ($amountlist as $amountk => $amountv) {
		switch ($key) {
			case "mdisk":
			case "devicespecialfile":
				// Must find the $val (e.g. ad0) in 'mdisk' (e.g. ad0s1).
				// Must find the $val (e.g. /dev/ad1) in 'devicespecialfile' (e.g. /dev/ad1s1).
				if (0 == preg_match("/^.+\/(\S{2,}\d+)([sp]\d+)$/", $var, $matches)) {
					// e.g. /dev/ad10, /dev/cd1
					if (preg_match("/" . preg_quote($var, "/") . "\D+/", $amountv[$key])) {
						$result = 1;
						break;
					}
				} else {
					// e.g. /dev/da1p2
					if (false !== strstr($amountv[$key], $var)) {
						$result = 1;
						break;
					}
				}
				break;

			default:
				if (0 == strcmp($amountv[$key], $var)) {
					$result = 1;
					break;
				}
				break;
		}
	}

	return $result;
}

// Configure, create and start gvinum volume.
// $uuid - UUID of the RAID to be configured.
// Return 0 if successful, 1 if error
function disks_raid_gvinum_configure($uuid) {
	global $g, $config;

	if (!is_array($config['gvinum']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['gvinum']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['gvinum']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Generate the raid.conf file
	$fd = fopen("{$g['varetc_path']}/raid-{$vdisk['name']}.conf", "w");
	if (!$fd) {
		write_console("Error: Failed to open raid.conf in services_raid_configure().\n");
		return 1;
	}

	$raidconf="";
	foreach ($vdisk['device'] as $devicek => $devicev) {
		$raidconf .= <<<EOD
drive disk_{$devicev} device {$devicev}

EOD;
	}

	$raidconf .= <<<EOD
volume {$vdisk['name']}

EOD;
	switch ($vdisk['type']) {
			case "0":
				$raidconf .= <<<EOD
plex org striped 256k

EOD;
				foreach ($vdisk['device'] as $devicek => $devicev) {
					/* Get the disksize */
					$diskinfo=disks_get_diskinfo($devicev);
					/* $raidconf .= <<<EOD
sd length {$diskinfo['mediasize_mbytes']}M drive disk_{$devicev}

EOD; */
					$raidconf .= <<<EOD
sd length 0 drive disk_{$devicev}

EOD;
				}
				break;
			case "1":
				foreach ($vdisk['device'] as $devicek => $devicev) {
					$raidconf .= <<<EOD
plex org concat

EOD;
					/* Get the disksize */
					$diskinfo=disks_get_diskinfo($devicev);
					/* $raidconf .= <<<EOD
sd length {$diskinfo['mediasize_mbytes']}M drive disk_{$devicev}

EOD; */

					$raidconf .= <<<EOD
sd length 0 drive disk_{$devicev}

EOD;
				}

				break;
			case "5":
			$raidconf .= <<<EOD
plex org raid5 256k

EOD;
				foreach ($vdisk['device'] as $devicek => $devicev) {
					/* Get the disksize */
					$diskinfo=disks_get_diskinfo($devicev);
					/* $raidconf .= <<<EOD
sd length {$diskinfo['mediasize_mbytes']}M drive disk_{$devicev}

EOD; */

					$raidconf .= <<<EOD
sd length 0 drive disk_{$devicev}

EOD;
				}
				break;
	}
	fwrite($fd, $raidconf);
	fclose($fd);

	// Create volume
	mwexec("/sbin/gvinum create {$g['varetc_path']}/raid-{$vdisk['name']}.conf", true);

	// Start volume
	mwexec("/sbin/gvinum start {$vdisk['name']}", true);

	return 0;
}

// Configure, create and start gmirror volume.
// $uuid - UUID of the RAID to be configured.
// Return 0 if successful, 1 if error
function disks_raid_gmirror_configure($uuid) {
	global $config;

	if (!is_array($config['gmirror']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['gmirror']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['gmirror']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Create volume
	$cmd = "/sbin/gmirror label -b {$vdisk['balance']} {$vdisk['name']} ";
	foreach ($vdisk['device'] as $devicek => $devicev) {
		$cmd .= "{$devicev} ";
	}

	return mwexec($cmd, true);
}

// Configure, create and start gconcat volume.
// $uuid - UUID of the RAID to be configured.
// Return 0 if successful, 1 if error
function disks_raid_gconcat_configure($uuid) {
	global $config;

	if (!is_array($config['gconcat']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['gconcat']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['gconcat']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Create volume
	$cmd = "/sbin/gconcat label {$vdisk['name']} ";
	foreach ($vdisk['device'] as $devicek => $devicev) {
		$cmd .= "{$devicev} ";
	}

	return mwexec($cmd, true);
}

// Configure, create and start gstripe volume.
// $uuid - UUID of the RAID to be configured.
// Return 0 if successful, 1 if error
function disks_raid_gstripe_configure($uuid) {
	global $config;

	if (!is_array($config['gstripe']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['gstripe']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['gstripe']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Create volume
	$cmd = "/sbin/gstripe label {$vdisk['name']} ";
	foreach ($vdisk['device'] as $devicek => $devicev) {
		$cmd .= "{$devicev} ";
	}

	return mwexec($cmd, true);
}

// Configure, create and start graid5 volume.
// $uuid - UUID of the RAID to be configured.
// Return 0 if successful, 1 if error
function disks_raid_graid5_configure($uuid) {
	global $config;

	if (!is_array($config['graid5']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['graid5']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['graid5']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Create volume
	$cmd = "/sbin/graid5 label -s 131072 {$vdisk['name']} ";
	foreach ($vdisk['device'] as $devicek => $devicev) {
		$cmd .= "{$devicev} ";
	}

	return mwexec($cmd, true);
}

// Delete geom gvinum volume given in parameter.
// $uuid - UUID of the RAID to be deleted.
// Return 0 if successful, 1 if error
function disks_raid_gvinum_delete($uuid) {
	global $config;

	if (!is_array($config['gvinum']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['gvinum']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['gvinum']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	exec("/sbin/gvinum lv {$vdisk['name']}", $rawdata);
	if (strpos($rawdata[0],"State: up") === false) {
		return 0;
	}

	mwexec("/sbin/gvinum rm -r {$vdisk['name']}");

	foreach ($vdisk['device'] as $device){
		mwexec("/sbin/gvinum rm -r disk_{$device}");
	}

	return 0;
}

// Delete geom mirror volume given in parameter.
// $uuid - UUID of the RAID to be deleted.
// Return 0 if successful, 1 if error
function disks_raid_gmirror_delete($uuid) {
	global $config;

	if (!is_array($config['gmirror']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['gmirror']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['gmirror']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Stop the volume
	mwexec("/sbin/gmirror stop {$vdisk['name']}");

	// Clear the gmirror information on the hard drive
	foreach ($vdisk['device'] as $device) {
		mwexec("/sbin/gmirror clear {$device}");
	}

	return 0;
}

// Delete geom concat volume given in parameter.
// $uuid - UUID of the RAID to be deleted.
// Return 0 if successful, 1 if error
function disks_raid_gconcat_delete($uuid) {
	global $config;

	if (!is_array($config['gconcat']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['gconcat']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['gconcat']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Stop the volume
	mwexec("/sbin/gconcat stop {$vdisk['name']}");

	// Clear the gconcat information on the hard drive
	foreach ($vdisk['device'] as $device) {
		mwexec("/sbin/gconcat clear {$device}");
	}
	mwexec("/sbin/gconcat destroy {$vdisk['name']}");

	return 0;
}

// Delete geom stripe volume given in parameter.
// $uuid - UUID of the RAID to be deleted.
// Return 0 if successful, 1 if error
function disks_raid_gstripe_delete($uuid) {
	global $config;

	if (!is_array($config['gstripe']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['gstripe']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['gstripe']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Stop the volume
	mwexec("/sbin/gstripe stop {$vdisk['name']}");

	// Clear the gconcat information on the hard drive
	foreach ($vdisk['device'] as $device) {
		mwexec("/sbin/gstripe clear {$device}");
	}
	mwexec("/sbin/gstripe destroy {$vdisk['name']}");

	return 0;
}

// Delete geom raid5 volume given in parameter.
// $uuid - UUID of the RAID to be deleted.
// Return 0 if successful, 1 if error
function disks_raid_graid5_delete($uuid) {
	global $config;

	if (!is_array($config['graid5']['vdisk']))
		return 1;

	$index = array_search_ex($uuid, $config['graid5']['vdisk'], "uuid");
	if (false === $index)
		return 1;

	$vdisk = $config['graid5']['vdisk'][$index];
	if (!is_array($vdisk))
		 return 1;

	// Stop the volume
	// Arne Woerner advice: Don't stop the volume before deleting it.
	// mwexec("/sbin/graid5 stop {$vdisk['name']}");

	// Clear the graid5 information on the hard drive
	foreach ($vdisk['device'] as $device){
		// Get the name of the provider
		$id = array_search_ex($device, $config['disks']['disk'], "devicespecialfile");
		$provider = $config['disks']['disk'][$id]['name'];
		// Remove provider
		mwexec("/sbin/graid5 remove {$vdisk['name']} {$provider}");
	}
	mwexec("/sbin/graid5 destroy {$vdisk['name']}");

	return 0;
}

// Initialize provider which needs to be encrypted.
// Return 0 if successful, 1 if error
function disks_geli_init($provider, $aalgo, $ealgo, $keylen, $passphrase, $verbose = false)
{
	$param = "-t";
	$result = 1;

	// Add additional parameters
	if (true == $verbose) $param .= " -v";
	if (0 !== strcmp($aalgo, "none")) $param .= " -a {$aalgo}";
	if (!empty($ealgo)) $param .= " -e {$ealgo}";
	if (!empty($keylen)) $param .= " -l {$keylen}";

	// Init disk
	system("(/bin/echo {$passphrase}; /bin/echo {$passphrase}) | /sbin/geli init {$param} {$provider} 2>&1", $result);

	return $result;
}

// Attach the given provider. The master key will be decrypted
// using the given passphrase/keyfile and a new GEOM provider
// will be created using the given provider's name with an ".eli"
// suffix.
// Return 0 if successful, 1 if error
function disks_geli_attach($provider, $passphrase, $verbose = false)
{
	$param = "-t";
	$result = 1;

	// Add additional parameters
	if (true == $verbose) $param .= " -v";

	// Attach disk
	system("(/bin/echo {$passphrase}; /bin/echo {$passphrase}) | /sbin/geli attach {$param} {$provider} 2>&1", $result);

	return $result;
}

// Detach the given providers, which means remove the devfs entry
// and clear the keys from memory.
// Return 0 if successful, 1 if error
function disks_geli_detach($provider, $verbose = false)
{
	if (true == $verbose)
		$result = system("/sbin/geli detach {$provider} 2>&1", $result);
	else
		$result = mwexec("/sbin/geli detach {$provider}");

	return $result;
}

// Kill (destroy) a geli encrypted volume.
// Return 0 if successful, 1 if error
function disks_geli_kill($provider)
{
	$result = mwexec("/sbin/geli kill {$provider}");
	return $result;
}

// Detach all geli encrypted disks.
// Return 0 if successful, 1 if error
function disks_geli_detach_all()
{
	global $config;

	$result = 0;

	if (is_array($config['geli']['vdisk'])) {
		foreach($config['geli']['vdisk'] as $geli) {
			$result |= disks_geli_detach($geli['devicespecialfile']) ;
		}
	}

	return $result;
}

// Change or setup (if not yet initialized) selected key.
// The passphrase can always be changed: for an attached provider and
// a detached provider. When a provider is attached, the user does not
// have to provide an old passphrase.
// Parameter:
// $devicespecialfile: e.g. /dev/ad1.eli or /dev/raid5/xxx.eli
// $oldpassphrase: Old passphrase
// $passphrase: New passphrase
// $verbose: Display additional information
// Return 0 if successful, 1 if error
function disks_geli_setkey($devicespecialfile, $oldpassphrase, $passphrase, $verbose = false)
{
	$param = "-t";
	$result = 1;

	// Extract provider data.
	$provider = preg_replace('/(\S+)(\.eli)/', '${1}', $devicespecialfile);

	// Add additional parameters
	if (true == $verbose) $param .= " -v";

	if (0 == disks_exists($devicespecialfile)) {
		// Provider is already attached
		system("(/bin/echo {$passphrase}; /bin/echo {$passphrase}) | /sbin/geli setkey {$param} {$provider} 2>&1", $result);
	} else {
		// Provider has not already been attached
		system("(/bin/echo {$oldpassphrase}; /bin/echo {$passphrase}; /bin/echo {$passphrase}) | /sbin/geli setkey {$param} {$provider} 2>&1", $result);
	}

	return $result;
}

// Format disk. Error and warning messages are redirected to stdout.
// Return none
function disks_format($disk, $type, $notinitmbr, $minspace, $volumelabel, $aft4k)
{
	global $g;

	// Get disk information
	$diskinfo = disks_get_diskinfo($disk);

	system("/sbin/sysctl kern.geom.debugflags=0x10 2>&1");
	if (!$notinitmbr) {
		echo gettext("Deleting MBR and partition table")."...\n";
		$count = (1024*1024) / $diskinfo['sectorsize'];
		system("/bin/dd if=/dev/zero of={$disk} bs={$diskinfo['sectorsize']} count={$count} 2>&1");
		$oseek = $diskinfo['mediasize_sectors'] - $count;
		system("/bin/dd if=/dev/zero of={$disk} bs={$diskinfo['sectorsize']} oseek={$oseek} count={$count} 2>&1");
	}
	if ($aft4k) {
		// Advanced Format 4KB (512 x 8)
		// IDENTIFY = 6003
		$aft_secs = (1 << 3);
		$aft_bytes = 512 * $aft_secs;
		//$aft_align = $aft_secs; // 4KB
		$aft_align = 64; // 32KB
		// block:fragment ratio is 8:1
		$aft_blocksize = $aft_bytes * 8;
		// MBR(1) + Pri GPT header(1) + Pri GPT table(32)
		//  + Sec GPT table(32) + Sec GPT header(1)
		$aft_gptstart = 1 + 1 + 32;
		$aft_aligned = ceil($aft_gptstart / $aft_align) * $aft_align;
		$aft_counts = $diskinfo['mediasize_sectors'];
		$aft_counts -= 1 + ((1 + 32) * 2);
		$aft_counts -= ($aft_aligned - $aft_gptstart);
		$aft_counts = floor($aft_counts / $aft_align) * $aft_align;
	}

	switch ($type) {
		case "ufsgpt":
			echo gettext("Creating partition")."...\n";
			// Destroy old GTP partition
			//system("/sbin/gpt destroy {$disk} 1>/dev/null 2>&1");
			system("/sbin/gpart destroy {$disk} 1>/dev/null 2>&1");
			// Create new GPT partition
			//system("/sbin/gpt create -f {$disk} 2>&1");
			system("/sbin/gpart create -s gpt {$disk} 2>&1");
			if ($aft4k) {
				//system("/sbin/gpt add -b $aft_aligned -s $aft_counts -t ufs {$disk} 2>&1");
				system("/sbin/gpart add -b $aft_aligned -s $aft_counts -t freebsd-ufs {$disk} 2>&1");
			} else {
				//system("/sbin/gpt add -t ufs {$disk} 2>&1");
				system("/sbin/gpart add -t freebsd-ufs {$disk} 2>&1");
			}

			// Create filesystem
			echo gettext("Creating filesystem with 'Soft Updates'")."...\n";
			$param = "-U -j";
			if (!empty($volumelabel))
				$param .= " -L " . escapeshellarg($volumelabel);
			if (!empty($minspace))
				$param .= " -m {$minspace}";
			if ($aft4k) {
				echo sprintf(gettext("Using %s bytes fragment size."), $aft_bytes)."\n";
				$param .= " -S $aft_bytes -b $aft_blocksize -f $aft_bytes";
			}
			//echo "/sbin/newfs {$param} {$disk}p1\n";
			system("/sbin/newfs {$param} {$disk}p1 2>&1");

			echo "\n";
			echo gettext("Created UFS file system:")."\n";
			system("/sbin/dumpfs -l {$disk}p1 2>&1");
			echo "\n";
			break;

		case "softraid":
		case "zfs":
			// Calculate last sector of disk containing GEOM metadata.
			$oseek = $diskinfo['mediasize_sectors'] - 1;

			// Initialize disk
			// Ensure that existing GEOM metadata is completely removed.
			echo gettext("Deleting GEOM metadata")."...\n";
			system("/bin/dd if=/dev/zero of={$disk} bs={$diskinfo['sectorsize']} oseek={$oseek} count=1 2>&1");
			break;

		case "msdos":
			// Get valid CHS information. 'diskinfo' gets values from harddisk firmware that
			// are maybe unusable by 'fdisk' (e.g. sectors = 255, fdisk requires <= 63).
			disk_sanitize_bios_geom($diskinfo);

			// Create fdisk config file (fdisk.conf)
			if (1 == disks_create_fdisk_configfile(11, $diskinfo, "{$g['tmp_path']}/fdisk.conf")) {
				echo("Error: Failed to create '{$g['tmp_path']}/fdisk.conf'.\n");
				break;
			}

			// Initialize disk
			echo gettext("Creating partition")."...\n";
			system("/sbin/fdisk -v -f {$g['tmp_path']}/fdisk.conf {$disk} 2>&1");
			unlink("{$g['tmp_path']}/fdisk.conf");

			// Create filesystem
			echo gettext("Creating filesystem")."...\n";
			$param = "-F 32";
			if (!empty($volumelabel))
				$param .= " -L " . escapeshellarg($volumelabel);
			system("/sbin/newfs_msdos {$param} {$disk}s1 2>&1");

			break;

		case "ext2":
			// Get valid CHS information. 'diskinfo' gets values from harddisk firmware that
			// are maybe unusable by 'fdisk' (e.g. sectors = 255, fdisk requires <= 63).
			disk_sanitize_bios_geom($diskinfo);

			// Create fdisk config file (fdisk.conf)
			if (1 == disks_create_fdisk_configfile(131, $diskinfo, "{$g['tmp_path']}/fdisk.conf")) {
				echo("Error: Failed to create '{$g['tmp_path']}/fdisk.conf'.\n");
				break;
			}

			// Initialize disk
			echo gettext("Creating partition")."...\n";
			system("/sbin/fdisk -v -f {$g['tmp_path']}/fdisk.conf {$disk} 2>&1");
			unlink("{$g['tmp_path']}/fdisk.conf");

			// Create filesystem
			// Note: Use revision 0 when formating EXT2 filesystems, otherwise
			// mount_ext2fs fails because revision 1 uses a inode size of 256 bytes
			// which is not supported by FreeBSD. Using 128 bytes via '-I 128'
			// could solve that problem but the file system is permanently in
			// 'not clean' state.
			echo gettext("Creating filesystem")."...\n";
			$param = "-r 0";
			if (!empty($volumelabel))
				$param .= " -L " . escapeshellarg($volumelabel);
			system("/usr/local/sbin/mke2fs {$param} {$disk}s1 2>&1");

			break;
	}
	system("/sbin/sysctl kern.geom.debugflags=0 2>&1");

	echo gettext("Done")."!\n";
}

// File system consistency check and interactive repair.
// The filesystem type will be get from the mount configuration information.
// Parameter:
// device - Name of the device (e.g. /dev/ad1s1).
// umount - Unmount disk if necessary.
// Return 0 if successful, 1 if error
function disks_fsck($device,$umount = false)
{
	global $config;

	// Check if there exits any mounts
	if(!is_array($config['mounts']['mount']))
		return 1;

	// Get the id of the disk
	$id = array_search_ex($device, $config['mounts']['mount'], "devicespecialfile");
	if (false === $id)
		return 1;

	// Get the mount configuration
	$mount = $config['mounts']['mount'][$id];

	// Check if disk is mounted
	$ismounted = disks_ismounted($mount);

	// Unmount disk temporarily if necessary
	if($umount && $ismounted) {
		echo("<strong class='red'>" . gettext("Note") . ":</strong> " . gettext("The disk is currently mounted! The mount point will be removed temporarily to perform selected command.") . "<br /><br />");
		disks_umount($mount);
	}

	switch($mount['fstype']) {
		case "cd9660":
		case "ntfs":
			break;
		default:
			system("/sbin/fsck -t {$mount['fstype']} -y -f " . escapeshellarg($device));
			break;
	}

	// Remount disk again if necessary
	if($umount && $ismounted) {
		disks_mount($mount);
	}

	return 0;
}

// Get list of partition information from disk.
// $disk - Name of the disk (e.g. ad0)
// Result is in the form:
// [1] => Array
// (
// 	[start] => 31
// 	[size]  => 409169
// 	[type]  => 0xa5
// 	[flags] => 0x80
// )
//
// Called in disks_manage_tools.php
// Return array of partition info
function disks_get_partition_info($disk) {
  exec("/sbin/fdisk -s {$disk}", $rawdata);

  array_shift($rawdata);
  array_shift($rawdata);

  $result = array();

  foreach($rawdata as $partinfo)   {
    $apartinfo = preg_split("/\s+/", $partinfo);
    $partid = chop($apartinfo[1],":");

    $result[$partid] = array();
		$result[$partid]['start'] = chop($apartinfo[2]);
		$result[$partid]['size']  = chop($apartinfo[3]);
		$result[$partid]['type']  = chop($apartinfo[4]);
		$result[$partid]['flags'] = chop($apartinfo[5]);
  }

  return $result;
}

// Get UFS ID from partition.
// the partition (e.g. ada1p1)
// Result is ufsid.
function disks_get_ufsid($partition) {
	exec("/sbin/dumpfs -l ${partition}", $rawdata);
	$info = preg_split("/\s+/", $rawdata[0]);
	$ufsid = "";
	if (preg_match("/^\/dev\/ufsid\/(.*)$/", $info[0], $m)) {
		$ufsid = $m[1];
	}
	return $ufsid;
}

// Get all disk labels.
function disks_get_labels() {
	$result = array();
	exec("/sbin/geom label status -s", $rawdata);
	foreach($rawdata as $line) {
		if (preg_match("/^\s*(\S+)\s+.*?\s+(.+)$/", $line, $m)) {
			$result[$m[1]] = $m[2];
		}
	}
	return $result;
}

function disks_label_to_device($device) {
	$labels = disks_get_labels();
	foreach($labels as $key => $val) {
		if (strcmp($device, "/dev/".$key) == 0) {
			// found label
			return "/dev/".$val;
		}
	}
	return $device;
}

// Get ad disk map.
// Result is in the form:
// Array
// (
// 	[ad0] => ada0
// 	[ad2] => ada1
// 	[ad0p1] => ada0p1
// )
function disks_get_admap() {
	$result = array();
	$dir = "/dev";
	if (($dh = opendir($dir)) !== false) {
		while (($file = readdir($dh)) !== false) {
			if (!preg_match('/^ad[0-9]+.*/', $file)) continue;
			// adXX -> adaYYY
			if (is_link($dir."/".$file)) {
				$link = readlink($dir."/".$file);
				$result[$file] = $link;
			}
		}
		closedir($dh);
	}
	return $result;
}

// Get disk information.
// $disk - Name of the disk (e.g. ad0)
// Result is in the form:
// [1] => Array
// (
// 	[name] => ad0
// 	[sectorsize]  => 512
// 	[mediasize_bytes]  => 5242503168
// 	[mediasize_mbytes]  => 4999
// 	[mediasize_sectors]  => 10239264
// 	[stripesize] => 0
// 	[stripeoffset] => 0
// 	[cylinders] => 10158
// 	[heads] => 16
// 	[sectors] => 63
// )
// Return array containing diskinfo.
function disks_get_diskinfo($disk)
{
	exec("/usr/sbin/diskinfo {$disk}", $rawdata);

	$diskinfo = preg_split("/\s+/", $rawdata[0]);

	$result = array();
	$result[name] = chop($diskinfo[0]);
	$result[sectorsize] = chop($diskinfo[1]);
	$result[mediasize_bytes] = chop($diskinfo[2]);
	$result[mediasize_mbytes] = ceil(chop($diskinfo[2]) / 1024 / 1024);
	$result[mediasize_sectors] = chop($diskinfo[3]);
	$result[stripesize] = chop($diskinfo[4]);
	$result[stripeoffset] = chop($diskinfo[5]);
	$result[cylinders] = chop($diskinfo[6]);
	$result[heads] = chop($diskinfo[7]);
	$result[sectors] = chop($diskinfo[8]);

	return $result;
}

// Get cylinders/heads/sectors from disk.
// $info - Array containing CHS information.
//         [1] => Array
//         (
// 	          [cylinders] => 10158
//            [heads] => 16
//            [sectors] => 63
//         )
// Return 0 if successful, 1 if error.
function disks_get_chs_info($disk, &$info)
{
	if (!is_array($info))
		return 1;

	exec("/sbin/fdisk -t {$disk}", $rawdata);

	// Parse string: cylinders=77826 heads=255 sectors/track=63 (16065 blks/cyl)
	if (0 == preg_match("/.+cylinders=(\d+) heads=(\d+) sectors\/track=(\d+) .+/", implode($rawdata), $chs))
		return 1;

	$info['cylinders'] = $chs[1];
	$info['heads'] = $chs[2];
	$info['sectors'] = $chs[3];

	return 0;
}

// Check if given disk is available.
// $disk - e.g. /dev/ad4, /dev/ad2.eli
// Return 0 if yes, 1 if no.
function disks_exists($disk)
{
	if (file_exists($disk)) {
		return 0;
	}
	return 1;
}

// Wrapper to execute geom commands.
// $class   - Name of geom class (e.g. stripe, mirror, vinum, raid5, ...)
// $command - Command to execute (e.g. list, status, load, activate, ...).
//            The commands depend on the GEOM class.
// $param   - The command parameter.
// $verbose - Display command results or hide them.
// $stderr  - Redirect stderr to stdout to display error messages too.
// $output  - Output array filled with every line of output.
// Return 0 if successful, 1 if error.
function disks_geom_cmd($class, $command, $param, $verbose = false, $stderr = true, &$output = false)
{
	$result = 1;

	if ("vinum" !== $class)
		$cmd = "/sbin/geom {$class} {$command} {$param}";
	else
		$cmd = "/sbin/gvinum {$command} {$param}"; // gvinum can't be executed via geom

	if (true === $verbose) {
		if (true === $stderr)
			$cmd .= " 2>&1"; // Redirect error message to stdout

		if (false !== $output) {
			exec($cmd, $output, $result);
		} else {
			system($cmd, $result);
		}
	}	else {
		$result = mwexec($cmd);
	}

	return $result;
}

// Calculate valid cylinders/heads/sectors.
// Code taken from FreeBSD kernel /src/lib/libdisk/change.c.
// $info [in|out] - Array containing CHS information.
//                 [1] => Array
//                 (
// 	                 [name] => ad0
// 	                 [sectorsize]  => 512
// 	                 [mediasize_bytes]  => 5242503168
// 	                 [mediasize_mbytes]  => 4999
// 	                 [mediasize_sectors]  => 10239264
// 	                 [cylinders] => 10158
// 	                 [heads] => 16
// 	                 [sectors] => 63
//                 )
// Return: None
function disk_sanitize_bios_geom(&$info) {
	$sane = 1;

	if ($info['cylinders'] > 1024)
		$sane = 0;
	if ($info['heads'] > 16)
		$sane = 0;
	if ($info['sectors'] > 63)
		$sane = 0;
	if ($info['cylinders'] * $info['heads'] * $info['sectors'] != $info['mediasize_sectors'])
		$sane = 0;
	if (0 != $sane)
		return;

	// First try something that IDE can handle
	$info['sectors'] = 63;
	$info['heads'] = 16;
	$info['cylinders'] = intval($info['mediasize_sectors'] / ($info['sectors'] * $info['heads']));

	if ($info['cylinders'] < 1024)
		return;

	// Hmm, try harder...
	// Assume standard SCSI parameter
	$info['heads'] = 255;
	$info['cylinders'] = intval($info['mediasize_sectors'] / ($info['sectors'] * $info['heads']));
}

// Create fdisk configuration file.
// $type - File system identified (131 for EXT2, 11 for MSDOS)
// $diskinfo - Array
// (
// 	[name] => ad0
// 	[sectorsize]  => 512
// 	[mediasize_bytes]  => 5242503168
// 	[mediasize_mbytes]  => 4999
// 	[mediasize_sectors]  => 10239264
// 	[cylinders] => 10158
// 	[heads] => 16
// 	[sectors] => 63
// )
// $file - Filename of fdisk configuration file.
// Return 0 if successful, 1 if error.
function disks_create_fdisk_configfile($type, $diskinfo, $file) {
	global $g;

	if (!is_array($diskinfo))
		return 1;

	$fdiskconf .= <<<EOD
g c{$diskinfo['cylinders']} h{$diskinfo['heads']} s{$diskinfo['sectors']}
p 1 {$type} {$diskinfo['sectors']} {$diskinfo['mediasize_sectors']}
p 2 0 0 0
p 3 0 0 0
p 4 0 0 0
a 1

EOD;

	if (false === file_put_contents($file, $fdiskconf)) {
		write_log("Error: Failed to create '{$file}'.");
		return 1;
	}

	return 0;
}

/**
 * Rescan disks after a Hotswap has been performed
 */ 
function disks_rescan() {
	/*
	mwexec2("atacontrol list", $devicelist, $return_val);
	if (true == $return_val) {
		echo gettext("atacontrol list failed, atacontrol not found !!");
		return $return_val;
	}
	$atachannels = array();
	$channel = 'NONE';
	foreach ($devicelist as $item) {
		if (preg_match("/ATA\s+channel\s+(\d+):/i", $item, $matched)) {
			$channel = 'ata' . $matched[1];
			$atachannels[$channel] = 0;
		} else if (preg_match("/no device present/i", $item, $matched)) {
			$atachannels[$channel]++;
		}
	}
	foreach ($atachannels as $channel => $count) {
		if ($count == 2) {
			echo gettext("Resetting channel ") . "$channel\n";
			mwexec("atacontrol detach $channel");
			sleep(1);
			mwexec("atacontrol attach $channel");
		}
	}
	*/
	mwexec2("camcontrol rescan all", $buslist, $return_val);
}

// Convert old ad to ada
function disks_convert_vdisk_to_ada($key, &$admap, &$raidmap) {
	global $config;

	if (!isset($config[$key]) || !isset($config[$key]['vdisk'])
	    || !is_array($config[$key]['vdisk'])) {
		echo "$key: skip (not configured).\n";
		return;
	}

	$tmp = array();
	foreach ($config[$key]['vdisk'] as $a) {
		$ret = true;
		foreach ($a['device'] as &$device) {
			if (preg_match("/^\/dev\/(ad[0-9]+.*)$/", $device, $m)) {
				$dev = $m[1];
				if (!empty($admap[$dev])) {
					$device = "/dev/".$admap[$dev];
					echo "$key: $dev -> ".$admap[$dev]."\n";
				} else {
					echo "$key: $dev is not found (removed).\n";
					$ret = false;
				}
			}
		}
		unset($device);
		if ($ret !== false) {
			$tmp[] = $a;
			echo "Add ".$a['devicespecialfile']." to RAID map.\n";
			$raidmap[$a['devicespecialfile']] = true;
		}
	}
	$config[$key]['vdisk'] = $tmp;
}

function disks_convert_to_ada() {
	global $config;
	$admap = disks_get_admap();
	$raidmap = array();

	echo "\n";
	echo "Converting ad devices...\n";

	// Disks|Management
	if (isset($config['disks']) && isset($config['disks']['disk'])
	    && is_array($config['disks']['disk'])) {
		$tmp = array();
		foreach ($config['disks']['disk'] as $a) {
			if (preg_match("/^(ad[0-9]+.*)$/", $a['name'], $m)) {
				$dev = $m[1];
				if (!empty($admap[$dev])) {
					//$serial = system_get_volume_serial($admap[$dev]);
					$a['name'] = $admap[$dev];
					$a['devicespecialfile'] = "/dev/".$admap[$dev];
					$a['type'] = "SCSI";
					$tmp[] = $a;
					echo "disks: $dev -> ".$admap[$dev]."\n";
				} else {
					echo "disks: $dev is not found (removed).\n";
				}
			} else {
				$tmp[] = $a;
			}
		}
		$config['disks']['disk'] = $tmp;
	}

	// Disks|Management|S.M.A.R.T.
	if (isset($config['smartd']) && isset($config['smartd']['selftest'])
	    && is_array($config['smartd']['selftest'])) {
		$tmp = array();
		foreach ($config['smartd']['selftest'] as $a) {
			if (preg_match("/^\/dev\/(ad[0-9]+.*)$/", $a['devicespecialfile'], $m)) {
				$dev = $m[1];
				if (!empty($admap[$dev])) {
					$a['devicespecialfile'] = "/dev/".$admap[$dev];
					$tmp[] = $a;
					echo "smartd: $dev -> ".$admap[$dev]."\n";
				} else {
					echo "smartd: $dev is not found (removed).\n";
				}
			} else {
				$tmp[] = $a;
			}
		}
		$config['smartd']['selftest'] = $tmp;
	}

	// System|Advanced|Swap
	if (isset($config['system']) && isset($config['system']['swap'])) {
		$tmp = array();
		$a = $config['system']['swap'];
		if ($a['type'] == "device") {
			if (preg_match("/^\/dev\/(ad[0-9]+.*)$/", $a['devicespecialfile'], $m)) {
				$dev = $m[1];
				if (!empty($admap[$dev])) {
					$a['devicespecialfile'] = "/dev/".$admap[$dev];
					$tmp = $a;
					echo "swap: $dev -> ".$admap[$dev]."\n";
				} else {
					echo "swap: $dev is not found (removed).\n";
				}
			} else {
				$tmp = $a;
			}
		}
		$config['system']['swap'] = $tmp;
	}

	// Disks|Software RAID
	disks_convert_vdisk_to_ada("gmirror", $admap, $raidmap);
	disks_convert_vdisk_to_ada("graid5", $admap, $raidmap);
	disks_convert_vdisk_to_ada("gconcat", $admap, $raidmap);
	disks_convert_vdisk_to_ada("gstripe", $admap, $raidmap);
	disks_convert_vdisk_to_ada("gvinum", $admap, $raidmap);

	// Disks|Encryption
	if (isset($config['geli']) && isset($config['geli']['vdisk'])
	    && is_array($config['geli']['vdisk'])) {
		$tmp = array();
		foreach ($config['geli']['vdisk'] as $a) {
			if (preg_match("/^(ad[0-9]+.*)$/", $a['name'], $m)) {
				$dev = $m[1];
				if (!empty($admap[$dev])) {
					$a['device'] = "/dev/".$admap[$dev];
					$a['devicespecialfile']	= "/dev/".$admap[$dev].".eli";
					$tmp[] = $a;
					echo "geli: $dev -> ".$admap[$dev]."\n";
				} else {
					echo "geli: $dev is not found (removed).\n";
				}
			} else {
				$tmp[] = $a;
			}
		}
		$config['geli']['vdisk'] = $tmp;
	}

	// Disks|Mount Point|Management
	if (isset($config['mounts']) && isset($config['mounts']['mount'])
	    && is_array($config['mounts']['mount'])) {
		$tmp = array();
		foreach ($config['mounts']['mount'] as $a) {
			if (preg_match("/^\/dev\/(ad[0-9]+.*)$/", $a['mdisk'], $m)) {
				$dev = $m[1];
				if (!empty($admap[$dev])) {
					$a['mdisk'] = "/dev/".$admap[$dev];
					$p = $a['partition'];
					if ($a['fstype'] == "ufs") {
						$ufsid = disks_get_ufsid($a['mdisk'].$p);
						$a['devicespecialfile'] = "/dev/ufsid/".$ufsid;
					} else {
						$a['devicespecialfile'] = $a['mdisk'].$p;
					}
					$tmp[] = $a;
					echo "mounts: $dev -> ".$admap[$dev]."\n";
				} else {
					echo "mounts: $dev is not found (removed).\n";
				}
			} else {
				// non ad devices
				if (!empty($raidmap[$a['mdisk']])) {
					$tmp[] = $a;
					echo "mounts: ".$a['mdisk']." in RAID map.\n";
				} else if (file_exists($a['mdisk'])) {
					$tmp[] = $a;
					echo "mounts: ".$a['mdisk']." exists.\n";
				} else {
					echo "mounts: ".$a['mdisk']." is not found (removed).\n";
				}
			}
		}
		$config['mounts']['mount'] = $tmp;
	}

	// Disks|ZFS
	$config['zfs'] = array(
		'vdevices' => array(),
		'pools' => array(),
		'datasets' => array(),
		'volumes' => array(),
		'autosnapshots' => array(),
	);
	echo "ZFS: cleared.\n";
	echo "You must import ZFS pool and synchronize the config by WebGUI.\n";

	// Services|iSCSI Target|Extent
	if (isset($config['iscsitarget']) && isset($config['iscsitarget']['extent'])
	    && is_array($config['iscsitarget']['extent'])) {
		$tmp = array();
		foreach ($config['iscsitarget']['extent'] as $a) {
			if (preg_match("/^\/dev\/(ad[0-9]+.*)$/", $a['path'], $m)) {
				$dev = $m[1];
				if (!empty($admap[$dev])) {
					$a['path'] = "/dev/".$admap[$dev];
					$tmp[] = $a;
					echo "iscsi: $dev -> ".$admap[$dev]."\n";
				} else {
					echo "iscsi: $dev is not found (removed).\n";
				}
			} else {
				$tmp[] = $a;
			}
		}
		$config['iscsitarget']['extent'] = $tmp;
	}

	// wait a moment to see the result.
	sleep(10);
}
?>
