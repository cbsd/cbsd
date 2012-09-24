#!/usr/local/bin/php
<?php
/*
	services_afp.php

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

$pgtitle = array(gettext("Services"),gettext("AFP"));

if (!isset($config['afp']) || !is_array($config['afp']))
	$config['afp'] = array();

$pconfig['enable'] = isset($config['afp']['enable']);
$pconfig['afpname'] = $config['afp']['afpname'];
$pconfig['guest'] = isset($config['afp']['guest']);
$pconfig['local'] = isset($config['afp']['local']);
$pconfig['noddp'] = isset($config['afp']['noddp']);

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	if ($_POST['enable'] && !($_POST['guest'] || $_POST['local'])) {
		$input_errors[] = gettext("You must select at least one authentication method.");
	}

	if (!$input_errors) {
		$config['afp']['enable'] = $_POST['enable'] ? true : false;
		$config['afp']['afpname'] = $_POST['afpname'];
		$config['afp']['guest'] = $_POST['guest'] ? true : false;
		$config['afp']['local'] = $_POST['local'] ? true : false;
		$config['afp']['noddp'] = $_POST['noddp'] ? true : false;

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= rc_update_service("afpd");
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
	document.iform.afpname.disabled = endis;
	document.iform.guest.disabled = endis;
	document.iform.local.disabled = endis;
	document.iform.noddp.disabled = endis;
}
//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
        <li class="tabact"><a href="services_afp.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Settings");?></span></a></li>
        <li class="tabinact"><a href="services_afp_share.php"><span><?=gettext("Shares");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
			<form action="services_afp.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
					<?php html_titleline_checkbox("enable", gettext("Apple Filing Protocol"), $pconfig['enable'] ? true : false, gettext("Enable"), "enable_change(false)");?>
					<tr>
						<td width="22%" valign="top" class="vncell"><?=gettext("Server Name");?></td>
						<td width="78%" class="vtable">
							<input name="afpname" type="text" class="formfld" id="afpname" size="30" value="<?=htmlspecialchars($pconfig['afpname']);?>" /><br />
							<?=gettext("Name of the server. If this field is left empty the default server is specified.");?><br />
						</td>
					</tr>
					<tr>
						<td width="22%" valign="top" class="vncell"><strong><?=gettext("Authentication");?></strong></td>
						<td width="78%" class="vtable">
							<input name="guest" id="guest" type="checkbox" value="yes" <?php if ($pconfig['guest']) echo "checked=\"checked\"";?> />
							<?=gettext("Enable guest access.");?><br />
							<input name="local" id="local" type="checkbox" value="yes" <?php if ($pconfig['local']) echo "checked=\"checked\"";?> />
							<?=gettext("Enable local user authentication.");?>
						</td>
					</tr>
					<?php html_checkbox("noddp", gettext("DDP"), $pconfig['noddp'] ? true : false, gettext("Disable AFP-over-Appletalk to prevent DDP connections."));?>
			  </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save and Restart");?>" onclick="enable_change(true)" />
				</div>
				<div id="remarks">
					<?php html_remark("note", gettext("Note"), sprintf(gettext("You have to activate <a href='%s'>Zeroconf/Bonjour</a> to advertise this service to clients."), "system_advanced.php"));?>
				</div>
				<?php include("formend.inc");?>
			</form>
		</td>
  </tr>
</table>
<script type="text/javascript">
<!--
enable_change(false);
//-->
</script>
<?php include("fend.inc");?>
