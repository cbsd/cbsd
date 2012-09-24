#!/usr/local/bin/php
<?php
/*
	diag_log_settings.php
	
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
require("diag_log.inc");

$pgtitle = array(gettext("Diagnostics"), gettext("Log"), gettext("Settings"));

$pconfig['reverse']  = isset($config['syslogd']['reverse']);
$pconfig['nentries'] = $config['syslogd']['nentries'];
$pconfig['resolve']  = isset($config['syslogd']['resolve']);
if (is_array($config['syslogd']['remote'])) {
	$pconfig['enable'] = isset($config['syslogd']['remote']['enable']);
	$pconfig['ipaddr'] = $config['syslogd']['remote']['ipaddr'];
	$pconfig['daemon'] = isset($config['syslogd']['remote']['daemon']);
	$pconfig['ftp']    = isset($config['syslogd']['remote']['ftp']);
	$pconfig['rsyncd'] = isset($config['syslogd']['remote']['rsyncd']);
	$pconfig['smartd'] = isset($config['syslogd']['remote']['smartd']);
	$pconfig['sshd']   = isset($config['syslogd']['remote']['sshd']);
	$pconfig['system'] = isset($config['syslogd']['remote']['system']);
}

if (!$pconfig['nentries'])
	$pconfig['nentries'] = 50;

if ($_POST) {
	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	if ($_POST['enable'] && !is_ipaddr($_POST['ipaddr'])) {
		$input_errors[] = gettext("A valid IP address must be specified.");
	}
	if (($_POST['nentries'] < 5) || ($_POST['nentries'] > 1000)) {
		$input_errors[] = gettext("Number of log entries to show must be between 5 and 1000.");
	}

	if (!$input_errors) {
		$config['syslogd']['reverse'] = $_POST['reverse'] ? true : false;
		$config['syslogd']['nentries'] = (int)$_POST['nentries'];
		$config['syslogd']['resolve'] = $_POST['resolve'] ? true : false;
		$config['syslogd']['remote']['enable'] = $_POST['enable'] ? true : false;
		$config['syslogd']['remote']['ipaddr'] = $_POST['ipaddr'];
		$config['syslogd']['remote']['system'] = $_POST['system'] ? true : false;
		$config['syslogd']['remote']['ftp'] = $_POST['ftp'] ? true : false;
		$config['syslogd']['remote']['rsyncd'] = $_POST['rsyncd'] ? true : false;
		$config['syslogd']['remote']['sshd'] = $_POST['sshd'] ? true : false;
		$config['syslogd']['remote']['smartd'] = $_POST['smartd'] ? true : false;
		$config['syslogd']['remote']['daemon'] = $_POST['daemon'] ? true : false;

		write_config();

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval = rc_restart_service("syslogd");
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
	document.iform.ipaddr.disabled = endis;
	document.iform.sshd.disabled = endis;
	document.iform.system.disabled = endis;
	document.iform.ftp.disabled = endis;
	document.iform.rsyncd.disabled = endis;
	document.iform.smartd.disabled = endis;
	document.iform.daemon.disabled = endis;
}
// -->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav">
				<li class="tabinact"><a href="diag_log.php"><span><?=gettext("Log");?></span></a></li>
				<li class="tabact"><a href="diag_log_settings.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Settings");?></span></a></li>
			</ul>
		</td>
	</tr>
  <tr>
    <td class="tabcont">
    	<form action="diag_log_settings.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
		  	<table width="100%" border="0" cellpadding="6" cellspacing="0">
	        <tr>
	          <td width="22%" valign="top" class="vncell">&nbsp;</td>
	          <td width="78%" class="vtable">
							<input name="reverse" type="checkbox" id="reverse" value="yes" <?php if ($pconfig['reverse']) echo "checked=\"checked\""; ?> />
	            <strong><?=gettext("Show log entries in reverse order (newest entries on top)");?></strong>
						</td>
	        </tr>
	        <tr>
	          <td width="22%" valign="top" class="vncell">&nbsp;</td>
	          <td width="78%" class="vtable">
							<?=gettext("Number of log entries to show");?>:
	            <input name="nentries" id="nentries" type="text" class="formfld" size="4" value="<?=htmlspecialchars($pconfig['nentries']);?>" /></td>
	        </tr>
	        <tr>
	          <td width="22%" valign="top" class="vncell">&nbsp;</td>
	          <td width="78%" class="vtable">
							<input name="resolve" type="checkbox" id="resolve" value="yes" <?php if ($pconfig['resolve']) echo "checked=\"checked\""; ?> />
	            <strong><?=gettext("Resolve IP addresses to hostnames");?></strong><br />
	            <?php echo sprintf(gettext("Hint: If this is checked, IP addresses in %s logs are resolved to real hostnames where possible."), get_product_name());?><br />
							<?php echo sprintf(gettext("Warning: This can cause a huge delay in loading the %s log page!"), get_product_name());?>
						</td>
	        </tr>
	        <tr>
	          <td width="22%" valign="top" class="vncell">&nbsp;</td>
	          <td width="78%" class="vtable">
							<input name="enable" type="checkbox" id="enable" value="yes" <?php if ($pconfig['enable']) echo "checked=\"checked\""; ?> onclick="enable_change(false)" />
	            <strong><?=gettext("Enable syslog'ing to remote syslog server");?></strong></td>
	        </tr>
	        <tr>
	          <td width="22%" valign="top" class="vncell"><?=gettext("Remote syslog server");?></td>
	          <td width="78%" class="vtable">
							<input name="ipaddr" id="ipaddr" type="text" class="formfld" size="20" value="<?=htmlspecialchars($pconfig['ipaddr']);?>" />
	            <br />
	            <?=gettext("IP address of remote syslog server");?><br /><br />
							<input name="system" id="system" type="checkbox" value="yes" <?php if ($pconfig['system']) echo "checked=\"checked\""; ?> />
	            <?=gettext("System events");?><br />
							<input name="ftp" id="ftp" type="checkbox" value="yes" <?php if ($pconfig['ftp']) echo "checked=\"checked\""; ?> />
	            <?=gettext("FTP events");?><br />
							<input name="rsyncd" id="rsyncd" type="checkbox" value="yes" <?php if ($pconfig['rsyncd']) echo "checked=\"checked\""; ?> />
	            <?=gettext("RSYNC events");?><br />
							<input name="sshd" id="sshd" type="checkbox" value="yes" <?php if ($pconfig['sshd']) echo "checked=\"checked\""; ?> />
	            <?=gettext("SSH events");?><br />
	            <input name="smartd" id="smartd" type="checkbox" value="yes" <?php if ($pconfig['smartd']) echo "checked=\"checked\""; ?> />
	            <?=gettext("S.M.A.R.T. events");?><br />
	            <input name="daemon" id="daemon" type="checkbox" value="yes" <?php if ($pconfig['daemon']) echo "checked=\"checked\""; ?> />
	            <?=gettext("Daemon events");?><br />
	          </td>
	        </tr>
	      </table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" onclick="enable_change(true)" />
				</div>
				<div id="remarks">
					<?php html_remark("note", gettext("Note"), sprintf(gettext("Syslog sends UDP datagrams to port 514 on the specified remote syslog server. Be sure to set syslogd on the remote server to accept syslog messages from %s."), get_product_name()));?>
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
