<?php

// Czech Language Module for v2.3.2

$GLOBALS["charset"] = "UTF-8";
$GLOBALS["text_dir"] = "ltr"; // ('ltr' for left to right, 'rtl' for right to left)
$GLOBALS["date_fmt"] = "Y/m/d H:i";
$GLOBALS["error_msg"] = array(
	// error
	"error"			=> "CHYBA(Y)",
	"back"			=> "Zpìt",
	
	// root
	"home"			=> "Domovský adresáø neexistuje, opravte své zadání.",
	"abovehome"		=> "Daný adresáø nemùže být použit jako domovský adresáø.",
	"targetabovehome"	=> "Cílový adresáø nemùže být domovským adresáøem.",
	
	// exist
	"direxist"		=> "Adresáø neexistuje.",
	//"filedoesexist"	=> "Soubor existuje.",
	"fileexist"		=> "Soubor neexistuje.",
	"itemdoesexist"		=> "Tato položka existuje.",
	"itemexist"		=> "Tato položka neexistuje.",
	"targetexist"		=> "Cílový adresáø neexistuje.",
	"targetdoesexist"	=> "Cílová položka existuje.",
	
	// open
	"opendir"		=> "Nemohu otevøít adresáø.",
	"readdir"		=> "Nemohu èíst adresáø.",
	
	// access
	"accessdir"		=> "Nemáte povolen pøístup do tohoto adresáøe.",
	"accessfile"		=> "Nemáte povolen pøístup k tomuto souboru.",
	"accessitem"		=> "Nemáte povolen pøístup k této položce.",
	"accessfunc"		=> "Nemáte povoleno užití této funkce.",
	"accesstarget"		=> "Nemáte povolen pøistup k tomuto cílovému adresáøi.",
	
	// actions
	"permread"		=> "Nastavení práv selhalo.",
	"permchange"		=> "Zmìna práv selhala.",
	"openfile"		=> "Otevøení souboru selhalo.",
	"savefile"		=> "Uložení souboru selhalo.",
	"createfile"		=> "Vytvoøení souboru selhalo.",
	"createdir"		=> "Vytvoøení adresáøe selhalo.",
	"uploadfile"		=> "Nahrání souboru se nezdaøilo.",
	"copyitem"		=> "Kopírování selhalo.",
	"moveitem"		=> "Pøesun se nezdaøil.",
	"delitem"		=> "Smazání se nezdaøilo.",
	"chpass"		=> "Zmìna hesla se nezdaøila.",
	"deluser"		=> "Smazání uživatele se nezdaøilo.",
	"adduser"		=> "Pøidání uživatele se nezdaøilo.",
	"saveuser"		=> "Uložení uživatele se nezdaøilo.",
	"searchnothing"		=> "Musíte zadat název hledaného souboru/adresáøe.",
	
	// misc
	"miscnofunc"		=> "Funkce nepøístupná.",
	"miscfilesize"		=> "Soubor pøekraèuje maximální velikost.",
	"miscfilepart"		=> "Soubor byl uložen pouze èásteènì.",
	"miscnoname"		=> "Musíte zadat jméno.",
	"miscselitems"		=> "Nevybral jste žádnou položku(y).",
	"miscdelitems"		=> "Jste si jisti, že chcete smazat tuto \"+num+\" položku(y)?",
	"miscdeluser"		=> "Jste si jisti, že chcete smazat tohoto uživatele '\"+user+\"'?",
	"miscnopassdiff"	=> "Nové heslo nesouhlasí s pùvodním.",
	"miscnopassmatch"	=> "Hesla se neshodují.",
	"miscfieldmissed"	=> "Zapomnìl jste vyplnit požadované pole.",
	"miscnouserpass"	=> "Zadané jméno nebo heslo je chybné.",
	"miscselfremove"	=> "Nemùžete smazat sám sebe.",
	"miscuserexist"		=> "Uživatel již existuje.",
	"miscnofinduser"	=> "Nemohu najít uživatele.",
);
$GLOBALS["messages"] = array(
	// links
	"permlink"		=> "ZMÌNA PRÁV",
	"editlink"		=> "EDITACE",
	"downlink"		=> "STÁHNOUT",
	"uplink"		=> "VÝŠ",
	"homelink"		=> "ÚVOD",
	"reloadlink"		=> "RELOAD",
	"copylink"		=> "KOPÍROVÁNÍ",
	"movelink"		=> "PØESUN",
	"dellink"		=> "SMAZAT",
	"comprlink"		=> "ARCHÍV",
	"adminlink"		=> "ADMIN",
	"logoutlink"		=> "ODHLÁŠENÍ",
	"uploadlink"		=> "NAHRÁT",
	"searchlink"		=> "VYHLEDAT",
	
	// list
	"nameheader"		=> "Název",
	"sizeheader"		=> "Velikost",
	"typeheader"		=> "Typ",
	"modifheader"		=> "Upraveno",
	"permheader"		=> "Práva",
	"actionheader"		=> "Akce",
	"pathheader"		=> "Cesta",
	
	// buttons
	"btncancel"		=> "Zrušit",
	"btnsave"		=> "Uložit",
	"btnchange"		=> "Zmìnit",
	"btnreset"		=> "Reset",
	"btnclose"		=> "Zavøít",
	"btncreate"		=> "Vytvoøit",
	"btnsearch"		=> "Vyhledat",
	"btnupload"		=> "Nahrát",
	"btncopy"		=> "Kopírovat",
	"btnmove"		=> "Pøesunout",
	"btnlogin"		=> "Pøihlásit",
	"btnlogout"		=> "Odhlásit",
	"btnadd"		=> "Pøidat",
	"btnedit"		=> "Editovat",
	"btnremove"		=> "Smazat",
	
	// actions
	"actdir"		=> "Adresáø",
	"actperms"		=> "Zmìna práv",
	"actedit"		=> "Editace souboru",
	"actsearchresults"	=> "Najít výsledky",
	"actcopyitems"		=> "Kopírovat položku(y)",
	"actcopyfrom"		=> "Kopírovat z /%s do /%s ",
	"actmoveitems"		=> "Pøesunout položku(y)",
	"actmovefrom"		=> "Pøesunout z /%s do /%s ",
	"actlogin"		=> "Pøihlásit k FTP ADASERVIS s.r.o.",
	"actloginheader"	=> "WEB/FTP QuiXplorer",
	"actadmin"		=> "Administrace",
	"actchpwd"		=> "Zmìna hesla",
	"actusers"		=> "Uživatelé",
	"actarchive"		=> "Archív položek",
	"actupload"		=> "Nahrát soubror(y)",
	
	// misc
	"miscitems"		=> "Položka(y)",
	"miscfree"		=> "Free",
	"miscusername"		=> "Jméno",
	"miscpassword"		=> "Heslo",
	"miscoldpass"		=> "Staré heslo",
	"miscnewpass"		=> "Nové heslo",
	"miscconfpass"		=> "Potvrdit heslo",
	"miscconfnewpass"	=> "Potvrdit nové heslo",
	"miscchpass"		=> "Zmìnit heslo",
	"mischomedir"		=> "Domovský adresáø",
	"mischomeurl"		=> "Domovké URL",
	"miscshowhidden"	=> "Zobrazit skryté položky",
	"mischidepattern"	=> "Skrýt vzor",
	"miscperms"		=> "Práva",
	"miscuseritems"		=> "(jméno, domovský adresáø, zobrazit skryté položky, práva, aktivní)",
	"miscadduser"		=> "Pøidat uživatele",
	"miscedituser"		=> "Editovat uživatele '%s'",
	"miscactive"		=> "Aktivní",
	"misclang"		=> "Jazyk",
	"miscnoresult"		=> "Nenalezeny žádné výsledky.",
	"miscsubdirs"		=> "Hledat podadresáøe",
	"miscpermnames"		=> array("Pouze ètení","Úpravy","Zmìna hesla","Úpravy & Zmìna hesla",
					"Administrátor"),
	"miscyesno"		=> array("Ano","Ne","A","N"),
	"miscchmod"		=> array("Vlastník", "Skupina", "Veøejné"),
);
?>