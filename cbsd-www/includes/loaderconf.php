<?php

// passive functions

function loaderconf_readsettings($filepath = false)
// returns loader settings in rich array
{
 $loaderpath = ($filepath) ? $filepath : '/boot/loader.conf';
 $loaderconf = @file_get_contents($loaderpath);
 $loadervars = array(
  'vm.kmem_size', 'vm.kmem_size_max',
  'vfs.zfs.arc_min', 'vfs.zfs.arc_max',
  'vfs.zfs.arc_meta_limit',
  'vfs.zfs.zfetch.array_rd_sz', 'vfs.zfs.zfetch.block_cap',
  'vfs.zfs.vdev.min_pending', 'vfs.zfs.vdev.max_pending'
 );
 // regexp for active loader variables
 $preg_loader = array(
  1=>'/^[\s]*([a-zA-Z0-9._-]+)[\s]*\=[\s]*\"?([a-zA-Z0-9._-]+)\"?[\s]*$/m',
  2=>'/^[\s]*\#([a-zA-Z0-9._-]+)[\s]*\=[\s]*\"?([a-zA-Z0-9._-]+)\"?[\s]*$/m');
 preg_match_all($preg_loader[1], $loaderconf, $active);
 // we also detect 'commented out' variables, prefixed with a #
 preg_match_all($preg_loader[2], $loaderconf, $commented);
 // start with hardcoded vars
 $loader = array();
 foreach ($loadervars as $loadervar)
  $loader[$loadervar] = array('enabled' => false, 'value' => '');
 // now process each known loader variable
 foreach ($commented[1] as $id => $loadervar)
  $loader[$loadervar] = array('enabled' => false,
   'value' => $commented[2][$id]);
 foreach ($active[1] as $id => $loadervar)
  $loader[$loadervar] = array('enabled' => true,
   'value' => $active[2][$id]);
 return $loader;
}


/*
** active functions
*/


function loaderconf_reset($profile, $loadersettings = false, 
                          $loaderconf = false)
// resets loader.conf with some recommended values; preserves existing options
{
 global $guru;

 // if loadersettings is not supplied, retrieve a loader.conf from files dir
 if ($loadersettings === false)
 {
  // required library
  activate_library('guru');

  // call function
  $currentver = guru_fetch_current_systemversion();
  $dist = @$currentver['dist'];
  if ($dist == 'livecd' OR $dist == 'embedded')
   $source_loaderconf = $guru['docroot'].'files/emb_loader.conf';
  elseif ($dist == 'rootonzfs')
   $source_loaderconf = $guru['docroot'].'files/roz_loader.conf';
  else
  {
   // set warning and use root-on-ZFS loader.conf
   page_feedback('unknown distribution type detected! '
    .'Using Root-on-ZFS loader.conf instead.', 'a_warning');
   $source_loaderconf = $guru['docroot'].'files/roz_loader.conf';
  }

  // root privileges
  activate_library('super');

  // copy loader.conf and reset proper permissions
  super_execute('/bin/cp -p '.$source_loaderconf.' /boot/loader.conf');
  super_execute('/usr/sbin/chown root:wheel /boot/loader.conf');
  super_execute('/bin/chmod 644 /boot/loader.conf');
  // now fetch loadersettings
  $loadersettings = loaderconf_readsettings();
 }

 // memory tuning
 $pro_static = array();
 switch ($profile)
 {
  case "aggressive":
   $pro_multiply = array(
    'vm.kmem_size' => 1.5,
    'vfs.zfs.arc_max' => 0.75,
    'vfs.zfs.arc_min' => 0.5
   );
   $pro_static = array(
    'vfs.zfs.prefetch_disable' => '0',
   );
   break;
  case "performance":
   $pro_multiply = array(
    'vm.kmem_size' => 1.5,
    'vfs.zfs.arc_max' => 0.6,
    'vfs.zfs.arc_min' => 0.4
   );
   $pro_static = array(
    'vfs.zfs.prefetch_disable' => '0',
   );
   break;
  case "conservative":
   $pro_multiply = array(
    'vm.kmem_size' => 1.5,
    'vfs.zfs.arc_max' => 0.3,
    'vfs.zfs.arc_min' => 0.2
   );
   break;
  case "minimal":
   $pro_multiply = array(
    'vm.kmem_size' => 1.5,
    'vfs.zfs.arc_max' => 0.1,
    'vfs.zfs.arc_min' => 0.1
   );
   break;
  case "none":
   $pro_multiply = array();
   break;
  case "i386":
   $pro_static = array(
    'vm.kmem_size' => '512M',
    'vfs.zfs.arc_max' => '128M',
    'vfs.zfs.arc_min' => '128M',
    'vfs.zfs.prefetch_disable' => '1'
   );
   break;
  case "default":
  default:
   $pro_multiply = array(
    'vm.kmem_size' => 1.5,
    'vfs.zfs.arc_max' => 0.5,
    'vfs.zfs.arc_min' => 0.2
   );
 }

 // calculate physical memory in GiB
 $physmem = (int)guru_sysctl('hw.physmem');
 $physmem_gib = round($physmem / (1024 * 1024 * 1024), 1);
 if ($physmem_gib < 0.75)
  error('Less than 768MiB physical memory. Memory tuning not possible; '
   .'add more RAM!');

 // process factors that multiply with the physical RAM in GiB
 foreach ($pro_multiply as $loadervar => $factor)
  $loadersettings[$loadervar] = array('enabled' => true, 'value' =>
   round($physmem_gib * $factor, 1).'g');

 // process static values
 foreach ($pro_static as $loadervar => $value)
  $loadersettings[$loadervar] = array('enabled' => true, 'value' => $value);

 // save loadersettings
 loaderconf_update($loadersettings, $loaderconf);
 return $loadersettings;
}

