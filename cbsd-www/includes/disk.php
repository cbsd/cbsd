<?php

function disk_info($disk_name)
// returns array with diskinfo output for given disk
{
 // diskinfo unfortunately requires super-user privileges
 activate_library('super');
 // TODO - SECURITY (sanitize disk_name)
 $super = super_execute('/usr/sbin/diskinfo '.$disk_name);
 if (($super['rv'] === 0) AND (@strlen($super['output_str']) > 0))
 {
  $arr = preg_split('/\s/m', $super['output_str'], -1, PREG_SPLIT_NO_EMPTY);
  $diskinfo['disk_name'] = $disk_name;
  $diskinfo['sectorsize'] = $arr[1];
  $diskinfo['mediasize'] = $arr[2];
  $diskinfo['sectorcount'] = $arr[3];
  return $diskinfo;
 }
 else
  return false;
}

function disk_smartinfo($disk_name)
// returns SMART data from smartctl query (requires smartmontools package)
{
 global $guru;

 // threshold settings (generate warnings beyond these values)
 $threshold = array(
  'temp_crit'	=> 55,
  'temp_high'	=> 45,
  'pcycle'	=> 10000,
  'lcc'		=> 300000,
  'lcc_rate'	=> 300,		/* 300 = once every 5 minutes */
  'cable'	=> 100,
  'sect_pas'	=> 100,
  'sect_active'	=> 1
 );
 $_SESSION['smart']['threshold'] = $threshold;
 if (strlen($disk_name) < 1)
  return;

 // requires super privileges
 activate_library('super');

 // device path
 $dev = '/dev/'.$disk_name;

 // passthrough modes
 $passthrough = array(
  '3ware,0',
  '3ware,1',
  'areca,0',
  'areca,1',
  'ata',
  'cciss,0',
  'cciss,1',
  'hpt,1/1',
  'hpt,1/2',
  'hpt,1/3',
  'marvell',
  'sat',
  'scsi',
  'usbcypress',
  'usbjmicron',
  'usbsunplus'
 );

 // activate SMART on disk
 super_execute('/usr/local/sbin/smartctl -s on '.$dev);

 // retrieve SMART attributes
 $result = super_execute('/usr/local/sbin/smartctl -A '.$dev);
 if ($result['rv'] != 0)
 {
  // try with -d <passthrough> option
  $success = false;
  foreach ($passthrough as $mode)
  {
   $result = super_execute('/usr/local/sbin/smartctl -A -d '.$mode.' '.$dev);
   if ($result['rv'] == 0)
   {
    $success = true;
    break;
   }
  }

  if (!$success)
  {
   // bail out due to error, but save this in SESSION cache
   @$_SESSION['smart'][$disk_name] = array(
    'status'		=> 'SMART incapable',
    'temp_c'		=> 'no sensor',
    'class_status'	=> 'smart_status_incapable',
    'class_temp'	=> 'smart_temp_nosensor',
    'class_badsectors'	=> 'smart_badsectors_incapable'
   );
   return @$_SESSION['smart'][$disk_name];
  }
 }

 // extract SMART data from raw output
 $smart = array('data' => array());
 $str = $result['output_str'];
 $preg = '/^[\s]*([0-9]+)[\s]+([a-zA-Z_-]+)[\s]+(0x[0-9a-f]+)[\s]+([0-9]+)[\s]+'
  .'([0-9]+)[\s]+([0-9]+)[\s]+([a-zA-Z_-]+)[\s]+([a-zA-Z_-]+)[\s]+'
  .'([a-zA-Z_-]+)[\s]+(.+)[\s]*$/m';
 preg_match_all($preg, $str, $matches);
 if (@is_array($matches[1]))
  foreach ($matches[1] as $nr => $id)
   $smart['data'][(int)$id] = @array('id' => $matches[1][$nr],
    'attribute' => $matches[2][$nr], 'flag' => $matches[3][$nr],
    'value' => $matches[4][$nr], 'worst' => $matches[5][$nr],
    'threshold' => $matches[6][$nr], 'type' => $matches[7][$nr],
    'updated' => $matches[8][$nr], 'failed' => $matches[9][$nr],
    'raw' => $matches[10][$nr]);

 // process status
 $class_status = 'smart_status_healthy';
 $status = 'Healthy';
 foreach ($smart['data'] as $id => $data)
  if ($data['failed'] != '-')
  {
   if ($data['failed'] == '')
   {
    $class_status = 'smart_status_smartincapable';
    $status = 'SMART incapable';
   }
   elseif (strtolower($data['failed']) == 'in_the_past')
   {
    $class_status = 'smart_status_inthepast';
    $status = $data['failed'];
   }
   else
   {
    $class_status = 'smart_status_failed';
    $status = $data['failed'];
   }
  }

 // process temperature
 $temp = (int)@$smart['data'][194]['raw'];
 $temp_f = ($temp > 0) ? round(($temp * (9/5)) + 32) : '';
 $temp_c = ($temp > 0) ? $temp : 'no sensor';
 if ((int)$temp_c >= $threshold['temp_crit'])
 {
  $class_temp = 'smart_temp_hot';
  $class_status = 'smart_status_failed';
  $status = 'CRITICAL';
 }
 elseif ((int)$temp_c >= $threshold['temp_high'])
 {
  $class_temp = 'smart_temp_warm';
  $class_status = 'smart_status_warning';
  $status = 'Warning';
 }
 elseif ((int)$temp_c > 0)
  $class_temp = 'smart_temp_cool';
 else
  $class_temp = 'smart_temp_nosensor';

 // process power cycles
 $class_powercycles = 'smart_powercycles_low';
 if (@$smart['data'][12]['raw'] >= $threshold['pcycle'])
 {
  $class_powercycles = 'smart_powercycles_high';
  $class_status = 'smart_status_warning';
  $status = 'Warning';
 }

 // determine LCC (Load Cycle Count) attribute
 if (@isset($smart['data'][193]['raw']))
  $lcc = 193;
 else
  $lcc = 225;

 // process load cycles
 $class_loadcycles = 'smart_loadcycles_low';
 if (@$smart['data'][$lcc]['raw'] >= $threshold['lcc'])
 {
  $class_loadcycles = 'smart_loadcycles_high';
  $class_status = 'smart_status_warning';
  $status = 'Warning';
 }

 // process cable errors
 if (@$smart['data'][199]['raw'] == 0)
  $class_cableerrors = 'smart_cableerrors_none';
 elseif (@$smart['data'][199]['raw'] < $threshold['cable'])
  $class_cableerrors = 'smart_cableerrors_low'; 
 else
 {
  $class_cableerrors = 'smart_cableerrors_high'; 
  $class_status = 'smart_status_warning';
  $status = 'Warning';
 }

 // process bad sectors
 $sect_reallocated = (is_numeric(@$smart['data'][5]['raw']))
  ? $smart['data'][5]['raw'] : '?';
 $sect_pending = (is_numeric(@$smart['data'][197]['raw']))
  ? $smart['data'][197]['raw'] : '?';
 if ((int)$sect_pending > 0)
  $class_badsectors = 'smart_badsectors_pending';
 elseif ((int)$sect_reallocated > 0)
  $class_badsectors = 'smart_badsectors_passive';
 else
  $class_badsectors = 'smart_badsectors_none';

 // process lifetime (power on hours)
 $poh = (int)@$smart['data'][9]['raw'];
 $class_lifetime = 'smart_lifetime_normal';
 if ($poh == 0)
  $power_on = '<span class="minortext">???</span>';
 elseif ($poh < 48)
  $power_on = $poh.' hours';
 elseif ($poh < (2*30*24))
  $power_on = (int)($poh/24).' days';
 elseif ($poh < (12*30*24))
  $power_on = round($poh/(30*24), 1).' months';
 elseif ($poh > (5*12*30*24))
 {
  $class_lifetime = 'smart_lifetime_old';
  $power_on = round($poh/(12*30*24), 1).' years';
 }
 else
  $power_on = round($poh/(12*30*24), 1).' years';

 // begin building quick array
 $smart['quick'] = @array(
  'status'		=> $status,
  'power_on'		=> $power_on,
  'power_on_hours'	=> $poh,
  'power_cycles'	=> @$smart['data'][12]['raw'],
  'load_cycles'		=> @$smart['data'][$lcc]['raw'],
  'temp_c'		=> $temp_c,
  'temp_f'		=> $temp_f,
  'reallocated_sectors'	=> $sect_reallocated,
  'pending_sectors'	=> $sect_pending,
  'cable_errors'	=> @$smart['data'][199]['raw'],
  'class_status'	=> $class_status,
  'class_temp'		=> $class_temp,
  'class_powercycles'	=> $class_powercycles,
  'class_loadcycles'	=> $class_loadcycles,
  'class_cableerrors'	=> $class_cableerrors,
  'class_badsectors'	=> $class_badsectors,
  'class_lifetime'	=> $class_lifetime
 );

 // cache quick array to $_SESSION array
 @$_SESSION['smart'][$disk_name] = $smart['quick'];
 return $smart;
}

