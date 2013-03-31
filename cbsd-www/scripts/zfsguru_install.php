#!/usr/local/bin/php
<?php

/*
*
** ZFSguru installation script
** installs Root-on-ZFS or Embedded distribution on target device
** version 5
** part of ZFSguru distribution
*
*/


// variables
$tag = '* ';
$data = array();
$scriptversion = 5;
$fileversion = 1;
$filelocation = '/tmp/guru_install.dat';
$temp_mp = '/mnt/';
$source = trim(`pwd`).'/files';

// procedures
install_init();
install_summary();
install_verify();
if ($data['dist'] == 'rootonzfs')
 install_rootonzfs();
elseif ($data['dist'] == 'embedded')
 install_embedded();
else
 install_error('Invalid distribution!');

// functions

function install_init()
// fetches configuration from file
{
 global $data, $fileversion, $filelocation;
 // sanity checks
 if (!file_exists($filelocation))
  install_error('Data file "'.$filelocation.'" not found!');
 if (!is_readable($filelocation))
  install_error('Data file "'.$filelocation.'" not readable!');
 // file statistics
 clearstatcache();
 $stat = @stat($filelocation);
 // check if owner is root
 if (@$stat['uid'] != 0)
  install_error('Data file "'.$filelocation.'" is not owned by root!');
 // check if permissions are '0644' (only writable by root)
 if (@substr(sprintf('%o', $stat['mode']), -4) != '0644')
  install_error('Data file "'.$filelocation.'" has invalid file permissions!');

 // read configuration file and unserialize
 $filecontents = file_get_contents($filelocation);
 $data = @unserialize($filecontents);
 if (!is_array($data))
  install_error('Data file "'.$filelocation.'" bad contents!');

 // remove file again
 unlink($filelocation);
}

function install_summary()
// prints a summary to standard output with all configuration settings
{
 global $data, $tag, $scriptversion, $source;

 echo($tag.'Starting installation'.chr(10));
 flush_buffer();
 echo('install script version: '.$scriptversion.chr(10));
 echo('distribution type: '.$data['dist'].chr(10));
 echo('target device: '.$data['target'].chr(10));
 echo('source data: '.$source.chr(10));
 if ($data['dist'] == 'rootonzfs')
  echo('boot filesystem: '.$data['bootfs'].chr(10));
 elseif ($data['dist'] == 'embedded')
 {
  echo('MBR bootcode: '.$data['path_mbr'].chr(10));
  echo('loader bootcode: '.$data['path_loader'].chr(10));
 }
 echo('system version: '.$data['sysver'].chr(10));
 echo('system location: '.$data['sysloc'].chr(10));
 echo('system size: '.$data['sysimg_size'].' bytes'.chr(10));
 echo('loader.conf: '.$data['loaderconf'].chr(10));
 echo('system MD5 checksum: '.$data['checksum_md5'].chr(10));
 echo('system SHA1 checksum: '.$data['checksum_sha1'].chr(10));
 echo('system size: '.$data['sysimg_size'].' bytes'.chr(10));
 echo('memory tuning: '.(int)$data['options']['memtuning'].chr(10));
 if ($data['dist'] == 'rootonzfs')
 {
  echo('configure portstree: '.(int)$data['options']['portstree']
   .chr(10));
  echo('preserve system image: '.(int)$data['options']['copysysimg']
   .chr(10));
  echo('compression: '.$data['options']['compression'].chr(10));
 }
 echo(chr(10));
 flush_buffer();
}

