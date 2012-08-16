<?php
/*
	system.inc
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard (olivier@freenas.org).
	All rights reserved.

	Portions of m0n0wall (http://m0n0.ch/wall).
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met: 

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer. 
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies, 
	either expressed or implied, of the NAS4Free Project.
*/
require_once("functions.inc");
require_once("util.inc");
require_once("rc.inc");

function system_reboot() {
	// Initiate halt. Everything will be done automatically
	// in /etc/rc.shutdown by executing rc-init scripts in
	// reverse order (the 'KEYWORD: shutdown' must be defined).
	mwexec("/sbin/shutdown -r now");
}

function system_halt() {
	// Initiate halt. Everything will be done automatically
	// in /etc/rc.shutdown by executing rc-init scripts in
	// reverse order (the 'KEYWORD: shutdown' must be defined).
	mwexec("/sbin/shutdown -p now");
}

/* Init language environment */
function system_language_load()
{
	global $config, $g_languages;

	/* Get the language configured. */
	$language = $config['system']['language'];
	$domain = strtolower(get_product_name());
	$codeset = $g_languages[$language]['codeset'];

	putenv("LANG=$language");
	setlocale(LC_MESSAGES, $language);
	bindtextdomain($domain, "/usr/local/share/locale");
	bind_textdomain_codeset($domain, $codeset);
	textdomain($domain);
}

/* Get the codeset of the current configured language. */
/* Return: String containing codeset of current laguage. */
function system_get_language_codeset() {
	global $config, $g_languages;

	$language = $config['system']['language'];
	$codeset = $g_languages[$language]['codeset'];

	if (empty($codeset))
		$codeset = "ISO-8859-1"; // Set default codeset.

	return $codeset;
}

/* Get the entity of LanguageCode of the current configured language. */
/* Return: String containing language code of current laguage. */
function system_get_language_code() {
	global $config, $g_languages;

	// a language code, as per [RFC3066]
	$language = $config['system']['language'];
	//$code = $g_languages[$language]['code'];
	$code = str_replace("_", "-", $language);

	if (empty($code))
		$code = "en-us"; // Set default code.

	return $code;
}

// Get list of available groups from /etc/group.
// Result:
//	Array (
//		[wheel] => 0
//		[sshd] => 22
//		[www] => 80
//		[nobody] => 65534
//		[admin] => 1000
//		...
//	)
function system_get_group_list() {
	// List of groups to filter from result list.
	$filterlist = array("_dhcp", "_pflogd");

	$grouplist = array();

	preg_match_all("/(\S+):\*:(\d+):.*\n/", file_get_contents("/etc/group"), $matches, PREG_SET_ORDER);

	if (is_array($matches)) {
		foreach ($matches as $group) {
			if (false === in_array($group[1], $filterlist)) {
				$grouplist[$group[1]] = $group[2];
			}
		}

		ksort($grouplist);
	}

	return $grouplist;
}

// Get list of available users from /etc/master.passwd.
// Result:
//  Array (
//    [test] => Array ( [name] => test
//                      [password] => $1$yuQLaTPN$lkwYlZEB7B8n85flXPkHd0
//                      [uid] => 1001
//                      [gid] => 1001
//                      [class] =>
//                      [change] => 0
//                      [expire] => 0
//                      [gecos] => test
//                      [home_dir] => /mnt
//                      [shell] => /usr/local/bin/scponly
//                    )
//    [root] => ...
//    [toor] => ..
//    [daemon] => ...
//    [operator] => ...
//    ...
//  )
function system_get_user_list() {
	$userlist = array();

	foreach (explode("\n", file_get_contents("/etc/master.passwd")) as $userinfov) {
		if (empty($userinfov))
			continue;

		// Extract user information
		$userinfo = explode(":", $userinfov);

		$user = array();
		$user['name'] = $userinfo[0];
		$user['password'] = $userinfo[1];
		$user['uid'] = $userinfo[2];
		$user['gid'] = $userinfo[3];
		$user['class'] = $userinfo[4];
		$user['change'] = $userinfo[5];
		$user['expire'] = $userinfo[6];
		$user['gecos'] = $userinfo[7];
		$user['home_dir'] = $userinfo[8];
		$user['shell'] = $userinfo[9];

		$userlist[$user['name']] = $user;
	}

	return $userlist;
}

