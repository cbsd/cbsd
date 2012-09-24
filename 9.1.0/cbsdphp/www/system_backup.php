#!/usr/local/bin/php
<?php
/*
	system_backup.php
	
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

$pgtitle = array(gettext("System"), gettext("Backup/Restore"));

/* omit no-cache headers because it confuses IE with file downloads */
$omit_nocacheheaders = true;

if ($_POST) {
	unset($errormsg);

	if (0 == strcmp($_POST['Submit'], gettext("Restore configuration"))) {
		$mode = "restore";
	} else if (0 == strcmp($_POST['Submit'], gettext("Download configuration"))) {
		$mode = "download";
	}

	if ($mode) {
		if ($mode === "download") {
			config_lock();

			if(function_exists("date_default_timezone_set") and function_exists("date_default_timezone_get"))
                 @date_default_timezone_set(@date_default_timezone_get());
			$fn = "config-{$config['system']['hostname']}.{$config['system']['domain']}-" . date("YmdHis") . ".xml";
			$fs = get_filesize("{$g['conf_path']}/config.xml");

			header("Content-Type: application/octet-stream");
			header("Content-Disposition: attachment; filename={$fn}");
			header("Content-Length: {$fs}");
			header("Pragma: hack");
			readfile("{$g['conf_path']}/config.xml");
			config_unlock();

			exit;
		} else if ($mode === "restore") {
			if (is_uploaded_file($_FILES['conffile']['tmp_name'])) {
				// Validate configuration backup
				if (!validate_xml_config($_FILES['conffile']['tmp_name'], $g['xml_rootobj'])) {
					$errormsg = sprintf(gettext("The configuration could not be restored. %s"),
						gettext("Invalid file format."));
				} else {
					// Install configuration backup
					if (config_install($_FILES['conffile']['tmp_name']) == 0) {
						system_reboot();
						$savemsg = sprintf(gettext("The configuration has been restored. %s is now rebooting."),
							get_product_name());
					} else {
						$errormsg = gettext("The configuration could not be restored.");
					}
				}
			} else {
				$errormsg = sprintf(gettext("The configuration could not be restored. %s"),
					$g_file_upload_error[$_FILES['conffile']['error']]);
			}
		}
	}
}
?>
<?php include("fbegin.inc");?>
<form action="system_backup.php" method="post" enctype="multipart/form-data">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabcont">
				<?php if ($errormsg) print_error_box($errormsg);?>
				<?php if ($savemsg) print_info_box($savemsg);?>
			  <table width="100%" border="0" cellspacing="0" cellpadding="6">
			    <tr>
			      <td colspan="2" class="listtopic"><?=gettext("Backup configuration");?></td>
			    </tr>
			    <tr>
					<td width="22%" valign="baseline" class="vncell">&nbsp;</td>
					<td width="78%" class="vtable">
						<?=gettext("Click this button to download the system configuration in XML format.");?><br />
						<div id="submit">
							<input name="Submit" type="submit" class="formbtn" id="download" value="<?=gettext("Download configuration");?>" />
						</div>
					</td>
			    </tr>
			    <tr>
			      <td colspan="2" class="list" height="12"></td>
			    </tr>
			    <tr>
			      <td colspan="2" class="listtopic"><?=gettext("Restore configuration");?></td>
			    </tr>
			    <tr>
					<td width="22%" valign="baseline" class="vncell">&nbsp;</td>
					<td width="78%" class="vtable">
						<?php echo sprintf(gettext("Open a %s configuration XML file and click the button below to restore the configuration."), get_product_name());?><br />
						<div id="remarks">
							<?php html_remark("note", gettext("Note"), sprintf(gettext("%s will reboot after restoring the configuration."), get_product_name()));?>
						</div>
						<div id="submit">
						<input name="conffile" type="file" class="formfld" id="conffile" size="40" />
						</div>
						<div id="submit">
						<input name="Submit" type="submit" class="formbtn" id="restore" value="<?=gettext("Restore configuration");?>" />
						</div>
					</td>
			    </tr>
			  </table>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<?php include("fend.inc");?>
