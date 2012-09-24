#!/usr/local/bin/php
<?php
/*
	services_samba_share_edit.php

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

$uuid = $_GET['uuid'];
if (isset($_POST['uuid']))
	$uuid = $_POST['uuid'];

$pgtitle = array(gettext("Services"), gettext("CIFS/SMB"), gettext("Share"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['mounts']['mount']) || !is_array($config['mounts']['mount']))
	$config['mounts']['mount'] = array();

if (!isset($config['samba']['share']) || !is_array($config['samba']['share']))
	$config['samba']['share'] = array();

array_sort_key($config['mounts']['mount'], "devicespecialfile");
array_sort_key($config['samba']['share'], "name");

$a_mount = &$config['mounts']['mount'];
$a_share = &$config['samba']['share'];
$default_shadowformat = "auto-%Y%m%d-%H%M%S";

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_share, "uuid")))) {
	$pconfig['uuid'] = $a_share[$cnid]['uuid'];
	$pconfig['name'] = $a_share[$cnid]['name'];
	$pconfig['path'] = $a_share[$cnid]['path'];
	$pconfig['comment'] = $a_share[$cnid]['comment'];
	$pconfig['readonly'] = isset($a_share[$cnid]['readonly']);
	$pconfig['browseable'] = isset($a_share[$cnid]['browseable']);
	$pconfig['guest'] = isset($a_share[$cnid]['guest']);
	$pconfig['inheritpermissions'] = isset($a_share[$cnid]['inheritpermissions']);
	$pconfig['recyclebin'] = isset($a_share[$cnid]['recyclebin']);
	$pconfig['hidedotfiles'] = isset($a_share[$cnid]['hidedotfiles']);
	$pconfig['shadowcopy'] = isset($a_share[$cnid]['shadowcopy']);
	$pconfig['shadowformat'] = $a_share[$cnid]['shadowformat'];
	$pconfig['zfsacl'] = isset($a_share[$cnid]['zfsacl']);
	$pconfig['hostsallow'] = $a_share[$cnid]['hostsallow'];
	$pconfig['hostsdeny'] = $a_share[$cnid]['hostsdeny'];
	if (is_array($a_share[$cnid]['auxparam']))
		$pconfig['auxparam'] = implode("\n", $a_share[$cnid]['auxparam']);
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['name'] = "";
	$pconfig['path'] = "";
	$pconfig['comment'] = "";
	$pconfig['readonly'] = false;
	$pconfig['browseable'] = true;
	$pconfig['guest'] = true;
	$pconfig['inheritpermissions'] = true;
	$pconfig['recyclebin'] = false;
	$pconfig['hidedotfiles'] = true;
	$pconfig['shadowcopy'] = true;
	$pconfig['shadowformat'] = $default_shadowformat;
	$pconfig['zfsacl'] = false;
	$pconfig['hostsallow'] = "";
	$pconfig['hostsdeny'] = "";
	$pconfig['auxparam'] = "";
}
if ($pconfig['shadowformat'] == "") {
	$pconfig['shadowformat'] = $default_shadowformat;
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: services_samba_share.php");
		exit;
	}

	// Input validation.
	$reqdfields = explode(" ", "name comment");
	$reqdfieldsn = array(gettext("Name"), gettext("Comment"));
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

	$reqdfieldst = explode(" ", "string string");
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	// Check for duplicates.
	$index = array_search_ex($_POST['name'], $a_share, "name");
	if (FALSE !== $index) {
		if (!((FALSE !== $cnid) && ($a_share[$cnid]['uuid'] === $a_share[$index]['uuid']))) {
			$input_errors[] = gettext("The share name is already used.");
		}
	}

	if (!$input_errors) {
		$share = array();
		$share['uuid'] = $_POST['uuid'];
		$share['name'] = $_POST['name'];
		$share['path'] = $_POST['path'];
		$share['comment'] = $_POST['comment'];
		$share['readonly'] = $_POST['readonly'] ? true : false;
		$share['browseable'] = $_POST['browseable'] ? true : false;
		$share['guest'] = $_POST['guest'] ? true : false;
		$share['inheritpermissions'] = $_POST['inheritpermissions'] ? true : false;
		$share['recyclebin'] = $_POST['recyclebin'] ? true : false;
		$share['hidedotfiles'] = $_POST['hidedotfiles'] ? true : false;
		$share['shadowcopy'] = $_POST['shadowcopy'] ? true : false;
		$share['shadowformat'] = $_POST['shadowformat'];
		$share['zfsacl'] = $_POST['zfsacl'] ? true : false;
		$share['hostsallow'] = $_POST['hostsallow'];
		$share['hostsdeny'] = $_POST['hostsdeny'];

		# Write additional parameters.
		unset($share['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$share['auxparam'][] = $auxparam;
		}

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_share[$cnid] = $share;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_share[] = $share;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("smbshare", $mode, $share['uuid']);
		write_config();

    header("Location: services_samba_share.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
				<li class="tabinact"><a href="services_samba.php"><span><?=gettext("Settings");?></span></a></li>
				<li class="tabact"><a href="services_samba_share.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Shares");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="services_samba_share_edit.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors); ?>
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
			  	<tr>
			      <td width="22%" valign="top" class="vncellreq"><?=gettext("Name");?></td>
			      <td width="78%" class="vtable">
			        <input name="name" type="text" class="formfld" id="name" size="30" value="<?=htmlspecialchars($pconfig['name']);?>" />
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncellreq"><?=gettext("Comment");?></td>
			      <td width="78%" class="vtable">
			        <input name="comment" type="text" class="formfld" id="comment" size="30" value="<?=htmlspecialchars($pconfig['comment']);?>" />
			      </td>
			    </tr>
			    <tr>
						<td width="22%" valign="top" class="vncellreq"><?=gettext("Path"); ?></td>
						<td width="78%" class="vtable">
							<input name="path" type="text" class="formfld" id="path" size="60" value="<?=htmlspecialchars($pconfig['path']);?>" />
							<input name="browse" type="button" class="formbtn" id="Browse" onclick='ifield = form.path; filechooser = window.open("filechooser.php?p="+escape(ifield.value)+"&amp;sd=<?=$g['media_path'];?>", "filechooser", "scrollbars=yes,toolbar=no,menubar=no,statusbar=no,width=550,height=300"); filechooser.ifield = ifield; window.ifield = ifield;' value="..." /><br />
							<span class="vexpl"><?=gettext("Path to be shared.");?></span>
					  </td>
					</tr>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Read only");?></td>
			      <td width="78%" class="vtable">
							<input name="readonly" type="checkbox" id="readonly" value="yes" <?php if ($pconfig['readonly']) echo "checked=\"checked\""; ?> />
							<?=gettext("Set read only");?><br />
							<span class="vexpl"><?=gettext("If this parameter is set, then users may not create or modify files in the share.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Browseable");?></td>
			      <td width="78%" class="vtable">
			      	<input name="browseable" type="checkbox" id="browseable" value="yes" <?php if ($pconfig['browseable']) echo "checked=\"checked\""; ?> />
			      	<?=gettext("Set browseable");?><br />
			        <span class="vexpl"><?=gettext("This controls whether this share is seen in the list of available shares in a net view and in the browse list.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Guest");?></td>
			      <td width="78%" class="vtable">
			      	<input name="guest" type="checkbox" id="guest" value="yes" <?php if ($pconfig['guest']) echo "checked=\"checked\""; ?> />
			      	<?=gettext("Enable guest access");?><br />
			        <span class="vexpl"><?=gettext("This controls whether this share is accessible by guest account.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Inherit permissions");?></td>
			      <td width="78%" class="vtable">
			        <input name="inheritpermissions" type="checkbox" id="inheritpermissions" value="yes" <?php if ($pconfig['inheritpermissions']) echo "checked=\"checked\""; ?> />
			        <?=gettext("Enable permission inheritance");?><br />
							<span class="vexpl"><?=gettext("The permissions on new files and directories are normally governed by create mask and directory mask but the inherit permissions parameter overrides this. This can be particularly useful on systems with many users to allow a single share to be used flexibly by each user.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Recycle bin");?></td>
			      <td width="78%" class="vtable">
			        <input name="recyclebin" type="checkbox" id="recyclebin" value="yes" <?php if ($pconfig['recyclebin']) echo "checked=\"checked\""; ?> />
			        <?=gettext("Enable recycle bin");?><br />
			        <span class="vexpl"><?=gettext("This will create a recycle bin on the share.");?></span>
			      </td>
			    </tr>
			    <tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Hide dot files");?></td>
			      <td width="78%" class="vtable">
							<input name="hidedotfiles" type="checkbox" id="hidedotfiles" value="yes" <?php if ($pconfig['hidedotfiles']) echo "checked=\"checked\"";?> />
							<span class="vexpl"><?=gettext("This parameter controls whether files starting with a dot appear as hidden files.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Shadow Copy");?></td>
			      <td width="78%" class="vtable">
			        <input name="shadowcopy" type="checkbox" id="shadowcopy" value="yes" <?php if ($pconfig['shadowcopy']) echo "checked=\"checked\""; ?> />
			        <?=gettext("Enable shadow copy");?><br />
			        <span class="vexpl"><?=gettext("This will provide shadow copy created by auto snapshot. (ZFS only)");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Shadow Copy format");?></td>
			      <td width="78%" class="vtable">
			        <input name="shadowformat" type="text" class="formfld" id="shadowformat" size="60" value="<?=htmlspecialchars($pconfig['shadowformat']);?>" /><br />
			        <span class="vexpl"><?=sprintf(gettext("The custom format of the snapshot for shadow copy service can be specified. The default format is %s used for ZFS auto snapshot."), $default_shadowformat);?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("ZFS ACL");?></td>
			      <td width="78%" class="vtable">
			        <input name="zfsacl" type="checkbox" id="zfsacl" value="yes" <?php if ($pconfig['zfsacl']) echo "checked=\"checked\""; ?> />
			        <?=gettext("Enable ZFS ACL");?><br />
			        <span class="vexpl"><?=gettext("This will provide ZFS ACL support. (ZFS only)");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Hosts allow");?></td>
			      <td width="78%" class="vtable">
			        <input name="hostsallow" type="text" class="formfld" id="hostsallow" size="60" value="<?=htmlspecialchars($pconfig['hostsallow']);?>" /><br />
			        <span class="vexpl"><?=gettext("This option is a comma, space, or tab delimited set of hosts which are permitted to access this share. You can specify the hosts by name or IP number. Leave this field empty to use default settings.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Hosts deny");?></td>
			      <td width="78%" class="vtable">
			        <input name="hostsdeny" type="text" class="formfld" id="hostsdeny" size="60" value="<?=htmlspecialchars($pconfig['hostsdeny']);?>" /><br />
			        <span class="vexpl"><?=gettext("This option is a comma, space, or tab delimited set of host which are NOT permitted to access this share. Where the lists conflict, the allow list takes precedence. In the event that it is necessary to deny all by default, use the keyword ALL (or the netmask 0.0.0.0/0) and then explicitly specify to the hosts allow parameter those hosts that should be permitted access. Leave this field empty to use default settings.");?></span>
			      </td>
			    </tr>
			    <?php html_textarea("auxparam", gettext("Auxiliary parameters"), $pconfig['auxparam'], sprintf(gettext("These parameters are added to [Share] section of %s."), "smb.conf") . " " . sprintf(gettext("Please check the <a href='%s' target='_blank'>documentation</a>."), "http://us1.samba.org/samba/docs/man/manpages-3/smb.conf.5.html"), false, 65, 5, false, false);?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=(isset($uuid) && (FALSE !== $cnid)) ? gettext("Save") : gettext("Add")?>" />
					<input name="Cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>" />
					<input name="uuid" type="hidden" value="<?=$pconfig['uuid'];?>" />
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
