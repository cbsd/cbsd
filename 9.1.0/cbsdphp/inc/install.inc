<?php
/*
	install.inc
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
require_once("xmlparse.inc");

// Mount the CDROM.
// Return 0 if successful, otherwise 1
function install_mount_cd($cdrom)
{
	write_console("Mount CDROM.\n");

	$mntpath = "/mnt/cdrom";

	// Creating temporary directory to mount CDROM.
	if (1 == mwexec("/bin/mkdir -p {$mntpath}")) {
		write_console("Error: Failed to create directory '{$mntpath}'!\n");
		return 1;
	}

	// Mounting the CDROM.
	if (1 == mwexec("/sbin/mount_cd9660 /dev/{$cdrom} {$mntpath}")) {
		write_console("Error: Failed to mount device '{$cdrom}'!\n");
		rmdir($mntpath);
		return 1;
	}

	return 0;
}

// Unmount CDROM.
// Return 0 if successful, otherwise 1
function install_unmount_cd()
{
	write_console("Unmount CDROM.\n");

	$result = 0;
	$mntpath = "/mnt/cdrom";

	// Unmount CDROM
	if (1 == mwexec("/sbin/umount {$mntpath}")) {
		$result = 1;
	}

	// Remove temporary mount directory
	rmdir($mntpath);

	return $result;
}

// Install the IMG on the destination harddrive.
// Return 0 if successful, otherwise 1
function install_dd_image($harddrive)
{
	global $g;

	$imgfilename=get_product_name(). "-{$g['arch']}-embedded.gz";

	// Test firmware file integrity.
	if (mwexec("/usr/bin/gzip -t /mnt/cdrom/{$imgfilename}")) {
		write_console("Error: Firmware file is corrupted.\n");
		return 1;
	}

	// dd image file on the destination disk.
	write_console("Installing system image on device {$harddrive}.\n");
	if (mwexec("/usr/bin/gzip -cd /mnt/cdrom/{$imgfilename} | /bin/dd of=/dev/{$harddrive} ibs=16k obs=16k > /dev/null 2>&1")) {
		write_console("Error: Failed to dd image on '/dev/{$harddrive}'!\n");
		return 1;
	}

	return 0;
}

// Install NAS4Free on the destination harddrive.
// Return 0 if successful, otherwise 1
function install_harddrive_image($harddrive)
{
	global $g;

	$mntpath = "/mnt/inst_{$harddrive}";

	// Mount disk used to install OS.
	mwexec("/bin/mkdir -p {$mntpath}");
	if (1 == mwexec("/sbin/mount /dev/{$harddrive}s1a {$mntpath}")) {
		write_console("Error: Failed to mount '/dev/{$harddrive}s1a'!\n");
		return 1;
	}

	// Install system files.
	write_console("Installing system files on device {$harddrive}s1a.\n");
	mwexec("/usr/bin/tar --exclude mnt/ --exclude dev/ --exclude var/ --exclude tmp/ --exclude {$g['cf_path']}/ -c -f - -C / . | tar -xvpf - -C {$mntpath}");
	mwexec("/bin/mkdir -p {$mntpath}/var");
	mwexec("/bin/mkdir -p {$mntpath}/dev");
	mwexec("/bin/mkdir -p {$mntpath}/mnt");
	mwexec("/bin/mkdir -p {$mntpath}/tmp");
	mwexec("/bin/chmod 1777 {$mntpath}/tmp");
	mwexec("/bin/mkdir -p {$mntpath}/boot/defaults");
	mwexec("/bin/cp -v /mnt/cdrom/boot/* {$mntpath}/boot");
	mwexec("/bin/cp -v /mnt/cdrom/boot/defaults/* {$mntpath}/boot/defaults");
	mwexec("/bin/cp -v /mnt/cdrom/boot/kernel/* {$mntpath}/boot/kernel");

	// Install configuration file
	mwexec("/bin/mkdir -p {$mntpath}{$g['cf_conf_path']}");
	mwexec("/bin/cp -v {$g['conf_default_path']}/config.xml {$mntpath}{$g['conf_path']}");

	// Generate new loader.conf file.
	$loaderconf .= <<<EOD
kernel="kernel"
bootfile="kernel"
kernel_options=""
kern.hz="100"
hw.est.msr_info="0"
hw.hptrr.attach_generic="0"
kern.maxfiles="65536"
kern.maxfilesperproc="50000"
kern.cam.boot_delay="8000"
vfs.zfs.prefetch_disable="1"
autoboot_delay="5"
isboot_load="YES"

EOD;

	if (false === file_put_contents("{$mntpath}/boot/loader.conf", $loaderconf)) {
		write_console("Error: Failed to create '/boot/loader.conf'.\n");
		return 1;
	}

	// Set the release type.
	$platform .= <<<EOD
{$g['arch']}-full

EOD;

	if (false === file_put_contents("{$mntpath}/etc/platform", $platform)) {
		write_console("Error: Failed to create '/etc/platform'.\n");
		return 1;
	}

	// Decompress kernel.
	mwexec("/usr/bin/gzip -d -f {$mntpath}/boot/kernel/kernel.gz");

	// Generate /etc/fstab.
	$ufsid = disks_get_ufsid("/dev/{$harddrive}s1a");
	if (empty($ufsid)) {
		write_console("Error: Failed to get UFS ID.\n");
		return 1;
	}
	$fstab .= <<<EOD
# Device                    Mountpoint  FStype  Options         Dump    Pass#
/dev/ufsid/{$ufsid}     /       ufs     rw              1       1

EOD;

	if (false === file_put_contents("{$mntpath}/etc/fstab", $fstab)) {
		write_console("Error: Failed to create '/etc/fstab'.\n");
		return 1;
	}

	// Generating the /etc/cfdevice (this file is linked in /var/etc at bootup)
	// This file is used by the firmware and mount check and is normally
	// generated with 'liveCD' and 'embedded' during startup, but need to be
	// created during install of 'full'.
	$cfdevice .= <<<EOD
{$harddrive}s1a

EOD;

	if (false === file_put_contents("{$mntpath}/etc/cfdevice", $cfdevice)) {
		write_console("Error: Failed to create '/etc/cfdevice'.\n");
		return 1;
	}

	// Unmount disk containing OS
	$retry = 10;
	while ($retry > 0) {
		if (0 == mwexec("/sbin/umount {$mntpath}")) {
			break;
		}
		$retry--;
		sleep(1);
	}
	if ($retry == 0) {
		write_console("Error: Failed to umount {$mntpath}!\n");
		return 1;
	}
	mwexec("/bin/rm -f -r {$mntpath}");

	return 0;
}

// Upgrade a NAS4Free 'full' release on the destination harddrive.
// Return 0 if successful, otherwise 1
function install_upgrade_full($harddrive)
{
	global $g;

	$mntpath = "/mnt/inst_{$harddrive}";
	$sysbackup = "/tmp/inst_{$harddrive}_bak";

	// Mount disk used to install OS
	mwexec("/bin/mkdir -p {$mntpath}");
	if (1 == mwexec("/sbin/mount /dev/{$harddrive}s1a {$mntpath}")) {
		write_console("Error: Failed to mount '/dev/{$harddrive}s1a'!\n");
		return 1;
	}

	// Backup system files
	write_console("Backup system configuration.\n");
	mwexec("/bin/mkdir -p {$sysbackup}");
	mwexec("/bin/cp -p {$mntpath}/boot/loader.conf {$sysbackup}");
	if (file_exists("{$mntpath}/boot.config")) {
		mwexec("/bin/cp -p {$mntpath}/boot.config {$sysbackup}");
	}
	mwexec("/bin/cp -p {$mntpath}/etc/platform {$sysbackup}");
	mwexec("/bin/cp -p {$mntpath}/etc/fstab {$sysbackup}");
	mwexec("/bin/cp -p {$mntpath}/etc/cfdevice {$sysbackup}");

	// Start upgrade script to remove obsolete files. This should be done
	// before system is updated because it may happen that some files
	// may be reintroduced in the system.
	write_console("Remove obsolete files.\n");
	mwexec("/etc/install/upgrade.sh clean {$mntpath}");

	// Install system files
	write_console("Installing system files on device {$harddrive}s1a.\n");
	mwexec("/usr/bin/tar --exclude mnt/ --exclude dev/ --exclude var/ --exclude tmp/ --exclude {$g['cf_path']}/ -c -f - -C / . | tar -xvpf - -C {$mntpath}");
	mwexec("/bin/mkdir -p {$mntpath}/var");
	mwexec("/bin/mkdir -p {$mntpath}/dev");
	mwexec("/bin/mkdir -p {$mntpath}/mnt");
	mwexec("/bin/mkdir -p {$mntpath}/tmp");
	mwexec("/bin/chmod 1777 {$mntpath}/tmp");
	mwexec("/bin/mkdir -p {$mntpath}/boot/defaults");
	mwexec("/bin/cp -v /mnt/cdrom/boot/* {$mntpath}/boot");
	mwexec("/bin/cp -v /mnt/cdrom/boot/defaults/* {$mntpath}/boot/defaults");
	mwexec("/bin/cp -v /mnt/cdrom/boot/kernel/* {$mntpath}/boot/kernel");

	// Decompress kernel
	mwexec("/usr/bin/gzip -d -f {$mntpath}/boot/kernel/kernel.gz");

	// Restore system files
	write_console("Restore system configuration.\n");
	mwexec("/bin/cp -p {$sysbackup}/loader.conf {$mntpath}/boot");
	if (file_exists("{$sysbackup}/boot.config")) {
		mwexec("/bin/cp -p {$sysbackup}/boot.config {$mntpath}/");
	} else {
		mwexec("/bin/rm -f {$mntpath}/boot.config");
	}
	mwexec("/bin/cp -p {$sysbackup}/platform {$mntpath}/etc");
	mwexec("/bin/cp -p {$sysbackup}/fstab {$mntpath}/etc");
	mwexec("/bin/cp -p {$sysbackup}/cfdevice {$mntpath}/etc");

	// Cleanup system backup files
	mwexec("/bin/rm -f -r {$sysbackup}");

	// Unmount disk containing OS
	$retry = 10;
	while ($retry > 0) {
		if (0 == mwexec("/sbin/umount {$mntpath}")) {
			break;
		}
		$retry--;
		sleep(1);
	}
	if ($retry == 0) {
		write_console("Error: Failed to umount {$mntpath}!\n");
		return 1;
	}
	mwexec("/bin/rm -f -r {$mntpath}");

	return 0;
}

// Check version of the hard drive/CF
// Return 0 if successful, otherwise 1
function install_check_version($disk)
{
	global $g;

	$mnt_cf_disk = "/mnt/chkver_{$disk}";

	write_console("Check version.\n");

	/* Creating temporary directory for the disk containing the version file. */
	if (1 == mwexec("/bin/mkdir -p {$mnt_cf_disk}")) {
		write_console("Error: Failed to create '{$mnt_cf_disk}'!\n");
		return 1;
	}

	/* Mount the drive to be upgraded (readonly mode). */
	if (1 == mwexec("/sbin/mount -r /dev/{$disk} {$mnt_cf_disk}")) {
		write_console("Error: Failed to mount device '{$disk}'!\n");
		return 1;
	}

	/* Load configuration file into array. */
	$tmp_config = parse_xml_config("{$mnt_cf_disk}/conf/config.xml", $g['xml_rootobj']);
	if (false === $tmp_config) {
		write_console("Error: Failed to load config.xml.\n");
		return 1;
	}
	/* Check config version */
	//if ((float)$tmp_config['version'] < (float)1.0) { // r1
	if ((float)$tmp_config['version'] < (float)1.0) { // r1
		write_console("Error: Does not support upgrading form this version.\n");
		mwexec("/sbin/umount {$mnt_cf_disk}");
		mwexec("/bin/rm -f -r {$mnt_cf_disk}");
		return 1;
	}

	/* Unmount the drive to be upgraded */
	if (1 == mwexec("/sbin/umount {$mnt_cf_disk}")) {
		write_console("Error: Failed to unmount '{$mnt_cf_disk}'!\n");
		return 1;
	}

	/* Cleanup */
	if (1 == mwexec("/bin/rm -f -r {$mnt_cf_disk}")) {
		write_console("Error: Failed to remove temporary data!\n");
		return 1;
	}

	return 0;
}

