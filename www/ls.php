<?php
include("root.php");
include($ROOT."common.php");

   $pwd = realpath($ROOT . $_GET['from'] . "/" . $_GET['current']);
   $base = realpath(dirname($_SERVER["SCRIPT_FILENAME"])."/".$ROOT);
   $current = implode("/", array_diff(explode("/", $pwd),
																			explode("/", $base)));

//   echo "pwd: ",     $pwd,     "<br>";
//   echo "base: ",    $base,    "<br>";
//   echo "current: ", $current, "<br>";

   if($current == "") {
		 $pwd = $base;
	 }

   $rel_current = $ROOT.$current;
   $current = "/" . $current;

//   echo "current: ", $current, "<br>";
//   echo "rel_current: ", $rel_current, "<br>";

	 if (is_dir($pwd)) {
		 preambule("Directory: " . $current, "ls");

		 echo "<form class=ls action=", $_SERVER["SCRIPT_NAME"], " method=get>";
		 echo "<input type=hidden name=from value=\"", $current, "\">";

	   if ($dh = opendir($pwd)) {
       while (($file = readdir($dh)) !== false) {
	       if(is_dir($pwd . "/" . $file) && ($file != ".git") && (($file != "..") || ($pwd != $base)) && ($file != ".")) {
           echo "<input type=submit name=current value=\"", $file, "/\"/><br>\n";
	       }
       }
       closedir($dh);
     }

		 echo "<p>";
	   if ($dh = opendir($pwd)) {
       while (($file = readdir($dh)) !== false) {
	       if(!is_dir($pwd . "/" . $file) && ($file != ".gitignore")) {
           echo "<a href=\"", $rel_current . "/" . $file, "\">$file</a><br>";
	       }
       }
       closedir($dh);

     }

		 echo "</form>";

		 epilogue();
	 } else {
		 echo "should not happen; current is: '" . $current . "'";
	 }
?>