// SMBIOS get various info
function get_smbios_info() {
	$smbios = array();
	$cmd = "/bin/kenv -q";
	unset($rawdata, $rc);
	mwexec2($cmd, $rawdata, $rc);
	if ($rc != 0) {
		return $smbios;
	}
	foreach ($rawdata as $line) {
		if (preg_match("/^smbios\.([^\.=]+)\.([^\.=]+)=\"(.*)\"/", $line, $match)) {
			$smbios[$match[1]][$match[2]] = $match[3];
		}
	}
	return $smbios;
}

// Get number of CPUs
function system_get_cpus() {
	$cpus = exec("/sbin/sysctl -q -n kern.smp.cpus");
	if ($cpus == '' || $cpus < 1) $cpus = 1;
	return $cpus;
}

// Get current CPU usage.
// Return current CPU usage in percent.
function cb_map_array_diff2($a, $b) {
	return $b - $a;
}
function system_get_smp_cpu_usage() {
	$cpus = system_get_cpus();
	$a_stat1 = explode(" ", exec("/sbin/sysctl -q -n kern.cp_times"));
	sleep(1);
	$a_stat2 = explode(" ", exec("/sbin/sysctl -q -n kern.cp_times"));
	$a_dstat = array_map("cb_map_array_diff2", $a_stat1, $a_stat2);

	$a_usage = array_fill_keys(range(0, $cpus - 1), 0);
	for ($i = 0; $i < $cpus; $i++) {
		// user, nice, system, interrupt, idle
		$a = array_splice($a_dstat, 0, 5);
		$idle = $a[4];
		$total = array_sum($a);
		$a_usage[$i] = floor(100 * (($total - $idle) / $total));
	}
	return $a_usage;
}

function system_get_cpu_usage() {
	$cpuTicks1 = explode(" ", `/sbin/sysctl -n kern.cp_time`);
	sleep(1);
	$cpuTicks2 = explode(" ", `/sbin/sysctl -n kern.cp_time`);

	$diff = array();
	$diff['user'] = ($cpuTicks2[0] - $cpuTicks1[0]);
	$diff['nice'] = ($cpuTicks2[1] - $cpuTicks1[1]);
	$diff['sys'] = ($cpuTicks2[2] - $cpuTicks1[2]);
	$diff['intr'] = ($cpuTicks2[3] - $cpuTicks1[3]);
	$diff['idle'] = ($cpuTicks2[4] - $cpuTicks1[4]);

	$totalDiff = $diff['user'] + $diff['nice'] + $diff['sys'] + $diff['intr'] + $diff['idle'];
	$totalused = $diff['user'] + $diff['nice'] + $diff['sys'] + $diff['intr'];

	if (isset($totalused) && $totalused <= 0) {
		$totalused = 0.001;
	}

	return floor(100 * ($totalused / $totalDiff));
}

// Get CPUID
function system_get_cpuid($cpuidx, $level) {
	$r = explode(" ", exec("/usr/sbin/cpucontrol -i $level /dev/cpuctl${cpuidx} | awk '{ print $4, $5, $6, $7 }'"));
	$a = array();
	$a['eax'] = $r[0];
	$a['ebx'] = $r[1];
	$a['ecx'] = $r[2];
	$a['edx'] = $r[3];
	return $a;
}

// Get various CPU informations.
// Result:
//  Array (
//    [number] => 1
//    [model] => AMD Athlon(tm) 64 Processor 3200+
//    [clockrate] => 1994
//    [temperature] => 65C !!! May be empty if sensor is not available. !!!
//    [freq] => 995
//    [freqlevels] => 1990/67000 1791/64700 995/28600
//  )
function system_get_cpu_info() {
	$cpuinfo = array();

	$cpuinfo['number'] = chop(`/sbin/sysctl -n hw.ncpu`);
	$cpuinfo['model'] = chop(`/sbin/sysctl -n hw.model`);
	$cpuinfo['clockrate'] = chop(`/sbin/sysctl -n hw.clockrate`);
	$cpuinfo['temperature'] = chop(`/sbin/sysctl -n hw.acpi.thermal.tz0.temperature`);
	$cpus = system_get_cpus();
	$cpuinfo['temperature2'] = array();
	$temp = exec("/sbin/sysctl -q -n dev.cpu.0.temperature");
	if ($temp != '') {
		$adj = 0;
		$cpuid = system_get_cpuid(0, "0x0");
		// AuthenticAMD ?
		if ($cpuid['ebx'] == 0x68747541 && $cpuid['ecx'] == 0x444d4163 && $cpuid['edx'] == 0x69746e65) {
			$cpuid2 = system_get_cpuid(0, "0x80000001");
			// Brisbane BH-G1/BH-G2 ?
			if ($cpuid2['eax'] == 0x00060fb1 || $cpuid2['eax'] == 0x00060fb2) {
// for freebsd 9.x $adj = 21.0; set to $adj = 0.0; | $adj = 21.0; is needed on freebsd 7 series!
				$adj = 0.0;
			}

		}
		for ($i = 0; $i < $cpus; $i++) {
			$temp = exec("/sbin/sysctl -q -n dev.cpu.{$i}.temperature");
			if ($temp != '') {
				$temp = sprintf("%.1f", ($temp + $adj));
			}
			$cpuinfo['temperature2'][$i] = $temp;
		}
	}
	$cpuinfo['freq'] = chop(`/sbin/sysctl -n dev.cpu.0.freq`);
	$cpuinfo['freqlevels'] = chop(`/sbin/sysctl -n dev.cpu.0.freq_levels`);

	return $cpuinfo;
}