// Backup the configuration file on the hard drive/CF and unmount this drive.
// Return 0 if successful, otherwise 1
function install_backup_config($disk)
{
	global $g;

	$tmp_cf_bak = "/tmp/cf_bak";
	$mnt_cf_disk = "/mnt/inst_{$disk}";

	write_console("Backup configuration.\n");

	/* Creating temporary directory for the configuration file. */
	if (1 == mwexec("/bin/mkdir -p {$tmp_cf_bak}")) {
		write_console("Error: Failed to create '{$tmp_cf_bak}'!\n");
		return 1;
	}

	/* Creating temporary directory for the disk containing the configuration file. */
	if (1 == mwexec("/bin/mkdir -p {$mnt_cf_disk}")) {
		write_console("Error: Failed to create '{$mnt_cf_disk}'!\n");
		return 1;
	}

	/* Mount the drive to be upgraded (readonly mode). */
	if (1 == mwexec("/sbin/mount -r /dev/{$disk} {$mnt_cf_disk}")) {
		write_console("Error: Failed to mount device '{$disk}'!\n");
		return 1;
	}

	/* Backup the configuration file */
	if (1 == mwexec("/bin/cp -p {$mnt_cf_disk}/conf/config.xml {$tmp_cf_bak}")) {
		write_console("Error: Failed to backup the configuration file!\n");
		return 1;
	}
	if (file_exists("{$mnt_cf_disk}/boot.config")
	    && 1 == mwexec("/bin/cp -p {$mnt_cf_disk}/boot.config {$tmp_cf_bak}")) {
		write_console("Error: Failed to backup the configuration file!\n");
		return 1;
	}

	/* Unmount the drive to be upgraded */
	if (1 == mwexec("/sbin/umount {$mnt_cf_disk}")) {
		write_console("Error: Failed to unmount '{$mnt_cf_disk}'!\n");
		return 1;
	}

	return 0;
}

