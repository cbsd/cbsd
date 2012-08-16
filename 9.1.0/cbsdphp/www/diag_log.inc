<?php
/*
	diag_log.inc

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.
	
	Modified for XHTML by Daisuke Aoyama <aoyama@peach.ne.jp>
	Copyright (C) 2010 Daisuke Aoyama <aoyama@peach.ne.jp>.	
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	Copyright (C) 2009 Volker Theile <votdev@gmx.de>.
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
require_once("globals.inc");
require_once("rc.inc");

// Get the location of the log files (may be modified via rc.conf variables).
$clogdir = rc_getenv_ex("clog_logdir", $g['varlog_path']);
$rsync_client_logfile = rc_getenv_ex("rsync_client_logfile", "{$g['varlog_path']}/rsync_client.log");
$rsync_local_logfile = rc_getenv_ex("rsync_local_logfile", "{$g['varlog_path']}/rsync_local.log");
$fuppes_logfile = rc_getenv_ex("fuppes_logfile", "{$g['varlog_path']}/fuppes.log");
$mtdaapd_logfile = rc_getenv_ex("mtdaapd_logfile", "{$g['varlog_path']}/mt-daapd.log");
$proftpd_modban_logfile = rc_getenv_ex("proftpd_modban_banlog", "{$g['varlog_path']}/proftpd/ban.log");

// The log configuration
$loginfo = array(
	array(
		"visible" => TRUE,
		"desc" => gettext("System"),
		"logfile" => "{$clogdir}/system.log",
		"filename" => "system.log",
		"type" => "clog",
		"size" => "262144",
		"pattern" => "/^(\S+\s+\d+\s+\S+)\s+(\S+)\s+(.*)$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("User"), "class" => "listr", "param" => "nowrap=\"nowrap\"", "pmid" => 2),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 3)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("SSH"),
		"logfile" => "{$clogdir}/sshd.log",
		"filename" => "sshd.log",
		"type" => "clog",
		"size" => "32768",
		"pattern" => "/^(\S+\s+\d+\s+\S+)\s+(\S+)\s+(.*)$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("User"), "class" => "listr", "param" => "nowrap=\"nowrap\"", "pmid" => 2),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 3)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("Daemon"),
		"logfile" => "{$clogdir}/daemon.log",
		"filename" => "daemon.log",
		"type" => "clog",
		"size" => "32768",
		"pattern" => "/^(\S+\s+\d+\s+\S+)\s+(\S+)\s+(.*)$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("User"), "class" => "listr", "param" => "nowrap=\"nowrap\"", "pmid" => 2),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 3)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("FTP"),
		"logfile" => "{$clogdir}/ftp.log",
		"filename" => "ftp.log",
		"type" => "clog",
		"size" => "32768",
		"pattern" => "/^(\S+\s+\d+\s+\S+)\s+(\S+)\s+(.*)$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("User"), "class" => "listr", "param" => "nowrap=\"nowrap\"", "pmid" => 2),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 3)
		)),
	array(
		"visible" => isset($config['ftpd']['mod_ban']['enable']),
		"desc" => gettext("FTP - Ban module"),
		"logfile" => $proftpd_modban_logfile,
		"filename" => "ban.log",
		"type" => "plain",
		"pattern" => "/^(\S+\s+\d+\s+\S+)\s+(.*)$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 2)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("FTP - File transfer"),
		"logfile" => "{$g['varlog_path']}/xferlog",
		"filename" => "xferlog",
		"type" => "plain",
		"pattern" => "/^(\S+\s+\S+\s+\d+\s+\S+\s+\d+) ((\d+) (\S+) (\d+) (\S+) (a|b) (C|U|T|_) (o|i|d) (a|g|r) (\S+) (\S+) (0|1) (\S+) (c|i))$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("IP"), "class" => "listr", "param" => "", "pmid" => 4),
			array("title" => gettext("User"), "class" => "listr", "param" => "", "pmid" => 11),
//		array("title" => gettext("File"), "class" => "listr", "param" => "", "pmid" => 6),
//		array("title" => gettext("Filesize"), "class" => "listr", "param" => "", "pmid" => 5),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 2)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("S.M.A.R.T."),
		"logfile" => "{$clogdir}/smartd.log",
		"filename" => "smartd.log",
		"type" => "clog",
		"size" => "32768",
		"pattern" => "/^(\S+\s+\d+\s+\S+)\s+(\S+)\s+(.*)$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("User"), "class" => "listr", "param" => "nowrap=\"nowrap\"", "pmid" => 2),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 3)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("RSYNC - Server"),
		"logfile" => "{$clogdir}/rsyncd.log",
		"filename" => "rsyncd.log",
		"type" => "clog",
		"size" => "32768",
		"pattern" => "/^(\S+\s+\d+\s+\S+)\s+(\S+)\s+(.*)$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("User"), "class" => "listr", "param" => "nowrap=\"nowrap\"", "pmid" => 2),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 3)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("RSYNC - Client"),
		"logfile" => $rsync_client_logfile,
		"filename" => "rsync_client.log",
		"type" => "plain",
		"pattern" => "/^(.*)$/",
		"columns" => array(
			array("title" => gettext("Event"), "class" => "listlr", "param" => "", "pmid" => 1)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("RSYNC - Local"),
		"logfile" => $rsync_local_logfile,
		"filename" => "rsync_local.log",
		"type" => "plain",
		"pattern" => "/^(.*)$/",
		"columns" => array(
			array("title" => gettext("Event"), "class" => "listlr", "param" => "", "pmid" => 1)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("UPnP"),
		"logfile" => $fuppes_logfile,
		"filename" => "upnp.log",
		"type" => "plain",
		"pattern" => "/^(.*)$/",
		"columns" => array(
			array("title" => gettext("Event"), "class" => "listlr", "param" => "", "pmid" => 1)
		)),
	array(
		"visible" => TRUE,
		"desc" => gettext("DAAP"),
		"logfile" => $mtdaapd_logfile,
		"filename" => "daap.log",
		"type" => "plain",
		"pattern" => "/^(\S+\s+\S+)\s+\S+:\s+(.*)$/",
		"columns" => array(
			array("title" => gettext("Date & Time"), "class" => "listlr", "param" => "nowrap=\"nowrap\"", "pmid" => 1),
			array("title" => gettext("Event"), "class" => "listr", "param" => "", "pmid" => 2)
		))
);

function log_get_contents($logfile, $type) {
	global $config;

	$content = array();

	$param = (isset($config['syslogd']['reverse']) ? "-r " : "");
	$param .= "-n " . $config['syslogd']['nentries'];

	switch ($type) {
		case "clog":
			exec("/usr/sbin/clog {$logfile} | /usr/bin/tail {$param}", $content);
			break;

		case "plain":
			exec("/bin/cat {$logfile} | /usr/bin/tail {$param}", $content);
	}

	return $content;
}

function log_display($loginfo) {
	if (!is_array($loginfo))
		return;

	// Create table header
	echo "<tr>";
	foreach ($loginfo['columns'] as $columnk => $columnv) {
		echo "<td {$columnv['param']} class='" . (($columnk == 0) ? "listhdrlr" : "listhdrr") . "'>".htmlspecialchars($columnv['title'])."</td>\n";
	}
	echo "</tr>";

	// Get log file content
	$content = log_get_contents($loginfo['logfile'], $loginfo['type']);
	if (empty($content))
		return;

	// Create table data
	foreach ($content as $contentv) {
		// Skip invalid pattern matches
		$result = preg_match($loginfo['pattern'], $contentv, $matches);
		if ((FALSE === $result) || (0 == $result))
			continue;

		// Skip empty lines
		if (count($loginfo['columns']) == 1 && empty($matches[1]))
			continue;

		echo "<tr valign=\"top\">\n";
		foreach ($loginfo['columns'] as $columnk => $columnv) {
			echo "<td {$columnv['param']} class='{$columnv['class']}'>" . htmlspecialchars($matches[$columnv['pmid']]) . "</td>\n";
		}
		echo "</tr>\n";
	}
}

function log_clear($loginfo) {
	if (!is_array($loginfo))
		return;

	switch ($loginfo['type']) {
		case "clog":
			exec("/usr/sbin/clog -i -s {$loginfo['size']} {$loginfo['logfile']}");
			break;

		case "plain":
			exec("/bin/cat /dev/null > {$loginfo['logfile']}");
	}
}

function log_download($loginfo) {
	if (!is_array($loginfo))
		return;

	$fs = get_filesize($loginfo['logfile']);

	header("Content-Type: application/octet-stream");
	header("Content-Disposition: attachment; filename={$loginfo['filename']}");
	header("Content-Length: {$fs}");
	header("Pragma: hack");

	switch ($loginfo['type']) {
		case "clog":
			exec("/usr/sbin/clog {$loginfo['logfile']}", $content);
			echo implode("\n", $content);
			break;

		case "plain":
			readfile($loginfo['logfile']);
	}
}
?>
