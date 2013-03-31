<?php

function content_network_networkquery()
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

 // queried filesystem
 $int = $interfaces[$queryif];

 // ipv4 list
 $table_ipv4 = array();
 foreach ($int['inet'] as $id => $ipdata)
  $table_ipv4[] = array(
   'IPV4_IP'		=> $ipdata['ip'],
   'IPV4_SUBNET'	=> network_netmask($ipdata['netmask']),
   'IPV4_BROADCAST'	=> $ipdata['broadcast']
  );

 // ipv6 list
 $table_ipv6 = array();
 foreach ($int['inet6'] as $id => $ipdata)
  $table_ipv6[] = array(
   'IPV6_IP'		=> $ipdata['ip'],
   'IPV6_PREFIXLEN'	=> $ipdata['prefixlen'],
   'IPV6_SCOPEID'	=> $ipdata['scopeid']
  );

 // export new tags
 $newtags = array(
  'PAGE_ACTIVETAB'		=> 'Interfaces',
  'PAGE_TITLE'			=> 'Network interface '.$queryif,
  'TABLE_NETWORK_IFLIST'	=> $iflist,
  'TABLE_NETWORK_IPV4'		=> $table_ipv4,
  'TABLE_NETWORK_IPV6'		=> $table_ipv6,
  'QUERY_IFNAME'		=> $int['ifname'],
  'QUERY_IDENT'			=> htmlentities($int['ident']),
  'QUERY_STATUS'		=> $int['status'],
  'QUERY_LINKSPEED'		=> htmlentities($int['linkspeed']),
  'QUERY_FLAGS'			=> htmlentities($int['flags_str']),
  'QUERY_CAPABILITIES'		=> htmlentities($int['options_str']),
  'QUERY_MAC'			=> $int['ether'],
  'QUERY_MTU'			=> $int['mtu']
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
