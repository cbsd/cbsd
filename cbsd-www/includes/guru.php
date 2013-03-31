<?php

function guru_zfsversion()
// returns array with key ZFS version (zpl+spa)
{
 $zpl = guru_sysctl('vfs.zfs.version.zpl');
 $spa = guru_sysctl('vfs.zfs.version.spa');
 $arr = array('zpl' => $zpl, 'spa' => $spa);
 return $arr;
}

function guru_getsystemplatform()
// returns system platform string from sysctl variable
{
 $platform = guru_sysctl('hw.machine_arch');
 return $platform;
}

function guru_fetch_current_systemversion()
// returns currently running system version in an array
{
 global $guru;
 // fetch systemversions file first
 $sysvers = guru_fetch_systemversions();
 // distribution types (note: usb is depricated; renamed to embedded now)
 $disttypes = array(
  'rootonzfs' => '.dist',
  'livecd' => '.livecd',
  'embedded' => '.embedded',
  'usb' => '.usb');
 $name = '/'.strtolower($guru['product_name']);
 foreach ($disttypes as $dist => $suffix)
  if (file_exists($name.$suffix))
   if (is_readable($name.$suffix))
   {
    $md5 = @trim(file_get_contents($name.$suffix));
    foreach ($sysvers as $sysver => $data)
     if ($data['md5hash'] == $md5)
      return array('dist' => $dist, 'sysver' => $sysver, 'md5' => $md5);
    // current running system not known by remote system version list
    return array('dist' => $dist, 'sysver' => 'unknown', 'md5' => $md5);
   }
 $default = array('dist' => 'unknown', 'sysver' => 'unknown', 'md5' => '0');
 return $default;
}

function guru_check_livecd($sysvers)
{
 if (file_exists('/cdrom/system.ufs.uzip'))
  if (is_readable('/cdrom/system.ufs.uzip.md5'))
  {
   $md5 = @trim(file_get_contents('/cdrom/system.ufs.uzip.md5'));
   $sha1 = @trim(file_get_contents('/cdrom/system.ufs.uzip.sha1'));
   $filesize = @filesize('/cdrom/system.ufs.uzip');
   // check for match with existing $sysvers members
   $match = false;
   foreach ($sysvers as $sysver)
    if ($sysver['md5hash'] == $md5)
     $match = true;
   if (!$match)
    return array('name' => 'Unknown', 'bsdversion' => '???',
     'branch' => '???', 'platform' => '???', 'spa' => '???', 'notes' => '',
     'md5hash' => @trim(file_get_contents('/cdrom/system.ufs.uzip.md5')),
     'sha1hash' => @trim(file_get_contents('/cdrom/system.ufs.uzip.sha1')),
     'filesize' => @filesize('/cdrom/system.ufs.uzip')
    );
  }
 return false;
}

