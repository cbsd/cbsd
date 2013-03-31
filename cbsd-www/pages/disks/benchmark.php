<?php

function content_disks_benchmark()
{
 // required library
 activate_library('service');

 // benchmark output file
 $benchpath = 'benchmarks/';
 $outputfile = $benchpath.'benchmarkoutput.dat';
 $benchoutput = @htmlentities(file_get_contents($outputfile));
 $refresh_sec = 20;

 // check whether benchmark in progress
 $bip = service_isprocessrunning('benchmark.php');

 // benchmark in progress
 if ($bip)
 {
  // set automatic refresh
  page_refreshinterval($refresh_sec);
  // visible classes
  $class_inprogress = 'normal';
  $class_completed = 'hidden';
  $class_interrupted = 'hidden';
  $class_newbench = 'hidden';
 }
 else
 {
  // required library
  activate_library('html');

  // call external function for memberdisks
  $memberdisks = html_memberdisks();

  // check whether a benchmark was completed
  $completed = false;
  if ((@file_exists($benchpath.'bench_seqread.png')) OR
      (@file_exists($benchpath.'bench_raidtest.read.png')))
   $completed = true;

  // check whether a benchmark was interrupted
  $interrupted = false;
  if ((@file_exists($benchpath.'running_seqread.png')) OR
      (@file_exists($benchpath.'running_raidtest.read.png')))
   $interrupted = true;
  elseif ((!$completed) AND (strlen($benchoutput) > 0))
   $interrupted = true;

  // visible classes
  $class_inprogress = 'hidden';
  $class_completed = ($completed) ? 'normal' : 'hidden';
  $class_interrupted = ($interrupted) ? 'normal' : 'hidden';
  $class_newbench = 'normal';
 }

 // display benchmark images if applicable
 $class_bench_seq = ($benchpath.'bench_seqread.png') ? 'normal' : 'hidden';
 $class_bench_random = ($benchpath.'bench_raidtest.read.png') 
  ? 'normal' : 'hidden';

 // export new tags
 return array(
  'PAGE_ACTIVETAB'		=> 'Benchmark',
  'PAGE_TITLE'			=> 'Benchmark',
  'CLASS_INPROGRESS'		=> $class_inprogress,
  'CLASS_COMPLETED'		=> $class_completed,
  'CLASS_INTERRUPTED'		=> $class_interrupted,
  'CLASS_NEWBENCHMARK'		=> $class_newbench,
  'CLASS_BENCH_SEQ'		=> $class_bench_seq,
  'CLASS_BENCH_RANDOM'		=> $class_bench_random,
  'BENCHMARK_OUTPUT'		=> $benchoutput,
  'BENCHMARK_MEMBERDISKS'	=> @$memberdisks
 );
}