function loaderconf_update($new_settings, $filepath = false)
// updates loader.conf with new settings in rich $new_settings array
{
 global $guru;
 // determine which file to work on
 $loaderpath = ($filepath) ? $filepath : '/boot/loader.conf';
 // read raw config
 $rawconf = @file_get_contents($loaderpath);
 // read current config
 $loaderconf = loaderconf_readsettings($filepath);
 // compare new config
 foreach ($new_settings as $loadervar => $data)
 {
  $preg_commented = '/^[\s]*\#[\s]*'.str_replace('.', '\.', $loadervar)
   .'[\s]*\=.*$/m';
  if (@$loaderconf[$loadervar]['enabled'])
  {
   if ($data['enabled'])
   {
    // currently enabled but want different value; adjust!
    $preg = '/^[\s]*'.str_replace('.', '\.', $loadervar).'[\s]*\=.*$/m';
    $newvar = $loadervar.'="'.$data['value'].'"';
    $rawconf = preg_replace($preg, $newvar, $rawconf, 1);
   }
   else
   {
    // currently enabled but want to disable (comment out)
    $preg = '/^[\s]*'.str_replace('.', '\.', $loadervar).'[\s]*\=.*$/m';
    $newvar = '#'.$loadervar.'="'.$data['value'].'"';
    $rawconf = preg_replace($preg, $newvar, $rawconf, 1);
   }
  }
  elseif (preg_match($preg_commented, $rawconf))
  {
   // variable is commented out
   if ($data['enabled'])
   {
    // activate commented out variable
    $newvar = $loadervar.'="'.$data['value'].'"';
    $rawconf = preg_replace($preg_commented, $newvar, $rawconf, 1);
   }
  }
  elseif ($data['enabled'])
  {
   // non-existent; append to loader.conf
   $rawconf .= chr(10).chr(10).$loadervar.'="'.$data['value'].'"'.chr(10);
  }
 }

 // root privileges
 activate_library('super');

 // write configuration to disk
 $result = file_put_contents($guru['tempdir'].'/newloader.conf', $rawconf);
 if (!$result)
  return false;
 $result = super_execute('/bin/mv '.$guru['tempdir'].'/newloader.conf '
  .$loaderpath);
 if ($result['rv'] != 0)
  return false;
 super_execute('/usr/sbin/chown root:wheel '.$loaderpath);
 super_execute('/bin/chmod 644 '.$loaderpath);
 return true;
}

?>