function guru_fetch_systemversions()
// downloads remote text file containing all system version data and returns it
// also checks for mounted media (LiveCD/Embedded)
{
 global $guru;

 // power cache
 $pcache = powercache_read('systemversions');
 if ($pcache != false)
 {
  // we cannot simply return cached result; it might be stale
  // so we check for unknown system version in /cdrom mountpoint first
  // check for mounted media (LiveCD/Embedded) at /cdrom mountpoint
  $cdrom_ver = guru_check_livecd($pcache);
  if ($cdrom_ver)
   $pcache['Unknown'] = $cdrom_ver;
  return $pcache;
 }

 // fetch remote system image file
 $systemimage_raw = remoteserver($guru['remote_system_url']);
 if (($systemimage_raw === false) OR (strlen($systemimage_raw) < 1))
 {
  // try to read an outdated version of the powercache
  $ocache = powercache_read('systemversions', true);
  if (@is_array($ocache))
  {
   page_feedback('using an outdated offline copy for system images.');
   // again, still check for /cdrom mounted image
   $cdrom_ver = guru_check_livecd($ocache);
   if ($cdrom_ver)        
    $ocache['Unknown'] = $cdrom_ver;
   return $ocache;
  }
  else
   error('Could not fetch remote system image from remote website; '
    .'check server internet connection. ZFSguru server may also be down.');
 }

 // parse system image file
 $preg_systemimg = '/^\[([a-zA-Z0-9\.\-\_]+)\]$/m';
 $preg = preg_split($preg_systemimg, $systemimage_raw);
 if (!is_array($preg))
  error('Could not fetch remote system version file');
 // split the entities inside [..] containers (..=..)
 $preg2 = array();
 foreach ($preg as $id => $txt)
 {
  $preg_entity = '/^([a-zA-Z0-9\.\-\_]+)\=(.+)$/m';
  preg_match_all($preg_entity, $txt, $preg2[$id]);
 }
 // second phase of parsing system image file
 $temparr = array();
 foreach (@$preg2 as $chunkid => $largechunk)
  foreach (@$largechunk[1] as $id => $property)
   if (@strlen($largechunk[2][$id]) > 0)
    $temparr[$chunkid][trim($property)] = $largechunk[2][$id];

 // check if ident and version matches this scripts known version
 if (@$temparr[1]['ident'] != 'systemimage')
  error('Invalid remote system identification!');
 if (@$temparr[1]['version'] != $guru['remote_system_version'])
  error('Invalid remote system version; update web-GUI!');

 // now follow the $temparr array and use it to fill the $sysvers array
 $sysvers = array();
 foreach ($temparr as $data)
  if (@isset($data['name']))
   if ($data['platform'] == guru_getsystemplatform())
   {
    $name = @trim($data['name']);
    if (strlen($name) > 0)
     $sysvers[$name] = $data;
   }

 // check for mounted media (LiveCD/Embedded) at /cdrom mountpoint
 if (file_exists('/cdrom/system.ufs.uzip'))
  if (is_readable('/cdrom/system.ufs.uzip.md5'))
  {
   $md5 = @trim(file_get_contents('/cdrom/system.ufs.uzip.md5'));
   $sha1 = @trim(file_get_contents('/cdrom/system.ufs.uzip.sha1'));
   $filesize = @filesize('/cdrom/system.ufs.uzip');
   // check for match with existing $sysvers members
   $match = false;
   foreach ($sysvers as $sysver)
    if ($sysver['md5hash'] == $md5)
     $match = true;
   if (!$match)
    $sysvers['Unknown'] = array('name' => 'Unknown', 'bsdversion' => '???',
     'branch' => '???', 'platform' => '???', 'spa' => '???', 'notes' => '',
     'md5hash' => @trim(file_get_contents('/cdrom/system.ufs.uzip.md5')),
     'sha1hash' => @trim(file_get_contents('/cdrom/system.ufs.uzip.sha1')),
     'filesize' => @filesize('/cdrom/system.ufs.uzip')
    );
  }

 // return result
 if (empty($sysvers))
  return false;
 else
 {
  // powercache store (3600 = 1 hour expiry)
  powercache_store('systemversions', $sysvers, 3600);
  return $sysvers;
 }
}

function guru_readremotefile($url)
// reads remote configuration file and parses it and returns an array
{
 global $guru;
 $contents = remoteserver($url);
 $carr = @explode(chr(10), $contents);
 if (!is_array($carr))
  return false;
 // store contents and return an array
 $configuration = array();
 foreach ($carr as $line)
  if ($s = strpos($line,'='))
   $configuration[substr($line,0,$s)] = trim(substr($line,$s+1));
 return $configuration;
}

function guru_locate_download($filename, $searchpath)
// searches path for filename (wildcard suffix) and returns path
{
 // execute find command
 $command = '/usr/bin/find '.$searchpath.' -type f -name "'.$filename.'*"';
 exec($command, $matches, $rv);
 // search for line that begins with forward slash (/) and return that line
 if (!empty($matches))
  foreach ($matches as $line)
   if ($line{0} == '/')
    return $line;
 // no hit - return false
 return false;
}

function guru_check_directdownload()
// returns true if a fetch command is running; false if not
{
 // required library
 activate_library('service');
 return service_isprocessrunning('fetch');
}

function guru_check_torrentdownload($filename = false, $searchpath = false)
// returns true if specified file is in files/downloading directory
{
 global $guru;

 // set default searchpath
 if ($searchpath == false)
  $searchpath = $guru['torrent']['path_downloading'].'/';

 if ($filename === false)
 {
  // query if any download is in the downloading directory
  $command = '/usr/bin/find '.$searchpath.' -type f';
  exec($command, $matches, $rv);
  if (empty($matches))
   return false;
  elseif ($rv != 0)
  {
   page_feedback('got return value '.$rv.' on find command!', 'a_error');
   return false;
  }
  else
   return true;
 }
 else
 {
  // queried direct filename; scan torrent downloading directory for filename
  // determine path to torrent filename
  // note the star suffix so we find each file that begins with $filename
  $command = '/usr/bin/find '.$searchpath.' -type f -name "'.$filename.'*"';
  exec($command, $matches, $rv);
  if (empty($matches))
   return false;
  elseif ($rv != 0)
  {
   page_feedback('got return value '.$rv.' on find command!', 'a_error');
   return false;
  }
  else
   return true;
 }
}


/* active functions */

function guru_loadkernelmodule($kmod)
// loads a given kernel module (excluding .ko suffix)
{
 activate_library('super');
 $result = super_execute('/sbin/kldload /boot/kernel/'.$kmod.'.ko');
 if ($result['rv'] == 0)
  return true;
 else
  return false;
}