function install_restore_config($disk)
{
	global $g;

	$tmp_cf_bak = "/tmp/cf_bak";
	$mnt_cf_disk = "/mnt/inst_{$disk}";

	write_console("Restore configuration.\n");

	/* Mount the drive to be upgraded */
	if (1 == mwexec("/sbin/mount /dev/{$disk} {$mnt_cf_disk}")) {
		write_console("Error: Failed to mount device '{$disk}'!\n");
		return 1;
	}

	/* Restore the configuration file */
	if (1 == mwexec("/bin/cp -p {$tmp_cf_bak}/config.xml {$mnt_cf_disk}/conf")) {
		write_console("Error: Failed to restore the configuration file!\n");
		return 1;
	}
	if (file_exists("{$tmp_cf_bak}/boot.config")) {
		if (1 == mwexec("/bin/cp -p {$tmp_cf_bak}/boot.config {$mnt_cf_disk}/")) {
			write_console("Error: Failed to restore the configuration file!\n");
			return 1;
		}
	} else {
		if (1 == mwexec("/bin/rm -f {$mnt_cf_disk}/boot.config")) {
			write_console("Error: Failed to remove the configuration file!\n");
			return 1;
		}
	}

	/* Unmount the upgraded drive */
	if (1 == mwexec("/sbin/umount {$mnt_cf_disk}")) {
		write_console("Error: Failed to unmount '{$mnt_cf_disk}'!\n");
		return 1;
	}

	/* Cleanup */
	if (1 == mwexec("/bin/rm -f -r {$mnt_cf_disk} {$tmp_cf_bak}")) {
		write_console("Error: Failed to remove temporary backup data!\n");
		return 1;
	}

	return 0;
}

