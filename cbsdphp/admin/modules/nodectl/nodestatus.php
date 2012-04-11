<?php
//session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: /admin/login.php');
   exit;
}

$fp=fopen($workdir."/nc.inventory","r");
if ($fp) {
?>
<table border=0>
<?php
while (($buffer = fgets($fp, 4096)) !== false) {
//echo $buffer;
list($par,$val)=explode("=",$buffer);
echo "<tr><td><b>$par</b></td><td>: $val</td></tr>\n";
}
?>
</table>
<?php
fclose($fp);
}

//$fp = popen('/usr/local/bin/sudo /usr/local/bin/cbsd jls 2>&1', 'r');
//if ($fp) {
//while (!feof($fp)) {
//$read = fread($fp, 2096);
//echo $read."<br>";
//}
//pclose($fp);
//}

?>

