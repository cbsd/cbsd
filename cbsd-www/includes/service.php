<?php

/* new service code */

function service_list()
// returns an array with currently installed services
{
 global $guru;

 // check for cached list
 if (@is_array($guru['cache']['servicelist']))
  return $guru['cache']['servicelist'];

 // check if services directory exists
 if (!is_dir($guru['path_services']))
  service_initialsetup();

 // fetch service database
 $db = service_db();

 // assemble and return services array
 $services = array();
 exec('/bin/ls -1 '.$guru['path_services'], $result, $rv);
 if ($rv != 0)
  page_feedback('got return value '.$rv.' on service scan.', 'a_error');
 foreach ($result as $line)
 {
  // service path
  $spath = $guru['path_services'].'/'.trim($line);

  // name
  $name = trim($line);
  $longname = (@$db['services'][$name]['longname']) 
   ? @$db['services'][$name]['longname'] : htmlentities($name);

  // description
  $desc = @$db['services'][$name]['desc'];

  // derive CAT from service database
  $cat = @$db['services'][$name]['cat'];
  if (strlen($cat) < 1)
   $cat = '???';

  // required: VERSION file
  $version = @trim(file_get_contents($spath.'/VERSION'));
  // skip if no VERSION file present
  if (@strlen($version) < 1)
   continue;
  // split version into two components; extension version and product version
  $pos = strpos($version, '-');
  if ($pos !== false)
  {
   $ver_ext = substr($version, 0, $pos);
   $ver_prod = substr($version, $pos+1);
  }
  else
  {
   $ver_ext = $version;
   $ver_prod = $version;
  }

  // expected: SYSVER
  $sysver = @trim(file_get_contents($spath.'/SYSVER'));

  // expected: PLATFORM
  $platform = @trim(file_get_contents($spath.'/PLATFORM'));

  // expected: LICENSE
  $license = @trim(file_get_contents($spath.'/LICENSE'));

  /* optional components */

  // DEPEND
  $depend = @trim(file_get_contents($spath.'/DEPEND'));

  // JAILED
  $jailed = (@trim(file_get_contents($spath.'/JAILED')) == '1') ? true : false;

  // CHROOTED
  $chrooted = (@trim(file_get_contents($spath.'/CHROOTED')) == '1') 
   ? true : false;

  // security model
  if ($jailed)
   $security = 'jail';
  elseif ($chrooted)
   $security = 'chroot';
  else
   $security = 'none';

  // can be started
  $canstart = (@fileowner($spath.'/service_start.sh') === 0) ? true : false;

  // can be stopped
  $canstop = (@fileowner($spath.'/service_stop.sh') === 0) ? true : false;

  // check if passive
  $passive = (@trim(file_get_contents($spath.'/PASSIVE')) == '1') 
   ? true : false;

  // is running (could be extended in future?)
  if ($passive)
   $status = 'passive';
  else
  {
   $status = 'unknown';
   $processnames = @trim(file_get_contents($spath.'/PROCESSNAMES'));
   if (strlen($processnames) > 0)
   {
    $procarr = explode(chr(10), $processnames);
    if (is_array($procarr))
     foreach ($procarr as $process)
      if (strlen($process) > 0)
       if (service_isprocessrunning($process))
        $status = 'running';
       elseif ($status == 'running')
        $status = 'partial';
       else
        $status = 'stopped';
   }
  }

  // size (du -sk reports in units of 1K so multiply 1024)
  $size = `/usr/bin/du -sk ${spath}`;
  $size = (int)@substr($size, 0, strpos($size, '	'));
  $size = $size * 1024;

  // panel path (at least .php file has to exist)
  $panelpath = $spath.'/panel/'.$name;
  if (@is_file($panelpath.'.php'))
   $path_panel = $panelpath;
  else
   $path_panel = false;

  // add to services array
  $services[$name] = array(
   'name'	=> $name,
   'shortname'	=> $name,
   'longname'	=> $longname,
   'cat'	=> $cat,
   'version'	=> $version,
   'ver_ext'	=> $ver_ext,
   'ver_prod'	=> $ver_prod,
   'sysver'	=> $sysver,
   'platform'	=> $platform,
   'license'	=> $license,
//   'desc'	=> $desc,
   'depend'	=> $depend,
   'jailed'	=> $jailed,
   'chrooted'	=> $chrooted,
   'security'	=> $security,
   'can_start'	=> $canstart,
   'can_stop'	=> $canstop,
   'status'	=> $status,
   'size'	=> $size,
   'path_panel'	=> $path_panel
  );
 }

 // cache list in guru variable and return value
 $guru['cache']['servicelist'] = $services;
 return $services;
}

