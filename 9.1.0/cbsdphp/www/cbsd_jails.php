#!/usr/local/bin/php
<?php
require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("CBSD"), gettext("Jail"), gettext("Management"));



if (isset($_GET['vm'])) {
include ("fbegin.inc");
$jname=$_GET['vm'];
switch ($_GET['status']) {
case "stop": echo "Popup windows here: STOPING $jname";
$fp = popen("/usr/local/bin/cbsd jstop ${jname}","r");
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
break;

case "start": echo "Popup windows here: STARTING $jname";
$fp = popen("/usr/local/bin/cbsd jstart ${jname}","r");
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
break;

}
}





$fp = popen('/usr/local/bin/cbsd jls 2>&1', 'r');
if ($fp) {
?>
<form name="myform" action="/cbsd_jails_ed.php" method="POST">
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
<?php
$head=0;
while (($buffer=fgets($fp,4096)) !== false) {
list($jname,$jid,$ips,$hostname,$path,$status)=preg_split("/[ ]+/", $buffer);
if ($head==0) {
?>
<tr><td width="1%" class="listhdrlr">&nbsp</td><td width="30%" class="listhdrr">jname</td><td width="1%" class="listhdrr">jid</td><td width="10%" class="listhdrr">ips</td><td width="10%" class="listhdrr">hostname</td><td width="10%" class="listhdrr">path</td><td width="10%" class="listhdrr">status</td></tr>
<?php
}
else {
//$nextstatus="start";
switch (trim($status)) {
case "On": $nextstatus="stop"; break;
case "Off": $nextstatus="start"; break;
case "Sl": $nextstatus="master"; break;
case "Master": $nextstatus="start"; break;
//default: $nextstatus="start";
}
?>
<tr><td class="listlr"><input type="checkbox" name="option<?php echo $jid ?>" value="<?php echo $jid ?>"></td><td class="listlr"><?php echo $jname ?></td><td class="listlr"><?php echo $jid ?></td><td class="listlr"><?php echo $ips ?></td><td class="listlr"><?php echo $hostname ?></td><td class="listlr"><?php echo $path ?></td><td class="listlr"><?php echo "${status} <input type=\"button\" value=\"${nextstatus}\" onclick=\"location.href='/cbsd_jails.php?vm=${jname}&status=${nextstatus}'\""?>/></td></tr>
<?php
}
//}

//echo $head." ".$buffer."<br>";
$head++;
}
?>
</table>
<select name="menu" size="1">
<option value="Start">Start</option>
<option value="Stop">Stop</option>
<option value="Destroy">Destroy</option>
<option value="Clone">Clone</option>
<option value="Migrate">Migrate</option>
<option value="Snapshots">Snapshots</option>
<option value="Export">Export</option>
<option value="Rename">Rename</option>
</select>

<input type="submit">
</form>
<?php
pclose($fp);
}
?>
<a href="jcreate.php">Create New Jail</a>


<?php include("fend.inc");?>