/**
 * Get the system uptime (how long the system is already running).
 * @return The current uptime as string.
 */
function system_get_uptime() {
	exec("/sbin/sysctl -n kern.boottime", $boottime);
	preg_match("/sec = (\d+)/", $boottime[0], $matches);
	$boottime = $matches[1];
	$uptime = time() - $boottime;

	if ($uptime > 60) $uptime += 30;
	$updays = (int)($uptime / 86400);
	$uptime %= 86400;
	$uphours = (int)($uptime / 3600);
	$uptime %= 3600;
	$upmins = (int)($uptime / 60);
	$upsecs = $uptime % 60;

	if (($updays < 1) && ($uphours < 1)) {
		$uptime = sprintf("%d %s %d %s", $upmins, gettext("minute(s)"), $upsecs, gettext("second(s)"));
	} else if ($updays < 1) {
		$uptime = sprintf("%d %s %d %s %d %s", $uphours, gettext("hour(s)"),
			$upmins, gettext("minute(s)"), $upsecs, gettext("second(s)"));
	} else {
		$uptime = sprintf("%d %s %d %s %d %s %d %s", $updays, gettext("day(s)"),
			$uphours, gettext("hour(s)"), $upmins, gettext("minute(s)"), $upsecs,
			gettext("second(s)"));
	}

	return htmlspecialchars($uptime);
}

// Get the current RAM information.
// Returns an array listing the amount of memory installed in the hardware.
function system_get_ram_info() {
	exec("/sbin/sysctl -n vm.stats.vm.v_active_count vm.stats.vm.v_inactive_count vm.stats.vm.v_wire_count vm.stats.vm.v_cache_count vm.stats.vm.v_free_count hw.physmem", $memory);
	exec("/sbin/sysctl -n hw.realmem", $hwmemory);

	$raminfo = array();
	$raminfo['real'] = $hwmemory[0];
	$raminfo['physical'] = $memory[5];
	$raminfo['total'] = $memory[0] + $memory[1] + $memory[2] + $memory[3] + $memory[4];
	$raminfo['free'] = $memory[4] + $memory[1];
	$raminfo['used'] = $raminfo['total'] - $raminfo['free'];

	return $raminfo;
}

// Get the current swap information.
// Result:
//  Array (
//    [device] => "/dev/ad0s2b"
//    [total] => "20M"
//    [used] => "0B"
//    [avail] => "20M"
//    [capacity] => "0%"
//  )
function system_get_swap_info() {
	exec("/usr/sbin/swapinfo", $swap);

	// blocks
	$val = 512;
	if (preg_match("/(?:Device)(?:\s+)(\d+)(?:k-blocks|-blocks)(?:\s+)(?:Used)(?:\s+)(?:Avail)(?:\s)(?:Capacity)/", $swap[0], $matches)) {
		if ($matches[1] == 1) {
			$val = $matches[1] * 1024;
		} else {
			$val = $matches[1];
		}
	}

	array_shift($swap);
	$swapinfo = array();

	foreach ($swap as $swapv) {
		if (preg_match("/(\/dev\/.*)(?:\s+)(\d+)(?:\s+)(\d+)(?:\s+)(\d+)(?:\s)(.+)/", $swapv, $matches)) {
			$devswap = array();
			$devswap['device'] = trim($matches[1]);
			$devswap['total'] = decode_size(trim($matches[2])*$val);
			$devswap['used'] = decode_size(trim($matches[3])*$val);
			$devswap['avail'] = decode_size(trim($matches[4])*$val);
			$devswap['capacity'] = trim($matches[5]);

			$swapinfo[] = $devswap;
		}
	}

	return $swapinfo;
}

