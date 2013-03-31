<?php

/*
** ZFSguru Web-interface - common.php
** common functions part of every request
*/


function activate_library($library)
// activates a library php function in the includes directory on request
{
 // TODO - SECURITY
 $librarypath = 'includes/'.$library.'.php';
 include_once($librarypath);
}

function dangerouscommand($commands, $redirect_url)
{
 $data = array(
  'commands'		=> $commands,
  'redirect_url'	=> $redirect_url
 );
 $content = content_handle('internal', 'dangerouscommand', $data, true);
 page_handle($content);
 die();
}

function error($message)
{
 page_injecttag(array('MESSAGE' => $message));
 $content = content_handle('internal', 'error', false, true);
 page_handle($content);
 die();
}

function friendlyerror($message, $url)
{
 page_feedback($message, 'a_warning');
 redirect_url($url);
}

function friendlynotice($message, $url)
{
 page_feedback($message);
 redirect_url($url);
}

function redirect_url($url)
{
 header('Location: '.$url);
 die();
}

function sizehuman($bytes, $precision = 0)
// returns human readable size in bytes from integer (1000)
{
 $units = array('B', 'KB', 'MB', 'GB', 'TB');
 $bytes = max($bytes, 0);
 $pow = floor(($bytes ? log($bytes) : 0) / log(1000));
 $pow = min($pow, count($units) - 1);
 $bytes /= pow(1000, $pow);
 return round($bytes, $precision) . ' ' . $units[$pow];
}

function sizebinary($bytes, $precision = 0)
// returns human readable size in binary bytes (1024)
{
 $units = array('B', 'KiB', 'MiB', 'GiB', 'TiB');
 $bytes = max($bytes, 0);
 $pow = floor(($bytes ? log($bytes) : 0) / log(1024));
 $pow = min($pow, count($units) - 1);
 $bytes /= pow(1024, $pow);
 return round($bytes, $precision) . ' ' . $units[$pow];
}

function remoteserver($url_suffix, $forced_server = false)
// finds a suitable remote server and returns contents of file or false
{
 global $guru;
 // check offline mode
 if (@$guru['preferences']['offline_mode'])
  return false;
 // sanity checks
 if (strlen($url_suffix) < 1)
  error('bad call to remoteserver; invalid URL requested');
 if (!is_array($guru['master_servers']))
  error('no master servers known; cannot retrieve remote file');
 // prepare server list
 if ($forced_server === false)
  $servers = $guru['master_servers'];
 else
  $servers = array($forced_server);
 while (count($servers) > 0)
 {
  // check preferred server
  $key = @array_search($guru['preferences']['preferred_server'], $servers);
  if ($key !== false)
  {
   $url = $guru['preferences']['preferred_server'] . $url_suffix;
   unset($servers[$key]);
  }
  else
   $url = array_shift($servers) . $url_suffix;
  // use external fetch command to retrieve file since it has proper timeouts
  $timeout_sec = (@$guru['preferences']['connect_timeout'] > 0) ? 
   (int)$guru['preferences']['connect_timeout'] : 1;
  $command = '/usr/bin/fetch -o - -T '.$timeout_sec.' http://'.$url;
  exec($command, $output, $rv);
  // only accept result if return value zero and non-empty result
  if ($rv == 0 AND is_array($output))
  {
   // success - before we return result first process late activation data
   if (!@isset($guru['no_delayed_activation']))
   {
    activate_library('persistent');
    $act = persistent_read('activation_delayed');
    if ($act != false)
    {
     activate_library('activation');
     activation_delayed();
    }
   }
   // return result
   return implode(chr(10), $output);
  }
 }
 // no success - output message and return false
 page_feedback('failed fetching remote file <i class="blue">'
  .htmlentities($url_suffix).'</i> from any master server - '
  .'check your internet connection!', 'a_warning');
 return false;
}

