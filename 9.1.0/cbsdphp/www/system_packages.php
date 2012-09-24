#!/usr/local/bin/php
<?php
/*
	system_packages.php

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
require("packages.inc");

$pgtitle = array(gettext("System"), gettext("Packages"));

$a_packages = packages_get_installed();

if ($_GET['act'] == "del") {
	if ($a_packages[$_GET['id']]) {
		packages_uninstall($a_packages[$_GET['id']]['name']);
		header("Location: system_packages.php");
		exit;
	}
}
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
		<td class="tabnavtbl">
  		<ul id="tabnav">
				<li class="tabact"><a href="system_packages.php" title="<?=gettext("Reload page");?>"><span><?=gettext("Packages");?></span></a></li>
  		</ul>
  	</td>
	</tr>
  <tr>
    <td class="tabcont">
			<form action="system_packages.php" method="post" name="iform" id="iform">
				<?php if ($input_errors) print_input_errors($input_errors); ?>
				<?php if ($savemsg) print_info_box($savemsg); ?>
				<?php if (file_exists($d_packagesconfdirty_path)) print_config_change_box();?>
				<table width="100%" border="0" cellpadding="6" cellspacing="0">
			    <tr>
			      <td width="40%" class="listhdrlr"><?=gettext("Package Name");?></td>
			      <td width="50%" class="listhdrr"><?=gettext("Description");?></td>
			      <td width="10%" class="list"></td>
			    </tr>
				  <?php $i = 0; foreach($a_packages as $packagev): ?>
			    <tr>
			      <td class="listr"><?=htmlspecialchars($packagev['name']);?>&nbsp;</td>
			      <td class="listbg"><?=htmlspecialchars($packagev['desc']);?>&nbsp;</td>
			      <td valign="middle" nowrap="nowrap" class="list"> <a href="system_packages.php?act=del&amp;id=<?=$i;?>" onclick="return confirm('<?=gettext("Do you really want to uninstall this package?"); ?>')"><img src="x.gif" title="<?=gettext("Uninstall package"); ?>" border="0" alt="<?=gettext("Uninstall package"); ?>" /></a></td>
			    </tr>
			    <?php $i++; endforeach; ?>
			    <tr>
						<td class="list" colspan="2"></td>
						<td class="list"> <a href="system_packages_edit.php"><img src="plus.gif" title="<?=gettext("Install package"); ?>" border="0" alt="<?=gettext("Install package"); ?>" /></a></td>
					</tr>
			  </table>
			  <?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>