/**
 * Get the system hostname.
 * @return The hostname
 */
function system_get_hostname() {
	return exec("hostname");
}

/**
 * Get device I/O statistics.
 * @param[in] device	The device name, e.g. ad1
 * @return	An array containing the I/O statistics kpt (kilobytes per transfer),
 *					tps (transfers per second) and mps (megabytes per second). On
 *					failure, FALSE will be returned.
 * @code
 * array(
 *   [kpt] => 11.93
 *   [tps] => 61
 *   [mps] => 0.71)
 * @endcode
 */
function system_get_device_iostat($device) {
	$result = FALSE;

	mwexec2("iostat -d {$device}", $rawdata);
	if (preg_match("/^\s*(\S+)\s*(\S+)\s*(\S+)/", $rawdata[2], $matches)) {
		$result = array();
		$result['kpt'] = $matches[1];
		$result['tps'] = $matches[2];
		$result['mps'] = $matches[3];
	}

	return $result;
}

/**
 * Get device temperature.
 * @param[in] device	The device name, e.g. ad1
 * @return	The temperature in C if available, otherwise FALSE.
 */
function system_get_device_temp($device) {
	$result = FALSE;

	/* without smart */
	mwexec2("ataidle /dev/{$device} | grep 'SMART Enabled'", $rawdata);
	if (1 === preg_match("/^SMART Enabled:\s+(\S+)/", $rawdata[0], $matches)) {
		if (strcasecmp($matches[1], "no") == 0) {
			/* SMART disabled */
			return $result;
		}
	}
	/* with smart */
	mwexec2("smartctl -A /dev/{$device}", $rawdata);
	foreach ($rawdata as $rawdatav) {
		$arawdatav = preg_split("/\s+/", $rawdatav);
		if ((0 == strncmp(trim($arawdatav[1]), "Temperature_", 12)) &&
			(0 != strcmp(trim($arawdatav[0]), "190"))) {
			$result = chop($arawdatav[9]);
			break;
		} else if (preg_match("/Current Drive Temperature:\s+(\d+) C.*/", $rawdatav, $matches)) {
			$result = $matches[1];
			break;
		}
	}

	return $result;
}

/**
 * Get volume serial number.
 * @param[in] device	The device name, e.g. /dev/ad1
 * @return The disk device serial number.
 */
function system_get_volume_serial($devicefile) {
	/* without smart */
	mwexec2("ataidle {$devicefile} | grep 'Serial'", $rawdata);
	if (1 === preg_match("/^Serial:\s+(.+)$/", $rawdata[0], $matches))
		return $matches[1];
	/* failback to old method */
	mwexec2("smartctl -i {$devicefile} | grep 'Serial Number'", $rawdata);
	if (1 !== preg_match("/^Serial Number:\s+(.+)$/", $rawdata[0], $matches))
		return gettext("n/a");
	return $matches[1];
}

/**
 * Get volume device model.
 * @param[in] device	The device name, e.g. /dev/ad1
 * @return The disk device model.
 */
function system_get_volume_model($devicefile) {
	/* without smart */
	mwexec2("ataidle {$devicefile} | grep 'Model'", $rawdata);
	if (1 === preg_match("/^Model:\s+(.+)$/", $rawdata[0], $matches))
		return $matches[1];
	/* failback to old method */
	mwexec2("smartctl -i {$devicefile} | grep 'Device Model'", $rawdata);
	if (1 !== preg_match("/^Device Model:\s+(.+)$/", $rawdata[0], $matches))
		return gettext("n/a");
	return $matches[1];
}

/**
 * Get mount point usage information.
 * @return An array containg the requested informations:
 * @code
 * [mountpoint] => array(
 *   [mountpoint] => /mnt/xyz
 *   [name] => xyz
 *   [filesystem] => /dev/ad0a
 *   [capacity] => 48%
 *   [used] => 2.4G
 *   [avail] => 2.6G
 *   [size] => 5.4G)
 * @endcode
 */
