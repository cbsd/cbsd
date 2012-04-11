<?php
include('../../session.php');
// you define this variable here so that it exists for the call to exec
$output = null;

// Windows users: 'dir c:\\' or something similar
exec('ls  /vz/template/cache/', $output);

#echo "<pre>" . var_export($output, TRUE) . "</pre><br/>";

echo "<form name='selectos' action='oschange.php' method='post'>";
echo "VPS Container ID";
echo " <input type='text' name='vid' value='' />";
echo "<br/>";
echo "<SELECT name='os'>";
foreach ($output as $fileName)
{
$file_without_ext = substr($fileName, 0, -7);
echo "<OPTION value=$file_without_ext> $file_without_ext";
}
echo '</select>'; 
echo "<input type='submit' name='submit' value='change' />";
echo "</form>";
?>
<a href="../../index.php">Home</a>

