<?php

/*
** ZFSguru Web-interface - zfs.php
** ZFS related function library
*/


/* query functions */

// pool functions

function zfs_pool_list($poolname = false)
// detect zpool by looking at zpool list output
{
 $zpools = array();
 if ($poolname == false)
  exec('/sbin/zpool list', $zpools_raw);
 else
  exec('/sbin/zpool list '.$poolname, $zpools_raw);
 $zpool_count = count($zpools_raw)-1;
 for ($i = 1; $i <= $zpool_count; $i++)
 {
  $chunks = preg_split('/\s/m', $zpools_raw[$i], -1, PREG_SPLIT_NO_EMPTY);
  $zpool_name = $chunks[0];
  $zpools[$zpool_name]['size'] = $chunks[1];
  $zpools[$zpool_name]['used'] = $chunks[2];
  $zpools[$zpool_name]['free'] = $chunks[3];
  $zpools[$zpool_name]['cap'] = $chunks[4];
  // zfs v20 and below do not have DEDUP column
  if (count($chunks) == 7)
   $zpools[$zpool_name]['status'] = $chunks[5];
  elseif (count($chunks) == 8)
  {
   $zpools[$zpool_name]['dedup'] = $chunks[5];
   $zpools[$zpool_name]['status'] = $chunks[6];
  }
  else
   error('detecting ZFS pools failed; unexpected output from zfs list');
 }
 if (($poolname != false) AND (count($zpools) == 1))
  return current($zpools);
 else
  return $zpools;
}

function zfs_pool_status($poolname)
// returns detailed array of information about given pool
{
 // execute zpool status command (does not need root)
 $zpool_status = `/sbin/zpool status $poolname`;

 // pool data
 preg_match('/^[\s]*state\: (.*)$/m', $zpool_status, $state);
 preg_match('/^[\s]*status\: (.*)^action\: /sm', $zpool_status, $status);
 preg_match('/^[\s]*action\: (.*)$/m', $zpool_status, $action);
 preg_match('/^[\s]*see\: (.*)$/m', $zpool_status, $see);
 preg_match('/^[\s]*scrub\: (.*)$/m', $zpool_status, $scrub);
 preg_match('/^[\s]*config\: (.*)$/m', $zpool_status, $config);

 // split data
 $split_regexp = '/^[\s]+NAME[\s]+STATE[\s]+READ[\s]+WRITE[\s]+CKSUM[\s]*$/m';
 $split = preg_split($split_regexp, $zpool_status);
 $memberchunk = substr(@$split[1], 0, strpos(@$split[1], 'errors: '));
 $errors = substr(@$split[1], strpos(@$split[1], 'errors: '));

 // retrieve pool details
 $details = array();
 $dsplit = preg_split('/^[\s]*([a-zA-Z]+)\:/m', @$split[0], null, 
  PREG_SPLIT_DELIM_CAPTURE);
 for ($i = 1; $i < 99; $i++)
  if (@isset($dsplit[$i]))
   $details[trim($dsplit[$i])] = trim($dsplit[++$i]);
  else
   break;
 // rename scan to scrub for compatibility with ZFS v15 data format
 if (@isset($details['scan']))
 {
  $details['scrub'] = $details['scan'];
  unset($details['scan']);
 }

 // retrieve pool members
 $poolmembers = array();
 if (@strlen($memberchunk) > 0)
 {
  $status_arr = explode(chr(10), $memberchunk);
  $regexp_string = '/^[\s]*([^\s]+)[\s]+([^\s]+)[\s]+'
   .'([0-9]+[^\s]*)[\s]+([0-9]+[^\s]*)[\s]+([0-9]+[^\s]*)[\s]*(.*)$/';
  foreach ($status_arr as $line)
   if (preg_match($regexp_string, $line, $memberdata))
    $poolmembers[] = @array('name' => $memberdata[1],
     'state' => $memberdata[2], 'read' => $memberdata[3],
     'write' => $memberdata[4], 'cksum' => $memberdata[5],
     'extra' => $memberdata[6]);
 }

 // assemble and return data
 $pool_info = $details;
 $pool_info['errors'] = $errors;
 $pool_info['members'] = $poolmembers;
 return $pool_info;
}