function system_get_mount_usage() {
	global $config, $g;

	$result = array();

	exec("/bin/df -h", $rawdata);
	foreach ($rawdata as $line) {
		if (0 == preg_match("/^(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\d+%)\s+(.+)/", $line, $aline))
			continue;

		$filesystem = chop($aline[1]);
		$size = chop($aline[2]);
		$used = chop($aline[3]);
		$avail = chop($aline[4]);
		$capacity = chop($aline[5]);
		$mountpoint = chop($aline[6]);

		if (is_array($config['mounts'])) {
			foreach ($config['mounts']['mount'] as $mountcfg) {
				if (0 == strcmp($mountpoint, "{$g['media_path']}/{$mountcfg['sharename']}")) {
					$result[$mountpoint] = array();
					$result[$mountpoint]['mountpoint'] = $mountpoint;
					$result[$mountpoint]['name'] = $mountcfg['sharename'];
					$result[$mountpoint]['filesystem'] = $filesystem;
					$result[$mountpoint]['capacity'] = $capacity;
					$result[$mountpoint]['avail'] = $avail;
					$result[$mountpoint]['used'] = $used;
					$result[$mountpoint]['size'] = $size;
				}
			}
		}
	}

	return $result;
}

/**
 * Get various system informations.
 * @return An array containing the system informations.
 */
function system_get_sysinfo() {
	$value['hostname'] = system_get_hostname();

	// Get CPU counts.
	$cpus = system_get_cpus();
	$value['cpus'] = $cpus;

	// Get uptime and date.
	$value['uptime'] = system_get_uptime();
	$value['date'] = shell_exec("date");

	// Get RAM usage.
	$raminfo = system_get_ram_info();
	$percentage = round(($raminfo['used'] * 100) / $raminfo['total'], 0);
	$value['memusage']['percentage'] = $percentage;
	$value['memusage']['caption'] = sprintf(gettext("%d%% of %dMiB"), $percentage, round($raminfo['physical'] / 1024 / 1024));

	// Get load average.
	exec("uptime", $result);
	$value['loadaverage'] = substr(strrchr($result[0], "load averages:"), 15);
	// Get up-to-date CPU informations.
	$cpuinfo = system_get_cpu_info();
	$value['cputemp'] = $cpuinfo['temperature'];
	$value['cpufreq'] = $cpuinfo['freq'];
	if (!empty($cpuinfo['temperature2'])) {
		$value['cputemp2'] = $cpuinfo['temperature2'];
	}

	// Get CPU usage.
	if ($cpus > 1) {
		$value['cpuusage2'] = system_get_smp_cpu_usage();
		$value['cpuusage'] = floor(array_sum($value['cpuusage2']) / $cpus);
	} else {
		$value['cpuusage'] = system_get_cpu_usage();
		$value['cpuusage2'] = array($value['cpuusage']);
	}

	// Get disk usage.
	$a_diskusage = system_get_mount_usage();
	if (is_array($a_diskusage) && (0 < count($a_diskusage))) {
		foreach ($a_diskusage as $diskusagek => $diskusagev) {
			$fsid = get_mount_fsid($diskusagev['filesystem'], $diskusagek);
			$diskinfo = array();
			$diskinfo['id'] = $fsid;
			$diskinfo['mountpoint'] = $diskusagev['mountpoint'];
			$diskinfo['name'] = $diskusagev['name'];
			$diskinfo['capacity'] = $diskusagev['capacity'];
			$diskinfo['percentage'] = rtrim($diskusagev['capacity'], "%");
			$diskinfo['size'] = $diskusagev['size'];
			$diskinfo['used'] = $diskusagev['used'];
			$diskinfo['avail'] = $diskusagev['avail'];
			$diskinfo['tooltip']['used'] = sprintf(gettext("%sB used of %sB"), $diskusagev['used'], $diskusagev['size']);
			$diskinfo['tooltip']['available'] = sprintf(gettext("%sB available of %sB"), $diskusagev['avail'], $diskusagev['size']);

			$value['diskusage'][] = $diskinfo;
		}
	}

	// Get swap info.
	$swapinfo = system_get_swap_info();
	if (is_array($swapinfo) && (0 < count($swapinfo))) {
		$id = 0;
		foreach ($swapinfo as $swapv) {
			$swapinfo = array();
			$swapinfo['id'] = $id++;
			$swapinfo['device'] = $swapv['device'];
			$swapinfo['capacity'] = $swapv['capacity'];
			$swapinfo['percentage'] = rtrim($swapv['capacity'], "%");
			$swapinfo['total'] = $swapv['total'];
			$swapinfo['used'] = $swapv['used'];
			$swapinfo['avail'] = $swapv['avail'];
			$swapinfo['tooltip']['used'] = sprintf(gettext("%sB used of %sB"), $swapv['used'], $swapv['total']);
			$swapinfo['tooltip']['available']  = sprintf(gettext("%sB available of %sB"), $swapv['avail'], $swapv['total']);

			$value['swapusage'][]= $swapinfo;
		}
	}

	return $value;
}
?>
