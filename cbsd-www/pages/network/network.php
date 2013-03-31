<?php

function content_network_network()
{
 // required library
 activate_library('network');

 // call function
 $interfaces = network_interfaces();

 // queried interface
 $queryif = @$_GET['query'];

 // process table IFLIST
 $iflist = array();
 foreach ($interfaces as $ifname => $ifdata)
 {
  // activerow
  if ((strlen($ifname) > 0) AND ($ifname == $queryif))
   $activerow = 'class="activerow"';
  else
   $activerow = '';

  // ident
  $ident_maxlen = 50;
  if (@strlen($ifdata['ident']) > $ident_maxlen)
   $ident = '<acronym title="'.htmlentities($ifdata['ident']).'">'
    .substr(htmlentities($ifdata['ident']), 0, $ident_maxlen).'..</acronym>';
  else
   $ident = htmlentities($ifdata['ident']);
  // manual ident for loopback adapter
  if ($ifname == 'lo0')
   $ident = 'Loopback adapter (special system adapter)';

  $iflist[] = array(
   'IFLIST_ACTIVEROW'	=> $activerow,
   'IFLIST_IFNAME'	=> $ifname,
   'IFLIST_IDENT'	=> $ident,
   'IFLIST_IP'		=> $ifdata['ip'],
   'IFLIST_STATUS'	=> $ifdata['status'],
   'IFLIST_MTU'		=> $ifdata['mtu'],
   'IFLIST_MAC'		=> $ifdata['ether']
  );
 }

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'		=> 'Interfaces',
  'PAGE_TITLE'			=> 'Network interfaces',
  'TABLE_NETWORK_IFLIST'	=> $iflist
 );
 return $newtags;
}

?>