function install_verify()
// performs verification of system image size and checksum
{
 global $data, $tag;

 // check if system image is set, actually exists and is readable
 echo($tag.'Verifying system image'.chr(10));
 flush_buffer();
 if (@strlen($data['sysloc']) < 1)
  install_error('Data file contains invalid system image location');
 if (!@file_exists($data['sysloc']))
  install_error('System image "'.$data['sysloc'].'" not found');
 if (!is_readable($data['sysloc']))
  install_error('System image "'.$data['sysloc'].'" not readable');

 // check for proper size
 if (filesize($data['sysloc']) < 1)
  install_error('System image "'.$data['sysloc'].'" has invalid size');
 if (filesize($data['sysloc']) != $data['sysimg_size'])
  install_error('System image "'.$data['sysloc'].'" has incorrect size');

 // verify checksum of system image file
 echo($tag.'Verifying system image MD5 checksum'.chr(10));
 flush_buffer();
 $md5 = md5_file($data['sysloc']);
 if ($data['checksum_md5'] != $md5)
  install_error('System image "'.$data['sysloc']
   .'" failed MD5 checksum! Expected: '.$data['checksum_md5']);
 // skip SHA1 check if not supplied by data array
 if (strlen(@$data['checksum_sha1']) > 0)
 {
  echo($tag.'Verifying system image SHA1 checksum'.chr(10));
  flush_buffer();
  $sha1 = sha1_file($data['sysloc']);
  if ($data['checksum_sha1'] != $sha1)
   install_error('System image "'.$data['sysloc']
    .'" failed SHA1 checksum! Expected: '.$data['checksum_sha1']);
 }
}

