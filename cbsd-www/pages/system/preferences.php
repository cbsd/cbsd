<?php

function content_system_preferences()
{
 global $guru;

 // required libraries
 activate_library('guru');

 // tabbar
 $tabbar = array(
  'main' => 'Main preferences',
  'access' => 'Access control',
  'usability' => 'Usability',
 );
 $url = 'system.php?pref';

 // select tab
 $class_tab_main = 'hidden';
 $class_tab_access = 'hidden';
 $class_tab_usability = 'hidden';
 if (@isset($_GET['access']))
 {
  $tab = '&access';
  $tabname = $tabbar['access'];
  $class_tab_access = 'normal';
 }
 elseif (@isset($_GET['usability']))
 {
  $tab = '&usability';
  $tabname = $tabbar['usability'];
  $class_tab_usability = 'normal';
 }
 else
 {
  $tab = '';
  $tabname = $tabbar['main'];
  $class_tab_main = 'normal';
 }

 // fetch preferences
 $pref = $guru['preferences'];

 // main preferences
 $class_activated = (@strlen($pref['uuid']) > 0) ? 'normal' : 'hidden';
 $class_notactivated = (@strlen($pref['uuid']) > 0) ? 'hidden' : 'normal';
 $cb_advancedmode = ($pref['advanced_mode']) ? 'checked="checked"' : '';
 $cb_offlinemode = ($pref['offline_mode']) ? 'checked="checked"' : '';
 // timezones
 $timezones = '';
 $tz = fetch_timezones();
 foreach ($tz as $timezone)
  if ($timezone == $pref['timezone'])
   $timezones .= '   <option value="'.htmlentities($timezone).'" '
    .'selected="selected">'.htmlentities($timezone).'</option>'.chr(10);
  else
   $timezones .= '   <option value="'.htmlentities($timezone).'">'
    .htmlentities($timezone).'</option>'.chr(10);
 $system_time = `date`;
 $php_time = date('D M j H:i:s e Y');
 // master servers (note that we hide the trailing forward slash)
 $serverlist = '';
 if (is_array($guru['master_servers']))
  foreach ($guru['master_servers'] as $country => $server)
   if ($server == $pref['preferred_server'])
    $serverlist .= '   <option value="'.htmlentities($server).'"'
     .' selected="selected">'
     .htmlentities($country.' - '.substr($server, 0, -1)).'</option>'.chr(10);
   else
    $serverlist .= '   <option value="'.htmlentities($server).'">'
     .htmlentities($country.' - '.substr($server, 0, -1)).'</option>'.chr(10);
 // timeouts
 $timeouts = array();
 $timeout_choices = array(2, 3, 4, 5, 7, 10, 15, 20, 25, 30);
 foreach ($timeout_choices as $timeout)
  $timeouts[] = array(
   'TIMEOUT_ACTIVE'	=> ($pref['connect_timeout'] == $timeout) 
			? 'selected="selected"' : '',
   'TIMEOUT_SEC' => $timeout
  );
 $torrent_download = ($pref['download_method'] === 'torrent') ? 
  'selected="selected"' : '';

 // access control
 if (!@isset($pref['access_control']))
  $pref['access_control'] = 2;
 $radio_ac_1 = ($pref['access_control'] == 1) ? 'checked="checked"' : '';
 $radio_ac_2 = ($pref['access_control'] == 2) ? 'checked="checked"' : '';
 $radio_ac_3 = ($pref['access_control'] == 3) ? 'checked="checked"' : '';
 $class_auth_set = (@strlen($pref['authentication']) > 0) ? 'normal' : 'hidden';
 $class_auth_unset = (@strlen($pref['authentication']) > 0) ? 'hidden' : 'normal';
 $whitelist = '';
 if (@is_array($pref['access_whitelist']))
  $whitelist = implode(', ', $pref['access_whitelist']);

 // usability
 $cb_commandconfirm = ($pref['command_confirm']) ?
  'checked="checked"' : '';
 $cb_destroypools = ($pref['destroy_pools']) ?
  'checked="checked"' : '';
 // visual themes
 $themelist = array();
 exec('/bin/ls -1 '.$guru['docroot'].'/theme/', $output);
 if (is_array($output))
  foreach ($output as $dir)
   if ($dir != 'default')
    if (is_dir($guru['docroot'].'/theme/'.$dir))
     $themelist[] = array(
      'THEME_ACTIVE'	=> ($dir == $guru['preferences']['theme'])
			? 'selected="selected"' : '',
      'THEME_DIR'	=> htmlentities($dir),
      'THEME_NAME'	=> htmlentities(ucfirst($dir))
     );

 // export new tags
 return array(
  'PAGE_ACTIVETAB'		=> 'Preferences',
  'PAGE_TITLE'			=> 'Preferences',
  'PAGE_TABBAR'			=> $tabbar,
  'PAGE_TABBAR_URL'		=> $url,
  'TABLE_THEMES'		=> $themelist,
  'TABLE_CONNECTION_TIMEOUT'	=> $timeouts,
  'CLASS_TAB_MAIN'		=> $class_tab_main,
  'CLASS_TAB_ACCESS'		=> $class_tab_access,
  'CLASS_TAB_USABILITY'		=> $class_tab_usability,
  'CLASS_ACTIVATED'		=> $class_activated,
  'CLASS_NOTACTIVATED'		=> $class_notactivated,
  'CLASS_AUTH_SET'		=> $class_auth_set,
  'CLASS_AUTH_UNSET'		=> $class_auth_unset,
  'PREF_TAB'			=> $tab,
  'PREF_TABNAME'		=> $tabname,
  'PREF_ACTIVATION_UUID'	=> @$pref['uuid'],
  'PREF_SERVERS'		=> $serverlist,
  'PREF_ADVANCED_MODE'		=> $cb_advancedmode,
  'PREF_OFFLINE_MODE'		=> $cb_offlinemode,
  'PREF_TIMEZONES'		=> $timezones,
  'PREF_SYSTEM_TIME'		=> $system_time,
  'PREF_PHP_TIME'		=> $php_time,
  'PREF_TORRENT_DOWNLOAD'	=> $torrent_download,
  'RADIO_AC_1'			=> $radio_ac_1,
  'RADIO_AC_2'			=> $radio_ac_2,
  'RADIO_AC_3'			=> $radio_ac_3,
  'PREF_ACCESS_WHITELIST'	=> $whitelist,
  'PREF_COMMAND_CONFIRM'	=> $cb_commandconfirm,
  'PREF_DESTROY_POOLS'		=> $cb_destroypools
 );
}

