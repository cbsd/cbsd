#!/usr/local/bin/php
<?php
/*
	disks_crypt_edit.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
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
require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("Disks"),gettext("Encryption"),gettext("Add"));

if (!isset($config['geli']['vdisk']) || !is_array($config['geli']['vdisk']))
	$config['geli']['vdisk'] = array();

array_sort_key($config['geli']['vdisk'], "devicespecialfile");
$a_geli = &$config['geli']['vdisk'];

// Get list of all configured disks (physical and virtual).
$a_alldisk = get_conf_all_disks_list_filtered();

// Check whether there are disks configured, othersie display a error message.
if (!count($a_alldisk)) {
	$nodisks_error = gettext("You must add disks first.");
}

// Check if protocol is HTTPS, otherwise display a warning message.
if ("http" === $config['system']['webgui']['protocol']) {
	$nohttps_error = gettext("You should use HTTPS as WebGUI protocol for sending passphrase.");
}

if ($_POST) {
	unset($input_errors);
	unset($errormsg);
	unset($pconfig['do_action']);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: disks_crypt.php");
		exit;
	}

	// Input validation.
  $reqdfields = explode(" ", "disk ealgo passphrase passphraseconf");
  $reqdfieldsn = array(gettext("Disk"),gettext("Encryption algorithm"),gettext("Passphrase"),gettext("Passphrase"));
  do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

	// Check for duplicate disks.
	if (array_search_ex("{$_POST['disk']}.eli", $a_geli, "devicespecialfile")) {
		$input_errors[] = gettext("This disk already exists in the disk list.");
	}

	// Check for a passphrase mismatch.
	if ($_POST['passphrase'] !== $_POST['passphraseconf']) {
		$input_errors[] = gettext("Passphrase don't match.");
	}

	if (!$input_errors) {
		$pconfig['do_action'] = true;
		$pconfig['init'] = $_POST['init'] ? true : false;
		$pconfig['name'] = $a_alldisk[$_POST['disk']]['name']; // e.g. da2
		$pconfig['devicespecialfile'] = $a_alldisk[$_POST['disk']]['devicespecialfile']; // e.g. /dev/da2
		$pconfig['aalgo'] = "none";

		// Check whether disk is mounted.
		if (disks_ismounted_ex($pconfig['devicespecialfile'], "devicespecialfile")) {
			$errormsg = sprintf( gettext("The disk is currently mounted! <a href='%s'>Unmount</a> this disk first before proceeding."), "disks_mount_tools.php?disk={$pconfig['devicespecialfile']}&action=umount");
			$pconfig['do_action'] = false;
		}

		if ($pconfig['do_action']) {
			// Set new file system type attribute ('fstype') in configuration.
			set_conf_disk_fstype($pconfig['devicespecialfile'], "geli");

			// Get disk information.
			$diskinfo = disks_get_diskinfo($pconfig['devicespecialfile']);

			$geli = array();
			$geli['uuid'] = uuid();
			$geli['name'] = $pconfig['name'];
			$geli['device'] = $pconfig['devicespecialfile'];
			$geli['devicespecialfile'] = "{$geli['device']}.eli";
			$geli['desc'] = "Encrypted disk";
			$geli['size'] = "{$diskinfo['mediasize_mbytes']}MB";
			$geli['aalgo'] = $pconfig['aalgo'];
			$geli['ealgo'] = $pconfig['ealgo'];

			$a_geli[] = $geli;

			write_config();
		}
	}
}

if (!isset($pconfig['do_action'])) {
	// Default values.
	$pconfig['do_action'] = false;
	$pconfig['init'] = false;
	$pconfig['disk'] = 0;
	$pconfig['aalgo'] = "";
	$pconfig['ealgo'] = "AES";
	$pconfig['keylen'] = "";
	$pconfig['passphrase'] = "";
	$pconfig['name'] = "";
	$pconfig['devicespecialfile'] = "";
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">
<!--
function ealgo_change() {
	// Disable illegal values in 'Key length' selective list.
	for (i = 0; i < document.iform.keylen.length; i++) {
		var disabled = false;
		switch (document.iform.ealgo.value) {
		case "3DES":
			disabled = (document.iform.keylen.options[i].value >= 256);
			break;
		case "AES":
		case "Camellia":
			disabled = (document.iform.keylen.options[i].value > 256);
			break;
		}
		document.iform.keylen.options[i].disabled = disabled;
	}

	// Set key length to 'default' whether an illegal value is selected.
	var selected = document.iform.keylen.selectedIndex;
	if (document.iform.keylen.options[selected].disabled == true)
		document.iform.keylen.selectedIndex = 0;
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
        <li class="tabact"><a href="disks_crypt.php" title="<?=gettext("Reload page");?>" ><span><?=gettext("Management");?></span></a></li>
        <li class="tabinact"><a href="disks_crypt_tools.php"><span><?=gettext("Tools");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="disks_crypt_edit.php" method="post" name="iform" id="iform">
				<?php if ($nohttps_error) print_warning_box($nohttps_error);?>
				<?php if ($nodisks_error) print_error_box($nodisks_error);?>
				<?php if ($errormsg) print_error_box($errormsg);?>
				<?php if ($input_errors) print_input_errors($input_errors);?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
			    <tr>
			      <td valign="top" class="vncellreq"><?=gettext("Disk");?></td>
			      <td class="vtable">
							<select name="disk" class="formfld" id="disk">
								<option value=""><?=gettext("Must choose one");?></option>
								<?php $i = -1; foreach ($a_alldisk as $diskv):?>
								<?php ++$i;?>
								<?php if (0 == strcmp($diskv['class'], "geli")) continue;?>
								<?php if (0 == strcmp($diskv['size'], "NA")) continue;?>
								<?php if (1 == disks_exists($diskv['devicespecialfile'])) continue;?>
								<option value="<?=$i;?>" <?php if ($pconfig['disk'] == $i) echo "selected=\"selected\"";?>>
								<?php $diskinfo = disks_get_diskinfo($diskv['devicespecialfile']); echo htmlspecialchars("{$diskv['name']}: {$diskinfo['mediasize_mbytes']}MB ({$diskv['desc']})");?>
								</option>
								<?php endforeach;?>
			    		</select>
			      </td>
			    </tr>
					<?php
					/* Remove Data Intergrity Algorithhm : there is a bug when enabled
					<tr>
						<td valign="top" class="vncellreq"><?=gettext("Data integrity algorithm");?></td>
			      <td class="vtable">
			        <select name="aalgo" class="formfld" id="aalgo">
								<option value="none" <?php if ($pconfig['aalgo'] === "none") echo "selected=\"selected\""; ?>>none</option>
			          <option value="HMAC/MD5" <?php if ($pconfig['aalgo'] === "HMAC/MD5") echo "selected=\"selected\""; ?>>HMAC/MD5</option>
			          <option value="HMAC/SHA1" <?php if ($pconfig['aalgo'] === "HMAC/SHA1") echo "selected=\"selected\""; ?>>HMAC/SHA1</option>
			          <option value="HMAC/RIPEMD160" <?php if ($pconfig['aalgo'] === "HMAC/RIPEMD160") echo "selected=\"selected\""; ?>>HMAC/RIPEMD160</option>
			          <option value="HMAC/SHA256" <?php if ($pconfig['aalgo'] === "HMAC/SHA256") echo "selected=\"selected\""; ?>>HMAC/SHA256</option>
			          <option value="HMAC/SHA384" <?php if ($pconfig['aalgo'] === "HMAC/SHA384") echo "selected=\"selected\""; ?>>HMAC/SHA384</option>
			          <option value="HMAC/SHA512" <?php if ($pconfig['aalgo'] === "HMAC/SHA512") echo "selected=\"selected\""; ?>>HMAC/SHA512</option>
			        </select>
			      </td>
			    </tr>
					*/
					?>
					<?php $options = array("AES" => "AES", "Blowfish" => "Blowfish", "Camellia" => "Camellia", "3DES" => "3DES");?>
					<?php html_combobox("ealgo", gettext("Encryption algorithm"), $pconfig['ealgo'], $options, gettext("Encryption algorithm to use."), true, false, "ealgo_change()");?>
					<?php $options = array("" => gettext("Default"), 128 => "128", 192 => "192", 256 => "256", 448 => "448");?>
					<?php html_combobox("keylen", gettext("Key length"), $pconfig['keylen'], $options, gettext("Key length to use with the given cryptographic algorithm.") . " " . gettext("The default key lengths are: 128 for AES, 128 for Blowfish, 128 for Camellia and 192 for 3DES."), false);?>
					<?php html_passwordconfbox("passphrase", "passphraseconf", gettext("Passphrase"), "", "", "", true);?>
					<?php html_checkbox("init", gettext("Initialize"), $pconfig['init'] ? true : false, gettext("Initialize and encrypt disk."), gettext("This will erase ALL data on your disk! Do not use this option if you want to add an existing encrypted disk."));?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Add");?>" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
				</div>
				<?php if ($pconfig['do_action']) {
				echo(sprintf("<div id='cmdoutput'>%s</div>", gettext("Command output:")));
				echo('<pre class="cmdoutput">');
				ob_end_flush();

				if (true === $pconfig['init']) {
					// Initialize and encrypt the disk.
					echo sprintf(gettext("Encrypting '%s'... Please wait") . "!<br />", $pconfig['devicespecialfile']);
					disks_geli_init($pconfig['devicespecialfile'], $pconfig['aalgo'], $pconfig['ealgo'], $pconfig['keylen'], $pconfig['passphrase'], true);
				}

				// Attach the disk.
				echo(sprintf(gettext("Attaching provider '%s'."), $pconfig['devicespecialfile']) . "<br />");
				disks_geli_attach($pconfig['devicespecialfile'], $pconfig['passphrase'], true);

				echo('</pre>');
				}
				?>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<script type="text/javascript">
<!--
ealgo_change();
//-->
</script>
<?php include("fend.inc");?>