// Create two partitions on the destination hard drive.
// $disk - Disk name (e.g. ad0)
// $partlayout - The layout description of the disk, e.g.
// array(
//   array("slice" => 1, "fstype" => "4.2BSD", "size" => 64, "volname" => "root"),
//   array("slice" => 2, "fstype" => "4.2BSD", "size" => "*", "volname" => "data"),
//   array("slice" => 3, "fstype" => "swap", "size" => 200, "volname" => "")
//   array("slice" => 4, "fstype" => "unused", "size" => 0, "volname" => "")
// )
// Return 0 if successful, otherwise 1
function install_init_disk($disk, $partlayout)
{
	// Initialize the destination using fdisk.
	write_console("Create slice/partition table on device {$disk}.\n");
	$slicesize = array(0, 0, 0, 0);
	foreach ($partlayout as $partlayoutv) {
		$slicesize[$partlayoutv['slice'] - 1] = $partlayoutv['size'];
	}
	if (install_fdisk($disk, $slicesize)) {
		write_console("Error: Failed to create slice/partition table!\n");
		return 1;
	}

	// Get partition information
	$partinfo = disks_get_partition_info($disk);

	// AFT support
	//$alignsector = 8; // 4KB/AFT
	$alignsector = 64; // 32KB/blocksize
	$newfs_sector = 4096;
	$newfs_fsize = 4096;
	$newfs_bsize = $newfs_fsize * 8;

	// Create disk labels.
	write_console("Create BSD label on slice/partition:\n");
	foreach ($partlayout as $partlayoutv) {
		$diskdevicename = "{$disk}s{$partlayoutv['slice']}";
		$startsector = $partinfo[$partlayoutv['slice']]['start'];
		$slicesize = $partinfo[$partlayoutv['slice']]['size'];

		write_console("  {$diskdevicename}\n");

		// Do not create partitions for swap.
		if ("swap" === $partlayoutv['fstype']) {
			// Completely wipe any prior information
			if (mwexec("/bin/dd if=/dev/zero of=/dev/{$diskdevicename} bs=512 count=34")) {
				write_console("\nError: Failed to wipe any prior information on {$diskdevicename}.\n");
				return 1;
			}
			continue;
		}

		if (install_bsdlabel($diskdevicename, $partlayoutv['fstype'], $startsector, $slicesize, $alignsector)) {
			write_console("\nError: Failed to create BSD label on {$diskdevicename}!\n");
			return 1;
		}
	}

	// Create file systems and set volume label.
	write_console("Create file system on slice/partition:\n");
	foreach ($partlayout as $partlayoutv) {
		// Do not create a file system for swap/unused partitions.
		if ("swap" === $partlayoutv['fstype'] || "unused" === $partlayoutv['fstype'])
			continue;

		$diskdevicename = "{$disk}s{$partlayoutv['slice']}a";

		write_console("  {$diskdevicename}\n");
		if (install_newfs($diskdevicename, $partlayoutv['volname'],
		   		  $newfs_sector, $newfs_fsize, $newfs_bsize)) {
			write_console("\nError: Failed to create file system on {$diskdevicename}!\n");
			return 1;
		}
	}

	return 0;
}

