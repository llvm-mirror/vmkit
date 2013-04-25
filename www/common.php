<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">

<meta http-equiv="content-type" content="text/html; charset=utf-8" />

<?php
	 error_reporting(E_ALL ^ E_NOTICE);

	 date_default_timezone_set('Europe/Paris');

	 function section($dir, $name, $active, $link, $text) {
	   if($name == $active) {
			 echo "<h3 class=cadre><a class=cadre-selected href=", $dir, "$link>$text</a></h3>";
	   } else {
			 echo "<h3 class=cadre><a class=cadre href=", $dir, "$link>$text</a></h3>";
	   }
	 }

	 function preambule($title, $active) {
		 $root=$GLOBALS["ROOT"];
		 $dir=$root."/../";
?>
<HTML>
  <HEAD>
			 <link rel="stylesheet" type="text/css" href="<?=$dir?>stylesheet.css">
		<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-15">
		<meta name="description" content="Gaël Thomas - Home Page">
    <meta name="author" content="Gaël Thomas">
	  <TITLE>Gaël Thomas - Home Page</TITLE>
	</HEAD>

<body>
  <table class=cadre cellpadding=0 cellspacing=0 width=100%>
    <tr>
		  <td valign="middle" align="center"  width="25%">
				<a href="http://www.inria.fr">
					<img border="0" src="http://www.inria.fr/var/inria/storage/images/medias/inria/images-corps/logo-inria-institutionnel-couleur/410230-1-fre-FR/logo-inria-institutionnel-couleur.jpg" height="80"
					  alt="INRIA"/>
				</a>
			</td>
			<td valign="middle" align="center" width="50%">
			  <h1 class=name><a class="name" href="http://vmkit2.gforge.inria.fr/">VMKit2 Project</a></h1>
			</td>
			<td valign="middle" align="center"  width="25%">
			  <a href="http://www.lip6.fr">
				  <img border="0" src="<?=$dir?>lip6.gif" height="70"
					  alt="Laboratoire d'Informatique de Paris6"/>
				</a>
			</td>
		</tr>	
	</table>

	<h1><center><?php echo $title?></center></h1>

	<table class="cadre" cellpadding="5">
	  <tr class="cadre" >
	  <td align="right" valign="top" width=150>
		  <br>
				<?php 
			     section($dir, "overview",     $active, "index.php",        "Overview");
			     section($dir, "start",        $active, "start.php",        "Get Started");
			     section($dir, "involved",     $active, "involved.php",     "Get Involved");
			     section($dir, "publi",        $active, "publis/Publications.php", "Publications");

					 if(basename($_SERVER["SCRIPT_FILENAME"]) == "ls.php") {
						 echo "<p>";
						 section($dir, "ls",    $active, "ls.php", "Explore"); 
					 }
				?>
		</td>
		<td class="cadre" width="10" align=left/>
		<td valign="top">
<?php
}
?>

<?php
	 function epilogue() {
		 //	 echo ini_get('user_dir');
		 //	 echo "???<p>";
		 //	 print_r(ini_get_all());
?>
		</td>
		</tr>
 		<tr>
			 <td/><td/><td align=right class=tiny>Last update: <?=date ("F d Y H:i:s.", getlastmod());?></td>
 		</tr>
	</table> 
</body></html>
<?php
}
?>

