<?php

function content_files_snapshots()
{
 // snapshot list
 $snaplist = htmlentities(trim(`zfs list -t snapshot`));

 // new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'Snapshots',
  'PAGE_TITLE'		=> 'Snapshots',
  'FILES_SNAPLIST'	=> $snaplist
 );
 return $newtags;
}

?>