function sanitize($input, $rules = false, &$modify, $maxlen = 0)
// returns true if input conforms to rule pattern; optional modify string
// note: empty input is also returned as false
{
 if (!is_string($rules))
  $rules = 'a-zA-Z0-9_-';
 if ($maxlen == 0)
  $modify = preg_replace('/[^'.$rules.']/', '', $input);
 else
  $modify = substr(preg_replace('/[^'.$rules.']/', '', $input), 0, $maxlen);
 if ($modify == '')
  return false;
 elseif ($modify == $input)
  return true;
 else
  return false;
}

/* power cache */

function powercache_read($element = false, $serve_expired = false)
{
 global $guru;

 // read cache.bin file to $guru['powercache'] array
 if (!@is_array($guru['powercache']))
 {
  // read cache.bin file
  $raw = @file_get_contents($guru['docroot'].'/config/cache.bin');
  $arr = @unserialize($raw);
  if (!is_array($arr))
   return false;
  else
   $guru['powercache'] = $arr;
 }
 else
  $arr = $guru['powercache'];

 // check for element
 if (@isset($arr[$element]))
 {
  $expired = (@$arr[$element]['expiry'] <= time()) ? true : false;
  if ((!$expired) OR ($expired AND $serve_expired))
   return $arr[$element]['data'];
  else
   return false;
 }
 else
  return false;
}

function powercache_store($element = false, $data = false, $expiry = 5)
// stores element into powercache file with default expiration of 5 seconds
{
 global $guru;

 // check if powercache has been read first
 if (!@isset($guru['powercache']))
  powercache_read();

 // store value in $guru['powercache']
 if (($element !== false) AND ($data !== false))
  $guru['powercache'][$element] = array(
   'expiry'	=> time() + $expiry,
   'data'	=> $data
  );

 // check if cache.bin exists, if not create with root user
 if (!@file_exists($guru['docroot'].'/config/cache.bin'))
 {
  activate_library('super');
  super_execute('/usr/bin/touch '.$guru['docroot'].'/config/cache.bin');
  super_execute('/usr/sbin/chown www:www '.$guru['docroot'].'/config/cache.bin');
 }

 // now store powercache to cache.bin file
 $ser = serialize($guru['powercache']);
 file_put_contents($guru['docroot'].'/config/cache.bin', $ser);
}

function powercache_purge($element = false)
{
 global $guru;

 if ($element === false)
 {
  // purge all
  @unlink($guru['docroot'].'/config/cache.bin');
  unset($guru['powercache']);
  return true;
 }
 else
 {
  // purge specific element

  // check if powercache has been read first
  if (!@isset($guru['powercache']))
   powercache_read();

  // unset element and store cache.bin
  unset($guru['powercache'][$element]);
  powercache_store();
 }
}

/* query functions */

function guru_sysctl($sysctl_var)
// returns sysctl variable
{
 $sysctl = @trim(`/sbin/sysctl -n $sysctl_var`);
 return $sysctl;
}

/* debug functions */

function viewarray($arr)
{
 echo('<table cellpadding="0" cellspacing="0" border="1">');
 foreach ((array)$arr as $key1 => $elem1)
 {
  echo('<tr>');
  echo('<td>'.htmlentities($key1).'&nbsp;</td>');
  if (is_array($elem1))
   extarray($elem1);
  else
   echo('<td>'.nl2br(htmlentities($elem1)).'&nbsp;</td>');
  echo('</tr>');
 }
 echo('</table>');
}

function extarray($arr)
{
 echo('<td>');
 echo('<table cellpadding="0" cellspacing="0" border="1">');
 foreach ($arr as $key => $elem)
 {
  echo('<tr>');
  echo('<td>'.htmlentities($key).'&nbsp;</td>');
  if (is_array($elem))
   extArray($elem);
  else
   echo('<td>'.nl2br(htmlentities($elem)).'&nbsp;</td>');
  echo('</tr>');
 }
 echo('</table>');
 echo('</td>');
}


?>
