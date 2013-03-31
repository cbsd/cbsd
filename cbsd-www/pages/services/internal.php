<?php

function fetch_internal_services()
{
 global $guru;

 // determine webserver in use; lighttpd or apache
 if (stripos($_SERVER["SERVER_SOFTWARE"], 'lighttpd') !== false)
  $websrv = array(
   'longname' => 'Lighttpd',
   'process' => 'lighttpd',
   'bg_script' => $guru['docroot'].'/scripts/restart_lighttpd.sh'
  );
 elseif (stripos($_SERVER["SERVER_SOFTWARE"], 'apache') !== false)
  $websrv = array(
   'longname' => 'Apache',
   'process' => 'httpd',
   'bg_script' => $guru['docroot'].'/scripts/restart_apache.sh'
  );
 else
  $websrv = array(
   'longname' => 'unknown webserver',
   'process' => ''
  );

 // return internal services array
 return array(
  'webserver' => array(
   'longname' => $websrv['longname'],
   'desc' => 'Webserver used for ZFSguru',
   'process' => $websrv['process'],
   'script' => '',
   'bg_script' => $websrv['bg_script'],
   'only_restart' => true
   ),
  'cron' => array(
   'longname' => 'cron daemon',
   'desc' => 'Task scheduler',
   'process' => 'cron',
   'script' => '/etc/rc.d/cron'
   ),
  'moused' => array(
   'longname' => 'mouse daemon',
   'desc' => 'Provides mouse support on the monitor',
   'process' => 'moused',
   'script' => '/etc/rc.d/moused'
   ),
  'named' => array(
   'longname' => 'name daemon',
   'desc' => 'DNS internet name server',
   'process' => 'named',
   'script' => '/etc/rc.d/named'
   ),
  'nfs' => array(
   'longname' => 'NFS daemon',
   'desc' => 'NFS file sharing servers',
   'process' => 'nfsd',
   'script' => '/etc/rc.d/nfsd'
   ),
  'ntpd' => array(
   'longname' => 'NTP daemon',
   'desc' => 'Network date/time synchronization server',
   'process' => 'ntpd',
   'script' => '/etc/rc.d/ntpd'
   ),
  'openssh' => array(
   'longname' => 'OpenSSH',
   'desc' => 'SSH remote login server',
   'process' => 'sshd',
   'script' => '/etc/rc.d/sshd'
   ),
  'powerd' => array(
   'longname' => 'Power daemon',
   'desc' => 'CPU power (throttle) service',
   'process' => 'powerd',
   'script' => '/etc/rc.d/powerd'
   ),
  'pf' => array(
   'longname' => 'Package Filter',
   'desc' => 'Packet firewall kernel module',
   'process' => '',
   'script' => '/etc/rc.d/pf',
   'func_isrunning' => 'isrunning_pf'
   ),
  'samba' => array(
   'longname' => 'Samba',
   'desc' => 'Windows-native filesharing service',
   'process' => 'smbd',
   'script' => '/usr/local/etc/rc.d/samba'
   ),
  'sendmail' => array(
   'longname' => 'Sendmail',
   'desc' => 'SMTP email server',
   'process' => 'sendmail',
   'script' => '/etc/rc.d/sendmail'
   )
  );
}

function isrunning_pf()
// special function for pf firewall; returns true if running, false if not
{
 // super privileges
 activate_library('super');

 // gather info
 $result = super_execute('/sbin/pfctl -s info');

 if (preg_match('/^Status\: Enabled/m', $result['output_str']))
  return true;
 elseif (preg_match('/^Status\: Disabled/m', $result['output_str']))
  return false;
 else
 {
  page_feedback('could not determine pf firewall status', 'a_warning');
  return false;
 }
}