function install_rootonzfs()
// performs actual Root-on-ZFS installation
{
 global $data, $tag, $temp_mp, $source;

 // variables
 $root = $data['target'].'/'.$data['bootfs'];
 $proot = '/'.$root;
 $umount_fs = array($root.'/usr', $root.'/var', $root);

 // destroy existing boot filesystem
 exec('/sbin/zfs destroy -r '.$root.' > /dev/null 2>&1');

 // patch portsnap to allow non-interactive use
 if ($data['options']['portstree'])
 {
  echo($tag.'Patching portsnap binary'.chr(10));
  flush_buffer();
  $file_portsnap_patched = '/tmp/portsnap.patched';
  $file_portsnap = file_get_contents('/usr/sbin/portsnap');
  $patt = '/if \[ \! -t 0 \]; then/m';
  $repl = 'if [ 0 -eq 1 ]; then';
  $file_portsnap = preg_replace($patt, $repl, $file_portsnap, 1, $count);
  if ($count != 1)
   echo('*** could not patch portsnap; skipping!'.chr(10));
  else
   file_put_contents($file_portsnap_patched, $file_portsnap);
 }

 // create filesystems
 echo($tag.'Creating ZFS filesystems'.chr(10));
 flush_buffer();
 $compression = $data['options']['compression'];
 // create standard zfs filesystem structure
 exec('/sbin/zfs create '.$root);
 exec('/sbin/zfs create '.$root.'/usr');
 exec('/sbin/zfs create '.$root.'/var');
 // compression
 if ($compression != 'off')
 {
  exec('/sbin/zfs set compression='.$compression.' '.$root.'/usr');
  exec('/sbin/zfs set compression='.$compression.' '.$root.'/var');
 }

 // check if filesystems were created
 if ((!@is_dir($proot.'/usr')) OR (!@is_dir($proot.'/var')))
  install_error('Could not create boot filesystems');

 // mount system image
 echo($tag.'Mounting system image'.chr(10));
 flush_buffer();
 exec('/sbin/mdmfs -P -F '.$data['sysloc'].' -o ro md.uzip '.$temp_mp, 
  $output, $rv);
 if (!@is_dir($temp_mp.'/boot'))
  install_error('Mounting system image to '.$temp_mp.' was unsuccessful!');

 // copy system image
 echo($tag.'Transferring system image to boot filesystem'.chr(10));
 flush_buffer();
 exec('/usr/bin/tar cPf - '.$temp_mp.' | tar x -C '.$proot
  .' --strip-components 2 -f -', $output, $rv);
 if ($rv != 0)
  install_error('Transferring data from system image to '.$proot
   .' filesystem has failed!', $rv);
 exec('/sbin/umount '.$temp_mp);

 // optional portstree creation
 if ($data['options']['portstree'] AND file_exists('/usr/ports/distfiles'))
  echo($tag.'Skipping portstree creation because /usr/ports/distfiles '
   .'directory already exists!'.chr(10));
 elseif ($data['options']['portstree'])
 {
  echo($tag.'Creating portstree structure'.chr(10));
  flush_buffer();
  // create portstree filesystems with explicit mountpoint
  exec('/sbin/zfs create -o mountpoint=/usr/ports '.$root.'/usr/ports');
  exec('/sbin/zfs create '.$root.'/usr/ports/distfiles');
  // compression
  exec('/sbin/zfs set compression=gzip '.$root.'/usr/ports');
  exec('/sbin/zfs set compression=off '.$root.'/usr/ports/distfiles');
  exec('/bin/mkdir /usr/ports/distfiles/portsnap');
  if (!is_dir('/usr/ports/distfiles/portsnap'))
   echo('==> Error creating portsnap directory - aborting portstree!'.chr(10));
  else
  {
   echo($tag.'Downloading portstree'.chr(10));
   flush_buffer();
   // include ports filesystems in unmount array
   $umount_fs[] = $root.'/usr/ports/distfiles';
   $umount_fs[] = $root.'/usr/ports';
   // check for patched portsnap binary
   if (!file_exists($file_portsnap_patched))
    echo('*** could not find patched portsnap; skipping!'.chr(10));
   else
   {
    // run patched portsnap
    exec('/bin/sh '.$file_portsnap_patched
     .' -d /usr/ports/distfiles/portsnap/ fetch', $op, $rv);
    if ($rv != 0)
    {
     echo('GOT RETURN VALUE '.$rv.' when trying to fetch portstree snapshot!'
      .chr(10));
     echo(implode(chr(10), $op));
     echo(chr(10));
    }
    echo($tag.'Extracting portstree'.chr(10));
    flush_buffer();
    exec('/usr/sbin/portsnap -d /usr/ports/distfiles/portsnap/ extract');
    // remove patched portsnap file
    unlink($file_portsnap_patched);
   }
  }
 }

 // activate boot filesystem, making it bootable
 echo($tag.'Activating boot filesystem'.chr(10));
 flush_buffer();
 exec('/sbin/zpool set bootfs='.$root.' '.$data['target'], $output, $rv);
 if ($rv != 0)
  install_error('Activating boot filesystem '.$root.' has failed!', $rv);

 // write system distribution file to boot filesystem
 $distfile = $proot.'/zfsguru.dist';
 echo($tag.'Writing distribution file '.$distfile.chr(10));
 flush_buffer();
 file_put_contents($distfile, $data['checksum_md5']);

 // transfer configuration files
 echo($tag.'Copying system configuration files to '.$proot.chr(10));
 flush_buffer();
 $rv = array();
 exec('/bin/cp -p '.$data['loaderconf'].' '.$proot.'/boot/loader.conf', 
  $output, $rv[]);
 exec('/bin/cp -p '.$source.'/roz_rc.conf '.$proot.'/etc/rc.conf', $output, 
  $rv[]);
 exec('/bin/cp -p '.$source.'/roz_motd '.$proot.'/etc/motd', $output, $rv[]);
 exec('/bin/rm /'.$root.'/etc/rc.d/zfsguru', $output, $rv[]);
 exec('/bin/cp -p /usr/local/etc/smb.conf '.$proot.'/usr/local/etc/', 
  $output, $rv[]);
 exec('/bin/cp -p /boot/zfs/zpool.cache '.$proot.'/boot/zfs/', $output, $rv[]);
 foreach ($rv as $returnvalue)
  if ($returnvalue !== 0)
   install_error('Got return value '.(int)$returnvalue
    .' while copying zpool.cache file', $returnvalue);

 // write fstab file
 echo($tag.'Crafting new fstab on boot filesystem:'.chr(10));
 flush_buffer();
 $fstab = $root.'	/	zfs	rw 0 0'.chr(10)
  .$root.'/usr	/usr	zfs	rw 0 0'.chr(10)
  .$root.'/var	/var	zfs	rw 0 0'.chr(10);
 echo($fstab);
 flush_buffer();
 file_put_contents($proot.'/etc/fstab', $fstab);
 if (!file_exists($proot.'/etc/fstab'))
  install_error('Could not create '.$proot.'/etc/fstab file');

 // copy web interface
 echo($tag.'Copying ZFSguru web interface'.chr(10));
 flush_buffer();
 exec('/bin/mkdir -p '.$proot.'/usr/local/www/zfsguru');
 exec('/bin/cp -Rp /usr/local/www/zfsguru/* '.$proot.'/usr/local/www/zfsguru/');

 // copy system image (optional)
 if (@$data['options']['copysysimg'])
 {
  echo($tag.'Copying optional system image to target /tmp directory'.chr(10));
  flush_buffer();
  exec('/bin/cp -p '.$data['sysloc'].'* '.$proot.'/tmp/');
 }

 // unmount filesystems
 echo($tag.'Unmounting boot filesystem'.chr(10));
 flush_buffer();
 foreach ($umount_fs as $fs)
 {
  exec('/sbin/zfs umount '.$fs, $output, $rv);
  if ($rv != 0)
   install_error('Could not unmount filesystem: '.$fs, $rv);
 }

 // sync
 exec('/bin/sync');
 exec('/bin/sync');
 exec('/bin/sync');
 sleep(0.5);

 // mark boot filesystem as legacy mountpoint
 echo($tag.'Setting legacy mountpoint on '.$root.chr(10));
 flush_buffer();
 exec('/sbin/zfs set mountpoint=legacy '.$root, $output, $rv);
 if ($rv != 0)
  install_error('Could not set legacy mountpoint on '.$root, $rv);

 // done
 echo(chr(10));
 echo('*** Done! *** Reboot system now and boot from any of the pool members');
 flush_buffer();
}

