<?php

function content_pools_pools()
{
 // required libraries
 activate_library('zfs');

 // call functions
 $zpools = zfs_pool_list();

 // pool count
 $poolcount = count($zpools);
 $poolcountstr = ($poolcount == 1) ? '' : 's';

 // process table poollist
 $poollist = array();
 foreach ($zpools as $poolname => $pooldata)
 {
  $class = (@$_GET['query'] == $poolname) ? 'activerow' : 'normal';
  $poolspa = zfs_pool_version($poolname);
  $zpool_status = `zpool status $poolname`;
  if (strpos($zpool_status,'raidz3') !== false)
   $redundancy = 'RAID7 (triple parity)';
  elseif (strpos($zpool_status,'raidz2') !== false)
   $redundancy = 'RAID6 (double parity)';
  elseif (strpos($zpool_status,'raidz1') !== false)
   $redundancy = 'RAID5 (single parity)';
  elseif (strpos($zpool_status,'mirror') !== false)
   $redundancy = 'RAID1 (mirroring)';
  else
   $redundancy = 'RAID0 (no redundancy)';
  $statusclass = 'normal';
  if ($pooldata['status'] == 'ONLINE')
   $statusclass = 'pool_online';
  elseif ($pooldata['status'] == 'FAULTED')
  {
   $statusclass = 'pool_faulted';
   if ($class == 'normal')
    $class = 'pool_faulted';
  }
  elseif ($pooldata['status'] == 'DEGRADED')
  {
   $statusclass = 'pool_degraded';
   if ($class == 'normal')
    $class = 'pool_degraded';
  }
  $poollist[] = array(
   'POOLLIST_CLASS'		=> $class,
   'POOLLIST_POOLNAME'		=> htmlentities(trim($poolname)),
   'POOLLIST_SPA'		=> $poolspa,
   'POOLLIST_REDUNDANCY'	=> $redundancy,
   'POOLLIST_SIZE'		=> $pooldata['size'],
   'POOLLIST_USED'		=> $pooldata['used'],
   'POOLLIST_FREE'		=> $pooldata['free'],
   'POOLLIST_STATUS'		=> $pooldata['status'],
   'POOLLIST_STATUSCLASS'	=> $statusclass,
   'POOLLIST_POOLNAME_URLENC'	=> htmlentities(trim($poolname))
  );
 }

 // import pool buttons
 $import_buttons = '';
 $import_rawoutput = '';
 if (@isset($_POST['import_pool']) OR @isset($_POST['import_pool_deleted']))
 {
  $importdestroyed = (@isset($_POST['import_pool_deleted'])) ? true : false;
  if ($importdestroyed)
   $result = zfs_pool_importable(true);
  else
   $result = zfs_pool_importable(false);
  if ($result['rv'] == 0)
  {
   $importarr = array();
   if (@is_array($result['importable']))
    foreach ($result['importable'] as $importable)
     if ($importdestroyed)
      $importarr[] = '<input type="submit" name="import_destroyed_'
       .$importable['id'].'" '
       .'value="Import pool '.htmlentities($importable['pool']).'" /> ';
     else
      $importarr[] = '<input type="submit" name="import_hidden_'
       .$importable['id'].'" '
       .'value="Import pool '.htmlentities($importable['pool']).'" /> ';
   $import_buttons = implode(' ', $importarr);
   $import_rawoutput = $result['output_str'];
  }
  elseif ($result['rv'] == 1)
   $import_rawoutput = 'No importable pools have been found.';
  else
   $import_rawoutput = '<span class="warning">WARNING: got return value '
    .$result['rv'].chr(10).$result['output_str'];
 }

 $import_class = (strlen($import_rawoutput) > 0) ? 'normal' : 'hidden';

 // new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'Pool status',
  'PAGE_TITLE'		=> 'Pool status',
  'TABLE_POOL_POOLLIST'	=> $poollist,
  'POOL_COUNT'		=> $poolcount,
  'POOL_COUNT_STRING'	=> $poolcountstr,
  'POOL_IMPORTBUTTONS'	=> $import_buttons,
  'POOL_IMPORTOUTPUT'	=> $import_rawoutput,
  'CLASS_IMPORTOUTPUT'	=> $import_class
 );
 return $newtags;
}

function submit_pools_importpool()
{
 // required library
 activate_library('zfs');

 // scan POST variables for hidden or destroyed pool import buttons
 $url = 'pools.php';
 $result = false;
 $poolname = '';
 foreach ($_POST as $var => $value)
  if (substr($var, 0, strlen('import_hidden_')) == 'import_hidden_')
  {
   $poolname = substr($var, strlen('import_hidden_'));
   $result = zfs_pool_import($poolname, $url, false);
  }
  elseif (substr($var, 0, strlen('import_destroyed_')) == 'import_destroyed_')
  {
   $poolname = substr($var, strlen('import_destroyed_'));
   $result = zfs_pool_import($poolname, $url, true);
  }
 // dangerouscommand may have handled request by now, if not redirect
 if ($result)
 {
  page_feedback('pool imported successfully', 'b_success');
  redirect_url($url);
 }
 elseif ((strlen($poolname) > 0) AND !$result)
  friendlyerror('failed importing pool', $url);
}

?>
