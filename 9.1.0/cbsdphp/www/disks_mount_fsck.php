#!/usr/local/bin/php
<?php
/*
	disks_mount_fsck.php
	
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

$pgtitle = array(gettext("Disks"),gettext("Mount Point"),gettext("Fsck"));

if (!isset($config['mounts']['mount']) || !is_array($config['mounts']['mount']))
	$config['mounts']['mount'] = array();

array_sort_key($config['mounts']['mount'], "devicespecialfile");

$a_mount = $config['mounts']['mount'];

if ($_POST) {
	unset($input_errors);
	unset($errormsg);
	unset($do_action);

	// Input validation
	$reqdfields = explode(" ", "disk");
	$reqdfieldsn = array(gettext("Disk"));
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, $input_errors);

	if (!$input_errors) {
		$do_action = true;
		$disk = $_POST['disk'];
		$umount = $_POST['umount'] ? true : false;
	}
}

if (!isset($do_action)) {
	$do_action = false;
	$disk = '';
	$umount = false;
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabnavtbl">
      <ul id="tabnav">
				<li class="tabinact"><a href="disks_mount.php"><span><?=gettext("Management");?></span></a></li>
        <li class="tabinact"><a href="disks_mount_tools.php"><span><?=gettext("Tools");?></span></a></li>
				<li class="tabact"><a href="disks_mount_fsck.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Fsck");?></span></a></li>
      </ul>
    </td>
  </tr>
  <tr>
    <td class="tabcont">
      <?php if ($input_errors) print_input_errors($input_errors); ?>
			<form action="disks_mount_fsck.php" method="post" name="iform" id="iform">
			  <table width="100%" border="0" cellpadding="6" cellspacing="0">
          <tr>
            <td valign="top" class="vncellreq"><?=gettext("Disk");?></td>
            <td class="vtable">
              <select name="disk" class="formfld" id="disk">
              	<option value=""><?=gettext("Must choose one");?></option>
                <?php foreach ($a_mount as $mountv):?>
								<?php if (strcmp($mountv['fstype'],"cd9660") == 0) continue;?>
                <option value="<?=$mountv['devicespecialfile'];?>" <?php if ($mountv['devicespecialfile'] === $disk) echo "selected=\"selected\"";?>>
                <?php echo htmlspecialchars($mountv['sharename'] . ": " . $mountv['devicespecialfile']);?>
                </option>
                <?php endforeach;?>
              </select>
            </td>
      		</tr>
          <tr>
            <td width="22%" valign="top" class="vncell"></td>
            <td width="78%" class="vtable">
              <input name="umount" type="checkbox" id="umount" value="yes" <?php if ($umount) echo "checked=\"checked\""; ?> />
              <strong><?=gettext("Unmount disk/partition");?></strong><span class="vexpl"><br />
              <?=gettext("If the selected disk/partition is mounted it will be unmounted temporarily to perform selected command, otherwise the commands work in read-only mode.<br />Service disruption to users accessing this mount will occur during this process.");?></span>
            </td>
          </tr>
				</table>
				<div id="submit">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Execute");?>" />
				</div>
				<?php if($do_action) {
				echo(sprintf("<div id='cmdoutput'>%s</div>", gettext("Command output:")));
				echo('<pre class="cmdoutput">');
				ob_end_flush();
				/* Check filesystem */
				$result = disks_fsck($disk,$umount);
				/* Display result */
				echo((0 == $result) ? gettext("Successful") : gettext("Failed"));
				echo('</pre>');
				}
				?>
				<div id="remarks">
					<?php html_remark("note", gettext("Note"), gettext("You can't unmount a drive which is used by swap file, a iSCSI-target file or any other running process!"));?>
				</div>
				<?php include("formend.inc");?>
    	</form>
  	</td>
	</tr>
</table>
<?php include("fend.inc");?>
