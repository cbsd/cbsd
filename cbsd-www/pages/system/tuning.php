<?php

function content_system_tuning()
{
 // required libraries
 activate_library('guru');
 activate_library('loaderconf');
 activate_library('persistent');

 // tabbar
 $tabbar = array(
  'auto' => 'Automatic tuning',
  'zfs' => 'ZFS tuning',
  'advanced' => 'Advanced'
 );
 $url = 'system.php?tuning';

 // select tab
 $class_tab_auto = 'hidden';
 $class_tab_zfs = 'hidden';
 $class_tab_advanced = 'hidden';
 if (@isset($_GET['advanced']))
  $class_tab_advanced = 'normal';
 elseif (@isset($_GET['zfs']))
  $class_tab_zfs = 'normal';
 else
  $class_tab_auto = 'normal';

 // fetch data
 $loadersettings = loaderconf_readsettings();
 $currentver = guru_fetch_current_systemversion();
 $platform = guru_getsystemplatform();

 // livecd warning (warn that settings will not be preserved)
 if ($currentver['dist'] == 'livecd')
  page_feedback('LiveCD detected, your tuning settings will not be preserved!',
   'a_warning');

 /* tab: automatic */

 // physical memory
 $physmem = guru_sysctl('hw.physmem');
 $physmem_gib = @sizebinary($physmem, 1);

 // detect memory change
 $class_memorychange = 'hidden';
 $storedmem = persistent_read('tuning_physmem');
 if ($storedmem > 0)
  if (@($storedmem / $physmem < 0.95) OR @($storedmem / $physmem > 1.05))
   $class_memorychange = 'normal';

 // kmem (kernel memory)
 $kmem = @sizebinary(guru_sysctl('vm.kmem_size'), 1);
 $kmem_max = @sizebinary(guru_sysctl('vm.kmem_size_max'), 1);
 $kmem_free = @sizebinary(guru_sysctl('vm.kmem_map_free'), 1);

 // ARC (adaptive replacement cache)
 $arc_min = @sizebinary(guru_sysctl('vfs.zfs.arc_min'), 1);
 $arc_wired = '';
 $arc_max = @sizebinary(guru_sysctl('vfs.zfs.arc_max'), 1);
 $arc_min_pct = round(guru_sysctl('vfs.zfs.arc_min') / 
  guru_sysctl('vm.kmem_size') * 100, 1);
 $arc_max_pct = round(guru_sysctl('vfs.zfs.arc_max') / 
  guru_sysctl('vm.kmem_size') * 100, 1);

 // prefetching
 $prefetch = array(
  'i386_normal'		=> 'hidden',
  'i386_forced'		=> 'hidden',
  'amd64_enabled'	=> 'hidden',
  'amd64_disabled'	=> 'hidden',
  'amd64_forced'	=> 'hidden',
  'unknown'		=> 'hidden'
 );
 $enoughmemory = ($physmem > (4 * 1024 * 1024 * 1024));
 $forced = (@$loadersettings['vfs.zfs.prefetch_disable'] === 0);
 if ($platform == 'i386')
 {
  if ($forced)
   $prefetch['i386_forced'] = 'normal';
  else
   $prefetch['i386_normal'] = 'normal';
 }
 elseif ($platform == 'amd64')
 {
  if ($enoughmemory)
   $prefetch['amd64_enabled'] = 'normal';
  elseif ($forced)
   $prefetch['amd64_forced'] = 'normal';
  else
   $prefetch['amd64_disabled'] = 'normal';
 }
 else
  $prefetch['unknown'] = 'normal';

 // tuning profile selection (combobox)
 $tuning_selected = persistent_read('tuning_profile');
 @$tuning[$tuning_selected] = 'selected="selected"';
  
 /* tab: zfs */

 // zfs tuning table
 $tuningvars = array(
  'vfs.zfs.arc_min' => 
   'minimum ARC (Adaptive Replacement Cache) memory allocated',
  'vfs.zfs.arc_max' => 
   'maximum ARC (Adaptive Replacement Cache) memory allocated',
  'vfs.zfs.arc_meta_limit' => 
   'ARC memory allocated for metadata such as directory index',
  'vfs.zfs.prefetch_disable' =>
   'disable prefetching for i386 and low-memory systems',
  'vfs.zfs.vdev.min_pending' => 
   'minimum queue depth on virtual devices',
  'vfs.zfs.vdev.max_pending' => 
   'maximum queue depth on virtual devices',
  'vfs.zfs.txg.synctime' => 
   'target transaction group (txg) cycle time',
  'vfs.zfs.txg.timeout' => 
   'maximum transaction group (txg) cycle time',
  'vfs.zfs.txg.write_limit_override' => 
   'absolute transaction group size in bytes',
  'vfs.zfs.cache_flush_disable' => 
   'ignore application flush commands - <span class="red">DANGER!</span>',
  'vfs.zfs.zil_disable' =>
   'completely disable ZFS Intent Log (ZIL) - <span class="red">DANGER!!</span>'
 );
 $zfstuning = array();
 foreach ($loadersettings as $loadervar => $data)
  if (@isset($tuningvars[$loadervar]))
  {
   $enabled = ($data['enabled']) ? 'checked="checked"' : '';
   $zfstuning[] = array(
    'TUNING_VAR'	=> $loadervar,
    'TUNING_VAR_B64'	=> base64_encode($loadervar),
    'TUNING_ENABLED'	=> $enabled,
    'TUNING_VALUE'	=> htmlentities($data['value']),
    'TUNING_DESC'	=> ucfirst($tuningvars[$loadervar])
   );
  }

 /* tab: advanced */

 // advanced tuning table
 $advancedtuning = array();
 foreach ($loadersettings as $loadervar => $data)
 {
  $enabled = ($data['enabled']) ? 'checked="checked"' : '';
  $advancedtuning[] = array(
   'TUNING_VAR'		=> $loadervar,
   'TUNING_VAR_B64'	=> base64_encode($loadervar),
   'TUNING_ENABLED'	=> $enabled,
   'TUNING_VALUE'	=> htmlentities($data['value'])
  );
 }

 // export new tags
 return @array(
  'PAGE_ACTIVETAB'		=> 'Tuning',
  'PAGE_TITLE'			=> 'System Tuning',
  'PAGE_TABBAR'			=> $tabbar,
  'PAGE_TABBAR_URL'		=> $url,
  'TABLE_ZFS_TUNING_LIST'	=> $zfstuning,
  'TABLE_SYSTEM_TUNING_LIST'	=> $advancedtuning,
  'CLASS_TAB_AUTO'		=> $class_tab_auto,
  'CLASS_TAB_ZFS'		=> $class_tab_zfs,
  'CLASS_TAB_ADVANCED'		=> $class_tab_advanced,
  'CLASS_MEMORYCHANGE'		=> $class_memorychange,
  'TUN_PHYSMEM'			=> $physmem_gib,
  'TUN_KMEM'			=> $kmem,
  'TUN_KMEM_MAX'		=> $kmem_max,
  'TUN_KMEM_FREE'		=> $kmem_free,
  'TUN_ARC_MIN'			=> $arc_min,
  'TUN_ARC_MAX'			=> $arc_max,
  'TUN_ARC_MIN_PCT'		=> $arc_min_pct,
  'TUN_ARC_MAX_PCT'		=> $arc_max_pct,
  'TUN_PREFETCH_I386_NORMAL'	=> $prefetch['i386_normal'],
  'TUN_PREFETCH_I386_FORCED'	=> $prefetch['i386_forced'],
  'TUN_PREFETCH_AMD64_ENABLED'	=> $prefetch['amd64_enabled'],
  'TUN_PREFETCH_AMD64_DISABLED'	=> $prefetch['amd64_disabled'],
  'TUN_PREFETCH_AMD64_FORCED'	=> $prefetch['amd64_forced'],
  'TUN_PREFETCH_UNKNOWN'	=> $prefetch['unknown'],
  'TUNING_AGG'			=> $tuning['agg'],
  'TUNING_PER'			=> $tuning['per'],
  'TUNING_DEF'			=> $tuning['def'],
  'TUNING_CON'			=> $tuning['con'],
  'TUNING_MIN'			=> $tuning['min'],
  'TUNING_I38'			=> $tuning['i38']
 );
}