function service_runstatus($svc)
// returns string: running, partial, stopped, unknown, passive (false on error)
{
 $slist = service_list();
 if (strlen(@$slist[$svc]['status']) > 0)
  return $slist[$svc]['status'];
 else
  return false;
}

function service_start($svc, $silent = false)
// start service
{
 // check if already running
 $status = service_runstatus($svc);
 if (($status == 'running') OR ($status == 'partial'))
  if ($silent)
   return true;
  else
  {
   page_feedback('service <b>'.$svc.'</b> is already started!', 'a_warning');
   return true;
  }
 // start service by script
 $result = service_script($svc, 'start');
 // sleep to allow proper detection on next pageview
 sleep(1);
 // return result
 if ($result)
  return true;
 else
  return false;
}

function service_stop($svc, $silent = false)
// stop service
{
 // check if already stopped
 $status = service_runstatus($svc);
 if ($status == 'stopped')
  if ($silent)
   return true;
  else
  {
   page_feedback('service <b>'.$svc.'</b> is already stopped!', 'a_warning');
   return true;
  }
 // stop service by script
 $result = service_script($svc, 'stop');
 // sleep to allow proper detection on next pageview
 sleep(2);
 // return result
 if ($result)
  return true;
 else
  return false;
}

function service_purge($svc)
// purges services from all optional data files (clean up space)
{
 $result = service_script($svc, 'purge');
 if ($result)
  return true;
 else
  return false;
}

function service_uninstall($svc)
// uninstalls specified service
{
 global $guru;

 // elevated privileges
 activate_library('super');

 // stop service
 service_script($svc, 'stop');

 // check if directory exists
 $spath = $guru['path_services'].'/'.trim($svc);
 if (!is_dir($spath))
 {
  page_feedback('cannot uninstall service <b>'.$svc.'</b> - '
   .'directory does not exist.', 'a_failure');
  return false;
 }

 // call uninstall script
 $result = service_script($svc, 'uninstall');

 // remove VERSION file from directory
 $result2 = super_execute('/bin/rm -f '.$spath.'/VERSION');
 if ($result2['rv'] == 0)
  return true;
 else
  return false;
}

function service_install($svc)
// installs specified service
{
 global $guru;

 // required libraries
 activate_library('guru');
 activate_library('super');
 activate_library('torrent');

 // call functions
 $slist = service_list();
 $db = service_db();
 $curver = guru_fetch_current_systemversion();
 $platform = guru_getsystemplatform();

 // system image
 $sysimg_str = @$curver['sysver'];
 $sysimg = @$db['services'][$svc]['sysimg'][$sysimg_str][$platform];

 // service package name
 $service_pkg_name = @$sysimg['filename'];

 // sanity
 if (@isset($slist[$svc]))
  error('cannot install service '.$svc.' - service already installed!');
 if (@strlen($service_pkg_name) < 1)
  error('cannot install service '.$svc.' - not available in service database');

 // location of service package file
 $loc = false;
 // search for torrent
 $loc_torrent = guru_locate_download($service_pkg_name, 
  $guru['torrent']['path_finished']);
 // search tempdir
 $loc_http = guru_locate_download($service_pkg_name, $guru['tempdir']);
 // determine location
 if (@file_exists($loc_torrent))
  $loc = $loc_torrent;
 elseif (@file_exists($loc_http))
  $loc = $loc_http;

 // checksum (size+MD5+SHA1)
 if (@$sysimg['size'] < 1)
  error('incorrect size in remote service database for this service!');
 if (filesize($loc) != $sysimg['size'])
  error('downloaded file size ('.filesize($loc).') does not match expected '
   .'size ('.$sysimg['size'].').');
 if (md5_file($loc) != @$sysimg['md5'])
  error('downloaded file fails MD5 checksum, aborting!');
 if (sha1_file($loc) != @$sysimg['sha1'])
  error('downloaded file fails SHA1 checksum, aborting!');

 // install service
 if (@file_exists($loc))
 {
  // create directory for service (as root)
  $sroot = $guru['path_services'].'/'.$svc;
  super_execute('/bin/mkdir '.$sroot);
  // extract tarball to services directory (as root)
  $result = super_execute('/usr/bin/tar x -C '.$sroot.'/ -f '.$loc);
  // notify user of result
  if ($result['rv'] != 0)
   page_feedback('could not extract to '.$sroot.'!', 'a_failure');
  else
  {
   // call install script
   $installscript = $sroot.'/service_install.sh';
   if (@file_exists($installscript))
    $result2 = super_execute($installscript);
   // notify of result
   if ($result2['rv'] == 1)
    page_feedback('installation script failed for service '
     .'<b>'.$svc.'</b>', 'a_failure');
   elseif ($result2['rv'] == 2)
    page_feedback('service <b>'.$svc.'</b> installed to '
     .'<b>'.$sroot.'</b>', 'b_success');
   elseif ($result2['rv'] == 3)
    page_feedback('service <b>'.$svc.'</b> installed to '
     .'<b>'.$sroot.'</b> - requires a <u>reboot</u> before operation!',
     'b_success');
   else
    page_feedback('installation script invalid rv - aborting!', 'a_failure');
  }
 }
 else
  error('cannot install service '.$svc.' - could not find package file');
}