function install_embedded()
// performs actual Embedded installation
{
 global $data, $tag, $temp_mp, $source;
 // todo
 echo($tag.'NOT WORKING YET'.chr(10));
 die(1);

 $target = $data['target'];
 $tdev = '/dev/'.$target;
 $createdatapartition = false;
 $size_syspartition = 655360;
 $size_datapartition = 0;
 $gpt_system_label = 'GURU-EMBEDDED';
 $gpt_data_label = 'EMBEDDED-DATA';
 $path_syslabel = '/dev/gpt/'.$gpt_system_label;
 $path_datalabel = '/dev/gpt/'.$gpt_data_label;
 $webinterface_name = 'ZFSguru-webinterface.tgz';
 $webinterface_source = `realpath .`;

 // zero write target device
 echo($tag.'Zero-writing first 100MiB of target device '.$tdev.chr(10));
 flush_buffer();
 exec('/bin/dd if=/dev/zero of='.$tdev.' bs=1m count=100', $result, $rv);
 if ($rv != 0)
  install_error('Could not zero-write first 100MiB of target device', $rv);

 // create GPT partition scheme
 echo($tag.'Creating GPT partition scheme on target'.chr(10));
 flush_buffer();
 exec('/sbin/gpart create -s GPT '.$target);

 // create GPT partitions
 echo($tag.'Creating partitions'.chr(10));
 flush_buffer();
 exec('/sbin/gpart add -b 128 -s 512 -t freebsd-boot '.$target);
 exec('/sbin/gpart add -b 2048 -s '.$size_syspartition.' -t freebsd-ufs -l '
  .$gpt_system_label.' '.$target, $output, $rv);
 if ($createdatapartition)
  exec('/sbin/gpart add -b 2048 -s '.$size_datapartition.' -t freebsd-ufs -l '
   .$gpt_data_label.' '.$target, $output, $rv);
 if ($rv != 0)
  install_error('Failed adding GPT partitions to target device', $rv);

 // boot code
 echo($tag.'Inserting bootcode to target device'.chr(10));
 flush_buffer();
 exec('/sbin/gpart bootcode -b '.$data['path_mbr'].' -p '.$data['path_loader']
  .' -i 1 '.$target, $output, $rv);
 if ($rv != 0)
  install_error('Failed adding bootcode to target device', $rv);

 // create UFS filesystem on system partition
 echo($tag.'Creating UFS2 filesystem on system partition'.chr(10));
 flush_buffer();
 exec('/sbin/newfs -U -m 2 -i 2048 '.$path_syslabel.' > /dev/null', $output, 
  $rv);
 if ($rv != 0)
  install_error('Failed to create UFS filesystem on '.$path_syslabel, $rv);

 // mount the created UFS filesystem
 echo($tag.'Mounting '.$path_syslabel.' to temporary mountpoint'.chr(10));
 flush_buffer();
 exec('/bin/mkdir -p /usb');
 exec('/bin/mount -t ufs -o noatime '.$path_syslabel.' '.$temp_mp, $output, $rv);
 if ($rv != 0)
  install_error('Failed to mount UFS2 filesystem to '.$temp_mp, $rv);

 // populate mounted filesystem
 echo($tag.'Populating mounted filesystem'.chr(10));
 flush_buffer();

 echo('* copying boot directory'.chr(10));
 exec('/usr/bin/tar c -C / -f - boot | tar x -C '.$temp_mp.' -f -');

 echo('* copying rescue directory'.chr(10));
 exec('/usr/bin/tar c -C / -f - rescue | tar x -C '.$temp_mp.' -f -');

 echo('* copying /boot/loader.conf'.chr(10));
 exec('/bin/cp -p '.$source.'/emb_loader.conf '.$temp_mp.'/boot/loader.conf');

 echo('* copying system image and checksums'.chr(10));
 exec('/bin/cp -p '.$data['sysloc'].'* '.$temp_mp);

 echo('* creating config directory'.chr(10));
 exec('/bin/mkdir -p '.$temp_mp.'/config');
 exec('/usr/sbin/chown www:www '.$temp_mp.'/config');
 exec('/bin/chmod 770 '.$temp_mp.'/config');

 echo('* copying mod directory to /mod'.chr(10));
 exec('/bin/cp -Rp '.$source.'/mod '.$temp_mp.'/');
 exec('/usr/sbin/chown -R root:wheel '.$temp_mp.'/mod');
 exec('/bin/chmod 750 '.$temp_mp.'/mod');

 echo('* compressing web interface'.chr(10));
 exec('/usr/bin/tar cfz '.$temp_mp.'/'.$webinterface_name.' -C '
  .$webinterface_source.' *', $output, $rv);
 if ($rv != 0)
  install_error('Could not copy web-interface', $rv);
 echo('* done populating system filesystem'.chr(10));

/*
TUNE=${SCRIPT}/tunables
TMPFS=/tmpfs
SYSTEM=${TMPFS}/system
SYSTEM_NAME=system.ufs
CDROM=${TMPFS}/cdrom
CDROM_MBR=${CDROM}/boot/pmbr
CDROM_GPT=${CDROM}/boot/gptboot
PRELOADED=${TMPFS}/preloaded
USB=${TMPFS}/usb
SCHEME=GPT
SIZE_MB=325
#SIZE_PARTITION=819200          # = 400MiB
SIZE_PARTITION=655360           # = 320MiB
MD_NR=9
MD="md${MD_NR}"
MD_DEV="/dev/${MD}"
LABEL=GURU-USB
LABEL_DEV=/dev/gpt/${LABEL}
NAME=guruusb
WEBINTERFACE_SOURCE="`realpath ${SCRIPT}/zfsguru017`"
WEBINTERFACE_NAME="ZFSguru-webinterface.tgz"
*/

}

function install_error($errormsg, $rv = false)
// bails out with given error
{
 global $tag;
 echo(chr(10).chr(10));
 echo('ERROR: '.$errormsg.chr(10));
 if ($rv !== false)
  echo('Return value: '.(int)$rv.chr(10).chr(10));
 echo($tag.'Script execution halted!'.chr(10));
 die(1);
}

function flush_buffer()
{
 @ob_end_flush();
 @ob_flush();
 @flush();
 @ob_start(); 
}

?>
