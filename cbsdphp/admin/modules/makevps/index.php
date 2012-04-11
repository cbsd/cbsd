<?php
//this code makesure you are registered and logged in before you can mess with teh system
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: ../../login.php');
   exit;
}
?>

<b>Create VPS</b>
<br/><br/>
<form name="form" action="makevps.php" method="post">
<table width="25%" height="50%" cellpadding="0" cellspacing="0" border="0">
<tr><td align="left" valign="top">VPS ID</td><td align="left" valign="top"><input type="text" name="vid" value="" /></td></tr>
<tr><td align="left" valign="top">fosvm User</td><td align="left" valign="top"><input type="text" name="user" value="" /></td></tr>
<tr><td align="left" valign="top">IP-1</td><td align="left" valign="top"><input type="text" name="ip1" value="" /></td></tr>
<tr><td align="left" valign="top">IP2</td><td align="left" valign="top"><input type="text" name="ip2" value="" /></td></tr>
<tr><td align="left" valign="top">DNS-1</td><td align="left" valign="top"><input type="text" name="dns1" value="208.67.222.222" /></td></tr>
<tr><td align="left" valign="top">DNS-2</td><td align="left" valign="top"><input type="text" name="dns2" value="208.67.222.220" /></td></tr>
<tr><td align="left" valign="top">Dedicated RAM (MB)</td><td align="left" valign="top"><input type="text" name="dram" value="" /></td></tr>
<tr><td align="left" valign="top">Burstable RAM (MB)</td><td align="left" valign="top"><input type="text" name="bram" value="" /></td></tr>
<tr><td align="left" valign="top">DiskSpace (GB)</td><td align="left" valign="top"><input type="text" name="disk" value="" /></td></tr>
<tr><td align="left" valign="top">OS Template</td><td align="left" valign="top">

<?php
// you define this variable here so that it exists for the call to exec
$output = null;

// Windows users: 'dir c:\\' or something similar
exec('ls  /vz/template/cache/', $output);

echo "<SELECT name='os'>";
foreach ($output as $fileName)
{
$file_without_ext = substr($fileName, 0, -7);
echo "<OPTION value=$file_without_ext> $file_without_ext";
}
echo '</select>';
?>


</td></tr>
<tr><td align="left" valign="top">Hostname</td><td align="left" valign="top"><input type="text" name="host" value="" /></td></tr>
<tr><td align="left" valign="top">root Password</td><td align="left" valign="top"><input type="text" name="pswd" value="" /></td></tr>
</table>
<input type="submit" name="Submit" value="Create!" />
</form>
<br/><br/>
<a href="../../index.php">Home</a>