function service_script($svc, $script, $arg = '')
// calls service script
{
 global $guru;

 // elevated privileges
 activate_library('super');

 // path to service script
 $script = $guru['path_services'].'/'.$svc.'/service_'.$script.'.sh';
 if (!file_exists($script))
 {
  return false;
 }
 if ($arg != '')
  $result = super_execute('/bin/sh '.$script.' '.$arg);
 else
  $result = super_execute('/bin/sh '.$script);
 if ($result['rv'] == 0)
  return true;
 else
  return false;
}

function service_initialsetup()
// creates services directory
{
 global $guru;

 // elevated privileges
 activate_library('super');
 $result = super_execute('/bin/mkdir '.$guru['path_services']);

 // redirect and set session notice or warning
 if ($result['rv'] == 0)
  page_feedback('a new services directory has been created at: '
   .'<b>'.$guru['path_services'].'</b>.', 'c_notice');
 else
  page_feedback('failed to created services directory at: '
   .'<b>'.$guru['path_services'].'</b>.', 'a_failure');
}

/* service database */

function service_db_select($svc)
// returns service data from database
{
 return 'cat';
}

function service_db()
// returns entire service database
{
 global $guru;

 // check powercache
 $pcache = powercache_read('service_db');
 if ($pcache !== false)
  return $pcache;

 // fetch remote file
 $remotefile = remoteserver($guru['remote_services_url']);
 // unserialize
 if (@is_string($remotefile))
  $remotearr = unserialize($remotefile);

 if (@is_array($remotearr))
 {
  // verify ident
  if ($remotearr['ident'] != 'ZFSGURU::SERVICES')
   error('invalid identification on services database - update web interface');
  // verify version
  if ($remotearr['version'] != 1)
   error('invalid version of services database - upgrade web interface!');
  // store in powercache (3600 = 1 hour expiry)
  powercache_store('service_db', $remotearr, 3600);
  return $remotearr;
 }
 else
 {
  // try to use an oudated cached copy
  $ocache = powercache_read('service_db', true);
  if ($ocache !== false)
  {
   page_feedback('using an outdated local copy of the '
    .'service database.', 'c_notice');
   return $ocache;
  }
  else
   return false;
 }
}

/* service panels */

function service_panels()
// returns array with services with web-interface panel
{
 // grab services list
 $services = service_list();

 // traverse services for panels (require .php file)
 $panels = array();
 foreach ($services as $servicename => $data)
  if (@is_readable($data['path_panel'].'.php'))
   $panels[$data['cat']][$servicename] = $data;
 return $panels;
}

function service_panel_handle($svc)
{
 global $tabs;

 // grab services list
 $services = service_list();
 // determine panel path
 $panelpath = @$services[$svc]['path_panel'];
 // determine longname
 $longname = @$services[$svc]['longname'];
 if (strlen($longname) < 1)
  $longname = htmlentities($svc);
 // process panel path
 if (@is_file($panelpath.'.php'))
 {
  // create new tab for panel
  $tabs[$longname] = 'services.php?panel='.$svc;
  // activate the new tab
  page_injecttag(array('PAGE_ACTIVETAB' => $longname));
  // process panel
  $content = content_handle_path($panelpath, 'panel', $svc);
  // page handle
  page_handle($content);
  die();
 }
 elseif ($panelpath == false)
  error('Service '.$svc.' does not have a panel file!');
 else
  error('Panel file does not exist at: '.$panelpath);
 // unhandled termination
 error('unhandled termination of panel '.$svc);
}

/* old service code */

