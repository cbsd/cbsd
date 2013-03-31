<?php

function content_status_release()
{
 global $guru;

 // url
 // TODO - should make these rely on preferred server instead?
 $url = 'status.php?release';
 $iframe_current = 'http://www.bsdstore.ru/html/current.html';
 $iframe_upcoming = 'http://www.bsdstore.ru/html/upcoming.html';
 $iframe_changelog = 'http://www.bsdstore.ru/html/version.html';

 // tabbar
 $tabbar = array(
  'current' => 'Current release highlights',
  'upcoming' => 'Upcoming release features', 
  'changelog' => 'Changelog', 
 );

 // select tab
 if (isset($_GET['current']))
 {
  $release_header = $tabbar['current'];
  $release_url = $iframe_current;
 }
 elseif (isset($_GET['upcoming']))
 {
  $release_header = $tabbar['upcoming'];
  $release_url = $iframe_upcoming;
 }
 elseif (isset($_GET['changelog']))
 {
  $release_header = $tabbar['changelog'];
  $release_url = $iframe_changelog;
 }
 else
 {
  // default tab: 
  $release_header = $tabbar['current'];
  $release_url = $iframe_current;
 }

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'	=> 'Release information',
  'PAGE_TITLE'		=> 'Release information',
  'PAGE_TABBAR'		=> $tabbar,
  'PAGE_TABBAR_URL'	=> $url,
  'RELEASE_HEADER'	=> $release_header,
  'RELEASE_URL'		=> $release_url
 );
 return $newtags;
}

?>
