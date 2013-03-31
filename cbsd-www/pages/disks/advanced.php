<?php

function content_disks_advanced()
{
 // required library
 activate_library('disk');

 // call function
 $disks = disk_detect_physical();

 // queried disk
 $query = (strlen(@$_GET['query']) > 0) ? $_GET['query'] : false;

 // detailed information when querying disk
 if ($query)
 {
  // retrieve data via 'camcontrol identify' command
  $cap = disk_identify($query);

  // apply some fixes to the data format
  if ($cap['detail']['overlap']['support'] == 'no')
   $cap['detail']['overlap']['enabled'] = 'no';
  if ($cap['detail']['Native Command Queuing (NCQ)']['support'] == 'no')
   $cap['detail']['Native Command Queuing (NCQ)']['enabled'] = 'no';
  if (@is_numeric($cap['detail']['Native Command Queuing (NCQ)']['value']{0}))
   $cap['detail']['Native Command Queuing (NCQ)']['enabled'] = 'yes';
  if ($cap['detail']['data set management (TRIM)']['support'] == 'yes')
   $cap['detail']['data set management (TRIM)']['enabled'] = 'yes';
  if ($cap['detail']['data set management (TRIM)']['support'] == 'no')
   $cap['detail']['data set management (TRIM)']['enabled'] = 'no';

  // APM - Advanced Power Management
  if ($cap['detail']['advanced power management']['support'] == 'yes')
  {
   $class_apm = 'normal';
   $rawapm = trim($cap['detail']['advanced power management']['value']);
   $apm_dec = decode_raw_apmsetting($rawapm);
   $apm_current = ($apm_dec) ? $apm_dec : 
    '<span class="minortext">unknown</span>';
   // cache APM enabled status
   $_SESSION['disk_advanced'][$query]['apm_enabled'] = 
    $cap['detail']['advanced power management']['enabled'];
   // cache the decoded value of APM in SESSION array for the disk table
   $_SESSION['disk_advanced'][$query]['apm'] = $apm_dec;
   // display APM settings only when supported by disk
   $class_apm_enabled = ($cap['detail']['advanced power management']['enabled']
    == 'yes') ? 'normal' : 'hidden';
   $class_apm_disabled = ($cap['detail']['advanced power management']['enabled']
    != 'yes') ? 'normal' : 'hidden';
   // table apm_settinglist
   $table_apm_settinglist = array();
   $apm_settings = array(
    '255' => 'Disable',
    '1' => '1 (maximum power savings with spindown)',
    '32' => '32 (high power savings with spindown)',
    '64' => '64 (medium power savings with spindown)',
    '96' => '96 (low power savings with spindown)',
    '127' => '127 (lowest power savings with spindown)',
    '128' => '128 (maximum power savings without spindown)',
    '254' => '254 (maximum performance without spindown)'
   );
   foreach ($apm_settings as $id => $text)
    if ($apm_dec == $id)
     $table_apm_settinglist[] = array(
      'APM_ACTIVE'	=> 'selected="selected"',
      'APM_ID'		=> (int)$id,
      'APM_NAME'	=> htmlentities($text)
     );
    else
     $table_apm_settinglist[] = array(
      'APM_ACTIVE'	=> '',
      'APM_ID'		=> (int)$id,
      'APM_NAME'	=> htmlentities($text)
     );
  }
  else
   $class_apm = 'hidden';

  // information list for queried disk
  $infolist = array();
  foreach (@$cap['main'] as $property => $value)
  {
   // add 'rpm' suffix to the "media RPM" property value
   if (($property == 'media RPM') AND (is_numeric($value)))
    $value = $value.'rpm';
   // add new row
   $infolist[] = array(
    'INFO_PROPERTY'	=> htmlentities(ucwords($property)),
    'INFO_VALUE'	=> htmlentities($value)
   );
  }

  // capability information for queried disk
  $caplist = array();
  foreach (@$cap['detail'] as $feature => $data)
  {
   // support
   if ($data['support'] == 'yes')
   {
    $support = '';
    $support_yes = 'normal';
    $support_no = 'hidden';
   }
   elseif ($data['support'] == 'no')
   {
    $support = '';
    $support_yes = 'hidden';
    $support_no = 'normal';
   }
   else
   {
    $support = htmlentities($data['support']);
    $support_yes = 'hidden';
    $support_no = 'hidden';
   }

   // enabled
   if ($data['enabled'] == 'yes')
   {
    $enabled = '';
    $enabled_yes = 'normal';
    $enabled_no = 'hidden';
   }
   elseif ($data['enabled'] == 'no')
   {
    $enabled = '';
    $enabled_yes = 'hidden';
    $enabled_no = 'normal';
   }
   else
   {
    $enabled = htmlentities($data['enabled']);
    $enabled_yes = 'hidden';
    $enabled_no = 'hidden';
   }


   $caplist[] = array(
    'CAP_FEATURE'	=> htmlentities(ucwords($feature)),
    'CAP_SUPPORT'	=> $support,
    'CAP_SUPPORT_YES'	=> $support_yes,
    'CAP_SUPPORT_NO'	=> $support_no,
    'CAP_ENABLED'	=> $enabled,
    'CAP_ENABLED_YES'	=> $enabled_yes,
    'CAP_ENABLED_NO'	=> $enabled_no,
    'CAP_VALUE'		=> htmlentities($data['value']),
    'CAP_VENDOR'	=> htmlentities($data['vendor'])
   );
  }
 }

 // disk power setting table
 $powertable = array();
 foreach (@$disks as $diskname => $diskdata)
 {
  // active row
  $activerow = ($diskname == $query) ? 'activerow' : 'normal';

  // spinning status
  $spinning = disk_isspinning($diskname);
  $spinning_text = ($spinning) ? 'ready' : 'sleeping';
  $class_spinning_yes = ($spinning) ? 'normal' : 'hidden';
  $class_spinning_no = ($spinning) ? 'hidden' : 'normal';

  // APM status
  $apm_enabled = @$_SESSION['disk_advanced'][$diskname]['apm_enabled'];
  $apm_setting = @$_SESSION['disk_advanced'][$diskname]['apm'];
  if (strlen($apm_setting) < 1)
  {
   if (@isset($_SESSION['disk_advanced'][$diskname]))
    $apm_setting = '<span class="minortext">unsupported</span>';
   else
    $apm_setting = '<span class="minortext">unknown</span>';
  }
  else
   $apm_setting = '('.$apm_setting.')';
  $class_apm_yes = ($apm_enabled == 'yes') ? 'normal' : 'hidden';
  $class_apm_no = ($apm_enabled == 'no') ? 'normal' : 'hidden';

  // TODO: AAM status
  $aam_setting = '<span class="minortext">unknown</span>';

  // add row to array
  $powertable[] = array(
   'CLASS_SPINNING_YES'	=> $class_spinning_yes,
   'CLASS_SPINNING_NO'	=> $class_spinning_no,
   'CLASS_APM_YES'	=> $class_apm_yes,
   'CLASS_APM_NO'	=> $class_apm_no,
   'POWER_ACTIVEROW'	=> $activerow,
   'POWER_DISK'		=> htmlentities(trim($diskname)),
   'POWER_SPINNING'	=> $spinning_text,
   'POWER_APM'		=> $apm_setting,
   'POWER_AAM'		=> $aam_setting
  );
 }

 // classes
 $class_query = ($query) ? 'normal' : 'hidden';
 $class_noquery = (!$query) ? 'normal' : 'hidden';
 $class_details = ($query AND $cap) ? 'normal' : 'hidden';
 $class_nodetails = ($query AND $cap) ? 'hidden' : 'normal';

 // export new tags
 $newtags = @array(
  'PAGE_ACTIVETAB'		=> 'Advanced',
  'PAGE_TITLE'			=> 'Advanced disk settings',
  'TABLE_POWERLIST'		=> $powertable,
  'TABLE_QUERY_INFOLIST'	=> $infolist,
  'TABLE_QUERY_CAPABILITYLIST'	=> $caplist,
  'TABLE_APM_SETTINGLIST'	=> $table_apm_settinglist,
  'CLASS_QUERY'			=> $class_query,
  'CLASS_NOQUERY'		=> $class_noquery,
  'CLASS_DETAILS'		=> $class_details,
  'CLASS_NODETAILS'		=> $class_nodetails,
  'CLASS_APM'			=> $class_apm,
  'CLASS_APM_ENABLED'		=> $class_apm_enabled,
  'CLASS_APM_DISABLED'		=> $class_apm_disabled,
  'APM_CURRENT'			=> $apm_current,
  'QUERY_DISK'			=> $query
 );
 return $newtags;
}