function zfs_pool_version($poolname)
// retrieves version property from given pool and returns it
{
 $result = exec('/sbin/zpool get version '.$poolname);
 if (@strlen($result) > 3)
 {
  $preg_string = '/^[^\s]+[\s]+[^\s]+[\s]+([^\s]+)[\s]+[^\s]+$/';
  preg_match($preg_string, $result, $matches);
  if (strlen($matches[1]) > 0)
   return $matches[1];
 }
 return false;
}

function zfs_pool_getbootfs($poolname)
// retrieves bootfs property value from given pool and returns it
{
 // requires root privileges ?
 activate_library('super');
 // fetch bootfs property
 $result = super_execute('/sbin/zpool get bootfs '.$poolname);
 if (@strlen($result['output_arr'][1]) > 0)
 {
  $preg_string = '/^[^\s]+[\s]+[^\s]+[\s]+([^\s]+)[\s]+[^\s]+$/m';
  preg_match($preg_string, $result['output_arr'][1], $matches);
  if (@strlen($matches[1]) > 0)
   return $matches[1];
 }
 return false;
}

function zfs_pool_isbeingscrubbed($pool)
// returns true if scrub in progress on $pool, false if not
{
 $status_output = array();
 $status_str = '';
 exec('/sbin/zpool status '.$pool, $status_output);
 foreach ($status_output as $line)
  $status_str .= $line;
 if (strpos($status_str,'scrub in progress') === false)
  return false;
 else
  return true;
}

function zfs_pool_memberdisks()
// detect GEOM labeled disks part of a ZFS pool
{
 $cmd = array();
 exec('/sbin/zpool status', $cmd);
 $cmd_str = implode(chr(10), $cmd);
 $chunks = preg_split('/^[\s]*pool: /m', $cmd_str, -1,
  PREG_SPLIT_NO_EMPTY);
 if (is_array($chunks))
  foreach($chunks as $chunk)
  {
   $poolname = trim(substr($chunk,0,strpos($chunk,' ')));
   $members[$poolname] = array();
   // search for member disks
   unset($preg_gpt);
   unset($preg_geom);
   preg_match_all('/^[\s]*gpt\/[a-zA-Z0-9\-\_\.]*/m', $chunk, $preg['gpt']);
   preg_match_all('/^[\s]*label\/[a-zA-Z0-9\-\_\.]*/m', $chunk, $preg['geom']);
   preg_match_all('/^[\s]*ada?[0-9]+/m', $chunk, $preg['ata']);
   preg_match_all('/^[\s]*da[0-9]+/m', $chunk, $preg['scsi']);
   // process all preg matches in $preg array
   if (is_array($preg))
    foreach ($preg as $matches)
     if (@is_array($matches[0]))
      foreach ($matches[0] as $labelname)
       $members[$poolname][] = trim($labelname);
  }
 return $members;
}


function zfs_pool_ismemberdisk($disk, $members, $strict_comparison = true)
// checks if given disk is a member of a ZFS pool and returns the pool name
{
 if (is_array($members))
  foreach ($members as $poolname => $data)
   if (is_array($data))
    foreach ($data as $labelname)
     if ($disk == $labelname)
      return $poolname;
     elseif ((!$strict_comparison) AND
             (substr($labelname, 0, strlen($disk)) == $disk))
      return $poolname;
 return false;
}

function zfs_pool_ashift($poolname)
// returns ashift value for given pool
{
 activate_library('super');
 $result = super_execute('/usr/sbin/zdb -e '.$poolname.' | grep ashift');
 if (preg_match('/^[\s]*ashift\=([0-9]+)/m', $result['output_str'], 
  $matches))
  return $matches[1];
 elseif (preg_match('/^[\s]*ashift\:[\s]*([0-9]+)/m', $result['output_str'], 
  $matches))
  return $matches[1];
 else
  return false;
}

// filesystem functions

