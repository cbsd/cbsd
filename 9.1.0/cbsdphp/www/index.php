#!/usr/local/bin/php
<?php
/*
	index.php

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
// Configure page permission
$pgperm['allowuser'] = TRUE;

require("auth.inc");
require("guiconfig.inc");
require("zfs.inc");
require("sajax/sajax.php");

$pgtitle = array(gettext("System information"));
$pgtitle_omit = true;

$smbios = get_smbios_info();
$cpuinfo = system_get_cpu_info();

function update_controls() {
	$sysinfo = system_get_sysinfo();
	return json_encode($sysinfo);
}

sajax_init();
sajax_export("update_controls");
sajax_handle_client_request();

if(function_exists("date_default_timezone_set") and function_exists("date_default_timezone_get"))
     @date_default_timezone_set(@date_default_timezone_get());
?>
<?php include("fbegin.inc");?>
<script type="text/javascript">//<![CDATA[
<?php sajax_show_javascript();?>
//]]>
</script>
<script type="text/javascript" src="javascript/index.js"></script>
<table width="100%" border="0" cellspacing="0" cellpadding="0">
<td>&nbsp;</td>
</table>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="tabcont">
    	<table width="100%" border="0" cellspacing="0" cellpadding="0">
 			  <tr>
			    <td colspan="2" class="listtopic"><?=gettext("System information");?></td>
			  </tr>
			  <tr>
			    <td width="25%" class="vncellt"><?=gettext("Hostname");?></td>
			    <td width="75%" class="listr"><?=system_get_hostname();?></td>
			  </tr>
			  <tr>
			    <td width="25%" valign="top" class="vncellt"><?=gettext("Version");?></td>
			    <td width="75%" class="listr"><strong><?=get_product_version();?> <?=get_product_versionname();?></strong> (<?=gettext("revision");?> <?=get_product_revision();?>)</td>
			  </tr>
			  <tr>
			    <td width="25%" valign="top" class="vncellt"><?=gettext("Built date");?></td>
			    <td width="75%" class="listr"><?=get_product_buildtime();?>
			    </td>
			  </tr>
			  <tr>
			    <td width="25%" valign="top" class="vncellt"><?=gettext("Platform OS");?></td>
			    <td width="75%" class="listr">
			      <?
			        exec("/sbin/sysctl -n kern.ostype", $ostype);
			        exec("/sbin/sysctl -n kern.osrelease", $osrelease);
			        exec("/sbin/sysctl -n kern.osreldate", $osreldate);
			        echo("$ostype[0] $osrelease[0] (reldate $osreldate[0])");
			      ?>
			    </td>
			  </tr>
			  <tr>
			    <td width="25%" class="vncellt"><?=gettext("Platform");?></td>
			    <td width="75%" class="listr">
			    	<?=sprintf(gettext("%s on %s"), $g['fullplatform'], $cpuinfo['model']);?>
			    </td>
			  </tr>
			  <tr>
			    <td width="25%" class="vncellt"><?=gettext("System");?></td>
			    <td width="75%" class="listr"><?=htmlspecialchars($smbios['planar']['maker']);?> <?=htmlspecialchars($smbios['planar']['product']);?> <?=sprintf(gettext("Bios:"));?> <?=htmlspecialchars($smbios['bios']['version']);?> <?=htmlspecialchars($smbios['bios']['reldate']);?></td>
			    </td>
			  </tr>
			  <tr>
			    <td width="25%" class="vncellt"><?=gettext("System time");?></td>
			    <td width="75%" class="listr">
			      <input style="padding: 0; border: 0;" size="30" name="date" id="date" value="<?=htmlspecialchars(shell_exec("date"));?>" />
			    </td>
			  </tr>
			  <tr>
			    <td width="25%" class="vncellt"><?=gettext("System uptime");?></td>
			    <td width="75%" class="listr">
						<?php $uptime = system_get_uptime();?>
						<span name="uptime" id="uptime"><?=htmlspecialchars($uptime);?></span>
			    </td>
			  </tr>
			  <?php if (Session::isAdmin()):?>
			  <?php if ($config['lastchange']):?>
		    <tr>
		      <td width="25%" class="vncellt"><?=gettext("Last config change");?></td>
		      <td width="75%" class="listr">
						<input style="padding: 0; border: 0;" size="30" name="lastchange" id="lastchange" value="<?=htmlspecialchars(date("D M j G:i:s T Y", $config['lastchange']));?>" />
		      </td>
		    </tr>
				<?php endif;?>
				<?php if (!empty($cpuinfo['temperature2'])):
					echo "<tr>";
					echo "<td width='25%' class='vncellt'>".gettext("CPU temperature")."</td>";
					echo "<td width='75%' class='listr'>";
					echo "<table width='100%' border='0' cellspacing='0' cellpadding='0'><tr><td>\n";
					$cpus = system_get_cpus();
					for ($idx = 0; $idx < $cpus; $idx++) {
						echo "<tr><td>";
						echo "<input style='padding: 0; border: 0;' size='2' name='cputemp${idx}' id='cputemp${idx}' value='".htmlspecialchars($cpuinfo['temperature2'][$idx])."' />";
					echo $idx['temperature2']."&#176;C";	
					echo "</td></tr>";
					}
					echo "</table></td>";
					echo "</tr>\n";
				?>
				<?php elseif (!empty($cpuinfo['temperature'])):?>
				<tr>
					<td width="25%" class="vncellt"><?=gettext("CPU temperature");?></td>
					<td width="75%" class="listr">
						<input style="padding: 0; border: 0;" size="30" name="cputemp" id="cputemp" value="<?=htmlspecialchars($cpuinfo['temperature']);?>" />
					</td>
				</tr>
				<?php endif;?>
				<?php if (!empty($cpuinfo['freq'])):?>
				<tr>
					<td width="25%" class="vncellt"><?=gettext("CPU frequency");?></td>
					<td width="75%" class="listr">
						<input style="padding: 0; border: 0;" size="30" name="cpufreq" id="cpufreq" value="<?=htmlspecialchars($cpuinfo['freq']);?>MHz" title="<?=sprintf(gettext("Levels (MHz/mW): %s"), $cpuinfo['freqlevels']);?>" />
					</td>
				</tr>
				<?php endif;?>
				<tr>
					<td width="25%" class="vncellt"><?=gettext("CPU usage");?></td>
					<td width="75%" class="listr">
				    	<table width="100%" border="0" cellspacing="0" cellpadding="0"><tr><td>
						<?php
						$percentage = 0;
						echo "<img src='bar_left.gif' class='progbarl' alt='' />";
						echo "<img src='bar_blue.gif' name='cpuusageu' id='cpuusageu' width='" . $percentage . "' class='progbarcf' alt='' />";
						echo "<img src='bar_gray.gif' name='cpuusagef' id='cpuusagef' width='" . (100 - $percentage) . "' class='progbarc' alt='' />";
						echo "<img src='bar_right.gif' class='progbarr' alt='' /> ";
						?>
						<input style="padding: 0; border: 0;" size="30" name="cpuusage" id="cpuusage" value="<?=gettext("Updating in 5 seconds.");?>" />
					</td></tr>
						<?php
						$cpus = system_get_cpus();
						if ($cpus > 1) {
							echo "<tr><td><hr size='1' /></td></tr>";
							for ($idx = 0; $idx < $cpus; $idx++) {
								$percentage = 0;
								echo "<tr><td>";
								echo "<img src='bar_left.gif' class='progbarl' alt='' />";
								echo "<img src='bar_blue.gif' name='cpuusageu${idx}' id='cpuusageu${idx}' width='" . $percentage . "' class='progbarcf' alt='' />";
								echo "<img src='bar_gray.gif' name='cpuusagef${idx}' id='cpuusagef${idx}' width='" . (100 - $percentage) . "' class='progbarc' alt='' />";
								echo "<img src='bar_right.gif' class='progbarr' alt='' /> ";
								echo "<input style='padding: 0; border: 0;' size='30' name='cpuusage${idx}' id='cpuusage${idx}' value=\"".gettext("Updating in 5 seconds.")."\" />";
								echo "</td></tr>";
							}
						}
						?>
					</table>
					</td>
				</tr>
			  <tr>
			    <td width="25%" class="vncellt"><?=gettext("Memory usage");?></td>
			    <td width="75%" class="listr">
						<?php
						$raminfo = system_get_ram_info();
						$percentage = round(($raminfo['used'] * 100) / $raminfo['total'], 0);
						echo "<img src='bar_left.gif' class='progbarl' alt='' />";
						echo "<img src='bar_blue.gif' name='memusageu' id='memusageu' width='" . $percentage . "' class='progbarcf' alt='' />";
						echo "<img src='bar_gray.gif' name='memusagef' id='memusagef' width='" . (100 - $percentage) . "' class='progbarc' alt='' />";
						echo "<img src='bar_right.gif' class='progbarr' alt='' /> ";
						?>
						<input style="padding: 0; border: 0;" size="30" name="memusage" id="memusage" value="<?=sprintf(gettext("%d%% of %dMiB"), $percentage, round($raminfo['physical'] / 1024 / 1024));?>" />
			    </td>
			  </tr>
				<?php $swapinfo = system_get_swap_info(); if (!empty($swapinfo)):?>
				<tr>
					<td width="25%" class="vncellt"><?=gettext("Swap usage");?></td>
					<td width="75%" class="listr">
						<table width="100%" border="0" cellspacing="0" cellpadding="1">
							<?php
							array_sort_key($swapinfo, "device");
							$ctrlid = 0;
							foreach ($swapinfo as $swapk => $swapv) {
								$ctrlid++;
								$percent_used = rtrim($swapv['capacity'], "%");
								$tooltip_used = sprintf(gettext("%sB used of %sB"), $swapv['used'], $swapv['total']);
								$tooltip_available = sprintf(gettext("%sB available of %sB"), $swapv['avail'], $swapv['total']);

								echo "<tr><td><div id='swapusage'>";
								echo "<img src='bar_left.gif' class='progbarl' alt='' />";
								echo "<img src='bar_blue.gif' name='swapusage_{$ctrlid}_bar_used' id='swapusage_{$ctrlid}_bar_used' width='{$percent_used}' class='progbarcf' title='{$tooltip_used}' alt='' />";
								echo "<img src='bar_gray.gif' name='swapusage_{$ctrlid}_bar_free' id='swapusage_{$ctrlid}_bar_free' width='" . (100 - $percent_used) . "' class='progbarc' title='{$tooltip_available}' alt='' />";
								echo "<img src='bar_right.gif' class='progbarr' alt='' /> ";
								echo sprintf(gettext("%s of %sB"),
									"<span name='swapusage_{$ctrlid}_capacity' id='swapusage_{$ctrlid}_capacity' class='capacity'>{$swapv['capacity']}</span>",
									$swapv['total']);
								echo "<br />";
								echo sprintf(gettext("Device: %s | Total: %s | Used: %s | Free: %s"),
									"<span name='swapusage_{$ctrlid}_device' id='swapusage_{$ctrlid}_device' class='device'>{$swapv['device']}</span>",
									"<span name='swapusage_{$ctrlid}_total' id='swapusage_{$ctrlid}_total' class='total'>{$swapv['total']}</span>",
									"<span name='swapusage_{$ctrlid}_used' id='swapusage_{$ctrlid}_used' class='used'>{$swapv['used']}</span>",
									"<span name='swapusage_{$ctrlid}_free' id='swapusage_{$ctrlid}_free' class='free'>{$swapv['avail']}</span>");
								echo "</div></td></tr>";

								if ($ctrlid < count($swapinfo))
										echo "<tr><td><hr size='1' /></td></tr>";
							}?>
						</table>
					</td>
				</tr>
				<?php endif;?>
				<tr>
			  	<td width="25%" class="vncellt"><?=gettext("Load averages");?></td>
					<td width="75%" class="listr">
						<?php
						exec("uptime", $result);
						$loadaverage = substr(strrchr($result[0], "load averages:"), 15);
						?>
						<input style="padding: 0; border: 0;" size="14" name="loadaverage" id="loadaverage" value="<?=$loadaverage;?>" />
						<?="<small>[<a href='status_process.php'>".gettext("Show process information")."</a></small>]";?>
			    </td>
			  </tr>
				<tr>
			    <td width="25%" class="vncellt"><?=gettext("Disk space usage");?></td>
			    <td width="75%" class="listr">
				    <table width="100%" border="0" cellspacing="0" cellpadding="1">
				      <?php
				      $diskusage = system_get_mount_usage();
				      if (!empty($diskusage)) {
				      	array_sort_key($diskusage, "name");
				      	$index = 0;
								foreach ($diskusage as $diskusagek => $diskusagev) {
									$ctrlid = get_mount_fsid($diskusagev['filesystem'], $diskusagek);
									$percent_used = rtrim($diskusagev['capacity'],"%");
									$tooltip_used = sprintf(gettext("%sB used of %sB"), $diskusagev['used'], $diskusagev['size']);
									$tooltip_available = sprintf(gettext("%sB available of %sB"), $diskusagev['avail'], $diskusagev['size']);

									echo "<tr><td><div id='diskusage'>";
									echo "<span name='diskusage_{$ctrlid}_name' id='diskusage_{$ctrlid}_name' class='name'>{$diskusagev['name']}</span><br />";
									echo "<img src='bar_left.gif' class='progbarl' alt='' />";
									echo "<img src='bar_blue.gif' name='diskusage_{$ctrlid}_bar_used' id='diskusage_{$ctrlid}_bar_used' width='{$percent_used}' class='progbarcf' title='{$tooltip_used}' alt='' />";
									echo "<img src='bar_gray.gif' name='diskusage_{$ctrlid}_bar_free' id='diskusage_{$ctrlid}_bar_free' width='" . (100 - $percent_used) . "' class='progbarc' title='{$tooltip_available}' alt='' />";
									echo "<img src='bar_right.gif' class='progbarr' alt='' /> ";
									echo sprintf(gettext("%s of %sB"),
										"<span name='diskusage_{$ctrlid}_capacity' id='diskusage_{$ctrlid}_capacity' class='capacity'>{$diskusagev['capacity']}</span>",
										$diskusagev['size']);
									echo "<br />";
									echo sprintf(gettext("Total: %s | Used: %s | Free: %s"),
										"<span name='diskusage_{$ctrlid}_total' id='diskusage_{$ctrlid}_total' class='total'>{$diskusagev['size']}</span>",
										"<span name='diskusage_{$ctrlid}_used' id='diskusage_{$ctrlid}_used' class='used'>{$diskusagev['used']}</span>",
										"<span name='diskusage_{$ctrlid}_free' id='diskusage_{$ctrlid}_free' class='free'>{$diskusagev['avail']}</span>");
									echo "</div></td></tr>";

									if (++$index < count($diskusage))
										echo "<tr><td><hr size='1' /></td></tr>";
								}
							}

							$zfspools = zfs_get_pool_list();
							if (!empty($zfspools)) {
								array_sort_key($zfspools, "name");
								$index = 0;

								if (!empty($diskusage))
										echo "<tr><td><hr size='1' /></td></tr>";

								foreach ($zfspools as $poolk => $poolv) {
									$ctrlid = $poolv['name'];
									$percent_used = rtrim($poolv['cap'],"%");
									$tooltip_used = sprintf(gettext("%sB used of %sB"), $poolv['used'], $poolv['size']);
									$tooltip_available = sprintf(gettext("%sB available of %sB"), $poolv['avail'], $poolv['size']);

									echo "<tr><td><div id='diskusage'>";
									echo "<span name='diskusage_{$ctrlid}_name' id='diskusage_{$ctrlid}_name' class='name'>{$poolv['name']}</span><br />";
									echo "<img src='bar_left.gif' class='progbarl' alt='' />";
									echo "<img src='bar_blue.gif' name='diskusage_{$ctrlid}_bar_used' id='diskusage_{$ctrlid}_bar_used' width='{$percent_used}' class='progbarcf' title='{$tooltip_used}' alt='' />";
									echo "<img src='bar_gray.gif' name='diskusage_{$ctrlid}_bar_free' id='diskusage_{$ctrlid}_bar_free' width='" . (100 - $percent_used) . "' class='progbarc' title='{$tooltip_available}' alt='' />";
									echo "<img src='bar_right.gif' class='progbarr' alt='' /> ";
									echo sprintf(gettext("%s of %sB"),
										"<span name='diskusage_{$ctrlid}_capacity' id='diskusage_{$ctrlid}_capacity' class='capacity'>{$poolv['cap']}</span>",
										$poolv['size']);
									echo "<br />";
									echo sprintf(gettext("Total: %s | Used: %s | Free: %s | State: %s"),
										"<span name='diskusage_{$ctrlid}_total' id='diskusage_{$ctrlid}_total' class='total'>{$poolv['size']}</span>",
										"<span name='diskusage_{$ctrlid}_used' id='diskusage_{$ctrlid}_used' class='used'>{$poolv['used']}</span>",
										"<span name='diskusage_{$ctrlid}_free' id='diskusage_{$ctrlid}_free' class='free'>{$poolv['avail']}</span>",
										"<span name='diskusage_{$ctrlid}_state' id='diskusage_{$ctrlid}_state' class='state'><a href='disks_zfs_zpool_info.php?pool={$poolv['name']}'>{$poolv['health']}</a></span>");
									echo "</div></td></tr>";

									if (++$index < count($zfspools))
										echo "<tr><td><hr size='1' /></td></tr>";
								}
							}

							if (empty($diskusage) && empty($zfspools)) {
								echo "<tr><td>";
								echo gettext("No disk configured");
								echo "</td></tr>";
							}
							?>
						</table>
					</td>
				</tr>
				<?php endif;?>
			</table>
		</td>
	</tr>
</table>
<?php include("fend.inc");?>