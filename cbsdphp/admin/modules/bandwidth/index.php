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

<form name="form" action="bandwidth.php" method="post">
VPS IP
  <input type="text" name="q" value="" />
  <input type="submit" name="Submit" value="Search" />
</form>
<br/>
<a href="../../index.php">Home</a>