function content_services_internal()
{
 global $guru;

 // required library
 activate_library('service');

 // fetch internal services
 $iservices = fetch_internal_services();

 // queried service
 $query = @$_GET['query'];

 // servicelist table
 $iservicelist = array();
 foreach ($iservices as $iservice => $data)
 {
  $activerow = ($query == $iservice) ? 'activerow' : 'normal';

  // determine running status
  if ((strlen(@$data['func_isrunning']) > 0) AND (function_exists(@$data['func_isrunning'])))
  {
   $running = call_user_func($data['func_isrunning']);
   if ($running)
   {
    $status = 'RUNNING';
    $class_status = 'green';
   }
   else
   {
    $status = 'STOPPED';
    $class_status = 'red';
   }
  }
  elseif (strlen($data['process']) < 1)
  {
   $status = 'PASSIVE';
   $class_status = 'grey';
  }
  elseif (@$data['isrunning'] == 'unknown')
  {
   $status = 'Unknown';
   $class_status = 'grey';
  }
  elseif (service_isprocessrunning($data['process']))
  {
   $status = 'RUNNING';
   $class_status = 'green';
  }
  else
  {
   $status = 'STOPPED';
   $class_status = 'red';
  }

  // classes
  $class_startbutton = @(($status == 'STOPPED') AND (!$data['only_restart'])) 
   ? 'normal' : 'hidden';
  $class_stopbutton = @(($status == 'RUNNING') AND (!$data['only_restart'])) 
   ? 'normal' : 'hidden';
  $class_restartbutton = @($data['only_restart'] == true) ? 'normal' : 'hidden';

  // autostart
  $autostart = '-';

  // check for internal service settings
  $minipanel_path = $guru['docroot'].'/pages/services/minipanel_'.$iservice.'.page';
  $linkname = (@file_exists($minipanel_path)) ? $data['longname'] : '';
  $longname = (strlen($linkname) > 0) ? '' : $data['longname'];

  // add row to servicelist table
  $iservicelist[] = @array(
   'CLASS_ACTIVEROW'	=> $activerow,
   'SERVICE_NAME'	=> htmlentities($iservice),
   'SERVICE_LONGNAME'	=> $longname,
   'SERVICE_LINKNAME'	=> $linkname,
   'SERVICE_PROCESS'	=> $data['process'],
   'SERVICE_DESC'	=> htmlentities($data['desc']),
   'CLASS_STATUS'	=> $class_status,
   'SERVICE_STATUS'	=> $status,
   'CLASS_STOPBUTTON'	=> $class_stopbutton,
   'CLASS_STARTBUTTON'	=> $class_startbutton,
   'CLASS_RESTARTBUTTON' => $class_restartbutton,
   'SERVICE_AUTOSTART'	=> $autostart
  );
 }

 // hide noservices div when services are present
 $class_services = (@empty($iservices) OR strlen($query) > 0) ? 'hidden' : 'normal';
 $class_noservices = (@empty($iservices)) ? 'normal' : 'hidden';
 $class_qservice = (strlen($query) > 0) ? 'normal' : 'hidden';

 // queried minipanel
 // SECURITY: TODO - normalize $_GET input
 if (file_exists($guru['docroot'].'/pages/services/minipanel_'.$query.'.page'))
  $minipanel = content_handle('services', 'minipanel_'.$query);
 else
  $minipanel = 'No panel found';

 // export new tags
 return array(
  'PAGE_ACTIVETAB'	=> 'Internal',
  'PAGE_TITLE'		=> 'Internal services',
  'TABLE_SERVICELIST'	=> @$iservicelist,
  'QSERVICE'		=> $query,
  'QSERVICE_LONG'	=> @$iservices[$query]['longname'],
  'MINIPANEL'		=> $minipanel,
  'CLASS_SERVICES'	=> $class_services,
  'CLASS_NOSERVICES'	=> $class_noservices,
  'CLASS_QSERVICE'	=> $class_qservice
 );
}

function submit_services_internal()
{
 global $guru;

 // super privileges
 activate_library('super');

 // redirect url
 $url = 'services.php?internal';

 // fetch internal services
 $iservices = fetch_internal_services();

 // scan each POST variable
 foreach ($_POST as $name => $value)
 {

  if (substr($name, 0, strlen('svc_start_')) == 'svc_start_')
  {
   // start service
   $svc = trim(substr($name, strlen('svc_start_')));
   $lname = (@$iservices[$svc]['longname'])
    ? $iservices[$svc]['longname'] : $svc;
   if (@strlen($iservices[$svc]['script']) > 0)
   {
    $script = $iservices[$svc]['script'];
    $result = super_execute($script.' onestart');
    if (@$result['rv'] !== 0)
     friendlyerror('could not start '.htmlentities($lname).' service!', $url);
    else
     friendlynotice(htmlentities($lname).' service started!', $url);
   }
   else
    friendlyerror(htmlentities($lname).' service has no rc.d script!', $url);
  }

  if (substr($name, 0, strlen('svc_stop_')) == 'svc_stop_')
  {
   // stop service
   $svc = trim(substr($name, strlen('svc_stop_')));
   $lname = (@$iservices[$svc]['longname'])
    ? $iservices[$svc]['longname'] : $svc;
   if (@strlen($iservices[$svc]['script']) > 0)
   {
    $script = $iservices[$svc]['script'];
    $result = super_execute($script.' onestop');
    if (@$result['rv'] !== 0)
     friendlyerror('could not stop '.htmlentities($lname).' service!', $url);
    else
     friendlynotice(htmlentities($lname).' service stopped!', $url);
   }
   else
    friendlyerror(htmlentities($lname).' service has no rc.d script!', $url);
  }

  if (substr($name, 0, strlen('svc_restart_')) == 'svc_restart_')
  {
   // restart service
   $svc = trim(substr($name, strlen('svc_restart_')));
   $lname = (@$iservices[$svc]['longname'])
    ? $iservices[$svc]['longname'] : $svc;
   if (@strlen($iservices[$svc]['script']) > 0)
   {
    $script = $iservices[$svc]['script'];
    $result = super_execute($script.' restart');
    if (@$result['rv'] !== 0)
     friendlyerror('could not restart '.htmlentities($lname).' service!', $url);
    else
     friendlynotice(htmlentities($lname).' service restarted!', $url);
   }
   elseif (@strlen($iservices[$svc]['bg_script']) > 0)
   {
    // special script execution on background
    $script = $iservices[$svc]['bg_script'];
    $result = super_execute($script.' restart > /dev/null &');
    if (@$result['rv'] !== 0)
     friendlyerror('could not restart '.htmlentities($lname).' service!', $url);
    else
     friendlynotice('delayed restart of '.htmlentities($lname), $url);
   }
   else
    friendlyerror(htmlentities($lname).' service has no rc.d script!', $url);
  }

 }
}

?>