function disk_detect_dmesg($physdisks = false)
// detect physical disks by dmesg and returns an array
{
 $dmesg = @file_get_contents('/var/run/dmesg.boot');

 // fetch physical disks if not supplied
 if (!is_array($physdisks))
  $physdisks = disk_detect_physical();

 // now scan the dmesg file per physical disk we know
 $dmesg_arr = array();
 foreach ($physdisks as $diskname => $diskdata)
 {
  preg_match('/^('.$diskname.')\: (.*)$/m', $dmesg, $matches);
  $dmesg_arr[$diskname] = $matches[2];
 }

 // return the dmesg array
 return $dmesg_arr;
}

function disk_detect_physical()
// detect physical disks by ls output and returns an array
{
 // scan for these types of devices (/dev/XXX[number])
 $disk_drivers = array(
  'aacd',
  'ad',
  'ada',
  'ar',
  'da',
  'twed');

 // fetch active device nodes
 exec('/bin/ls -1 /dev', $devices);

 // now produce an array with disks that match a driver
 $disks = array();
 foreach ($devices as $device)
  foreach ($disk_drivers as $driver)
   if (preg_match('/^('.$driver.')[0-9]+$/', $device))
    $disks[] = $device;

 // now check if it's a real disk by determining its sector size
 // it should be 512 bytes or a multiple of that
 $validdisks = array();
 foreach ($disks as $diskid => $disk)
 {
  $diskinfo = disk_info($disk);
  if (@(int)$diskinfo['sectorsize'] >= 512)
   $validdisks[$disk] = $diskinfo;
 }
 return $validdisks;
}

