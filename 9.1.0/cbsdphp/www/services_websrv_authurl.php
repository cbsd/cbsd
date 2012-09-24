#!/usr/local/bin/php
<?php
/*
	services_websrv_authurl.php

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

$pgtitle = array(gettext("Services"), gettext("Webserver"), gettext("Authenticate URL"), isset($uuid) ? gettext("Edit") : gettext("Add"));

if (!isset($config['websrv']['authentication']['url']) || !is_array($config['websrv']['authentication']['url']))
	$config['websrv']['authentication']['url'] = array();

array_sort_key($config['websrv']['authentication']['url'], "path");
$a_authurl = &$config['websrv']['authentication']['url'];

if (isset($uuid) && (FALSE !== ($cnid = array_search_ex($uuid, $a_authurl, "uuid")))) {
	$pconfig['uuid'] = $a_authurl[$cnid]['uuid'];
	$pconfig['path'] = $a_authurl[$cnid]['path'];
	$pconfig['realm'] = $a_authurl[$cnid]['realm'];
} else {
	$pconfig['uuid'] = uuid();
	$pconfig['path'] = "";
	$pconfig['realm'] = "";
}

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['Cancel']) {
		header("Location: services_websrv.php");
		exit;
	}

	// Input validation.
	$reqdfields = explode(" ", "path realm");
	$reqdfieldsn = array(gettext("URL"), gettext("Realm"));
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

	// Check if URL is already configured.
	$index = array_search_ex($_POST['path'], $a_authurl, "path");
	if (FALSE !== $index) {
		if (!((FALSE !== $cnid) && ($a_authurl[$cnid]['uuid'] === $a_authurl[$index]['uuid']))) {
			$input_errors[] = gettext("This URL is already configured.");
		}
	}

	if (!$input_errors) {
		$url = array();
		$url['uuid'] = $_POST['uuid'];
		$url['path'] = $_POST['path'];
		$url['realm'] = $_POST['realm'];

		if (isset($uuid) && (FALSE !== $cnid)) {
			$a_authurl[$cnid] = $url;
			$mode = UPDATENOTIFY_MODE_MODIFIED;
		} else {
			$a_authurl[] = $url;
			$mode = UPDATENOTIFY_MODE_NEW;
		}

		updatenotify_set("websrvauth", $mode, $url['uuid']);
		write_config();

		header("Location: services_websrv.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
	<td class="tabcont">
		<form action="services_websrv_authurl.php" method="post" name="iform" id="iform">
			<?php if ($input_errors) print_input_errors($input_errors);?>
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<?php html_inputbox("path", gettext("Path"), $pconfig['path'], gettext("Path of the URL relative to document root."), true, 60);?>
				<?php html_inputbox("realm", gettext("Realm"), $pconfig['realm'], gettext("String displayed in the dialog presented to the user when accessing the URL."), true, 20);?>
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
