#!/usr/local/bin/php
<?php
/*
	disks_crypt.php
	
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

$pgtitle = array(gettext("Disks"),gettext("Encryption"),gettext("Management"));

if ($_POST) {
	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			// Process notifications
			$retval = updatenotify_process("geli", "geli_process_updatenotification");
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			updatenotify_delete("geli");
		}
		header("Location: disks_crypt.php");
		exit;
	}
}

if (!isset($config['geli']['vdisk']) || !is_array($config['geli']['vdisk']))
	$config['geli']['vdisk'] = array();

array_sort_key($config['geli']['vdisk'], "devicespecialfile");
$a_geli = &$config['geli']['vdisk'];

if ($_GET['act'] === "del") {
	if (FALSE !== ($cnid = array_search_ex($_GET['uuid'], $config['geli']['vdisk'], "uuid"))) {
		if (disks_exists($config['geli']['vdisk'][$cnid]['devicespecialfile'])) {
			updatenotify_set("geli", UPDATENOTIFY_MODE_DIRTY, $_GET['uuid']);
			header("Location: disks_crypt.php");
			exit;
		} else {
			$errormsg[] = gettext("The volume must be detached before it can be deleted.");
		}
	}
}

function geli_process_updatenotification($mode, $data) {
	global $config;
	
	$retval = 0;
	
	switch ($mode) {
		case UPDATENOTIFY_MODE_DIRTY:
			$cnid = array_search_ex($data, $config['geli']['vdisk'], "uuid");
			if (FALSE !== $cnid) {
				// Kill encrypted volume.
				disks_geli_kill($config['geli']['vdisk'][$cnid]['devicespecialfile']);
				// Reset disk file system type attribute ('fstype') in configuration.
				set_conf_disk_fstype($config['geli']['vdisk'][$cnid]['device'][0], "");

				unset($config['geli']['vdisk'][$cnid]);
				write_config();
			}
			break;
	}
	
	return $retval;
}
?>
<?php include("fbegin.inc"); ?>
<?php if($errormsg) print_input_errors($errormsg);?>
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
      <form action="disks_crypt.php" method="post">
        <?php if ($savemsg) print_info_box($savemsg); ?>
        <?php if (updatenotify_exists_mode("geli", UPDATENOTIFY_MODE_DIRTY)) print_warning_box(gettext("Warning: You are going to delete an encrypted volume. All data will get lost and can not be recovered."));?>
        <?php if (updatenotify_exists("geli")) print_config_change_box();?>
        <table width="100%" border="0" cellpadding="0" cellspacing="0">
          <tr>
            <td width="25%" class="listhdrlr"><?=gettext("Disk"); ?></td>
            <td width="25%" class="listhdrr"><?=gettext("Data integrity"); ?></td>
            <td width="20%" class="listhdrr"><?=gettext("Encryption"); ?></td>
            <td width="20%" class="listhdrr"><?=gettext("Status") ;?></td>
            <td width="10%" class="list"></td>
          </tr>
  			  <?php $i = 0; foreach($a_geli as $geli): ?>
          <tr>
            <td class="listlr"><?=htmlspecialchars($geli['name']);?>&nbsp;</td>
            <td class="listr"><?=htmlspecialchars($geli['aalgo']);?>&nbsp;</td>
            <td class="listr"><?=htmlspecialchars($geli['ealgo']);?>&nbsp;</td>
            <td class="listbg">
              <?php
              if (updatenotify_exists("geli")) {
								$status = gettext("Configuring");
								$notificationmode = updatenotify_get_mode("geli", $geli['uuid']);
								switch ($notificationmode) {
									case UPDATENOTIFY_MODE_DIRTY:
										$status = gettext("Deleting");
										break;
								}
								echo htmlspecialchars($status);
              } else {
                if(disks_exists($geli['devicespecialfile'])) {
                  echo("<a href=\"disks_crypt_tools.php?disk={$geli['devicespecialfile']}&amp;action=attach\">" . htmlspecialchars(gettext("Not attached")) . "</a>");
                } else {
                  echo(htmlspecialchars(gettext("Attached")));
                }
              }
              ?>&nbsp;
            </td>
            <?php if (UPDATENOTIFY_MODE_DIRTY != $notificationmode):?>
            <td valign="middle" nowrap="nowrap" class="list">
							<a href="disks_crypt_tools.php?disk=<?=$geli['devicespecialfile'];?>&amp;action=setkey"><img src="e.gif" title="<?=gettext("Change password"); ?>" border="0" alt="<?=gettext("Change password"); ?>" /></a>&nbsp;
              <a href="disks_crypt.php?act=del&amp;uuid=<?=$geli['uuid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this volume?\\n!!! Note, all data will get lost and can not be recovered. !!!");?>')"><img src="x.gif" title="<?=gettext("Kill encrypted volume"); ?>" border="0" alt="<?=gettext("Kill encrypted volume"); ?>" /></a>
            </td>
            <?php else:?>
						<td valign="middle" nowrap="nowrap" class="list">
							<img src="del.gif" border="0" alt="" />
						</td>
						<?php endif;?>
          </tr>
          <?php $i++; endforeach;?>
          <tr>
            <td class="list" colspan="4"></td>
            <td class="list">
							<a href="disks_crypt_edit.php"><img src="plus.gif" title="<?=gettext("Create encrypted volume");?>" border="0" alt="<?=gettext("Create encrypted volume");?>" /></a>
						</td>
			    </tr>
        </table>
        <?php include("formend.inc");?>
      </form>
	  </td>
  </tr>
</table>
<?php include("fend.inc");?>