function disk_detect_label()
// detect usable disks by looking at geom_label and returns an array
{
 $label = array();
 exec('/sbin/glabel status', $glabel_status);
 foreach ($glabel_status as $line)
  if (preg_match('/label\/(.*)$/m', $line, $glabel_preg))
  {
   $preg = current($glabel_preg);
   $component = trim(substr($preg, strrpos($preg, ' ')));
   $label_name = trim(substr($preg, strlen('label/'),
    strpos($preg,' ') - strlen('label/')));
   $label[$component] = $label_name;
  }
 return $label;
}

function sort_providers($a, $b)
// custom sort function to sort providers by LBA from low to high offset
{
 if (@$a['start'] < @$b['start'])
  return -1;
 elseif (@$a['start'] > @$b['start'])
  return 1;
 else
  return 0;
}

function disk_detect_gpart()
// structures output of 'gpart list' into array and returns it
{
 // start with empty gpart array
 $gpart = array();
 // execute "gpart list" to get information about all partitions
 exec('/sbin/gpart list', $raw_output);
 // split the information in chunks to deal with
 $raw_str = implode("\n", $raw_output);
 $split = preg_split('/^Geom name\: (.*)/Um', $raw_str);
 // split array should be at least 2 rows (= 1 real disk)
 if (count($split) < 2)
  return false;
 // handle each disk separately
 for ($i = 1; $i <= (count($split)-1); $i++)
 {
  $disk = @trim(substr($split[$i],0,strpos($split[$i], "\n")));
  // skip if disk does not exist
  if ((!file_exists('/dev/'.@$disk)) OR (strlen($disk) < 1))
   continue;
  // split data with simple string search
  $general = substr($split[$i], 0, strpos($split[$i], 'Providers:'.chr(10)));
  $providers = substr($split[$i], strpos($split[$i], 'Providers:'.chr(10)));
  $providers = substr($providers, 0, strpos($providers, 'Consumers:'.chr(10)));
  $consumers = substr($split[$i], strpos($split[$i], 'Consumers:'.chr(10)));
  // general
  preg_match_all('/^(.*)\: (.*)$/m', $general, $general_matches);
  if (@is_array($general_matches[1]))
   foreach ($general_matches[1] as $id => $gname)
    $gpart[$disk]['general'][@trim($gname)] =
     @trim($general_matches[2][$id]);
  // providers
  $partsplit = preg_split('/^[0-9]\. Name\: (.*)$/m', $providers);
  for ($y = 1; $y <= (count($partsplit)-1); $y++)
  {
   preg_match_all('/^(.*)\: (.*)$/m', $partsplit[$y], $partitiondata);
   if (@is_array($partitiondata[1]))
    foreach ($partitiondata[1] as $id => $data_name)
     $gpart[$disk]['providers'][$y][trim($data_name)] =
      trim(@$partitiondata[2][$id]);
  }
  // sort providers using start offset (sorting from low LBA to high LBA)
  usort($gpart[$disk]['providers'], 'sort_providers');
  // consumers
  preg_match_all('/^(.*)\: (.*)$/m', $consumers, $consumer_matches);
  if (@is_array($consumer_matches[1]))
   foreach ($consumer_matches[1] as $id => $cname)
    $gpart[$disk]['consumers'][@trim($cname)] =
     @trim($consumer_matches[2][$id]);
  // set label
  for ($z = 0; $z < 255; $z++)
   if (!@isset($gpart[$disk]['providers'][$z]))
    break;
   elseif ($gpart[$disk]['providers'][$z]['label'] == '(null)')
    $gpart[$disk]['providers'][$z]['label'] = false;
   elseif ((@strlen($gpart[$disk]['providers'][$z]['label']) > 0) AND
           (!isset($gpart[$disk]['label']))) 
   {
    $gpart[$disk]['label'] = trim($gpart[$disk]['providers'][$z]['label']);
    $gpart['labels'][trim($gpart[$disk]['providers'][$z]['label'])] = $disk;
   }
 }
 return $gpart;
}

