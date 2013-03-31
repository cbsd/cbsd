<?php

function content_services_services()
{
 // required library
 activate_library('service');

 // call function
 $panels = service_panels();

 // debug
 if (@isset($_GET['debug']))
  viewarray($panels);

 // process panels table
 $ptable = array();
 if (@is_array($panels))
  foreach ($panels as $cat => $data)
  {
   $loop = true;
   while ($loop)
   {
    // grab data (3 at a time for a complete row)
    $a1 = @each($data);
    $a2 = @each($data);
    $a3 = @each($data);

    // loop protection
    if (!is_array($a3))
     $loop = false;

    // hide columns if no data
    $hidden_one = (is_array($a1)) ? 'normal' : 'hidden';
    $hidden_two = (is_array($a2)) ? 'normal' : 'hidden';
    $hidden_three = (is_array($a3)) ? 'normal' : 'hidden';

    // assign panel names
    $one = @$a1['key'];
    $two = @$a2['key'];
    $three = @$a3['key'];
    $onelong = @htmlentities($panels[$cat][$one]['longname']);
    $twolong = @htmlentities($panels[$cat][$two]['longname']);
    $threelong = @htmlentities($panels[$cat][$three]['longname']);

    // add row to table array
    $ptable[] = array(
     'CLASS_HIDDEN_ONE'		=> $hidden_one,
     'CLASS_HIDDEN_TWO'		=> $hidden_two,
     'CLASS_HIDDEN_THREE'	=> $hidden_three,
     'PANEL_ONE'		=> $one,
     'PANEL_TWO'		=> $two,
     'PANEL_THREE'		=> $three,
     'PANEL_ONE_LONG'		=> $onelong,
     'PANEL_TWO_LONG'		=> $twolong,
     'PANEL_THREE_LONG'		=> $threelong
    );
   }
  }

 return array(
  'PAGE_ACTIVETAB'	=> 'Service panel',
  'PAGE_TITLE'		=> 'Service panel',
  'TABLE_PANELS'	=> $ptable
 );
}

?>