function service_isprocessrunning($process_name)
// checks whether given process is running and returns boolean
{
 $cmd = 'ps auxw | grep "'.$process_name.'" | grep -v grep';
 $process = trim(`$cmd`);
 if (@strlen($process) > 0)
  return true;
 else
  return false;
}

function service_manage_rc($service, $action, $ignore_errors = false)
// low-level function that allows tuning the action to be performed
{
 global $guru;

 // elevated privileges
 activate_library('super');

 if (@isset($guru['rc.d'][$service]))
  $result = super_execute($guru['rc.d'][$service].' '.$action.' 2>&1');
 else
  return false;

 if (($result['rv'] != 0) AND (!$ignore_errors))
  error('Got return value '.(int)$result['rv'].' when trying to '.$action
   .' service "'.$service.'" with output:<br />'.$result['output_str']);
 else
  return true;
}

function service_start_rc($service)
// starts specified service via rc.d script
{
 global $guru;
 if (service_runcontrol_isenabled($guru['runcontrol'][$service]))
  $result = service_manage($service, 'start');
 else
  $result = service_manage($service, 'onestart');
 return $result;
}

function service_restart_rc($service)
// restarts specified service via rc.d script
{
 $result = service_manage($service, 'restart');
 return $result;
}

function service_stop_rc($service)
// stops specified service via rc.d script
{
 global $guru;
 if (service_runcontrol_isenabled($guru['runcontrol'][$service]))
  $result = service_manage($service, 'stop');
 else
  $result = service_manage($service, 'onestop');
 return $result;
}

/* run control */

function service_runcontrol_isenabled($rc, $rcconf = '')
// returns true if $rc is enabled in rc.conf configuration; false if otherwise
{
 $preg = '/^[\s]*'.$rc.'\_enable[\s]*\="?([^\"]*)"?[\s]*$/m';
 if (strlen($rcconf) < 1)
  $rcconf = file_get_contents('/etc/rc.conf');
 // look for non-commented out $rc line
 if (preg_match($preg, $rcconf, $matches))
 {
  if (@strtoupper($matches[1]) == 'YES')
   return true;
  elseif (@strtoupper($matches[1]) == 'NO')
   return false;
  else
   return (string)@$matches[1];
 }
 else
  return 0;
}

function service_runcontrol_enable($rc)
// enables rc in rc.conf configuration
{
 global $guru;

 // elevated privileges
 activate_library('super');

 // regexp
 $preg1 = '/^[\s]*'.$rc.'\_enable[\s]*\=.*$/m';
 $preg2 = '/^[\s]*\#'.$rc.'\_enable[\s]*\=.*$/m';
 // read configuration
 $rcconf = file_get_contents('/etc/rc.conf');
 // look for non-commented out $rc line (already enabled)
 $enabled = service_runcontrol_isenabled($rc, $rcconf);
 if ($enabled === true)
  return true;
 if (($enabled === false) OR is_string($enabled))
  // appears the rc variable exists but has other value than YES/NO
  $rcconf = preg_replace($preg1, $rc.'_enable="YES"', $rcconf, 1);
 // look for commented out $rc line in $rcconf
 elseif (preg_match($preg2, $rcconf, $matches))
  // replace commented out line with non-commented version
  $rcconf = preg_replace($preg2, $rc.'_enable="YES"', $rcconf, 1);
 else
  // append rc variable to end of file
  $rcconf .= chr(10).$rc.'_enable="YES"'.chr(10);
 // save to disk
 file_put_contents($guru['tempdir'].'/newrc.conf', $rcconf);
 super_execute('/bin/mv '.$guru['tempdir'].'/newrc.conf /etc/rc.conf');
 super_execute('/usr/sbin/chown root:wheel /etc/rc.conf');
 super_execute('/bin/chmod 644 /etc/rc.conf');
}

function service_runcontrol_disable($rc)
// disables rc in rc.conf configuration
{
 global $guru;

 // elevated privileges
 activate_library('super');

 // read configuration
 $rcconf = file_get_contents('/etc/rc.conf');
 $preg = '/^[\s]*'.$rc.'\_enable[\s]*\=.*$/m';
 // replace active rc variable
 $rcconf = preg_replace($preg, $rc.'_enable="NO"', $rcconf, 1);
 // save to disk
 file_put_contents($guru['tempdir'].'/newrc.conf', $rcconf);
 super_execute('/bin/mv '.$guru['tempdir'].'/newrc.conf /etc/rc.conf');
 super_execute('/usr/sbin/chown root:wheel /etc/rc.conf');
 super_execute('/bin/chmod 644 /etc/rc.conf');
}

?>