// Initialize harddrive. It creates the root (p1), data (p2) and swap (p3) partitions.
// $disk - Disk name (e.g. ad0)
// $slicesize - Array containing the size of each slice in MB, e.g. array(64, 100, *, 0)
// Return 0 if successful, otherwise 1
function install_fdisk($disk, $slicesize)
{
	global $g;

	$slicetable = array(
		array("slice" => 1, "type" => 0, "start" => 0, "length" => 0),
		array("slice" => 2, "type" => 0, "start" => 0, "length" => 0),
		array("slice" => 3, "type" => 0, "start" => 0, "length" => 0),
		array("slice" => 4, "type" => 0, "start" => 0, "length" => 0)
	);
	$activeslice = 0;

	// Get disk informations
	$diskinfo = disks_get_diskinfo($disk);

	// Get valid CHS information. 'diskinfo' gets values from harddisk firmware that
	// are maybe unusable by 'fdisk' (e.g. sectors = 255, fdisk requires <= 63).
	disk_sanitize_bios_geom($diskinfo);

	// Calculate the size of the slices
	$usedsectors = $diskinfo['sectors']; // MBR start here
	$align = $diskinfo['heads'] * $diskinfo['sectors'];
	foreach ($slicesize as $slicesizek => $slicesizev) {
		if (is_numeric($slicesizev)) {
			$bytes = $slicesizev * 1024 * 1024;
			$slicetable[$slicesizek]['length'] = $bytes / $diskinfo['sectorsize'];
			$slicetable[$slicesizek]['length'] = floor($slicetable[$slicesizek]['length'] / $align) * $align;
			if ($slicesizek == 0) {
				$slicetable[$slicesizek]['length'] -= $diskinfo['sectors'];
			}

			// Sum total used sectors
			$usedsectors += $slicetable[$slicesizek]['length'];
		}
	}
	foreach ($slicesize as $slicesizek => $slicesizev) {
		if ("*" === $slicesizev) { // Use the rest of the disk space for this slice.
			$slicetable[$slicesizek]['length'] = $diskinfo['mediasize_sectors'] - $usedsectors;
			$slicetable[$slicesizek]['length'] = floor($slicetable[$slicesizek]['length'] / $align) * $align;
			if ($slicesizek == 0) {
				$slicetable[$slicesizek]['length'] -= $diskinfo['sectors'];
			}
			// Stop processing here (only one '*' is accepted and processed)
			break;
		}
	}

	// Check if partition layout fit disk size
	if ($diskinfo['mediasize_sectors'] <= $usedsectors) {
		write_console("Error: Partition layout does not fit on disk {$diskinfo['name']}.\n");
		write_console("  Requested sectors: {$usedsectors}\n");
		write_console("  Available sectors: {$diskinfo['mediasize_sectors']}\n");
		write_console("  Layout: ");
		foreach ($slicesize as $slicesizev)
			write_console(" {$slicesizev}");
		write_console("\n");
		return 1;
	}

	// Calculate slice start sectors and type
	$startsector = $diskinfo['sectors'];
	foreach ($slicesize as $slicesizek => $slicesizev) {
		if ( 0 >= $slicetable[$slicesizek]['length'])
			continue;

		// Set the first found slice with length > 0 as active
		if (0 == $activeslice)
			$activeslice = $slicesizek + 1;

		$slicetable[$slicesizek]['type'] = 165;
		$slicetable[$slicesizek]['start'] = $startsector;

		// Calculate start of next slice
		$startsector += $slicetable[$slicesizek]['length'];
	}

	// Create fdisk config file (fdisk.conf)
	$fdiskconf = "g c{$diskinfo['cylinders']} h{$diskinfo['heads']} s{$diskinfo['sectors']}\n";
	for ($i = 0; $i < 4; ++$i) {
		$fdiskconf .= "p {$slicetable[$i]['slice']} {$slicetable[$i]['type']} {$slicetable[$i]['start']} {$slicetable[$i]['length']}\n";
	}
	$fdiskconf .= "a {$activeslice}\n";

	if (false === file_put_contents("{$g['tmp_path']}/fdisk.conf", $fdiskconf)) {
		write_console("Error: Failed to create '{$g['tmp_path']}/fdisk.conf'.\n");
		return 1;
	}

	// Delete MBR and partition table.
	$count = (1024*1024) / $diskinfo['sectorsize'];
	if (mwexec("/bin/dd if=/dev/zero of=/dev/{$disk} bs={$diskinfo['sectorsize']} count={$count}")) {
		write_console("Error: Failed to delete MBR and partition table on disk {$disk}.\n");
		return 1;
	}

	// Delete GEOM metadata.
	$oseek = $diskinfo['mediasize_sectors'] - 1;
	if (mwexec("/bin/dd if=/dev/zero of=/dev/{$disk} bs={$diskinfo['sectorsize']} oseek={$oseek} count=1")) {
		write_console("Error: Failed to delete GEOM metadata on disk {$disk}.\n");
		return 1;
	}

	// Set slice values
	if (mwexec("/sbin/fdisk -f {$g['tmp_path']}/fdisk.conf {$disk}")) {
		write_console("Error: Failed to write slice table on disk {$disk}.\n");
		return 1;
	}

	// Reinitialize the boot code contained in sector 0 of the disk
	if (mwexec("(/bin/echo y; /bin/echo y) | /sbin/fdisk -B -b /boot/mbr {$disk}")) {
		write_console("Error: Failed to reinitialize the boot code on disk {$disk}!\n");
		return 1;
	}

	return 0;
}