function submit_system_tuning_auto()
{
 // required libraries
 activate_library('loaderconf');
 activate_library('persistent');

 // fetch loadersettings
 $loadersettings = loaderconf_readsettings();

 // redirect url
 $url = 'system.php?tuning';

 // sanity checks
 if (@!is_array($loadersettings))
  friendlyerror('could not read existing /boot/loader.conf settings!', $url);
 elseif (@strlen($_POST['memtuning_profile']) < 1)
  friendlyerror('invalid memory tuning profile selected!', $url);

 // perform memory tuning
 $result = loaderconf_reset($_POST['memtuning_profile'], $loadersettings);
 if ($result)
 {
  // save tuning profile to persistent storage
  persistent_store('tuning_profile', substr($_POST['memtuning_profile'], 0, 3));
  // save physical memory as well to detect changes which require retuning
  $physmem = (int)guru_sysctl('hw.physmem');
  if ($physmem > 0)
   persistent_store('tuning_physmem', (int)$physmem);
 }

 // redirect back to tuning page
 if ($result)
  page_feedback('memory tuning successful; reboot to activate!', 'b_success');
 else
  page_feedback('memory tuning failed!', 'a_failure');
 redirect_url($url);
}

function submit_system_tuning_advanced()
{
 // require library
 activate_library('loaderconf');

 // fetch loadersettings
 $loadersettings = loaderconf_readsettings();

 // process submit functions
 $result = false;
 if (@isset($_POST['update_tuning']))
 {
  // update loadersettings with POST variables
  foreach ($loadersettings as $name => $data)
   if (($data['enabled']) AND 
       (!@isset($_POST['enabled_'.base64_encode($name)])) AND 
       (@isset($_POST[base64_encode($name)])))
    $loadersettings[$name]['enabled'] = false;
   elseif (@isset($_POST['enabled_'.base64_encode($name)]) AND
           (@strlen($_POST[base64_encode($name)]) > 0))
    $loadersettings[$name] = array('enabled' => true,
     'value' => $_POST[base64_encode($name)]);
  // add new variable
  if (@strlen($_POST['new_tuning_name']) > 0)
   $loadersettings[$_POST['new_tuning_name']] = @array('enabled' => true,
    'value' => $_POST['new_tuning_value']);
  // save loadersettings
  $result = loaderconf_update($loadersettings);
  // set SESSION flag that user needs to reboot
  $_SESSION['loaderconf_needreboot'] = true;
 }
 if (@isset($_POST['reset_tuning']))
 {
  // this totally resets the loader.conf to the proper distribution
  loaderconf_reset();
  $result = true;
  // set SESSION flag that user needs to reboot
  $_SESSION['loaderconf_needreboot'] = true;
 }

 // page feedback
 if ($result)
  page_feedback('tuning settings updated; reboot to activate!', 'b_success');
 else
  page_feedback('could not update tuning settings!', 'a_failure');

 // redirect
 if (@isset($_GET['advanced']))
  redirect_url('system.php?tuning&advanced');
 elseif (@isset($_GET['zfs']))
  redirect_url('system.php?tuning&zfs');
 else
  redirect_url('system.php?tuning');
}

?>
