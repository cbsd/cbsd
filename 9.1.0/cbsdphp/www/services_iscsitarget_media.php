#!/usr/local/bin/php
<?php
/*
	services_iscsitarget_target_media.php
	
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	Portions of m0n0wall (http://m0n0.ch/wall)
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
require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("Services"), gettext("iSCSI Target"), gettext("Media"));

$pconfig['media_uctladdress'] = $config['iscsitarget']['media_uctladdress'];
$pconfig['media_uctlport'] = $config['iscsitarget']['media_uctlport'];
$pconfig['media_uctlauthmethod'] = $config['iscsitarget']['media_uctlauthmethod'];
$pconfig['media_uctluser'] = $config['iscsitarget']['media_uctluser'];
$pconfig['media_uctlsecret'] = $config['iscsitarget']['media_uctlsecret'];
$pconfig['media_uctlmuser'] = $config['iscsitarget']['media_uctlmuser'];
$pconfig['media_uctlmsecret'] = $config['iscsitarget']['media_uctlmsecret'];
$pconfig['media_uctlsave'] = FALSE;

function reset_uctlinfo(&$pconfig) {
	$pconfig['media_uctladdress'] = "127.0.0.1";
	$pconfig['media_uctlport'] = "3261";
	$pconfig['media_uctlauthmethod'] = "CHAP";
	$pconfig['media_uctluser'] = "";
	$pconfig['media_uctlsecret'] = "";
	$pconfig['media_uctlmuser'] = "";
	$pconfig['media_uctlmsecret'] = "";
	$pconfig['media_uctlsave'] = FALSE;
}

if (!isset($pconfig['media_uctladdress'])
    || $pconfig['media_uctladdress'] == '') {
	reset_uctlinfo($pconfig);
}

$pconfig['target_list'] = array();
function scan_target(&$pconfig) {
	$address = $pconfig['media_uctladdress'];
	$port = $pconfig['media_uctlport'];
	$user = $pconfig['media_uctluser'];
	$secret = $pconfig['media_uctlsecret'];
	$muser = $pconfig['media_uctlmuser'];
	$msecret = $pconfig['media_uctlmsecret'];
	$target = "ALL";
	$lun = 0;

	$uctl = "/usr/local/bin/istgtcontrol";
	$args = "-q -c /var/etc/iscsi/istgtcontrol.conf";
	$args .= " -h ".escapeshellarg($address);
	$args .= " -p ".escapeshellarg($port);
	$args .= " -t ".escapeshellarg($target);
	$args .= " -l ".escapeshellarg($lun);
	$args .= " -U ".escapeshellarg($user);
	$args .= " -S ".escapeshellarg($secret);
	$args .= " -M ".escapeshellarg($muser);
	$args .= " -R ".escapeshellarg($msecret);
	$cmd = "$uctl $args list 2>&1";
	unset($rawdata, $rc, $pconfig['error']);
	mwexec2($cmd, $rawdata, $rc);
	if ($rc != 0) {
		//$pconfig['error'] = "cmd: $cmd";
		$pconfig['error'] = $rawdata[0];
		return -1;
	}

	$pconfig['target_list'] = array();
	foreach ($rawdata as $line) {
		$pconfig['target_list'][]['name'] = $line;
	}
	return 0;
}

$pconfig['target_info'] = array();
function get_target_info(&$pconfig) {
	$address = $pconfig['media_uctladdress'];
	$port = $pconfig['media_uctlport'];
	$user = $pconfig['media_uctluser'];
	$secret = $pconfig['media_uctlsecret'];
	$muser = $pconfig['media_uctlmuser'];
	$msecret = $pconfig['media_uctlmsecret'];
	$target = $pconfig['target'];
	$lun = 0;

	$uctl = "/usr/local/bin/istgtcontrol";
	$args = "-q -c /var/etc/iscsi/istgtcontrol.conf";
	$args .= " -h ".escapeshellarg($address);
	$args .= " -p ".escapeshellarg($port);
	$args .= " -t ".escapeshellarg($target);
	$args .= " -l ".escapeshellarg($lun);
	$args .= " -U ".escapeshellarg($user);
	$args .= " -S ".escapeshellarg($secret);
	$args .= " -M ".escapeshellarg($muser);
	$args .= " -R ".escapeshellarg($msecret);
	$cmd = "$uctl $args list 2>&1";
	unset($rawdata, $rc, $pconfig['error']);
	mwexec2($cmd, $rawdata, $rc);
	if ($rc != 0) {
		$pconfig['error'] = $rawdata[0];
		return -1;
	}

	$pconfig['target_info'] = array();
	$index = 0;
	foreach ($rawdata as $line) {
		$pconfig['target_info'][$index]['line'] = $line;
		if (preg_match("/(lun[0-9]+) (\w+) \"([^\"]+)\" ([0-9]+)/", $line, $match)) {
			$pconfig['target_info'][$index]['type'] = "disk";
			$pconfig['target_info'][$index]['lun'] = $match[1];
			$pconfig['target_info'][$index]['extent'] = $match[3];
			$pconfig['target_info'][$index]['size'] = $match[4];
			preg_match("/(lun[0-9]+) (\w+) (.+)/", $line, $match);
			$pconfig['target_info'][$index]['info'] = $match[3];
		} else if (preg_match("/(lun[0-9]+) (\w+) (present|absent) (lock|unlock) (-) (ro|rw|rw,dynamic|rw,extend) \"([^\"]+)\" ([0-9]+|auto)/", $line, $match)) {
			$pconfig['target_info'][$index]['type'] = "removable";
			$pconfig['target_info'][$index]['lun'] = $match[1];
			$pconfig['target_info'][$index]['present'] = $match[3];
			$pconfig['target_info'][$index]['lock'] = $match[4];
			$pconfig['target_info'][$index]['mtype'] = $match[5];
			$pconfig['target_info'][$index]['extent'] = $match[6];
			$pconfig['target_info'][$index]['size'] = $match[7];
			preg_match("/(lun[0-9]+) (\w+) (.+)/", $line, $match);
			$pconfig['target_info'][$index]['info'] = $match[3];
		} else if (preg_match("/(lun[0-9]+) (\w+) \"([^\"]+)\"/", $line, $match)) {
			$pconfig['target_info'][$index]['type'] = "pass";
			$pconfig['target_info'][$index]['lun'] = $match[1];
			$pconfig['target_info'][$index]['extent'] = $match[3];
			preg_match("/(lun[0-9]+) (\w+) (.+)/", $line, $match);
			$pconfig['target_info'][$index]['info'] = $match[3];
		} else {
			$pconfig['target_info'][$index]['type'] = "unknown";
			$pconfig['target_info'][$index]['info'] = $line;
		}
		$index++;
	}
	return 0;
}

function do_load_unload(&$pconfig, $unload = 0) {
	$address = $pconfig['media_uctladdress'];
	$port = $pconfig['media_uctlport'];
	$user = $pconfig['media_uctluser'];
	$secret = $pconfig['media_uctlsecret'];
	$muser = $pconfig['media_uctlmuser'];
	$msecret = $pconfig['media_uctlmsecret'];
	$target = $pconfig['target'];
	$lun = 0;

	$uctl = "/usr/local/bin/istgtcontrol";
	$args = "-q -c /var/etc/iscsi/istgtcontrol.conf";
	$args .= " -h ".escapeshellarg($address);
	$args .= " -p ".escapeshellarg($port);
	$args .= " -t ".escapeshellarg($target);
	$args .= " -l ".escapeshellarg($lun);
	$args .= " -U ".escapeshellarg($user);
	$args .= " -S ".escapeshellarg($secret);
	$args .= " -M ".escapeshellarg($muser);
	$args .= " -R ".escapeshellarg($msecret);
	if ($unload == 0) {
		$cmd = "$uctl $args load 2>&1";
	} else {
		$cmd = "$uctl $args unload 2>&1";
	}
	unset($rawdata, $rc, $pconfig['error']);
	mwexec2($cmd, $rawdata, $rc);
	if ($rc != 0) {
		$pconfig['error'] = $rawdata[0];
		return -1;
	}
	return 0;
}

function do_change(&$pconfig) {
	$address = $pconfig['media_uctladdress'];
	$port = $pconfig['media_uctlport'];
	$user = $pconfig['media_uctluser'];
	$secret = $pconfig['media_uctlsecret'];
	$muser = $pconfig['media_uctlmuser'];
	$msecret = $pconfig['media_uctlmsecret'];
	$target = $pconfig['target'];
	$lun = 0;
	$path = $pconfig['path'];
	$size = $pconfig['size'];
	$sizeunit = $pconfig['sizeunit'];
	$flags = $pconfig['flags'];

	$uctl = "/usr/local/bin/istgtcontrol";
	$args = "-q -c /var/etc/iscsi/istgtcontrol.conf";
	$args .= " -h ".escapeshellarg($address);
	$args .= " -p ".escapeshellarg($port);
	$args .= " -t ".escapeshellarg($target);
	$args .= " -l ".escapeshellarg($lun);
	$args .= " -U ".escapeshellarg($user);
	$args .= " -S ".escapeshellarg($secret);
	$args .= " -M ".escapeshellarg($muser);
	$args .= " -R ".escapeshellarg($msecret);
	$args .= " -s ".escapeshellarg("$size$sizeunit");
	$args .= " -f ".escapeshellarg($flags);
	$file = escapeshellarg($path);
	$cmd = "$uctl $args change $file 2>&1";
	unset($rawdata, $rc, $pconfig['error']);
	mwexec2($cmd, $rawdata, $rc);
	if ($rc != 0) {
		$pconfig['error'] = $rawdata[0];
		return -1;
	}
	return 0;
}

if ($_POST) {
	unset($input_errors);
	unset($errormsg);
	$pconfig = $_POST;
	$pconfig['target_list'] = array();
	$pconfig['target_info'] = array();
	$pconfig['mediadirectory'] = $config['iscsitarget']['mediadirectory'];

	if ($_POST['Cancel']) {
		header("Location: services_iscsitarget.php");
		exit;
	}
	if ($_POST['Delete']) {
		reset_uctlinfo($pconfig);
		$config['iscsitarget']['media_uctladdress'] = $pconfig['media_uctladdress'];
		$config['iscsitarget']['media_uctlport'] = $pconfig['media_uctlport'];
		$config['iscsitarget']['media_uctlauthmethod'] = $pconfig['media_uctlauthmethod'];
		$config['iscsitarget']['media_uctluser'] = $pconfig['media_uctluser'];
		$config['iscsitarget']['media_uctlsecret'] = $pconfig['media_uctlsecret'];
		$config['iscsitarget']['media_uctlmuser'] = $pconfig['media_uctlmuser'];
		$config['iscsitarget']['media_uctlmsecret'] = $pconfig['media_uctlmsecret'];
		write_config();
		header("Location: services_iscsitarget_media.php");
		exit;
	}
	if ($_POST['Scan']) {
		$reqdfields = explode(" ", "media_uctladdress media_uctlport media_uctlauthmethod");
		$reqdfieldsn = array(gettext("Controller IP address"),
			     gettext("Controller TCP Port"),
			     gettext("Controller Auth Method"));
		$reqdfieldst = explode(" ", "string numericint string");

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

		if ($_POST['media_uctlauthmethod'] == 'CHAP'
		    || $_POST['media_uctlauthmethod'] == 'CHAP mutual') {
			$reqdfields = explode(" ", "media_uctluser media_uctlsecret");
			$reqdfieldsn = array(gettext("User"),
				     gettext("Secret"));
			$reqdfieldst = explode(" ", "string string");

			do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
			do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
		}
		if ($_POST['media_uctlauthmethod'] == 'CHAP mutual') {
			$reqdfields = explode(" ", "media_uctlmuser media_uctlmsecret");
			$reqdfieldsn = array(gettext("Peer User"),
				     gettext("Peer Secret"));
			$reqdfieldst = explode(" ", "string string");

			do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
			do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
		}

		if (isset($pconfig['media_uctlsave'])) {
			if (!$input_errors) {
				$config['iscsitarget']['media_uctladdress'] = $pconfig['media_uctladdress'];
				$config['iscsitarget']['media_uctlport'] = $pconfig['media_uctlport'];
				$config['iscsitarget']['media_uctlauthmethod'] = $pconfig['media_uctlauthmethod'];
				$config['iscsitarget']['media_uctluser'] = $pconfig['media_uctluser'];
				$config['iscsitarget']['media_uctlsecret'] = $pconfig['media_uctlsecret'];
				$config['iscsitarget']['media_uctlmuser'] = $pconfig['media_uctlmuser'];
				$config['iscsitarget']['media_uctlmsecret'] = $pconfig['media_uctlmsecret'];
				write_config();
			}
		} else {
			// ignore change
		}
		if (!$input_errors) {
			// scan target on specified Logical Unit Controller
			if (scan_target($pconfig) != 0) {
				if ($pconfig['error'] != "") {
					$errormsg = $pconfig['error'];
				} else {
					$errormsg = gettext("scan target failed.");
				}
			}
		}
	}
	if ($_POST['Info']) {
		if (scan_target($pconfig) != 0) {
			if ($pconfig['error'] != "") {
				$errormsg = $pconfig['error'];
			} else {
				$errormsg = gettext("scan target failed.");
			}
		}
		if ($pconfig['target'] == "") {
			$input_errors[] = gettext("Target is not selected.");
		}
		if (!$input_errors) {
			// get info on specified target
			if (get_target_info($pconfig) != 0) {
				if ($pconfig['error'] != "") {
					$errormsg = $pconfig['error'];
				} else {
					$errormsg = gettext("get target info failed.");
				}
			}
		}
	}
	if ($_POST['Unload'] || $_POST['Load']) {
		if (scan_target($pconfig) != 0) {
			if ($pconfig['error'] != "") {
				$errormsg = $pconfig['error'];
			} else {
				$errormsg = gettext("scan target failed.");
			}
		}
		if ($pconfig['target'] == "") {
			$input_errors[] = gettext("Target is not selected.");
		}
		if (!$input_errors) {
			// load/unload media
			if (do_load_unload($pconfig, $_POST['Unload'] ? 1 : 0) != 0) {
				if ($pconfig['error'] != "") {
					$errormsg = $pconfig['error'];
				} else {
					$errormsg = gettext("load/unload failed.");
				}
			} else {
				$savemsg = sprintf("%s %s",
					 ($_POST['Unload'] ? gettext("Unload") : gettext("Load")),
					 gettext("successfully."));
			}
		}
		if (get_target_info($pconfig) != 0) {
			if ($pconfig['error'] != "") {
				$errormsg = $pconfig['error'];
			} else {
				$errormsg = gettext("get target info failed.");
			}
		}
	}
	if ($_POST['Change']) {
		if (scan_target($pconfig) != 0) {
			if ($pconfig['error'] != "") {
				$errormsg = $pconfig['error'];
			} else {
				$errormsg = gettext("scan target failed.");
			}
		}
		if ($pconfig['target'] == "") {
			$input_errors[] = gettext("Target is not selected.");
		}
		if ($pconfig['sizeunit'] == 'auto'){
			$pconfig['size'] = "";
			$reqdfields = explode(" ", "path sizeunit flags");
			$reqdfieldsn = array(gettext("Path"),
				     gettext("Auto size"),
				     gettext("Flags"));
			$reqdfieldst = explode(" ", "string string string");
		}else{
			$reqdfields = explode(" ", "path size sizeunit flags");
			$reqdfieldsn = array(gettext("Path"),
				     gettext("File size"),
				     gettext("File sizeunit"),
				     gettext("Flags"));
			$reqdfieldst = explode(" ", "string numericint string");
		}
		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

		if (!$input_errors) {
			// change media
			if (do_change($pconfig) != 0) {
				if ($pconfig['error'] != "") {
					$errormsg = $pconfig['error'];
				} else {
					$errormsg = gettext("change failed.");
				}
			} else {
				$savemsg = sprintf("%s %s",
					 gettext("Change"),
					 gettext("successfully."));
			}
		}
		if (get_target_info($pconfig) != 0) {
			if ($pconfig['error'] != "") {
				$errormsg = $pconfig['error'];
			} else {
				$errormsg = gettext("get target info failed.");
			}
		}
	}
}

function target_list(&$pconfig) {
	if (empty($pconfig['target_list'])) return;

	html_titleline(gettext("Target list"));
	echo '<tr><td colspan="2">';
	echo '<table width="100%" border="0" cellpadding="0" cellspacing="0">';
	echo '<tr>';
	echo '  <td width="1%" class="listhdrlr">&nbsp;</td>';
	echo '  <td class="listhdrr">'.htmlspecialchars(gettext("Target Name")).'</td>';
	echo '</tr>'."\n";
	foreach ($pconfig['target_list'] as $targetv) {
		$name = $targetv['name'];
		$sel = ($pconfig['target'] == $name) ? " checked=\"checked\"" : "";
		echo '<tr>';
		echo '  <td width="1%" class="listlr"><input name="target" type="radio" value="'.htmlspecialchars($name).'"'.$sel.' /></td>';
		echo '  <td class="listr">'.htmlspecialchars($name).'&nbsp;</td>';
		echo '</tr>'."\n";
	}
	echo '</table>';
	echo '</td></tr>'."\n";
	echo '<tr><td colspan="2" valign="top">';
	echo '  <input name="Info" type="submit" class="formbtn" value="'.gettext("Get Target Info").'" />';
	echo '</td></tr>'."\n";
}

function target_info(&$pconfig) {
	if (empty($pconfig['target_info'])) return;
	$mediadir = $pconfig['mediadirectory'];
	if ($mediadir == "") $mediadir = "/mnt";

	html_titleline(gettext("Target information"));
	echo '<tr><td colspan="2">';
	echo '<table width="100%" border="0" cellpadding="0" cellspacing="0">';
	echo '<tr>';
	echo '  <td width="5%" class="listhdrlr">'.htmlspecialchars(gettext("Type")).'</td>';
	echo '  <td width="5%" class="listhdrr">'.htmlspecialchars(gettext("LUN")).'</td>';
	echo '  <td class="listhdrr">'.htmlspecialchars(gettext("Status")).'</td>';
	echo '</tr>'."\n";
	$removable = 0;
	foreach ($pconfig['target_info'] as $infov) {
		$type = $infov['type'];
		$lun = $infov['lun'];
		$extent = $infov['extent'];
		$line = $infov['line'];
		$info = $infov['info'];
		if ($type == 'removable') $removable = 1;
		echo '<tr>';
		echo '  <td width="5%" class="listlr">'.htmlspecialchars($type).'</td>';
		echo '  <td width="5%" class="listr">'.htmlspecialchars($lun).'</td>';
		echo '  <td class="listr">'.htmlspecialchars($info).'&nbsp;</td>';
		echo '</tr>'."\n";
	}
	echo '</table>';
	echo '</td></tr>'."\n";

	if ($removable == 0) return;
	echo '<tr><td colspan="2" valign="top">';
	echo '  <input name="Unload" type="submit" class="formbtn" value="'.gettext("Unload").'" />';
	echo '  <input name="Load" type="submit" class="formbtn" value="'.gettext("Load").'" />';
	echo '  <input name="Change" type="submit" class="formbtn" value="'.gettext("Change below file").'" />';
	echo '</td></tr>'."\n";

	echo "<tr id='path_tr'><td colspan='2' valign='top'>";
	echo "<input name='path' type='text' class='formfld' id='path' size='60' value=''  />";
	echo "<input name='pathbrowsebtn' type='button' class='formbtn' id='pathbrowsebtn' onclick='ifield = form.path; filechooser = window.open(\"filechooser.php?p=\"+escape(ifield.value)+\"&amp;sd=".$mediadir."\", \"filechooser\", \"scrollbars=yes,toolbar=no,menubar=no,statusbar=no,width=550,height=300\"); filechooser.ifield = ifield; window.ifield = ifield;' value='...' />";
	echo "</td></tr>\n";

	echo '<tr id="size_tr"><td colspan="2">';
	echo '<input name="size" type="text" class="formfld" id="size" size="10" value="" /> ';
	echo '<select name="sizeunit" onclick="sizeunit_change()">';
	echo '  <option value="MB" >'.htmlspecialchars(gettext("MiB")).'</option>';

	echo '  <option value="GB" >'.htmlspecialchars(gettext("GiB")).'</option>';
	echo '  <option value="TB" >'.htmlspecialchars(gettext("TiB")).'</option>';
	echo '  <option value="auto" selected="selected">'.htmlspecialchars(gettext("Auto")).'</option>';
	echo '</select>';
	echo '<br /><span class="vexpl">'.gettext("Size offered to the initiator. (up to 8EiB=8388608TiB. actual size is depend on your disks.)").'</span>';
	echo '</td></tr>'."\n";

	echo '<tr id="flags_tr"><td colspan="2">';
	echo "<select name='flags' class='formfld' id='flags' >";
	echo "  <option value='rw' selected='selected' >".gettext("Read/Write (rw)")."</option>";
	echo "  <option value='rw,dynamic' >".gettext("Read/Write (rw,dynamic) for removable types file size grow and shrink automatically by EOF (ignore specified size)")."</option>";
	echo "  <option value='rw,extend' >".gettext("Read/Write (rw,extend) for removable types extend file size if EOM reached")."</option>";
	echo "  <option value='ro' >".gettext("Read Only (ro)")."</option>";
	echo "</select>";
	echo '</td></tr>'."\n";
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function authmethod_change() {
	switch (document.iform.media_uctlauthmethod.value) {
	case "CHAP":
		showElementById("media_uctlmuser_tr", 'hide');
		showElementById("media_uctlmsecret_tr", 'hide');
		break;
	case "CHAP mutual":
		showElementById("media_uctlmuser_tr", 'show');
		showElementById("media_uctlmsecret_tr", 'show');
		break;
	default:
		showElementById("media_uctlmuser_tr", 'hide');
		showElementById("media_uctlmsecret_tr", 'hide');
		break;
	}
}

function sizeunit_change() {
	if (!("sizeunit" in document.iform)) return;
	switch (document.iform.sizeunit.value) {
	case "auto":
		document.iform.size.disabled = true;
		break;
	default:
		document.iform.size.disabled = false;
		break;
	}
}
//-->
</script>
<form action="services_iscsitarget_media.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
				<li class="tabinact"><a href="services_iscsitarget.php"><span><?=gettext("Settings");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_target.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Targets");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_pg.php"><span><?=gettext("Portals");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_ig.php"><span><?=gettext("Initiators");?></span></a></li>
				<li class="tabinact"><a href="services_iscsitarget_ag.php"><span><?=gettext("Auths");?></span></a></li>
				<li class="tabact"><a href="services_iscsitarget_media.php"><span><?=gettext("Media");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
      <?php if ($input_errors) print_input_errors($input_errors);?>
      <?php if ($errormsg) print_error_box($errormsg);?>
      <?php if ($savemsg) print_info_box($savemsg);?>
      <table width="100%" border="0" cellpadding="6" cellspacing="0">
      <?php html_titleline(gettext("Logical Unit Controller login information"));?>
      <?php html_inputbox("media_uctladdress", gettext("Controller IP address"), $pconfig['media_uctladdress'], "", true, 30);?>
      <?php html_inputbox("media_uctlport", gettext("Controller TCP Port"), $pconfig['media_uctlport'], "", true, 15);?>
      <?php html_combobox("media_uctlauthmethod", gettext("Controller Auth Method"), $pconfig['media_uctlauthmethod'], array("CHAP" => gettext("CHAP"), "CHAP mutual" => gettext("Mutual CHAP")), "", true, false, "authmethod_change()");?>
      <?php html_inputbox("media_uctluser", gettext("User"), $pconfig['media_uctluser'], "", true, 60);?>
      <?php html_passwordbox("media_uctlsecret", gettext("Secret"), $pconfig['media_uctlsecret'], "", true, 30);?>
      <?php html_inputbox("media_uctlmuser", gettext("Peer User"), $pconfig['media_uctlmuser'], "", true, 60);?>
      <?php html_passwordbox("media_uctlmsecret", gettext("Peer Secret"), $pconfig['media_uctlmsecret'], "", true, 30);?>
      <?php html_checkbox("media_uctlsave", gettext("Save"), $pconfig['media_uctlsave'] ? true : false, gettext("Save login information in configuration file."), "", false);?>
      <tr>
        <td colspan="1" valign="top">
          <input name="Scan" type="submit" class="formbtn" value="<?=gettext("Scan Targets");?>" />
        </td>
        <td colspan="1" valign="top" align="right">
          <input name="Delete" type="submit" class="formbtn" value="<?=gettext("Delete Login Info");?>" />
        </td>
      </tr>
      <?php target_list($pconfig) ?>
      <?php target_info($pconfig) ?>
      </table>
    </td>
  </tr>
</table>
<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
	authmethod_change();
	sizeunit_change();
//-->
</script>
<?php include("fend.inc");?>
