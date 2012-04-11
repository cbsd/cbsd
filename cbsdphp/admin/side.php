<?php
//this code makesure you are registered and logged in before you can mess with teh system
session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: login.php');
   exit;
}
?>

<li><a href="modules/jailctl/" target="main">Jails</a></li>
<li><a href="modules/nodectl/jls.php" target="main">Nodes</a></li>
<li><a href="modules/farms/farms.php" target="main">Farms</a></li>
<li><a href="modules/repo/" target="main">Repository</a></li>
<li><a href="logout.php" target="main">LogOut</a></li>
</html>


