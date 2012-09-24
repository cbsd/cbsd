#!/usr/local/bin/php
<?php
/*
	services_ups_drv.php

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

$pgtitle = array(gettext("Services"), gettext("UPS"), gettext("Driver list"));

function nut_get_driverlist() {
	$a_driverinfo = array();

	// Read file
	$a_driver = file("/usr/local/etc/nut/driver.list");
	if (!is_array($a_driver))
		return $a_driverinfo;

	// Parse data
	foreach ($a_driver as $driver) {
		// Syntax should look like: '"<manufacturer>" "<model name>" "<model extra>" "<driver>"'.
		if (preg_match("/^\"(.*)\".*\"(.*)\".*\"(.*)\".*\"(.*)\".*/", $driver, $matches)) {
			$driverinfo = array();
			$driverinfo['manufacturer'] = $matches[1];
			$driverinfo['modelname'] = $matches[2];
			$driverinfo['modelextra'] = $matches[3];
			$driverinfo['driver'] = $matches[4];
			
			$a_driverinfo[] = $driverinfo;
		}
	}

	return $a_driverinfo;
}
?>
<?php include("fbegin.inc");?>
<form action="services_ups_drv.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	  <tr>
	    <td class="tabcont">
				<table width="100%" border="0" cellpadding="0" cellspacing="0">
          <tr>
          	<td width="25%" class="listhdrlr"><?=gettext("Manufacturer");?></td>
            <td width="25%" class="listhdrr"><?=gettext("Model name");?></td>
            <td width="25%" class="listhdrr"><?=gettext("Model extra");?></td>
            <td width="25%" class="listhdrr"><?=gettext("Driver");?></td>
          </tr>
  			  <?php foreach(nut_get_driverlist() as $driverinfov):?>
          <tr>
          	<td class="listlr"><?=htmlspecialchars($driverinfov['manufacturer']);?>&nbsp;</td>
            <td class="listr"><?=htmlspecialchars($driverinfov['modelname']);?>&nbsp;</td>
            <td class="listr"><?=htmlspecialchars($driverinfov['modelextra']);?>&nbsp;</td>
            <td class="listr"><?=htmlspecialchars($driverinfov['driver']);?>&nbsp;</td>
          </tr>
          <?php endforeach;?>
        </table>
			</td>
		</tr>
	</table>
	<?php include("formend.inc");?>
</form>
<?php include("fend.inc");?>