function fetch_timezones()
// fetches and returns all usable timezones
{
 // fetch PHP known timezones
 $tz_php_raw = fetchphptimezones();
 $tz_php = array();
 foreach ($tz_php_raw as $city => $tzdat)
  $tz_php[] = $city;

 // fetch system known timezones
 $tz_system = array();
 exec('/usr/bin/find /usr/share/zoneinfo/ -type f', $rawoutput, $rv);
 $predir_length = strlen('/usr/share/zoneinfo/');
 if (@is_array($rawoutput))
  foreach ($rawoutput as $directory)
   if (substr($directory, 0, $predir_length) == '/usr/share/zoneinfo/')
    if (strlen($directory) > $predir_length)
     $tz_system[] = substr($directory, $predir_length);

 // combine
 $tz_combine = (array)extra_timezone_list();
 foreach ($tz_php as $city)
  if (!in_array($city, $tz_combine))
   if (strlen($city) > 0)
    if (file_exists('/usr/share/zoneinfo/'.$city))
     $tz_combine[] = $city;
 asort($tz_combine);

 // begin timezone array
 $timezones = array();

 // start with general timezones
 foreach ($tz_combine as $tz)
  if (strpos($tz, '/') === false)
   $timezones[] = $tz;

 // add other timezones
 foreach ($tz_combine as $tz)
  if (!in_array($tz, $timezones))
   $timezones[] = $tz;

 // return array
 return $timezones;
}

function extra_timezone_list()
{
 return array(
  'CET',
  'EST',
  'EET',
  'GMT',
  'HST',
  'MST',
  'WET',
  'Australia/ACT',
  'Australia/Brisbane',
  'Australia/Currie',
  'Australia/Darwin',
  'Australia/Hobart',
  'Australia/Lindeman',
  'Australia/Lord_Howe',
  'Australia/Melbourne',
  'Australia/Sydney'
 );
}

function fetchphptimezones()
// returns sanitized and sorted list of all timezones known by PHP
{
 $timezones = DateTimeZone::listAbbreviations();
 $cities = array();
 foreach ($timezones as $key => $zones)
  foreach ($zones as $id => $zone)
   $cities[$zone['timezone_id']][] = $key;
 foreach ($cities as $key => $value)
  $cities[$key] = implode(', ', $value);
 $cities = array_unique($cities);
 ksort($cities);
 return $cities;
}

