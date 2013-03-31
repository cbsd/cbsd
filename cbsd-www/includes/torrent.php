<?php

/*
** torrent library
*/

function torrent_list()
// returns array of all torrents currently known
{
 global $guru;

 // check for cached torrent list
 if (@is_array($guru['torrent']['listcache']))
  return $guru['torrent']['listcache'];

 // sanity check on torrent paths
 $tpaths = array('downloading', 'finished', 'torrents');
 foreach ($tpaths as $tpath)
  if (!is_dir($guru['torrent']['path_'.$tpath]))
   error('missing torrent directory <b>'.$tpath.'</b>!');

 // fetch directory contents for each directory
 $content = array();
 foreach ($tpaths as $tpath)
  exec('/bin/ls -1 '.$guru['torrent']['path_'.$tpath], $content[$tpath], $rv);

 // save cached version (just for this pageview)
 $guru['torrent']['listcache'] = $content;

 // return data
 return $content;
}

function torrent_status()
// specific torrent info?
{
}

function torrent_download($url)
// download given URL to .torrent file, and places in the torrents directory
{
 global $guru;

 // required library
 activate_library('service');

 // start rtorrent service (silent error if already started)
 service_start('rtorrent', true);

 // fetch torrent file directly to torrents directory
 $command = '/usr/bin/fetch -o '.$guru['tempdir'].'/ "'.$url.'" 2>&1';
 exec($command, $result, $rv);
 if ($rv == 0)
 {
  // determine local path to torrent file
  $localpath = substr($result[0], 0, strpos($result[0], ' '));
  // install torrent (moves temp file to torrents directory)
  $result2 = torrent_install(trim($localpath));
  if ($result2 == true)
   return true;
  else
   return false;
 }
 else
 {
  page_feedback('fetch command failed when retrieving torrent!', 'a_failure');
  return false;
 }
}

function torrent_install($localpath)
// installs torrent located at localpath to torrents directory
{
 global $guru;

 // elevated privileges
 activate_library('super');

 // first check for proper suffix of localpath (should end with .torrent)
 if (substr($localpath, -(strlen('.torrent'))) != '.torrent')
 {
  // bail out with warning and remove temporary file
  page_feedback('torrent file does not have .torrent suffix!', 'a_error');
  unlink($localpath);
  return false;
 }

 // sanity check on torrents directory
 if (!is_dir($guru['torrent']['path_torrents']))
 {
  // bail out with error and remove temporary file
  unlink($localpath);
  error('missing torrent directory for new torrents!');
 }

 // move temporary file to torrents directory
 $command = '/bin/mv '.$localpath.' '.$guru['torrent']['path_torrents'].'/';
 $result = super_execute($command);
 if ($result['rv'] == 0)
 {
  // sleep for torrent to be handled
  sleep(6);
  // return true
  return true;
 }
 else
 {
  // destroy temporary file (normal privileges)
  unlink($localpath);
  // return failure
  return false;
 }
}

function torrent_access($torrent)
// returns directory path to torrent
{
 global $guru;
 $torrentpath = $guru['torrent']['path_downloaded'].'/'.$torrent;
 if (is_file($torrentpath))
  return $torrentpath;
 else
  return false;
}

function torrent_purge($torrent)
// purges specific torrent (deletes .torrent file and downloaded file)
{
 global $guru;

 // remove torrent from downloaded directory
 exec('/bin/rm '.$guru['torrent']['path_downloaded'].'/'.$torrent, $result, $rv);
 if ($rv == 0)
  return true;
 else
 {
  page_feedback('Could not purge torrent '
   .'<b>'.htmlentities($torrent).'</b>', 'a_warning');
  return false;
 }
}

function torrent_purge_all()
// purges all torrents, session data and .torrent files
{
 // required library
 activate_library('service');

 // execute purge script
 service_script('rtorrent', 'purge');
}

?>