function disk_detect_gnop()
// detects /dev entries ending with .nop; these are geom_nop devices
// used commonly for changing sector size for 4K physical sector disks
{
 // find all device entires in /dev ending with .nop
 exec('/usr/bin/find /dev/ -type c -name "*.nop"', $output);
 if (!is_array($output))
  return false;
 // return array of gnop devices with diskinfo array attached
 $gnop = array();
 foreach ($output as $nopdevice)
 {
  $nop = str_replace('.nop', '', substr($nopdevice, strlen('/dev/')));
  $diskinfo = disk_info($nopdevice);
  $gnop[$nop] = $diskinfo;
 }
 return $gnop;
}

function disk_identify($disk)
// detailed disk information via camcontrol identify command output
{
 // needs increased privileges
 activate_library('super');

 // fetch command output
 $result = super_execute('/sbin/camcontrol identify /dev/'.$disk);
 // check return value
 if ($result['rv'] != 0)
  return false;

 // split output in three sections
 $bigsplit = preg_split('/\n\n/m', $result['output_str']);
 if (count($bigsplit) != 3)
  return false;

 // create ident array and assign first chunk of bigsplit to it
 $ident = array('pass' => $bigsplit[0]);

 // main data (second chunk)
 preg_match_all('/^(.*)\s\s+(.*)$/Um', $bigsplit[1], $secondsplit);
 // walk through secondsplit and assign to ident array
 if (is_array($secondsplit[2]))
  foreach($secondsplit[1] as $id => $property)
   $ident['main'][$property] = $secondsplit[2][$id];

 // detailed data (third chunk)
 $rexp = '/^([^\s\n]([^\s\n]+\s)*)\s{2,}'
  .'([^\t\n]*)\t?([^\t\n]*)\t?([^\t\n]*)\t?([^\t\n]*)$/m';
 preg_match_all($rexp, $bigsplit[2], $thirdsplit);
 // walk through thirdsplit and assign to ident array
 if (is_array($thirdsplit[4]))
  foreach($thirdsplit[1] as $id => $property)
   if ($id > 0)
    $ident['detail'][trim($property)] = @array(
     'support'	=> $thirdsplit[3][$id],
     'enabled'	=> $thirdsplit[4][$id],
     'value'	=> $thirdsplit[5][$id],
     'vendor'	=> $thirdsplit[6][$id],
    );

 // return final ident array
 return $ident;
}