function submit_system_preferences()
{
 global $guru;

 // fetch default preferences
 if (!is_array($guru['default_preferences']))
  error('HARD ERROR: no default preferences this should not happen!');
 foreach ($guru['default_preferences'] as $var => $value)
  if (@isset($guru['preferences'][$var]))
   $pref[$var] = $guru['preferences'][$var];
  else
   $pref[$var] = $guru['default_preferences'][$var];

 if (@isset($_POST['change_pref']))
 {
  // boolean (true/false) values first
  foreach ($pref as $name => $value)
   if (is_bool($guru['default_preferences'][$name]))
    $pref[$name] =
     (@$_POST['pref_'.$name]) ? true : false;

  // preferred master server
  if (@isset($_POST['pref_preferred_server']))
   $pref['preferred_server'] = $_POST['pref_preferred_server'];

  // connection timeout
  if (@is_numeric($_POST['pref_connect_timeout']))
   if ((int)$_POST['pref_connect_timeout'] > 0)
    $pref['connect_timeout'] = (int)$_POST['pref_connect_timeout'];

  // timezone
  if (@isset($_POST['pref_timezone']))
  {
   $pref['timezone'] = $_POST['pref_timezone'];
   // activate timezone for FreeBSD system as well if applicable
   $tz_map = procedure_timezone_map($pref['timezone']);
   if (strlen($tz_map['tz_system']) > 0)
    if (file_exists('/usr/share/zoneinfo/'.$tz_map['tz_system']))
    {
     // increased privileges
     activate_library('super');
     $cmd = '/bin/cp /usr/share/zoneinfo/'.$tz_map['tz_system']
      .' /etc/localtime';
     $result = super_execute($cmd);
      if ($result['rv'] !== 0)
      page_feedback('could not activate timezone', 'a_failure');
    }
  }

  // preferred download method
  if (@isset($_POST['pref_download_method']))
   $pref['download_method'] = $_POST['pref_download_method'];

  // access control
  if (@isset($_POST['pref_access_control']))
   if (((int)$_POST['pref_access_control'] == 1) OR
       ((int)$_POST['pref_access_control'] == 2) OR
       ((int)$_POST['pref_access_control'] == 3))
    $pref['access_control'] = (int)$_POST['pref_access_control'];

  // whitelist
  if (@isset($_POST['pref_access_whitelist']))
  {
   $whitelist_arr = explode(',', $_POST['pref_access_whitelist']);
   $pref['access_whitelist'] = array();
   foreach ($whitelist_arr as $ipaddr)
    $pref['access_whitelist'][trim($ipaddr)] = trim($ipaddr);
  }

  // authentication
  if (@strlen($_POST['pref_authentication']) > 0)
   if ($_POST['pref_authentication'] == @$_POST['pref_authentication2'])
    $pref['authentication'] = $_POST['pref_authentication'];
   else
    page_feedback('the password you entered needs to match the '
     .'verification field; please enter the same password twice!', 'a_warning');
  if (@$_POST['pref_reset_authentication'] == 'on')
   $pref['authentication'] = '';

  // visual theme
  if (@isset($_POST['pref_theme']))
   $pref['theme'] = $_POST['pref_theme'];

  // fool-proof authentication settings; do not allow bad auth settings
  $authcheck = procedure_authenticate($pref, true);
  if (!$authcheck)
   page_feedback('you have chosen settings which would deny you access to the '
    .'web-interface. To protect against mistakes, your changes have <b>not</b>'
    .' been saved!', 'a_warning');
  else
  {
   // save preferences
   if ((@is_array($pref)) and (@count($pref) > 0))
    $result = procedure_writepreferences($pref);
   else
    error('HARD ERROR: bad preferences data');
   if ($result === true)
    page_feedback('your preferences have been updated!', 'b_success');
   else
    page_feedback('error writing preferences!', 'a_error');
  }
 }
 elseif (@isset($_POST['reset_pref']))
 {
  // reset to default peferences
  $result = procedure_writepreferences($guru['default_preferences']);
  if ($result === true)
   page_feedback('preferences reset to default!', 'b_success');
  else
   page_feedback('error writing preferences!', 'a_error');
 }
 elseif (@isset($_POST['kill_pref']))
 {
  // remove preferences file (config.bin) so the welcome wizard is shown again
  // we need super privileges even if we have www write access to the file
  activate_library('super');
  $result = super_execute('/bin/rm '.$guru['configuration_file']);
  if ($result['rv'] === 0)
  {
   // save current preferences in $_SESSION to be preserved during welcome wiz
   $_SESSION['welcomewizard']['oldpreferences'] = $guru['preferences'];
   redirect_url('/');
  }
  else
   page_feedback('could not remove preferences file: '
    .$guru['configuration_file'], 'a_failure');
 }

 // redirect back to preferences page
 $tab = '';
 if (@isset($_GET['access']))
  $tab = '&access';
 elseif (@isset($_GET['usability']))
  $tab = '&usability';
 redirect_url('system.php?pref'.$tab);
}

?>