function zfs_filesystem_list($fs = '', $arguments = '')
// detect filesystems by looking at zfs list output
{
 // generate data
 $fsarr = array();
 $command = '/sbin/zfs list '.$arguments.' '.$fs;
 exec($command, $result, $rv);
 if ((@count($result) > 1) AND ($rv == 0))
 {
  // extract data from output
  // note that with $i starting at index 1 (not 0) we skip first line
  for ($i = 1; $i <= count($result)-1; $i++)
  {
   $split = preg_split('/[\s]+/m', @$result[$i], 5);
   $newarr = @array(
    'name'		=> $split[0],
    'used'		=> $split[1],
    'avail'		=> $split[2],
    'refer'		=> $split[3],
    'mountpoint'	=> $split[4]
   );
   $fsarr[$newarr['name']] = $newarr;
  }
  return $fsarr;
 }
 else
  return false;
}

function zfs_filesystem_list_one($fs = '', $arguments = '')
// simplified filesystem_list returning only one filesystem
{
 $fsarr = zfs_filesystem_list($fs, $arguments);
 return current($fsarr);
}

function zfs_filesystem_getproperties($filesystem, $property = false)
// retrieves filesystem properties from 'zfs get all' command
{
 $fsdetails = array();
 $fsdetails_info = array();
 $queryfs = trim($filesystem);

 // get detailed properties for this filesystem
 if ($property == false)
  $command = '/sbin/zfs get all '.$queryfs;
 else
  $command = '/sbin/zfs get '.$property.' '.$queryfs;
 exec($command, $details, $rv_details);
 if ($rv_details != 0)
  return false;
 if (@is_array($details))
  for ($i = 1; $i < count($details); $i++)
  {
   $split = preg_split('/[\s]+/m', $details[$i]);
   if ((@$split[3] == '-') OR (@$split[1] == 'creation'))
    $fsdetails_info[trim($split[1])] = $split;
   else
    $fsdetails[trim(@$split[1])] = $split;
  }
 if ($property == false)
  $returnarray = array('settings' => $fsdetails, 'info' => $fsdetails_info);
 else
  $returnarray = array_merge($fsdetails, $fsdetails_info);
 return $returnarray;
}

function zfs_filesystem_volumes()
// detect ZVOLs by looking at zfs list and diskinfo output
{
 $zvols = array();
 exec('/sbin/zfs list -t volume', $output, $rv);
 if ($rv != 0)
  return $rv;
 for ($i = 1; $i < count($output); $i++)
 {
  $split = preg_split('/[\s]/m', $output[$i], null, PREG_SPLIT_NO_EMPTY);
  $zvolname = @$split[0];
  if (@strlen($zvolname) > 0)
  {
   // requires disk library
   activate_library('disk');
   $diskinfo = disk_info('/dev/zvol/'.$zvolname);
   $zvols[$zvolname] = @array('zvol' => $split[0], 'used' => $split[1],
    'avail' => $split[2], 'refer' => $split[3], 'mountpoint' => $split[4],
    'diskinfo' => $diskinfo);
  }
 }
 return $zvols;
}



/* active functions that change or influence something */

function zfs_pool_setbootfs($poolname, $bootfs = false, $redirect_url = false)
// sets bootfs property on given pool; empty/false bootfs disables bootfs
{
 $zpools = zfs_detect_zpools();
 if (@!isset($zpools[$poolname]))
  error('Invalid poolname "'.$poolname.'"; pool does not exist.');
 if ($bootfs == false)
  $bootfs = '';
 if ($redirect_url === false)
  $redirect_url = 'pools.php?boot='.urlencode($poolname);
 dangerous_command('/sbin/zpool set bootfs='.$bootfs.' '.$poolname, 
  $redirect_url);
 error('HARD ERROR: unhandled exit zfs_pool_setbootfs');
}