/* ACTIVE functions (dangerous) */

function disk_spindown($disk)
// spins down disk using camcontrol
{
 // increased privileges
 activate_library('super');
 // use different command for ATA/AHCI disks than for SCSI/SAS disks
 if (substr($disk, 0, 2) == 'ad')
  $camcommand = 'standby';
 elseif (substr($disk, 0, 2) == 'da')
  $camcommand = 'stop';
 else
  return false;
 // execute camcontrol spindown command
 $result = super_execute('/sbin/camcontrol '.$camcommand.' '.$disk);
 if ($result['rv'] == 0)
  return true;
 else
  return false;
}

function disk_spinup($disk)
// spins up disk using dd read command
{
 // increased privileges
 activate_library('super');
 // read one sector from disk to make it spin up again
 $result = super_execute('/bin/dd if=/dev/'.$disk.' of=/dev/null count=1');
 if ($result['rv'] == 0)
  return true;
 else
  return false;
}

function disk_isspinning($disk)
// checks whether disk is spinning or not
// returns false if disk is spinned down, true if otherwise (including unknown)
{
 global $guru;
 // increased privileges
 activate_library('super');
 // device path
 if (strlen($disk) > 0)
  $dev = '/dev/'.$disk;
 else
  error('invalid disk name passed to disk_isspinning()');
 // execute command
 $r = super_execute('/usr/local/sbin/smartctl -A -n standby '.$dev);
 // check result
 if ($r['rv'] != 2)
  return true;
 elseif (strpos($r['output_str'], 'Device is in STANDBY mode, exit') === false)
  return true;
 else
  return false;
}

function disk_set_apm($disk, $apm_level)
// changes Advanced Power Management (APM) setting on specified disk
// levels 0-127 = power savings with spindown
// levels 128-254 = power savings without spindown
// level 255 = disable APM
{
 // increased privileges
 activate_library('super');
 // determine command for specific APM setting
 $apm_hex = strtoupper(dechex($apm_level));
 if (strlen($apm_hex) == 1)
  $apm_hex = '0'.$apm_hex;
 if ($apm_level == 255)
  $rawcmd = 'EF 85 00 00 00 00 00 00 00 00 00 00';
 else
  $rawcmd = 'EF 05 00 00 00 00 00 00 00 00 '.$apm_hex.' 00';
 // execute camcontrol command to set APM level
 $result = super_execute('/sbin/camcontrol cmd '.$disk.' -a "'.$rawcmd.'"');
 // check result
 if ($result['rv'] == 0)
  return true;
 else
  return false;
}

?>
