<?php
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: /admin/login.php');
   exit;
}
include('../../../includes/config.php');

//print_r($_POST);
//print_r($_GET);


if (isset($_GET['vm'])) {
$jname=$_GET['vm'];
switch ($_GET['status']) {
case "stop": echo "Popup windows here: STOPING $jname";
$fp = popen("/usr/local/bin/sudo /usr/local/bin/cbsd jstop ${jname}","r");
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
break;

case "start": echo "Popup windows here: STARTING $jname";
$fp = popen("/usr/local/bin/sudo /usr/local/bin/cbsd jstart ${jname}","r");
while (($buffer=fgets($fp,4096))!==false) {
echo "$buffer"."<br>";
flush();
}
pclose($fp);
break;

}
}



$fp = popen('/usr/local/bin/sudo /usr/local/bin/cbsd jls 2>&1', 'r');
if ($fp) {
?>
<form name="myform" action="/admin/modules/jailctl/" method="POST">
<table border=1><?php
$head=0;
while (($buffer=fgets($fp,4096)) !== false) {
list($jname,$jid,$ips,$hostname,$path,$status)=preg_split("/[ ]+/", $buffer);
if ($head==0)
echo "<tr><td>&nbsp</td><td>$jname</td><td>$jid</td><td>$ips</td><td>$hostname</td><td>$path</td><td>$status</td></tr>\n";
else {
//$nextstatus="start";
switch (trim($status)) {
case "Online": $nextstatus="stop"; break;
case "Offline": $nextstatus="start"; break;
case "Slave": $nextstatus="master"; break;
case "Master": $nextstatus="start"; break;
//default: $nextstatus="start";
}
echo "<tr><td><input type=\"checkbox\" name=\"option$jid\" value=\"$jid\"></td><td>$jname</td><td>$jid</td><td>$ips</td><td>$hostname</td><td>$path</td><td>${status} <input type=\"button\" value=\"${nextstatus}\" onclick=\"location.href='/admin/modules/jailctl/?vm=${jname}&status=${nextstatus}'\"/></td></tr>\n";
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