function guru_mountmedia()
// mounts USB/LiveCD media to /cdrom mountpoint
{
 global $guru;
 // requires root privileges
 activate_library('super');
 // unmount possibly mounted media mountpoint
 super_execute('/sbin/umount '.$guru['path_media_mp'].' > /dev/null 2>&1');
 // create mountpoint directory if non-existent
 super_execute('/bin/mkdir -p '.$guru['path_media_mp'].' > /dev/null 2>&1');
 // attempt to mount media
 if (file_exists($guru['path_livecd']))
 {
  // mount LiveCD
  super_execute('/sbin/mount -t cd9660 '
   .$guru['path_livecd'].' '.$guru['path_media_mp']);
 }
 elseif (file_exists($guru['path_embedded']))
 {
  // mount embedded distribution
  super_execute('/sbin/mount -t ufs '
   .$guru['path_embedded'].' '.$guru['path_media_mp']);
 }
 // check again for system image
 if (file_exists($guru['path_media_mp'].$guru['path_media_systemfile']))
  return true;
 else
  return false;
}

function guru_unmountmedia()
// unmounts USB/LiveCD media at /cdrom mountpoint
{
 global $guru;
 // requires root privileges
 activate_library('super');
 // unmount possibly mounted media mountpoint
 $result = super_execute('/sbin/umount '.$guru['path_media_mp']
  .' > /dev/null 2>&1');
 // return result
 if ($result['rv'] == 0)
  return true;
 else
  return false;
}

function guru_locate_systemimage($md5hash)
// locates the specified system version on the system and returns its path
{
 global $guru;

 // mount CDROM/Embedded media first (DISABLED)
 // guru_mountmedia();

 // check for image on /cdrom
 $cdrom_file = $guru['media_systemimage'];
 $cdrom_file_md5 = $cdrom_file.'.md5';
 if (file_exists($cdrom_file))
  if (is_readable($cdrom_file_md5))
  {
   $md5 = trim(file_get_contents($cdrom_file_md5));
   if ($md5 == $md5hash)
    return $cdrom_file;
  }

 // search for locations specified in scanlocations array (need .md5 file)
 $scanlocations = array(
  $guru['tempdir'].'/', 
  $guru['torrent']['path_finished'].'/'
 );
 foreach ($scanlocations as $location)
 {
  unset($ls);
  exec('/usr/bin/find '.$location, $ls);
  if (@is_array($ls))
   foreach ($ls as $line)
    if (preg_match('/^(.+)\.ufs\.uzip$/', $line, $matches))
     if (@strlen($matches[1]) > 0)
      if (is_readable($matches[1].'.ufs.uzip.md5'))
      {
       $md5 = trim(file_get_contents($matches[1].'.ufs.uzip.md5'));
       if ($md5 == $md5hash)
        return $matches[1].'.ufs.uzip';
       unset($matches);
      }
 }

 // no hit; return false
 return false;
}

function guru_update_webgui($branch)
// updates the web-interface by either official online branch or HTTP upload
{
 global $guru;

 $url = 'system.php?update';
 $url2 = 'status.php?release&changelog';

 // requires root privileges
 activate_library('super');

 if ($branch == 'upload')
 {
  // HTTP file upload, check for positive file size
  if (@$_FILES['import_webgui']['size'] < 1)
   friendlyerror('HTTP upload failed; web-interface not updated!', $url);
  // set import file
  $importfile = $_FILES['import_webgui']['tmp_name'];
 }
 else
 {
  // fetch remote version file
  $remote = guru_readremotefile($guru['remote_version_url']);

  // bail out if not able to fetch
  if (!$remote)
   friendlyerror('Could not fetch remote version file. '
    .'Check internet connection.', $url);
  // bail out if remote configuration file version differs from the script
  if (@$remote['version'] != $guru['remote_update_version'])
   error('Version mismatch -- your version is too old update from website');

  // fetch remote url
  $remotever = trim($remote[$branch]);
  $importfile = $guru['tempdir'].'webinterface-update-'.$remotever.'.tgz';
  @exec('/usr/bin/fetch -o '.$importfile.' '.$remote[$branch.'archive']);
  if (!is_readable($importfile))
   error('Downloading of "'.$remote[$branch.'archive'].'" failed');

  // filesize check
  if (filesize($importfile) != trim($remote[$branch.'filesize']))
   error('Remote file size does not match remote version file; aborting!');
  // hash check
  if (md5_file($importfile) != trim($remote[$branch.'md5hash']))
   error('Remote file did not conform to known MD5 hash; aborting!');
  if (sha1_file($importfile) != trim($remote[$branch.'sha1hash']))
   error('Remote file did not conform to known SHA1 hash; aborting!');
 }

 // craft our command string
 $command = '/usr/bin/tar xvfz '.$importfile;
 // and execute!
 $result = super_execute($command);
 // sleep just in case
 sleep(1);
 $rv = @key($result);
 if (@$rv == 0)
  redirect_url($url2);
 else
  error('Got return value '.(int)$rv.' while trying to extract scripts.');
}

?>
