#!/usr/local/bin/php
<?php
/*
	services_rsyncd_module_edit.php

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

$pgtitle = array(gettext("Services"), gettext("Rsync"), gettext("Server"), gettext("Module"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['mounts']['mount']) || !is_array($config['mounts']['mount']))
	$config['mounts']['mount'] = array();

if (!isset($config['rsyncd']['module']) || !is_array($config['rsyncd']['module']))
	$config['rsyncd']['module'] = array();

array_sort_key($config['mounts']['mount'], "devicespecialfile");
array_sort_key($config['rsyncd']['module'], "name");

$a_mount = &$config['mounts']['mount'];
$a_module = &$config['rsyncd']['module'];

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_module, "uuid")))) {
	$pconfig['uuid'] = $a_module[$cnid]['uuid'];
	$pconfig['name'] = $a_module[$cnid]['name'];
	$pconfig['path'] = $a_module[$cnid]['path'];
	$pconfig['comment'] = $a_module[$cnid]['comment'];
	$pconfig['list'] = isset($a_module[$cnid]['list']);
	$pconfig['rwmode'] = $a_module[$cnid]['rwmode'];
	$pconfig['maxconnections'] = $a_module[$cnid]['maxconnections'];
	$pconfig['hostsallow'] = $a_module[$cnid]['hostsallow'];
	$pconfig['hostsdeny'] = $a_module[$cnid]['hostsdeny'];
	$pconfig['uid'] = $a_module[$cnid]['uid'];
	$pconfig['gid'] = $a_module[$cnid]['gid'];
	if (is_array($a_module[$cnid]['auxparam']))
		$pconfig['auxparam'] = implode("\n", $a_module[$cnid]['auxparam']);
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['name'] = "";
	$pconfig['path'] = "";
	$pconfig['comment'] = "";
	$pconfig['list'] = true;
	$pconfig['rwmode'] = "rw";
	$pconfig['maxconnections'] = "0";
	$pconfig['hostsallow'] = "";
	$pconfig['hostsdeny'] = "";
	$pconfig['uid'] = "";
	$pconfig['gid'] = "";
	$pconfig['auxparam'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: services_rsyncd_module.php");
		exit;
	}

	// Input validation.
	$reqdfields = explode(" ", "name comment");
	$reqdfieldsn = array(gettext("Name"), gettext("Comment"));
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

	$reqdfieldst = explode(" ", "string string");
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if(!$input_errors) {
		$module = array();
		$module['uuid'] = $_POST['uuid'];
		$module['name'] = $_POST['name'];
		$module['path'] = $_POST['path'];
		$module['comment'] = $_POST['comment'];
		$module['list'] = $_POST['list'] ? true : false;
		$module['rwmode'] = $_POST['rwmode'];
		$module['maxconnections'] = $_POST['maxconnections'];
		$module['hostsallow'] = $_POST['hostsallow'];
		$module['hostsdeny'] = $_POST['hostsdeny'];
		$module['uid'] = $_POST['uid'];
		$module['gid'] = $_POST['gid'];

		# Write additional parameters.
		unset($module['auxparam']);
		foreach (explode("\n", $_POST['auxparam']) as $auxparam) {
			$auxparam = trim($auxparam, "\t\n\r");
			if (!empty($auxparam))
				$module['auxparam'][] = $auxparam;
		}

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_module[$cnid] = $module;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_module[] = $module;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("rsyncd", $mode, $module['uuid']);
		write_config();

    header("Location: services_rsyncd_module.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
		<td class="tabnavtbl">
  		<ul id="tabnav">
				<li class="tabact"><a href="services_rsyncd.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Server");?></span></a></li>
			  <li class="tabinact"><a href="services_rsyncd_client.php"><span><?=gettext("Client");?></span></a></li>
			  <li class="tabinact"><a href="services_rsyncd_local.php"><span><?=gettext("Local");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav2">
				<li class="tabinact"><a href="services_rsyncd.php"><span><?=gettext("Settings");?></span></a></li>
				<li class="tabact"><a href="services_rsyncd_module.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Modules");?></span></a></li>
			</ul>
		</td>
	</tr>
  <tr>
    <td class="tabcont">
			<form action="services_rsyncd_module_edit.php" method="post" name="iform" id="iform">
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
				  <td width="22%" valign="top" class="vncellreq"><?=gettext("Path");?></td>
				  <td width="78%" class="vtable">
				  	<input name="path" type="text" class="formfld" id="path" size="60" value="<?=htmlspecialchars($pconfig['path']);?>" />
				  	<input name="browse" type="button" class="formbtn" id="Browse" onclick='ifield = form.path; filechooser = window.open("filechooser.php?p="+escape(ifield.value)+"&amp;sd=<?=$g['media_path'];?>", "filechooser", "scrollbars=yes,toolbar=no,menubar=no,statusbar=no,width=550,height=300"); filechooser.ifield = ifield; window.ifield = ifield;' value="..." /><br />
				  	<span class="vexpl"><?=gettext("Path to be shared.");?></span>
				  </td>
				</tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("List");?></td>
			      <td width="78%" class="vtable">
			      	<input name="list" type="checkbox" id="list" value="yes" <?php if ($pconfig['list']) echo "checked=\"checked\""; ?> />
			      	<?=gettext("Enable module listing.");?><br />
			        <span class="vexpl"><?=gettext("This option determines if this module should be listed when the client asks for a listing of available modules. By setting this to false you can create hidden modules.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Access mode");?></td>
			      <td width="78%" class="vtable">
			        <select name="rwmode" size="1" id="rwmode">
		            <option value="ro" <?php if ("ro" === $pconfig['rwmode']) echo "selected=\"selected\"";?>><?=gettext("Read only");?></option>
		            <option value="rw" <?php if ("rw" === $pconfig['rwmode']) echo "selected=\"selected\"";?>><?=gettext("Read/Write");?></option>
		            <option value="wo" <?php if ("wo" === $pconfig['rwmode']) echo "selected=\"selected\"";?>><?=gettext("Write only");?></option>
			        </select><br />
			        <span class="vexpl"><?=gettext("This controls the access a remote host has to this module.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Maximum connections");?></td>
			      <td width="78%" class="vtable">
			        <input name="maxconnections" type="text" id="maxconnections" size="5" value="<?=htmlspecialchars($pconfig['maxconnections']);?>" /><br />
			        <span class="vexpl"><?=gettext("Maximum number of simultaneous connections. Default is 0 (unlimited).");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("User ID");?></td>
			      <td width="78%" class="vtable">
			        <input name="uid" type="text" class="formfld" id="uid" size="60" value="<?=htmlspecialchars($pconfig['uid']);?>" /><br />
			        <span class="vexpl"><?=sprintf(gettext("This option specifies the user name or user ID that file transfers to and from that module should take place. In combination with the '%s' option this determines what file permissions are available. Leave this field empty to use default settings."), gettext("Group ID"));?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Group ID");?></td>
			      <td width="78%" class="vtable">
			        <input name="gid" type="text" class="formfld" id="gid" size="60" value="<?=htmlspecialchars($pconfig['gid']);?>" /><br />
			        <span class="vexpl"><?=gettext("This option specifies the group name or group ID that file transfers to and from that module should take place. Leave this field empty to use default settings.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Hosts allow");?></td>
			      <td width="78%" class="vtable">
			        <input name="hostsallow" type="text" class="formfld" id="hostsallow" size="60" value="<?=htmlspecialchars($pconfig['hostsallow']);?>" /><br />
			        <span class="vexpl"><?=gettext("This option is a comma, space, or tab delimited set of hosts which are permitted to access this module. You can specify the hosts by name or IP number. Leave this field empty to use default settings.");?></span>
			      </td>
			    </tr>
			    <tr>
			      <td width="22%" valign="top" class="vncell"><?=gettext("Hosts deny");?></td>
			      <td width="78%" class="vtable">
			        <input name="hostsdeny" type="text" class="formfld" id="hostsdeny" size="60" value="<?=htmlspecialchars($pconfig['hostsdeny']);?>" /><br />
			        <span class="vexpl"><?=gettext("This option is a comma, space, or tab delimited set of host which are NOT permitted to access this module. Where the lists conflict, the allow list takes precedence. In the event that it is necessary to deny all by default, use the keyword ALL (or the netmask 0.0.0.0/0) and then explicitly specify to the hosts allow parameter those hosts that should be permitted access. Leave this field empty to use default settings.");?></span>
			      </td>
			    </tr>
			    <?php html_textarea("auxparam", gettext("Auxiliary parameters"), $pconfig['auxparam'], gettext("These parameters will be added to the module configuration in rsyncd.conf.") . " " . sprintf(gettext("Please check the <a href='%s' target='_blank'>documentation</a>."), "http://rsync.samba.org/ftp/rsync/rsync.html"), false, 65, 5, false, false);?>
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
