#!/usr/local/bin/php
<?php
/*
	services_samba.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
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

$pgtitle = array(gettext("Services"),gettext("CIFS/SMB"),gettext("Settings"));

if (!isset($config['samba']) || !is_array($config['samba']))
	$config['samba'] = array();

if (!isset($config['samba']['auxparam']) || !is_array($config['samba']['auxparam']))
	$config['samba']['auxparam'] = array();

sort($config['samba']['auxparam']);

if (!isset($config['mounts']['mount']) || !is_array($config['mounts']['mount']))
	$config['mounts']['mount'] = array();

array_sort_key($config['mounts']['mount'], "devicespecialfile");
$a_mount = &$config['mounts']['mount'];

$pconfig['netbiosname'] = $config['samba']['netbiosname'];
$pconfig['workgroup'] = $config['samba']['workgroup'];
$pconfig['serverdesc'] = $config['samba']['serverdesc'];
$pconfig['security'] = $config['samba']['security'];
$pconfig['maxprotocol'] = $config['samba']['maxprotocol'];
$pconfig['localmaster'] = $config['samba']['localmaster'];
$pconfig['winssrv'] = $config['samba']['winssrv'];
$pconfig['timesrv'] = $config['samba']['timesrv'];
$pconfig['unixcharset'] = $config['samba']['unixcharset'];
$pconfig['doscharset'] = $config['samba']['doscharset'];
$pconfig['loglevel'] = $config['samba']['loglevel'];
$pconfig['sndbuf'] = $config['samba']['sndbuf'];
$pconfig['rcvbuf'] = $config['samba']['rcvbuf'];
$pconfig['enable'] = isset($config['samba']['enable']);
$pconfig['largereadwrite'] = isset($config['samba']['largereadwrite']);
$pconfig['usesendfile'] = isset($config['samba']['usesendfile']);
$pconfig['easupport'] = isset($config['samba']['easupport']);
$pconfig['storedosattributes'] = isset($config['samba']['storedosattributes']);
$pconfig['createmask'] = $config['samba']['createmask'];
$pconfig['directorymask'] = $config['samba']['directorymask'];
$pconfig['guestaccount'] = $config['samba']['guestaccount'];
$pconfig['maptoguest'] = $config['samba']['maptoguest'];
$pconfig['nullpasswords'] = isset($config['samba']['nullpasswords']);
$pconfig['aio'] = isset($config['samba']['aio']);
$pconfig['aiorsize'] = $config['samba']['aiorsize'];
$pconfig['aiowsize'] = $config['samba']['aiowsize'];
$pconfig['aiowbehind'] = $config['samba']['aiowbehind'];
if (is_array($config['samba']['auxparam']))
	$pconfig['auxparam'] = implode("\n", $config['samba']['auxparam']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['enable']) {
		$reqdfields = explode(" ", "security netbiosname workgroup");
		$reqdfieldsn = array(gettext("Authentication"),gettext("NetBIOS name"),gettext("Workgroup"));
		$reqdfieldst = explode(" ", "string domain workgroup");

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

		// Do additional input type validation.
		$reqdfields = explode(" ", "sndbuf rcvbuf");
		$reqdfieldsn = array(gettext("Send Buffer Size"),gettext("Receive Buffer Size"));
		$reqdfieldst = explode(" ", "numericint numericint");

		if ($_POST['security'] == "share" && $_POST['maxprotocol'] == "SMB2") {
			$input_errors[] = gettext("It cannot be used combining SMB2 and Anonymous.");
		}
		if (!empty($_POST['createmask']) || !empty($_POST['directorymask'])) {
			$reqdfields = array_merge($reqdfields, explode(" ", "createmask directorymask"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("Create mask"), gettext("Directory mask")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "filemode filemode"));
		}
		if (!empty($_POST['winssrv'])) {
			$reqdfields = array_merge($reqdfields, explode(" ", "winssrv"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("WINS server")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "ipaddr"));
		}
		if ($_POST['aio']) {
			$reqdfields = array_merge($reqdfields, explode(" ", "aiorsize aiowsize"));
			$reqdfieldsn = array_merge($reqdfieldsn, array(gettext("AIO read size"), gettext("AIO write size")));
			$reqdfieldst = array_merge($reqdfieldst, explode(" ", "numericint numericint"));
		}

		do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
		do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);
	}

	if (!$input_errors) {
		$config['samba']['enable'] = $_POST['enable'] ? true : false;
		$config['samba']['netbiosname'] = $_POST['netbiosname'];
		$config['samba']['workgroup'] = $_POST['workgroup'];
		$config['samba']['serverdesc'] = $_POST['serverdesc'];
		$config['samba']['security'] = $_POST['security'];
		if ($_POST['security'] == "share") {
			$config['samba']['maxprotocol'] = "NT1";
		} else {
			$config['samba']['maxprotocol'] = $_POST['maxprotocol'];
		}
		$config['samba']['localmaster'] = $_POST['localmaster'];
		$config['samba']['winssrv'] = $_POST['winssrv'];
		$config['samba']['timesrv'] = $_POST['timesrv'];
		$config['samba']['doscharset'] = $_POST['doscharset'];
		$config['samba']['unixcharset'] = $_POST['unixcharset'];
		$config['samba']['loglevel'] = $_POST['loglevel'];
		$config['samba']['sndbuf'] = $_POST['sndbuf'];
		$config['samba']['rcvbuf'] = $_POST['rcvbuf'];
		$config['samba']['largereadwrite'] = $_POST['largereadwrite'] ? true : false;
		$config['samba']['usesendfile'] = $_POST['usesendfile'] ? true : false;
		$config['samba']['easupport'] = $_POST['easupport'] ? true : false;
		$config['samba']['storedosattributes'] = $_POST['storedosattributes'] ? true : false;
		if (!empty($_POST['createmask']))
			$config['samba']['createmask'] = $_POST['createmask'];
		else
			unset($config['samba']['createmask']);
		if (!empty($_POST['directorymask']))
			$config['samba']['directorymask'] = $_POST['directorymask'];
		else
			unset($config['samba']['directorymask']);
		if (!empty($_POST['guestaccount']))
			$config['samba']['guestaccount'] = $_POST['guestaccount'];
		else
			unset($config['samba']['guestaccount']);
		$config['samba']['maptoguest'] = $_POST['maptoguest'];
		$config['samba']['nullpasswords'] = $_POST['nullpasswords'] ? true : false;
		$config['samba']['aio'] = $_POST['aio'] ? true : false;
		if ($_POST['aio']) {
			$config['samba']['aiorsize'] = $_POST['aiorsize'];
			$config['samba']['aiowsize'] = $_POST['aiowsize'];
			$config['samba']['aiowbehind'] = '';
		}
		if ($config['samba']['maxprotocol'] == "SMB2") {
			$config['samba']['usesendfile'] = false;
			unset($pconfig['usesendfile']);
		}

		# Write additional parameters.
		unset($config['samba']['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$config['samba']['auxparam'][] = $auxparam;
		}

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("samba");
			$retval |= rc_update_service("mdnsresponder");
			config_unlock();
		}

		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis = !(document.iform.enable.checked || enable_change);
	document.iform.netbiosname.disabled = endis;
	document.iform.workgroup.disabled = endis;
	document.iform.localmaster.disabled = endis;
	document.iform.winssrv.disabled = endis;
	document.iform.timesrv.disabled = endis;
	document.iform.serverdesc.disabled = endis;
	document.iform.doscharset.disabled = endis;
	document.iform.unixcharset.disabled = endis;
	document.iform.loglevel.disabled = endis;
	document.iform.sndbuf.disabled = endis;
	document.iform.rcvbuf.disabled = endis;
	document.iform.security.disabled = endis;
	document.iform.maxprotocol.disabled = endis;
	document.iform.largereadwrite.disabled = endis;
	document.iform.usesendfile.disabled = endis;
	document.iform.easupport.disabled = endis;
	document.iform.storedosattributes.disabled = endis;
	document.iform.createmask.disabled = endis;
	document.iform.directorymask.disabled = endis;
	document.iform.guestaccount.disabled = endis;
	document.iform.maptoguest.disabled = endis;
	document.iform.nullpasswords.disabled = endis;
	document.iform.aio.disabled = endis;
	document.iform.aiorsize.disabled = endis;
	document.iform.aiowsize.disabled = endis;
	document.iform.auxparam.disabled = endis;
}

function authentication_change() {
	switch(document.iform.security.value) {
		case "share":
			showElementById('createmask_tr','show');
			showElementById('directorymask_tr','show');
			showElementById('winssrv_tr','hide');
			break;
		case "ads":
			showElementById('createmask_tr','hide');
			showElementById('directorymask_tr','hide');
			showElementById('winssrv_tr','show');
			break;
		default:
			showElementById('createmask_tr','hide');
			showElementById('directorymask_tr','hide');
			showElementById('winssrv_tr','hide');
			break;
	}
}

function aio_change() {
	switch (document.iform.aio.checked) {
		case true:
			showElementById('aiorsize_tr','show');
			showElementById('aiowsize_tr','show');
			break;

		case false:
			showElementById('aiorsize_tr','hide');
			showElementById('aiowsize_tr','hide');
			break;
	}
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
        <li class="tabact"><a href="services_samba.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Settings");?></span></a></li>
				<li class="tabinact"><a href="services_samba_share.php"><span><?=gettext("Shares");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
      <form action="services_samba.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Common Internet File System"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<?php html_combobox("security", gettext("Authentication"), $pconfig['security'], array("share" => gettext("Anonymous"), "user" => gettext("Local User"), "ads" => gettext("Active Directory")), "", true, false, "authentication_change()");?>
					<?php html_combobox("maxprotocol", gettext("Max Protocol"), $pconfig['maxprotocol'], array("SMB2" => gettext("SMB2"), "NT1" => gettext("NT1")), gettext("SMB2 is for recent OS like Windows 7 and Vista. NT1 is for legacy OS like XP."), true, false, "");?>
          <tr>
            <td width="22%" valign="top" class="vncellreq"><?=gettext("NetBIOS name");?></td>
            <td width="78%" class="vtable">
              <input name="netbiosname" type="text" class="formfld" id="netbiosname" size="30" value="<?=htmlspecialchars($pconfig['netbiosname']);?>" />
            </td>
          </tr>
          <tr>
            <td width="22%" valign="top" class="vncellreq"><?=gettext("Workgroup") ; ?></td>
            <td width="78%" class="vtable">
              <input name="workgroup" type="text" class="formfld" id="workgroup" size="30" value="<?=htmlspecialchars($pconfig['workgroup']);?>" />
              <br /><?=gettext("Workgroup the server will appear to be in when queried by clients (maximum 15 characters).");?>
            </td>
          </tr>
          <tr>
            <td width="22%" valign="top" class="vncell"><?=gettext("Description") ;?></td>
            <td width="78%" class="vtable">
              <input name="serverdesc" type="text" class="formfld" id="serverdesc" size="30" value="<?=htmlspecialchars($pconfig['serverdesc']);?>" />
              <br /><?=gettext("Server description. This can usually be left blank.") ;?>
            </td>
          </tr>
          <?php html_combobox("doscharset", gettext("Dos charset"), $pconfig['doscharset'], array("CP437" => gettext("CP437 (Latin US)"), "CP850" => gettext("CP850 (Latin 1)"), "CP852" => gettext("CP852 (Latin 2)"), "CP866" => gettext("CP866 (Cyrillic CIS 1)"), "CP932" => gettext("CP932 (Japanese Shift-JIS)"), "CP936" => gettext("CP936 (Simplified Chinese GBK)"), "CP949" => gettext("CP949 (Korean)"), "CP950" => gettext("CP950 (Traditional Chinese Big5)"), "CP1251" => gettext("CP1251 (Cyrillic)"), "ASCII" => "ASCII"), "", false);?>
          <?php html_combobox("unixcharset", gettext("Unix charset"), $pconfig['unixcharset'], array("UTF-8" => "UTF-8", "iso-8859-1" => "iso-8859-1", "iso-8859-15" => "iso-8859-15", "gb2312" => "gb2312", "EUC-JP" => "EUC-JP", "ASCII" => "ASCII"), "", false);?>
          <?php html_combobox("loglevel", gettext("Log Level"), $pconfig['loglevel'], array("1" => gettext("Minimum"), "2" => gettext("Normal"), "3" => gettext("Full"), "10" => gettext("Debug")), "", false);?>
          <tr>
            <td width="22%" valign="top" class="vncell"><?=gettext("Local Master Browser"); ?></td>
            <td width="78%" class="vtable">
              <select name="localmaster" class="formfld" id="localmaster">
              <?php $types = array(gettext("Yes"),gettext("No")); $vals = explode(" ", "yes no");?>
              <?php $j = 0; for ($j = 0; $j < count($vals); $j++): ?>
                <option value="<?=$vals[$j];?>" <?php if ($vals[$j] == $pconfig['localmaster']) echo "selected=\"selected\"";?>>
                <?=htmlspecialchars($types[$j]);?>
                </option>
              <?php endfor; ?>
              </select>
              <br /><?php echo sprintf(gettext("Allows %s to try and become a local master browser."), get_product_name());?>
            </td>
          </tr>
          <tr>
            <td width="22%" valign="top" class="vncell"><?=gettext("Time server"); ?></td>
            <td width="78%" class="vtable">
              <select name="timesrv" class="formfld" id="timesrv">
              <?php $types = array(gettext("Yes"),gettext("No")); $vals = explode(" ", "yes no");?>
              <?php $j = 0; for ($j = 0; $j < count($vals); $j++): ?>
                <option value="<?=$vals[$j];?>" <?php if ($vals[$j] == $pconfig['timesrv']) echo "selected=\"selected\"";?>>
                <?=htmlspecialchars($types[$j]);?>
                </option>
              <?php endfor; ?>
              </select>
              <br /><?php echo sprintf(gettext("%s advertises itself as a time server to Windows clients."), get_product_name());?>
            </td>
          </tr>
          <tr id="winssrv_tr">
            <td width="22%" valign="top" class="vncell"><?=gettext("WINS server"); ?></td>
            <td width="78%" class="vtable">
              <input name="winssrv" type="text" class="formfld" id="winssrv" size="30" value="<?=htmlspecialchars($pconfig['winssrv']);?>" />
              <br /><?=gettext("WINS server IP address (e.g. from MS Active Directory server).");?>
            </td>
  				</tr>
          <tr>
			      <td colspan="2" class="list" height="12"></td>
			    </tr>
			    <tr>
			      <td colspan="2" valign="top" class="listtopic"><?=gettext("Advanced settings");?></td>
			    </tr>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Guest account");?></td>
						<td width="78%" class="vtable">
							<input name="guestaccount" type="text" class="formfld" id="guestaccount" size="30" value="<?=htmlspecialchars($pconfig['guestaccount']);?>" />
							<br /><?=gettext("Use this option to override the username ('ftp' by default) which will be used for access to services which are specified as guest. Whatever privileges this user has will be available to any client connecting to the guest service. This user must exist in the password file, but does not require a valid login.");?>
						</td>
					</tr>
					<?php html_combobox("maptoguest", gettext("Map to guest"), $pconfig['maptoguest'], array("Never" => gettext("Never - default"), "Bad User" => gettext("Bad User - non existing users")), "", false, false, "");?>
					<tr id="createmask_tr">
						<td width="22%" valign="top" class="vncell"><?=gettext("Create mask"); ?></td>
						<td width="78%" class="vtable">
							<input name="createmask" type="text" class="formfld" id="createmask" size="30" value="<?=htmlspecialchars($pconfig['createmask']);?>" />
							<br /><?=gettext("Use this option to override the file creation mask (0666 by default).");?>
						</td>
					</tr>
					<tr id="directorymask_tr">
						<td width="22%" valign="top" class="vncell"><?=gettext("Directory mask"); ?></td>
						<td width="78%" class="vtable">
							<input name="directorymask" type="text" class="formfld" id="directorymask" size="30" value="<?=htmlspecialchars($pconfig['directorymask']);?>" />
							<br /><?=gettext("Use this option to override the directory creation mask (0777 by default).");?>
						</td>
					</tr>
	        <tr>
            <td width="22%" valign="top" class="vncell"><?=gettext("Send Buffer Size"); ?></td>
            <td width="78%" class="vtable">
              <input name="sndbuf" type="text" class="formfld" id="sndbuf" size="30" value="<?=htmlspecialchars($pconfig['sndbuf']);?>" />
              <br /><?=sprintf(gettext("Size of send buffer (%d by default)."), 64240); ?>
            </td>
  				</tr>
  				<tr>
            <td width="22%" valign="top" class="vncell"><?=gettext("Receive Buffer Size") ; ?></td>
            <td width="78%" class="vtable">
              <input name="rcvbuf" type="text" class="formfld" id="rcvbuf" size="30" value="<?=htmlspecialchars($pconfig['rcvbuf']);?>" />
              <br /><?=sprintf(gettext("Size of receive buffer (%d by default)."), 64240); ?>
            </td>
  				</tr>
  				<tr>
            <td width="22%" valign="top" class="vncell"><?=gettext("Large read/write");?></td>
            <td width="78%" class="vtable">
              <input name="largereadwrite" type="checkbox" id="largereadwrite" value="yes" <?php if ($pconfig['largereadwrite']) echo "checked=\"checked\""; ?> />
              <?=gettext("Enable large read/write");?><span class="vexpl"><br />
              <?=gettext("Use the new 64k streaming read and write varient SMB requests introduced with Windows 2000.");?></span>
            </td>
          </tr>
					<?php html_checkbox("usesendfile", gettext("Use sendfile"), $pconfig['usesendfile'] ? true : false, gettext("Enable use sendfile."), gettext("This may make more efficient use of the system CPU's and cause Samba to be faster. Samba automatically turns this off for clients that use protocol levels lower than NT LM 0.12 and when it detects a client is Windows 9x."), false);?>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("EA support");?></td>
						<td width="78%" class="vtable">
							<input name="easupport" type="checkbox" id="easupport" value="yes" <?php if ($pconfig['easupport']) echo "checked=\"checked\""; ?> />
							<?=gettext("Enable extended attribute support");?><span class="vexpl"><br />
							<?=gettext("Allow clients to attempt to store OS/2 style extended attributes on a share.");?></span>
						</td>
					</tr>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Store DOS attributes");?></td>
						<td width="78%" class="vtable">
							<input name="storedosattributes" type="checkbox" id="storedosattributes" value="yes" <?php if ($pconfig['storedosattributes']) echo "checked=\"checked\""; ?> />
							<?=gettext("Enable store DOS attributes");?><span class="vexpl"><br />
							<?=gettext("If this parameter is set, Samba attempts to first read DOS attributes (SYSTEM, HIDDEN, ARCHIVE or READ-ONLY) from a filesystem extended attribute, before mapping DOS attributes to UNIX permission bits. When set, DOS attributes will be stored onto an extended attribute in the UNIX filesystem, associated with the file or directory.");?></span>
						</td>
					</tr>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Null passwords");?></td>
						<td width="78%" class="vtable">
							<input name="nullpasswords" type="checkbox" id="nullpasswords" value="yes" <?php if ($pconfig['nullpasswords']) echo "checked=\"checked\""; ?> />
							<?=gettext("Allow client access to accounts that have null passwords.");?>
						</td>
					</tr>
					<?php html_checkbox("aio", gettext("Asynchronous I/O (AIO)"), $pconfig['aio'] ? true : false, gettext("Enable Asynchronous I/O (AIO)"), "", false, "aio_change()");?>
					<?php html_inputbox("aiorsize", gettext("AIO read size"), $pconfig['aiorsize'], sprintf(gettext("Samba will read from file asynchronously when size of request is bigger than this value. (%d by default)"), 1), true, 30);?>
					<?php html_inputbox("aiowsize", gettext("AIO write size"), $pconfig['aiowsize'], sprintf(gettext("Samba will write to file asynchronously when size of request is bigger than this value. (%d by default)"), 1), true, 30);?>
					<?php /*html_inputbox("aiowbehind", gettext("AIO write behind"), $pconfig['aiowbehind'], "", false, 60);*/?>
					<?php html_textarea("auxparam", gettext("Auxiliary parameters"), $pconfig['auxparam'], sprintf(gettext("These parameters are added to [Global] section of %s."), "smb.conf") . " " . sprintf(gettext("Please check the <a href='%s' target='_blank'>documentation</a>."), "http://us1.samba.org/samba/docs/man/manpages-3/smb.conf.5.html"), false, 65, 5, false, false);?>
        </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="enable_change(true)" />
				</div>
				<div id="remarks">
					<?php html_remark("note", gettext("Note"), sprintf(gettext("To increase CIFS performance try the following:<div id='enumeration'><ul><li>Enable 'Asynchronous I/O (AIO)' switch</li><li>Enable 'Large read/write' switch</li><li>Enable '<a href='%s'>Tuning</a>' switch</li></ul></div>"), "system_advanced.php", "interfaces_lan.php"));?>
				</div>
				<?php include("formend.inc");?>
      </form>
    </td>
  </tr>
</table>
<script type="text/javascript">
<!--
enable_change(false);
authentication_change();
aio_change();
//-->
</script>
<?php include("fend.inc");?>
