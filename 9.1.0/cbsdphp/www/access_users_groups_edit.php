#!/usr/local/bin/php
<?php
/*
	access_users_groups_edit.php
	
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

$uuid = $_GET['uuid'];
if (isset($_POST['uuid']))
	$uuid = $_POST['uuid'];

$pgtitle = array(gettext("Access"), gettext("Groups"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['access']['group']) || !is_array($config['access']['group']))
    $config['access']['group'] = array();
	
array_sort_key($config['access']['group'], "name");

$a_group = &$config['access']['group'];
$a_group_system = system_get_group_list();

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_group, "uuid")))) {
	$pconfig['uuid'] = $a_group[$cnid]['uuid'];
	$pconfig['groupid'] = $a_group[$cnid]['id'];
	$pconfig['name'] = $a_group[$cnid]['name'];
	$pconfig['desc'] = $a_group[$cnid]['desc'];
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['groupid'] = get_nextgroup_id();
	$pconfig['name'] = "";
	$pconfig['desc'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: access_users_groups.php");
		exit;
	}

	// Input validation
	$reqdfields = explode(" ", "name desc groupid");
	$reqdfieldsn = array(gettext("Name"),gettext("Description"),gettext("Group ID"));
	$reqdfieldst = explode(" ", "string string numeric");

	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);
	do_input_validation_type($_POST, $reqdfields, $reqdfieldsn, $reqdfieldst, $input_errors);

	if (($_POST['name'] && !is_domain($_POST['name']))) {
		$input_errors[] = gettext("The group name contains invalid characters.");
	}

	if (($_POST['desc'] && !is_validdesc($_POST['desc']))) {
		$input_errors[] = gettext("The group description contains invalid characters.");
	}

	// Check for name conflicts. Only check if group is created.
	if (!(isset($uuid) && (FALSE !== $cnid)) &&
		((is_array($a_group_system) && array_key_exists($_POST['name'], $a_group_system)) ||
		(is_array($a_group_system) && in_array($_POST['groupid'], $a_group_system)) ||
		(false !== array_search_ex($_POST['name'], $a_group, "name")))) {
		$input_errors[] = gettext("This group already exists in the group list.");
	}

	// Validate if ID is unique. Only check if user is created.
	if (!(isset($uuid) && (FALSE !== $cnid)) && (false !== array_search_ex($_POST['groupid'], $a_group, "id"))) {
		$input_errors[] = gettext("The unique group ID is already used.");
	}

	if (!$input_errors) {
		$groups = array();
		$groups['uuid'] = $_POST['uuid'];
		$groups['id'] = $_POST['groupid'];
		$groups['name'] = $_POST['name'];
		$groups['desc'] = $_POST['desc'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_group[$cnid] = $groups;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_group[] = $groups;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("userdb_group", $mode, $groups['uuid']);
		write_config();

		header("Location: access_users_groups.php");
		exit;
	}
}

// Get next group id.
// Return next free user id.
function get_nextgroup_id() {
	global $config;

	// Get next free user id.
	exec("/usr/sbin/pw groupnext", $output);
	$output = explode(":", $output[0]);
	$id = intval($output[0]);

	// Check if id is already in usage. If the user did not press the 'Apply'
	// button 'pw' did not recognize that there are already several new users
	// configured because the user db is not updated until 'Apply' is pressed.
	$a_group = $config['access']['group'];
	if (false !== array_search_ex(strval($id), $a_group, "id")) {
		do {
			$id++; // Increase id until a unused one is found.
		} while (false !== array_search_ex(strval($id), $a_group, "id"));
	}

	return $id;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabinact"><a href="access_users.php"><span><?=gettext("Users");?></span></a></li>
				<li class="tabact"><a href="access_users_groups.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Groups");?></span></a></li>
			</ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<form action="access_users_groups_edit.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors); ?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_inputbox("name", gettext("Name"), $pconfig['name'], gettext("Group name."), true, 20, isset($uuid) && (FALSE !== $cnid));?>
					<?php html_inputbox("groupid", gettext("Group ID"), $pconfig['groupid'], gettext("Group numeric id."), true, 20, isset($uuid) && (FALSE !== $cnid));?>
					<?php html_inputbox("desc", gettext("Description"), $pconfig['desc'], gettext("Group description."), true, 20);?>
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
