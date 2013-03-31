<?php

function content_system_install_submit()
{
 // should not be here; redirect
 $url = 'system.php?install&progress';
 friendlyerror('Oops; you were at the wrong place', $url);
}

function submit_system_install()
// installs Root-on-ZFS or Embedded distribution on target device
{
 global $guru;

 // required libraries
 activate_library('guru');
 activate_library('persistent');
 activate_library('super');
 activate_library('zfs');

 // variables
 $url = 'system.php?install';
 $dist = @$_POST['dist'];
 $sysver = @$_POST['sysver'];
 $target = @$_POST['target'];
 $url2 = 'system.php?install&dist='.$dist.'&sysver='.$sysver.'&target='.$target;
 $url3 = $url.'&progress';

 // check for enough free space in temporary directory
 if (disk_free_space($guru['tempdir']) < 64 * 1024)
  error('not enough free space available in <b>'.$guru['tempdir']
   .'</b> - you may need to reboot or increase RAM size!');

 // craft options array
 $options = @array(
  'memtuning' => (strlen($_POST['memtuning_profile']) > 0) ? true : false,
  'portstree' => ($_POST['portstree'] == 'on') ? true : false,
  'copysysimg' => ($_POST['copysysimg'] == 'on') ? true : false,
  'compression' => ($_POST['compression']) ? $_POST['compression'] : 'off',
  'configureswap' => ((int)$_POST['configureswap'] > 0) ? true : false
 );

 // CAM boot delay
 $cam_boot_delay = (@$_POST['cam_boot_delay'] == 'on') ? 
  @$_POST['cam_boot_delay_sec'] * 1000 : false;

 // sanity of user input
 if (($dist != 'rootonzfs') AND ($dist != 'embedded'))
  error('incorrect distribution type selected!');
 // check if script is available
 if (!file_exists(@$guru['script_install']))
  error('install script "'.@$guru['script_install'].'" not found!');

 // locate system image
 $sysvers = guru_fetch_systemversions();
 if ((!@isset($sysvers[$sysver])) AND (strlen($sysver) == 32))
  $sysloc = guru_locate_systemimage($sysver);
 elseif ((!@isset($sysvers[$sysver])) AND (strlen($sysver) != 32))
  error('Could not locate system version <b>'.$sysver.'</b>');
 else
  $sysloc = guru_locate_systemimage(@$sysvers[$sysver]['md5hash']);
 if ($sysloc === false)
  error('Could not locate system image <b>'.$sysver.'</b>');

 // start construction of the install data array the install script will use
 $idata = array();
 $idata['install_dat_version'] = 1;
 $idata['dist'] = $dist;
 $idata['target'] = $target;
 $idata['sysver'] = $sysver;
 $idata['sysloc'] = $sysloc;
 $idata['sysimg_size'] = @filesize($sysloc);
 $idata['options'] = $options;

 // lookup checksum information
 if (!@isset($sysvers[$sysver]))
 {
  // unknown system version; we only know MD5 hash
  $idata['checksum_md5'] = $sysver;
  // perform SHA1 hash (may take awhile!)
  $idata['checksum_sha1'] = sha1_file($sysloc);
 }
 else
 {
  $idata['checksum_md5'] = $sysvers[$sysver]['md5hash'];
  $idata['checksum_sha1'] = $sysvers[$sysver]['sha1hash'];
 }

 // Root-on-ZFS distribution specific elements
 if ($dist == 'rootonzfs')
 {
  // validate input of target (pool) or redirect back to $url
  sanitize($target, false, $s);
  if (strlen($target) < 1)
   friendlyerror('no pool selected', $url);
  if ($s != $target)
   friendlyerror('pool name contains illegal characters', $url);
  // check if pool exists
  $zpools = zfs_pool_list();
  if (!@isset($zpools[$target]))
   error('Pool "'.$target.'" does not exist');
  // check pool status, must be ONLINE or DEGRADED
  if (($zpools[$target]['status'] != 'ONLINE') AND
      ($zpools[$target]['status'] != 'DEGRADED'))
   error('Pool "'.$target.'" has status "'.$zpools[$target]['status'].'"');

  // determine target prefix
  $target_prefix = $target.'/zfsguru';
  // create prefix filesystem if required
  $zfs_target_prefix = zfs_filesystem_list($target_prefix);
  if (!$zfs_target_prefix)
   super_execute('/sbin/zfs create -o atime=off '.$target_prefix);

  // determine target filesystem
  $targetfs = $target_prefix.'/'.@$_POST['targetfs'];
  if (strlen(@$_POST['targetfs']) < 1)
   friendlyerror('invalid target filesystem given', $url2);
  $zfs_targetfs = zfs_filesystem_list($targetfs);
  if ($zfs_targetfs !== false)
   friendlyerror('target filesystem <b>'.$targetfs
    .'</b> already exists as ZFS dataset!', $url2);
  if (is_dir('/'.$targetfs))
   friendlyerror('target filesystem <b>'.$targetfs
    .'</b> already exists as directory!', $url2);
  if (file_exists('/'.$targetfs))
   friendlyerror('target filesystem <b>'.$targetfs
    .'</b> already exists as file!', $url2);
  $strippedtargetfs = substr($targetfs, strpos($targetfs, '/') + 1);
  if (strlen($strippedtargetfs) < 1)
   error('Sanity failure on stripped target filesystem');

  // configure swap ZVOL
  if (@$_POST['configureswap_size'] > 0)
  {
   // swap volume (/<pool>/zfsguru/SWAP)
   $swapvol = $target_prefix.'/SWAP';
   // size in binary gigabytes (can be fractions like 0.5)
   $swapsize = (strlen(@$_POST['configureswap_size']) > 0) 
    ? $_POST['configureswap_size'] : 2;
   // create ZVOL if nonexistent
   if (!file_exists('/dev/zvol/'.$swapvol))
    super_execute('/sbin/zfs create -V '.$swapsize.'g '.$swapvol);
   // activate auto swap
   super_execute('/sbin/zfs set org.freebsd:swap=on '.$swapvol);
   // activate swap
   super_execute('/sbin/swapon /dev/zvol/'.$swapvol);
  }

  // add specific items to install data array
  $idata['bootfs'] = $strippedtargetfs;
  $idata['loaderconf'] = $guru['docroot'].'/files/roz_loader.conf';
 }

 // Embedded distribution specific elements
 if ($dist == 'embedded')
 {
  $idata['path_mbr'] = @$_POST['path_boot_mbr'];
  $idata['path_loader'] = @$_POST['path_boot_loader'];
  $idata['loaderconf'] = $guru['docroot'].'/files/emb_loader.conf';
 }

 // optional memory tuning
 if ($options['memtuning'])
 {
  // required library
  activate_library('loaderconf');

  // memory tuning profile
  $memtuning_profile = @$_POST['memtuning_profile'];

  // perform memory tuning on copied file from www directory
  $tmploaderconf = $guru['tempdir'].'/guru_optimized_loader.conf';
  $loadersource = $idata['loaderconf'];
  exec('/bin/cp -p '.$loadersource.' '.$tmploaderconf);
  $loadersettings = loaderconf_readsettings($tmploaderconf);
  $result = false;
  if (is_array($loadersettings))
  {
   // perform memory tuning on temporary loader.conf file in /tmp/
   $result = loaderconf_reset($memtuning_profile, $loadersettings, 
    $tmploaderconf);
   $idata['loaderconf'] = $tmploaderconf;
  }

  if ($result)
  {
   // save tuning profile to persistent storage (preserved after install)
   persistent_store('tuning_profile', substr($memtuning_profile, 0, 3));
   // save physical memory as well to detect changes which require retuning
   $physmem = (int)guru_sysctl('hw.physmem');
   if ($physmem > 0)
    persistent_store('tuning_physmem', (int)$physmem);
  }
 }

 // CAM boot delay
 if (is_int($cam_boot_delay))
 {
  // required library
  activate_library('loaderconf');

  // copy loaderconf to temporary file
  $tmploaderconf = $guru['tempdir'].'/guru_optimized_loader.conf';
  $loadersettings = loaderconf_readsettings($idata['loaderconf']);
  if (is_array($loadersettings))
  {
   // save updated loader.conf
   $loadersettings['kern.cam.boot_delay'] = array('enabled' => true,
    'value' => $cam_boot_delay);
   loaderconf_update($loadersettings, $tmploaderconf);
   $idata['loaderconf'] = $tmploaderconf;
  }
 }

 // dump install array to a file
 // use a file to dump a serialized php array into, read by install script
 $data_path = $guru['tempdir'].'/guru_install.dat';
 // first remove existing files with root permissions
 super_execute('/bin/rm -f '.$data_path);
 // now create new file with serialized array
 $result = file_put_contents($data_path, serialize($idata), LOCK_EX);
 if ($result === false)
  error('Could not write file "'.$data_path.'"');
 // set proper permissions
 super_execute('/usr/sbin/chown root:wheel '.$data_path);
 super_execute('/bin/chmod 644 '.$data_path);
 usleep(1000);

 // execute installation script
 $outputfile = $guru['tempdir'].'/zfsguru_install_output.txt';
 @unlink($outputfile);
 $command = $guru['script_install'].' > '.$outputfile.' 2>&1 &';
 $result = super_execute($command);
 if ($result['rv'] != 0)
  error('could not start installation script ('.(int)$rv.')');
 sleep(1);

 // redirect
 redirect_url($url3);

 // return tags
 $newtags = array(
  'INSTALL_RV'          => $result['rv'],
  'INSTALL_OUTPUTSTR'   => $result['output_str'],
 );
 return $newtags;
}

?>
