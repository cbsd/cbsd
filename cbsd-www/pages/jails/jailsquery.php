<?php

function content_jails_jailsquery()
{

 // required library
 activate_library('jails');

 // call function
 $interfaces = jls();

 // queried interface
 $queryj = @$_GET['query'];

 // process table IFLIST
 $iflist = array();
 foreach ($interfaces as $ifname => $ifdata)
 {
  // activerow
  if ((strlen($ifname) > 0) AND ($ifname == $queryj))
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

 // queried filesystem
 $int = $interfaces[$queryj];

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'		=> 'Interfaces',
  'PAGE_TITLE'			=> 'Network interface '.$queryj,
  'TABLE_NETWORK_IFLIST'	=> $iflist,
  'TABLE_NETWORK_IPV4'		=> $table_ipv4,
  'TABLE_NETWORK_IPV6'		=> $table_ipv6,
  'QUERY_IFNAME'		=> $int['ifname'],
  'QUERY_IDENT'			=> htmlentities($int['ident']),
  'QUERY_STATUS'		=> $int['status'],
 );
 return $newtags;
}

function network_netmask($netmask)
// returns decimal subnet from hex netmask
{
 $subnet = array();
 for ($i = 2; $i <= 8; $i = $i+2)
  $subnet[] = hexdec($netmask{$i}.$netmask{$i+1});
 return implode('.', $subnet);
}

?>
