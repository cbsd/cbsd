<?php

function content_status_memory()
{
 global $guru;

 // required libraries
 activate_library('guru');

 // top output
 exec('/usr/bin/top -b -U nobody', $result);
 $topoutput = implode(chr(10), $result);
 preg_match('/^Mem: ([0-9]+[KMGT]) Active, ([0-9]+[KMGT]) Inact, '
  .'([0-9]+[KMGT]) Wired, ([0-9]+[KMGT]) Cache,( ([0-9]+[KMGT]) Buf,)? '
  .'([0-9]+[KMGT]) Free$/m', $topoutput, $topmemory);
 $top['active'] = @convertunitsize($topmemory[1]);
 $top['inact'] = @convertunitsize($topmemory[2]);
 $top['wired'] = @convertunitsize($topmemory[3]);
 $top['cache'] = @convertunitsize($topmemory[4]);
 $top['buf'] = @convertunitsize($topmemory[6]);
 $top['free'] = @convertunitsize($topmemory[7]);
 preg_match('/^Swap: ([0-9]+[KMGT]) Total,( ([0-9]+[KMGT]) Used,)? '
  .'([0-9]+[KMGT]) Free$/m', $topoutput, $swapmemory);
 $topswap['total'] = @convertunitsize($swapmemory[1]);
 $topswap['free'] = @convertunitsize($swapmemory[4]);
 $topswap['used'] = @convertunitsize($swapmemory[3]);

 // sysctl
 $mem = guru_sysctl('hw.physmem');
 $kmem = guru_sysctl('vm.kmem_size');
 $kmem_max = guru_sysctl('vm.kmem_size_max');
 $vmtotal = guru_sysctl('vm.vmtotal');

 // vmtotal breakdown
 preg_match_all('/^(Virtual|Real|Shared Virtual|Shared Real) Memory:[\s]+'
  .'\(Total: ([0-9]+)K Active: ([0-9]+)K\)$/m', $vmtotal, $vmpreg);
 preg_match('/^Free Memory Pages:[\s]+([0-9]+)K$/m', $vmtotal, $freepreg);

 // unknown memory
 $unknownmem = (int)$mem - (int)array_sum($top);

 // memory breakdown
 $mem_virtual_total = @sizebinary($vmpreg[2][0] * 1024, 1);
 $mem_virtual_active = @sizebinary($vmpreg[3][0] * 1024, 1);
 $mem_human = @sizebinary((int)$mem, 1);
 $mem_active = @sizebinary($top['active'], 1);
 $mem_inact = @sizebinary($top['inact'], 1);
 $mem_cache = @sizebinary($top['cache'], 1);
 $mem_buffer = @sizebinary($top['buf'], 1);
 $mem_kernel = @sizebinary($top['wired'], 1);
 $mem_free = @sizebinary($top['free'], 1);
 $mem_unknown = @sizebinary($unknownmem, 1);

 // percentages
 $pct_active = round(($top['active'] * 100) / $mem, 1); 
 $pct_inact = round(($top['inact'] * 100) / $mem, 1);
 $pct_cache = round(($top['cache'] * 100) / $mem, 1);
 $pct_buffer = round(($top['buf'] * 100) / $mem, 1);
 $pct_kernel = round(($top['wired'] * 100) / $mem, 1);
 $pct_free = round(($top['free'] * 100) / $mem, 1);
 $pct_unknown = round(($unknownmem * 100) / $mem, 1);

 // memory graph
 $graph['active'] = ($pct_active);
 $graph['inact'] = ($pct_inact);
 $graph['cache'] = ($pct_cache);
 $graph['buffer'] = ($pct_buffer);
 $graph['kernel'] = ($pct_kernel);
 $graph['free'] = ($pct_free);
 $graph['unknown'] = (100 - array_sum($graph));

 // swap
 $swapsize = sizebinary($topswap['total'], 1);
 $class_swap = ((int)$topswap['total'] > 0) ? 'normal' : 'hidden';
 $class_noswap = ((int)$topswap['total'] > 0) ? 'hidden' : 'normal';
 $swap = array(
  'used' => @round(($topswap['used'] * 100) / $topswap['total'], 1),
  'free' => @round(($topswap['free'] * 100) / $topswap['total'], 1)
 );

 // export tags
 return array(
  'PAGE_TITLE'		=> 'Memory usage',
  'CLASS_SWAP'		=> $class_swap,
  'CLASS_NOSWAP'	=> $class_noswap,
  'MEM_VIRTUAL_ACTIVE'	=> $mem_virtual_active,
  'MEM_VIRTUAL_TOTAL'	=> $mem_virtual_total,
  'MEM_PHYSICAL'	=> $mem_human,
  'MEM_ACTIVE'		=> $mem_active,
  'MEM_INACT'		=> $mem_inact,
  'MEM_CACHE'		=> $mem_cache,
  'MEM_BUFFER'		=> $mem_buffer,
  'MEM_KERNEL'		=> $mem_kernel,
  'MEM_FREE'		=> $mem_free,
  'MEM_UNKNOWN'		=> $mem_unknown,
  'PCT_ACTIVE'		=> $pct_active,
  'PCT_INACT'		=> $pct_inact,
  'PCT_CACHE'		=> $pct_cache,
  'PCT_BUFFER'		=> $pct_buffer,
  'PCT_KERNEL'		=> $pct_kernel,
  'PCT_FREE'		=> $pct_free,
  'PCT_UNKNOWN'		=> $pct_unknown,
  'GRAPH_ACTIVE'	=> $graph['active'],
  'GRAPH_INACT'		=> $graph['inact'],
  'GRAPH_CACHE'		=> $graph['cache'],
  'GRAPH_BUFFER'	=> $graph['buffer'],
  'GRAPH_KERNEL'	=> $graph['kernel'],
  'GRAPH_FREE'		=> $graph['free'],
  'GRAPH_UNKNOWN'	=> $graph['unknown'],
  'GRAPH_SWAPUSED'	=> $swap['used'],
  'GRAPH_SWAPFREE'	=> $swap['free'],
  'SWAP_SIZE'		=> $swapsize
 );
}

function convertunitsize($string)
// converts 350K string into integer (350 * 1024)
{
 if (strlen($string) < 1)
  return 0;
 $lastchar = $string{strlen($string)-1};
 $int = (int)substr($string, 0, -1);
 switch ($lastchar)
 {
  case "K":
   return $int * 1024;
  case "M":
   return $int * 1024 * 1024;
  case "G":
   return $int * 1024 * 1024 * 1024;
  case "T":
   return $int * 1024 * 1024 * 1024 * 1024;
  default:
   return $int;
 }
}

?>
