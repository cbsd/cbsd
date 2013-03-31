<?php

function content_disks_smart()
{
 // required library
 activate_library('disk');

 // threshold values
 disk_smartinfo();
 $thres = @$_SESSION['smart']['threshold'];

 // call function
 $disks = disk_detect_physical();

 // queried disk
 $query = (strlen(@$_GET['query']) > 0) ? $_GET['query'] : false;

 // query all button (stores in $_SESSION['smart'])
 if (@isset($_GET['queryall']))
 {
  foreach ($disks as $diskname => $diskdata)
   disk_smartinfo($diskname);
  redirect_url('disks.php?smart');
 }

 // retrieve SMART information for queried disk
 if (@strlen($query) > 0)
  $smart = disk_smartinfo($query);

 // classes
 $class_querydisk = ($query) ? 'normal' : 'hidden';
 $class_advice_activesect = 'hidden';
 $class_advice_cableerrors = 'hidden';
 $class_advice_criticaltemp = 'hidden';
 $class_advice_hightemp = 'hidden';
 $class_advice_passivesect = 'hidden';
 $class_advice_highlccrate = 'hidden';
 $class_advice_inthepast = 'hidden';
 $class_advice_unknownfailure = 'hidden';
 $class_advice_noproblems = 'hidden';
 $class_advice_needscan = 'hidden';

 // LCC rate (highest rate of load cycles relative to lifetime/power-on-hours)
 $lccrate = false;

 // disk list
 $disklist = array();
 foreach ($disks as $diskname => $diskdata)
 {
  // style classes for disk list
  $class_status = (@$_SESSION['smart'][$diskname]['status'] == 'Healthy')
   ? 'class="smart_status_healthy"' : 'class="smart_status_nothealthy"';
  $activerow = ($diskname == $query) ? 'class="activerow"' : '';
  $disklist[] = array(
   'SMART_ACTIVEROW'	=> $activerow,
   'SMART_DISK'		=> htmlentities(trim($diskname)),
   'SMART_STATUS'	=> @$_SESSION['smart'][$diskname]['status'],
   'SMART_TEMP_C'	=> @$_SESSION['smart'][$diskname]['temp_c'],
   'SMART_TEMP_F'	=> @$_SESSION['smart'][$diskname]['temp_f'],
   'SMART_POWERCYCLES'	=> @$_SESSION['smart'][$diskname]['power_cycles'],
   'SMART_LOADCYCLES'	=> @$_SESSION['smart'][$diskname]['load_cycles'],
   'SMART_CABLEERRORS'	=> @$_SESSION['smart'][$diskname]['cable_errors'],
   'SMART_PASSIVESECTORS' => 
    @$_SESSION['smart'][$diskname]['reallocated_sectors'],
   'SMART_PENDINGSECTORS' => 
    @$_SESSION['smart'][$diskname]['pending_sectors'],
   'SMART_LIFETIME'	=> @$_SESSION['smart'][$diskname]['power_on'],
   'CLASS_STATUS'	=> @$_SESSION['smart'][$diskname]['class_status'],
   'CLASS_TEMP'		=> @$_SESSION['smart'][$diskname]['class_temp'],
   'CLASS_POWERCYCLES'	=> @$_SESSION['smart'][$diskname]['class_powercycles'],
   'CLASS_LOADCYCLES'	=> @$_SESSION['smart'][$diskname]['class_loadcycles'],
   'CLASS_CABLEERRORS'	=> @$_SESSION['smart'][$diskname]['class_cableerrors'],
   'CLASS_BADSECTORS'	=> @$_SESSION['smart'][$diskname]['class_badsectors'],
   'CLASS_LIFETIME'	=> @$_SESSION['smart'][$diskname]['class_lifetime']
  );
  // set highest LCC rate (highest = lowest number = most cycles per timeunit)
  if (@$_SESSION['smart'][$diskname]['power_on_hours'] > 24)
  {
   $current_lccrate = @($_SESSION['smart'][$diskname]['power_on_hours'] / 
    $_SESSION['smart'][$diskname]['load_cycles']) * 3600;
   if ($lccrate === false OR 
       (($current_lccrate > 0) AND ($current_lccrate < $lccrate)))
   $lccrate = round($current_lccrate, 1);
  }
  // set advice level
  if (@$_SESSION['smart'][$diskname]['pending_sectors'] >= 
   $thres['sect_active'])
   $class_advice_activesect = 'normal';
  if (@$_SESSION['smart'][$diskname]['cable_errors'] >= $thres['cable'])
   $class_advice_cableerrors = 'normal';
  if (@$_SESSION['smart'][$diskname]['temp_c'] >= $thres['temp_crit'])
   $class_advice_criticaltemp = 'normal';
  elseif (@$_SESSION['smart'][$diskname]['temp_c'] >= $thres['temp_high'])
   $class_advice_hightemp = 'normal';
  if (@$_SESSION['smart'][$diskname]['reallocated_sectors'] >= 
   $thres['sect_pas'])
   $class_advice_passivesect = 'normal';
  if (is_numeric($lccrate) AND ($lccrate <= $thres['lcc_rate']))
   $class_advice_highlccrate = 'normal';
  if (@strtoupper($_SESSION['smart'][$diskname]['status']) == 'IN_THE_PAST')
   $class_advice_inthepast = 'normal';
  if (@$_SESSION['smart'][$diskname]['status'] == 'Failure')
   $class_advice_unknownfailure = 'normal';
  if (($class_advice_activesect == 'hidden') AND
      ($class_advice_cableerrors == 'hidden') AND
      ($class_advice_criticaltemp == 'hidden') AND
      ($class_advice_hightemp == 'hidden') AND
      ($class_advice_passivesect == 'hidden') AND
      ($class_advice_inthepast == 'hidden') AND
      (@strtoupper($_SESSION['smart'][$diskname]['status']) == 'FAILURE'))
   $class_advice_unknownfailure = 'normal';
  if (@!isset($_SESSION['smart'][$diskname]))
   $class_advice_needscan = 'normal';
 }

 // set no problems advice if applicable
 if (($class_advice_activesect == 'hidden') AND
     ($class_advice_cableerrors == 'hidden') AND
     ($class_advice_criticaltemp == 'hidden') AND
     ($class_advice_hightemp == 'hidden') AND
     ($class_advice_passivesect == 'hidden') AND
     ($class_advice_highlccrate == 'hidden') AND
     ($class_advice_inthepast == 'hidden') AND
     ($class_advice_unknownfailure == 'hidden') AND
     ($class_advice_needscan == 'hidden'))
  $class_advice_noproblems = 'normal';

 // smart list when querying disk
 $smartlist = array();
 if ($query AND is_array(@$smart['data']))
 {
  // query disk smart list
  foreach ($smart['data'] as $id => $data)
  {
   $arr = array(5,12);
   if (($id == 5 OR $id == 192 OR $id == 196 OR $id == 198) 
       AND ($data['raw'] != 0))
    $activerow = 'class="activerow"';
   elseif (($id == 197 OR $id == 199) AND ($data['raw'] != 0))
    $activerow = 'class="failurerow"';
   elseif ($data['failed'] != '-')
    $activerow = 'class="failurerow"';
   else
    $activerow = '';
   $smartlist[] = array(
    'SMART_ACTIVEROW'		=> $activerow,
    'SMART_ID'			=> (int)$id,
    'SMART_ATTR'		=> htmlentities($data['attribute']),
    'SMART_FLAG'		=> htmlentities($data['flag']),
    'SMART_VALUE'		=> htmlentities($data['value']),
    'SMART_WORST'		=> htmlentities($data['worst']),
    'SMART_THRESHOLD'		=> htmlentities($data['threshold']),
    'SMART_FAILED'		=> htmlentities($data['failed']),
    'SMART_RAW'			=> htmlentities($data['raw'])
   );
  }
 }

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'		=> 'SMART',
  'PAGE_TITLE'			=> 'SMART monitor',
  'TABLE_SMART_DISKLIST'	=> $disklist,
  'TABLE_QUERY_SMARTLIST'	=> $smartlist,
  'CLASS_QUERYDISK'		=> $class_querydisk,
  'CLASS_ADVICE_ACTIVESECT'	=> $class_advice_activesect,
  'CLASS_ADVICE_CABLEERRORS'	=> $class_advice_cableerrors,
  'CLASS_ADVICE_CRITICALTEMP'	=> $class_advice_criticaltemp,
  'CLASS_ADVICE_HIGHTEMP'	=> $class_advice_hightemp,
  'CLASS_ADVICE_PASSIVESECT'	=> $class_advice_passivesect,
  'CLASS_ADVICE_HIGHLCCRATE'	=> $class_advice_highlccrate,
  'CLASS_ADVICE_INTHEPAST'	=> $class_advice_inthepast,
  'CLASS_ADVICE_UNKNOWNFAILURE'	=> $class_advice_unknownfailure,
  'CLASS_ADVICE_NOPROBLEMS'	=> $class_advice_noproblems,
  'CLASS_ADVICE_NEEDSCAN'	=> $class_advice_needscan,
  'ADVICE_LCC_RATE'		=> $lccrate,
  'QUERY_DISK'			=> $query,
 );
 return $newtags;
}

?>
