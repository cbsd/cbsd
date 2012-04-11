<?php
//this code makesure you are registered and logged in before you can mess with teh system
//session_start();

// is the one accessing this page logged in or not?
if (!isset($_SESSION['db_admin_is_logged_in'])
   || $_SESSION['db_admin_is_logged_in'] !== true) {

   // not logged in, move to login page
   header('Location: login.php');
   exit;
}
?>

<?php
//function fileSizeInfo($fs)
//{
// $bytes = array('kb', 'kb', 'mb', 'GB', 'TB');
 // values are always displayed in at least 1 kilobyte:
// if ($fs <= 999) {
//  $fs = 1;
// }
// for ($i = 0; $fs > 999; $i++) {
//  $fs /= 1024;
// }
// return array(round($fs, 2), $bytes[$i]);
//}
//connect to your database ** EDIT REQUIRED HERE **
//mysql_connect("$MySQL_Host","$MySQL_User","$MySQL_Passw"); //(host, username, password)

//specify database ** EDIT REQUIRED HERE **
//mysql_select_db("$db") or die("Unable to select database"); //select which database we're using

// Build SQL Query  
//$query = "SELECT sum(bytes) FROM Traffic where month(measuringtime) = month(current_date)"; // EDIT HERE and specify your table and field names for the SQL query

// $numresults=mysql_query($query);
// $numrows=mysql_num_rows($numresults);

// If we have no results, offer a google search as an alternative

//if ($numrows == 0)
//  {
//  echo "<h4>Results</h4>";
//  echo "<p>Sorry, your search returned zero results</p>";
//  }

// next determine if s has been passed to script, if not use 0
//  if (empty($s)) {
//  $s=0;
//  }

// get results
//  $result = mysql_query($query) or die("Couldn't execute query");


// now you can display the results returned
//  while ($row= mysql_fetch_array($result)) {
//  $bwbc = $row["sum(bytes)"];
//  $ipadd = $row['ip'];

//$bwconarr = fileSizeInfo($bwbc);
//$bwcons = $bwconarr["0"];
//$bwconi = $bwconarr["1"];

//  echo "<b>Current Monthly Bandwidth Usage on all VPS containers:</b><br/>";
//  echo "$bwcons$bwconi";
//  $count++ ;

//  $count++ ;
//  }


// break before paging
//  echo "<br />";


?>