function zfs_pool_importable($deleted = false)
// returns array of importable pools
{
 // requires root privileges
 activate_library('super');

 // execute command to search for pools
 if ($deleted)
  $result = super_execute('/sbin/zpool import -D -d /dev');
 else
  $result = super_execute('/sbin/zpool import -d /dev');

 // dig through output to construct importables array
 $importables = array();
 if ($result['rv'] == 0)
 {
  $split = preg_split('/^[\s]*pool\: /m', $result['output_str']);
  if (is_array($split))
   foreach ($split as $splitid => $poolchunk)
    if (@(int)$splitid > 0)
    {
     // preg_match('/^[\s]*pool\: (.*)$/m', $poolchunk, $preg_pool);
     preg_match('/^[\s]*id\: ([0-9]*)$/m', $poolchunk, $preg_id);
     // $pool = (@$preg_pool[1]) ? $preg_pool[1] : false;
     $pool = trim(substr($poolchunk,0,strpos($poolchunk, chr(10))));
     $id = (@$preg_id[1]) ? $preg_id[1] : false;
     if ((strlen($pool) > 0) AND (strlen($id) > 0))
      $importables[] = array('pool' => $pool, 'id' => $id);
    }
 }

 // return output
 return array(
  'rv'         => $result['rv'],
  'output_arr' => $result['output_arr'],
  'output_str' => $result['output_str'],
  'importable' => $importables
 );
}

function zfs_pool_import($poolname, $redirect, $deleted = false)
// imports a specified pool; returns raw result
{
 // super privileges
 activate_library('super');
 // import actual pool
 if ($deleted)
  $command = '/sbin/zpool import -d /dev -D -f '.$poolname;
 else
  $command = '/sbin/zpool import -d /dev -f '.$poolname;
 $result = super_execute($command);
 return $result;
}

function zfs_pool_scrub($poolname, $stop_scrub = false)
// starts or stops a scrub on a pool
{
 // super privileges
 activate_library('super');
 if ($stop_scrub)
  $command = '/sbin/zpool scrub -s '.$poolname;
 else
  $command = '/sbin/zpool scrub '.$poolname;
 $result = super_execute($command);
 if ($result['rv'] == 0)
  return true;
 else
  return false;
}


/* ZFS-related POST extract */

function zfs_extractsubmittedvdevs($url, $add_nop_suffix = false)
// extracts POST submitted vdevs and returns an array
{
 $member_disks = array();
 foreach ($_POST as $id => $val)
  if ($val == 'on')
   if (preg_match('/^addmember_(.*)$/', $id, $addmember))
    if (@strlen($addmember[1]) > 0)
     $member_disks[] = $addmember[1];
 if (empty($member_disks))
  friendlyerror('looks like you forgot to select member disks, please correct!',
   $url);
 $member_str = '';
 // TODO - SECURITY
 foreach ($member_disks as $disklabel)
  if ($add_nop_suffix)
   $member_str .= $disklabel.'.nop ';
  else
   $member_str .= $disklabel.' ';
 $member_arr = array();
 $member_arr['member_str'] = trim($member_str);
 $member_arr['member_disks'] = $member_disks;
 $member_arr['member_count'] = (int)@count($member_disks);
 return $member_arr;
}

function zfs_extractsubmittedredundancy($redundancy, $member_count, $url)
// extracts supplied $redundancy into sanitized zpool redundancy string
{
 switch ($redundancy)
 {
  case "raid0":
   $red = '';
   break;
  case "raid1":
   if ((int)$member_count < 2)
    friendlyerror('you have chosen RAID1 (mirroring) but have selected less '
     .'than two disks. Please select at least two disks. Note that you can '
     .'transform a single disk into a mirror later, and vice versa.', $url);
   $red = 'mirror';
   break;
  case "raid5":
   if ($member_count < 2)
    friendlyerror('you have chosen RAID5 (single parity) but have selected '
     .'less than two disks. Please select at least 2 disks.', $url);
   $red = 'raidz';
   break;
  case "raid6":
   if ($member_count < 3)
    friendlyerror('you have chosen RAID6 (double parity) but have selected '
     .'less than three disks. Please select at least 3 disks.', $url);
   $red = 'raidz2';
   break;
  case "raid7":
   if ($member_count < 4)
    friendlyerror('you have chosen RAID7 (triple parity) but have selected '
     .'less than four disks. Please select at least 4 disks.', $url);
   $red = 'raidz3';
   break;
  default:
   error('internal error: unable to format redundancy string '
    .'(extractsubmittedredundancy;'.htmlentities($redundancy).';'
    .htmlentities($member_count).')');
 }
 return $red;
}


?>