function decode_raw_apmsetting($rawapm)
{
 if (strlen($rawapm) < 1)
  return false;
 if (substr($rawapm, 0, 2) == '0x')
  return @hexdec($rawapm);
 if ((($p = strpos($rawapm, '/0x80')) != false) AND (is_numeric($rawapm{$p+5})))
  return @hexdec(substr($rawapm, strpos($rawapm, '/0x80') + 5));
 if ((strpos($rawapm, '/0x') != false) AND (is_numeric($rawapm{0})))
  return @hexdec(substr($rawapm, strpos($rawapm, '/0x') + 3));
 // no luck
 return false;
}

function submit_disks_advanced()
{
 // redirect URL
 $redir = 'disks.php?advanced';

 // required library
 activate_library('disk');

 // scan each POST variable
 foreach ($_POST as $name => $value)
  if (substr($name, 0, strlen('spindown_')) == 'spindown_')
  {
   // fetch and sanitize disk
   $disk = substr($name, strlen('spindown_'));
   // TODO - SECURITY - sanitize disk
   // spindown disk
   $result = disk_spindown($disk);
   // provide feedback to user
   if ($result == true)
    friendlynotice('spinning down disk <b>'.$disk.'</b>', $redir);
   else
    friendlywarning('failed spinning down disk '.$disk, $redir);
  }
  elseif (substr($name, 0, strlen('spinup_')) == 'spinup_')
  {
   // fetch and sanitize disk
   $disk = substr($name, strlen('spinup_'));
   // TODO - SECURITY - sanitize disk
   // spinup disk
   $result = disk_spinup($disk);
   // provide feedback to user
   if ($result == true)
    friendlynotice('disk <b>'.$disk.'</b> is now spinned up again!', $redir);
   else
    friendlywarning('failed spinning up disk '.$disk, $redir);
  }

 // APM setting change
 if (@isset($_POST['apm_submit']) AND (is_numeric($_POST['apm_newsetting'])))
 {
  if (strlen($_POST['apm_setting_disk']) > 0)
   $redir .= '&query='.$_POST['apm_setting_disk'];
  else
   error('invalid disk specification for APM setting change!');
  $r = disk_set_apm($_POST['apm_setting_disk'], (int)$_POST['apm_newsetting']);
  if ($r)
   page_feedback('APM setting changed for disk <b>'
    .$_POST['apm_setting_disk'].'</b>.', 'b_success');
  else
   friendlyerror('failed changing APM setting for disk <b>'
    .$_POST['apm_setting_disk'].'</b>.', $redir);
 }

 // redirect
 redirect_url($redir);
}

?>