function submit_disks_benchmark_start()
{
 global $guru;

 // required libraries
 activate_library('guru');
 activate_library('service');
 activate_library('super');

 // fetch current system version
 $currentver = guru_fetch_current_systemversion();

 // redirect url
 $url = 'disks.php?benchmark';

 // construct data array
 $data = array('disks' => array());
 $data['magic_string'] = $guru['benchmark_magic_string'];
 $len = strlen('addmember_');
 foreach (@$_POST as $name => $value)
  if ((substr($name, 0, $len) == 'addmember_') AND ($value == 'on'))
   $data['disks'][] = trim(substr($name, $len));
 if (@$_POST['test_seq'] == 'on')
  $data['tests']['sequential'] = true;
 if (@$_POST['test_rio'] == 'on')
  $data['tests']['randomio'] = true;
 // sanity
 if (empty($data['disks']))
  friendlyerror('no disks were selected for testing!', $url);
 if ((!@$data['tests']['sequential']) AND (!@$data['tests']['randomio']))
  friendlyerror('no tests were selected, please select at least one test!', 
   $url);
 $data['testsize_gib'] = @$_POST['testsize_gib'];
 $data['testrounds'] = (int)$_POST['testrounds'];
 $data['cooldown'] = (int)$_POST['cooldown'];
 $data['seq_blocksize'] = (int)$_POST['seq_blocksize'];
 $data['rio_requests'] = (int)$_POST['rio_requests'];
 $data['rio_scalezvol'] = (@$_POST['rio_scalezvol']) ? true : false;
 $data['rio_alignment'] = (int)$_POST['rio_alignment'];
 $data['rio_queuedepth'] = (int)$_POST['rio_queuedepth'];
 $data['sectorsize_override'] = (int)$_POST['sectorsize_override'];
 $data['secure_erase'] = (@$_POST['secure_erase'] == 'on') ? true : false;

 // kill powerd daemon for accurate frequency scanning
 service_manage_rc('powerd', 'stop', true);
 usleep(1000);

 // sysinfo
 $data['sysinfo'] = array();
 $data['sysinfo']['product_name'] = $guru['product_name'];
 $data['sysinfo']['product_version'] = $guru['product_version_string'];
 $data['sysinfo']['distribution'] = $currentver['dist'];
 $data['sysinfo']['system_version'] = $currentver['sysver'];
 $data['sysinfo']['cpu_name'] = trim(`sysctl -n hw.model`);
 $data['sysinfo']['cpu_count'] = (int)(`sysctl -n hw.ncpu`);
 $freq_ghz = (int)(`sysctl -n dev.cpu.0.freq`) / 1000;
 $data['sysinfo']['cpu_freq_ghz'] = @number_format($freq_ghz, 1);
 $physmem_gib = (int)(`sysctl -n hw.physmem`) / (1024 * 1024 * 1024);
 $data['sysinfo']['physmem_gib'] = @number_format($physmem_gib, 1);
 $kmem_gib = (int)(`sysctl -n vm.kmem_size`) / (1024 * 1024 * 1024);
 $data['sysinfo']['kmem_gib'] = @number_format($kmem_gib, 1);
 if (@$_SESSION['loaderconf_needreboot'] === true)
  $data['sysinfo']['contamination'] = true;
 else
  $data['sysinfo']['contamination'] = false;

 // serialize and write array to file
 $serial = serialize($data);
 $filename = trim(`realpath .`).'/benchmarks/startbenchmark.dat';
 exec('/bin/rm '.$filename);
 $result = file_put_contents($filename, $serial, LOCK_EX);
 if ($result === false)
  error('Could not write benchmark data file');
 usleep(1000);

 // execute benchmark
 $filename2 = './benchmarks/benchmarkoutput.dat';
 exec('/bin/rm '.$filename2);
 $command = $guru['docroot'].'/benchmark.php startbenchmark '
  .'> '.$filename2.' 2>&1 &';
 $result = super_execute($command);
 if ($result['rv'] != 0)
  error('could not start benchmark process ('.(int)$rv.')');
 sleep(1);

 // redirect
 redirect_url($url);
}

function submit_disks_benchmark_stop()
{
 global $guru;

 // elevated privileges
 activate_library('super');

 // redirect url
 $url = 'disks.php?benchmark';

 // stop processes
 exec('/bin/ps xw | grep "benchmark.php|raidtest" | grep -v grep', 
  $output, $rv);
 if ($rv == 0)
 {
  $pids = array();
  foreach ($output as $line)
   if (is_numeric(substr($line,0,strpos($line, ' '))))
    $pids[] = (int)substr($line,0,strpos($line, ' '));
  foreach ($pids as $pid)
   super_execute('/bin/kill '.(int)$pid);
 }

 // oldschool kill everything
 super_execute('/usr/bin/killall benchmark.php raidtest dd');
 usleep(50000);

 // import and delete test pool as well
 super_execute('/sbin/zpool import -f '.$guru['benchmark_poolname']);
 usleep(1000);
 super_execute('/sbin/zpool destroy -f '.$guru['benchmark_poolname']);
 usleep(100000);

 // redirect
 redirect_url($url);
}

?>