// Create disk label.
// $disk - Disk name (e.g. ad0s1)
// $fstype - The file system type (e.g. 4.2BSD or swap)
// $startsector - Start sector of the slice
// $slicesize - Size of the slice
// $alignsector - alignment for the sector of the partition (e.g. 8=4KB)
// Return 0 if successful, otherwise 1
function install_bsdlabel($disk, $fstype, $startsector, $slicesize, $alignsector)
{
	global $g;

	// Check sectors
	if ($startsector == '' || $slicesize == '' || $alignsector ==''
	    || $alignsector < 1 || $slicesize < $alignsector) {
		write_console("\nError: Invalid parameter on {$disk}.\n");
		return 1;
	}
	$poffset = 16;
	$aligned_start = ceil(($startsector + $poffset) / $alignsector) * $alignsector;
	$aligned_start -= $startsector;
	$aligned_counts = $slicesize - $aligned_start;
	$aligned_counts = floor($aligned_counts / $alignsector) * $alignsector;

	// Completely wipe any prior information on the partition
	if (mwexec("/bin/dd if=/dev/zero of=/dev/{$disk} bs=512 count=2048")) {
		write_console("\nError: Failed to wipe any prior information on {$disk}.\n");
		return 1;
	}

	// Do an automatic label first
	if (mwexec("/sbin/bsdlabel -w -B {$disk}")) {
		write_console("\nError: Failed to write standard label.\n");
		return 1;
	}

	// Read the label to manipulate it
	exec("/sbin/bsdlabel {$disk} > {$g['tmp_path']}/label.tmp", $output, $result);
	if ($result) {
		write_console("\nError: Failed to backup label.\n");
		return 1;
	}

	// Put the file conent into an array
	$label = file("{$g['tmp_path']}/label.tmp");

	// Open this file in add mode
	$fd = fopen("{$g['tmp_path']}/label.tmp", 'w');

	while(list(,$val) = each($label)) {
		if (ereg ("unused",$val) && !ereg ("c:",$val)) {
			// Replace 'unused' with '$fstype'
			$val = ereg_replace("unused", $fstype, $val);
			// Replace with aligned size
			$val = preg_replace("/([a-z]:)\s+\d+\s+\d+/",
				"\\1 {$aligned_counts} {$aligned_start}",
				$val);
			// Adding this line add the end of the file
			fwrite($fd, $val);
		} else {
			fwrite($fd, $val);
		}
	}

	// Closing file
	fclose($fd);

	// Injecting modified label
	if (mwexec("/sbin/bsdlabel -R -B {$disk} {$g['tmp_path']}/label.tmp")) {
		write_console("\nError: Failed to write modified label.\n");
		return 1;
	}

	// Cleanup
	unlink("{$g['tmp_path']}/label.tmp");

	return 0;
}

