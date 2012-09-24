#!/usr/local/bin/php
<?php
/*
	disks_init.php
	
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
require("sajax/sajax.php");

$pgtitle = array(gettext("Disks"), gettext("Format"));

// Get list of all supported file systems.
$a_fst = get_fstype_list();
unset($a_fst['ntfs']); // Remove NTFS: can't format on NTFS under FreeNAS
unset($a_fst['geli']); // Remove geli
unset($a_fst['cd9660']); // Remove cd9660: can't format a CD/DVD !
$a_fst = array_slice($a_fst, 1); // Remove the first blank line 'unknown'
unset($a_fst['ufs']); // Remove old UFS type: Now FreeNAS will impose only one UFS type: GPT/EFI with softupdate
unset($a_fst['ufs_no_su']);
unset($a_fst['ufsgpt_no_su']);

// Load the /etc/cfdevice file to find out on which disk the OS is installed.
$cfdevice = trim(file_get_contents("{$g['etc_path']}/cfdevice"));
$cfdevice = "/dev/{$cfdevice}";

// Get list of all configured disks (physical and virtual).
$a_disk = get_conf_all_disks_list_filtered();

function get_fs_type($devicespecialfile) {
	global $a_disk;
	$index = array_search_ex($devicespecialfile, $a_disk, "devicespecialfile");
	if (false === $index)
		return "";
	return $a_disk[$index]['fstype'];
}

// Advanced Format
$pconfig['aft4k'] = false;

sajax_init();
sajax_export("get_fs_type");
sajax_handle_client_request();

if ($_POST) {
	unset($input_errors);
	unset($errormsg);
	unset($do_format);

	// Input validation.
	$reqdfields = explode(" ", "disk type");
	$reqdfieldsn = array(gettext("Disk"),gettext("Type"));
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

	$reqdfields = explode(" ", "volumelabel");
	$reqdfieldsn = array(gettext("Volume label"));
	$reqdfieldst = explode(" ", "alias");
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (!$input_errors) {
		$do_format = true;
		$disk = $_POST['disk'];
		$type = $_POST['type'];
		$minspace = $_POST['minspace'];
		$notinitmbr= $_POST['notinitmbr'];
		$volumelabel = $_POST['volumelabel'];
		$aft4k = $_POST['aft4k'] ? true : false;

		// Check whether disk is mounted.
		if (disks_ismounted_ex($disk, "devicespecialfile")) {
			$errormsg = sprintf(gettext("The disk is currently mounted! <a href='%s'>Unmount</a> this disk first before proceeding."), "disks_mount_tools.php?disk={$disk}&action=umount");
			$do_format = false;
		}

		// Check if user tries to format the OS disk.
		if (preg_match("/" . preg_quote($disk, "/") . "\D+/", $cfdevice)) {
			$input_errors[] = gettext("Can't format the OS origin disk!");
		}

		if ($do_format) {
			// Set new file system type attribute ('fstype') in configuration.
			set_conf_disk_fstype($disk, $type);

			write_config();

			// Update list of configured disks.
			$a_disk = get_conf_all_disks_list_filtered();
		}
	}
}

if (!isset($do_format)) {
	$do_format = false;
	$disk = '';
	$type = '';
	$minspace = '';
	$volumelabel = '';
	$aft4k = false;
}
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">//<![CDATA[
<?php sajax_show_javascript();?>
//]]>
</script>
<script type="text/javascript" src="javascript/disks_init.js"></script>
<form action="disks_init.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabcont">
				<?php if($input_errors) print_input_errors($input_errors);?>
				<?php if($errormsg) print_error_box($errormsg);?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
			    <tr>
			      <td valign="top" class="vncellreq"><?=gettext("Disk"); ?></td>
			      <td class="vtable">
			        <select name="disk" class="formfld" id="disk" onchange="disk_change()">
								<option value=""><?=gettext("Must choose one");?></option>
								<?php foreach ($a_disk as $diskv):?>
								<?php if (0 == strcmp($diskv['size'], "NA")) continue;?>
								<?php if (1 == disks_exists($diskv['devicespecialfile'])) continue;?>
								<option value="<?=$diskv['devicespecialfile'];?>" <?php if ($diskv['devicespecialfile'] === $disk) echo "selected=\"selected\"";?>>
								<?php $diskinfo = disks_get_diskinfo($diskv['devicespecialfile']); echo htmlspecialchars("{$diskv['name']}: {$diskinfo['mediasize_mbytes']}MB ({$diskv['desc']})");?>
								</option>
								<?php endforeach;?>
			        </select>
			      </td>
					</tr>
					<tr>
				    <td valign="top" class="vncellreq"><?=gettext("File system");?></td>
				    <td class="vtable">
				      <select name="type" class="formfld" id="type" onchange="type_change()">
				        <?php foreach ($a_fst as $fstval => $fstname): ?>
				        <option value="<?=$fstval;?>" <?php if($type == $fstval) echo 'selected="selected"';?>><?=htmlspecialchars($fstname);?></option>
				        <?php endforeach; ?>
				       </select>
				    </td>
					</tr>
					<tr id="volumelabel_tr">
						<td width="22%" valign="top" class="vncell"><?=gettext("Volume label");?></td>
						<td width="78%" class="vtable">
							<input name="volumelabel" type="text" class="formfld" id="volumelabel" size="20" value="<?=htmlspecialchars($volumelabel);?>" /><br />
							<?=gettext("Volume label of the new file system.");?>
						</td>
					</tr>
					<tr id="minspace_tr">
						<td width="22%" valign="top" class="vncell"><?=gettext("Minimum free space");?></td>
						<td width="78%" class="vtable">
							<select name="minspace" class="formfld" id="minspace">
							<?php $types = explode(",", "8,7,6,5,4,3,2,1"); $vals = explode(" ", "8 7 6 5 4 3 2 1");?>
							<?php $j = 0; for ($j = 0; $j < count($vals); $j++): ?>
								<option value="<?=$vals[$j];?>"><?=htmlspecialchars($types[$j]);?></option>
							<?php endfor; ?>
							</select>
							<br /><?=gettext("Specify the percentage of space held back from normal users. Note that lowering the threshold can adversely affect performance and auto-defragmentation.") ;?>
						</td>
					</tr>
			    <?php html_checkbox("aft4k", gettext("Advanced Format"), $pconfig['aft4k'] ? true : false, gettext("Enable Advanced Format (4KB sector)"), "", false, "");?>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Don't Erase MBR");?></td>
			      <td width="78%" class="vtable">
			        <input name="notinitmbr" id="notinitmbr" type="checkbox" value="yes" />
			        <?=gettext("Don't erase the MBR (useful for some RAID controller cards)");?>
						</td>
				  </tr>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Format disk");?>" onclick="return confirm('<?=gettext("Do you really want to format this disk? All data will be lost!");?>')" />
				</div>
				<?php if ($do_format) {
				echo(sprintf("<div id='cmdoutput'>%s</div>", gettext("Command output:")));
				echo('<pre class="cmdoutput">');
				ob_end_flush();
				disks_format($disk,$type,$notinitmbr,$minspace,$volumelabel, $aft4k);
				echo('</pre>');
				}
				?>
				<div id="remarks">
					<?php html_remark("Warning", gettext("Warning"), sprintf(gettext("UFS is the NATIVE file format for FreeBSD (the underlying OS of %s). Attempting to use other file formats such as FAT, FAT32, EXT2, EXT3, or NTFS can result in unpredictable results, file corruption, and loss of data!"), get_product_name()));?>
				</div>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<script type="text/javascript">
<!--
disk_change();
//-->
</script>
<?php include("fend.inc");?>