// Create new file system on disk.
// $disk - Disk name (e.g. ad0s1)
// $volname - The new volume label (e.g. 'data')
// $sector - The size of a sector in bytes.
// $fsize - The fragment size of the file system in bytes.
// $bsize - The block size of the file system, in bytes.
// Return 0 if successful, otherwise 1
function install_newfs($disk, $volname, $sector, $fsize, $bsize) {
	//$param = "-U -b 16384 -f 2048";
	$param = "-U -S $sector -b $bsize -f $fsize";
	if (!empty($volname))
		$param .= " -L \"{$volname}\"";

	if (mwexec("/sbin/newfs {$param} /dev/{$disk}")) {
		return 1;
	}

	return 0;
}

// Detect device where the OS is installed.
// $mode [in] - Search for 'embedded' or 'full' installation.
// $devinfo [out] - Array (
//		[device] => ad0
//		[partition] => s1
//	)
// Return 0 if successful, otherwise 1
function install_detect_installation($mode, &$devinfo) {
	global $g;

	$result = 1;
	$devinfo = array();

	echo "Try to detect installation...\n";

	// Create temporary mountpoint
	$mnt_cf_disk = "/mnt/inst_cf_tmp";
	mwexec("/bin/mkdir -p {$mnt_cf_disk}");

	// Scan all disks detected from kernel
	$adisk = explode(" ", trim(preg_replace("/kern.disks: /", "", exec("/sbin/sysctl kern.disks"))));
	foreach ($adisk as $diskv) {
		// Skip mfs-mounted filesystems
		if (0 < ereg("md[0-9]*", $diskv))
			continue;

		// Search on slice 1 for OS installed on disk with install script
		if (0 == mwexec("/sbin/mount -r /dev/{$diskv}s1a {$mnt_cf_disk}")) {
			// Check if configuration file exists
			if (file_exists("{$mnt_cf_disk}/conf/config.xml")) {
				$devinfo['device'] = $diskv;
				$devinfo['partition'] = "s1a";

				// Get partition information
				$partinfo = disks_get_partition_info($diskv);
				$devinfo['size'] = $partinfo[1]['size'];

				// Check install mode
				switch ($mode) {
					case 'embedded':
						if (file_exists("{$mnt_cf_disk}/mfsroot.gz")) {
							$result = 0;
						}
						break;

					case 'full':
						$platform = chop(file_get_contents("{$mnt_cf_disk}{$g['etc_path']}/platform"));
						if (FALSE !== ereg("^.*-full", $platform)) {
							$result = 0;
						}
						break;
				}
			}

			// Unmount device
			mwexec("/sbin/umount {$mnt_cf_disk}");

			if (0 == $result)
				break;
		}

		// Search on partition 'a' for OS IMG image written on disk
		if (mwexec("/sbin/mount -r /dev/{$diskv}a {$mnt_cf_disk}") == 0) {
			// Check if configuration file exists
			if (file_exists("{$mnt_cf_disk}/conf/config.xml")) {
				$devinfo['device'] = $diskv;
				$devinfo['partition'] = "a";

				// Get disk information
				$diskinfo = disks_get_diskinfo($diskv);
				$devinfo['size'] = $diskinfo['mediasize_sectors'];

				// Check install mode
				switch ($mode) {
					case 'embedded':
						if (file_exists("{$mnt_cf_disk}/mfsroot.gz")) {
							$result = 0;
						}
						break;

					case 'full':
						$platform = chop(file_get_contents("{$mnt_cf_disk}{$g['etc_path']}/platform"));
						if (FALSE !== ereg("^.*-full", $platform)) {
							$result = 0;
						}
						break;
				}
			}

			// Unmount device
			mwexec("/sbin/umount {$mnt_cf_disk}");

			if (0 == $result)
				break;
		}
	}

	// Cleanup
	mwexec("/bin/rm -f -r {$mnt_cf_disk}");

	return $result;
}
?>
