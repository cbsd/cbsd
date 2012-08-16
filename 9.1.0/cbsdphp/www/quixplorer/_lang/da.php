<?php

// Dansk sprog Module for v2.3.2

$GLOBALS["charset"] = "UTF-8";
$GLOBALS["text_dir"] = "ltr"; // ('ltr' for left to right, 'rtl' for right to left)
$GLOBALS["date_fmt"] = "Y/m/d H:i";
$GLOBALS["error_msg"] = array(
	// error
	"error"		=> "Fejl",
	"back"		=> "Tilbage",
	
	// root
	"home"		=> "Hjem mappen findes ikke, check indstillinger.",
	"abovehome"	=> "Aktuelle mappe kan ikke være over hjem mappen.",
	"targetabovehome"	=> "Mål mappen kan ikke være over hjem mappen.",
	
	// exist
	"direxist"		=> "Mappen findes ikke.",
	//"filedoesexist"	=> "Filen findes allerede.",
	"fileexist"		=> "Filen findes ikke.",
	"itemdoesexist"	=> "Emnet findes allerede.",
	"itemexist"		=> "Emnet findes ikke.",
	"targetexist"	=> "Målmappen findes ikke.",
	"targetdoesexist"	=> "Mål emnet findes allerede.",
	
	// open
	"opendir"		=> "Kan ikke åbne mappen.",
	"readdir"		=> "Kan ikke læse mappen.",
	
	// access
	"accessdir"		=> "Du har ikke adgang til denne mappe.",
	"accessfile"	=> "Du har ikke adgang til denne fil.",
	"accessitem"	=> "Du har ikke adgang til dette emne.",
	"accessfunc"	=> "Du har ikke adgang til at bruge denne funktion.",
	"accesstarget"	=> "Du har ikke adgang til mål mappen.",
	
	// actions
	"permread"	=> "Tilladelse fejlede..",
	"permchange"	=> "Ændring af tilladelse mislykkedes.",
	"openfile"		=> "Filen kunne ikke åbnes.",
	"savefile"		=> "Filen kunne ikke gemmes.",
	"createfile"		=> "Filen kunne ikke oprettes.",
	"createdir"		=> "Mappen kunne ikke oprettes.",
	"uploadfile"	=> "Filen kunne ikke hentes (upload).",
	"copyitem"		=> "Kopiering fejlede.",
	"moveitem"	=> "Flytning fejlede.",
	"delitem"		=> "Filen blev IKKE slettet.",
	"chpass"		=> "Kodeord kunne ikke ændres.",
	"deluser"		=> "Bruger kunne ikke fjernes.",
	"adduser"		=> "Oprettelse af ny bruger mislykkedes.",
	"saveuser"		=> "Bruger kunne ikke gemmes.",
	"searchnothing"	=> "Du skal indtaste noget at søge efter.",
	
	// misc
	"miscnofunc"	=> "Funktionen mangler.",
	"miscfilesize"	=> "Filen overskrider maksimum størrelse.",
	"miscfilepart"	=> "Kun en del af filen blev lagt op.",
	"miscnoname"	=> "Indtast et navn.",
	"miscselitems"	=> "Du har ikke valgt emne(r).",
	"miscdelitems"	=> "Er du sikker på du vil slette de(t) \"+num+\" emne(r)?",
	"miscdeluser"	=> "Er du sikker på at du vil slette bruger: '\"+user+\"'?",
	"miscnopassdiff"	=> "Det nye kodeord er det samme som det aktuelle.",
	"miscnopassmatch"	=> "Kodeord er ikke ens.",
	"miscfieldmissed"	=> "Du glemte et vigtigt felt.",
	"miscnouserpass"	=> "Bruger ID eller kodeord er forkert.",
	"miscselfremove"	=> "Du kan ikke slette dig selv.",
	"miscuserexist"	=> "Bruger findes allerede.",
	"miscnofinduser"	=> "Bruger findes ikke.",
);
$GLOBALS["messages"] = array(
	// links
	"permlink"		=> "RETTE TILLADELSER",
	"editlink"		=> "RETTE",
	"downlink"		=> "HENT",
	"uplink"		=> "OP",
	"homelink"		=> "HJEM",
	"reloadlink"	=> "GENINDLÆS",
	"copylink"		=> "KOPIER",
	"movelink"		=> "FLYT",
	"dellink"		=> "SLET",
	"comprlink"	=> "ARKIV",
	"adminlink"	=> "ADMIN",
	"logoutlink"	=> "LOG UD",
	"uploadlink"	=> "TILFØJ",
	"searchlink"	=> "SØG",
	
	// list
	"nameheader"	=> "Navn",
	"sizeheader"	=> "Størrelse",
	"typeheader"	=> "Type",
	"modifheader"	=> "Rettet",
	"permheader"	=> "Tillad",
	"actionheader"	=> "Handlinger",
	"pathheader"	=> "Sti",
	
	// buttons
	"btncancel"	=> "Fortryd",
	"btnsave"		=> "Gem",
	"btnchange"	=> "Rette",
	"btnreset"		=> "Nulstil",
	"btnclose"		=> "Luk",
	"btncreate"	=> "Opret",
	"btnsearch"	=> "Søg",
	"btnupload"	=> "Tilføj",
	"btncopy"		=> "Kopier",
	"btnmove"		=> "Flyt",
	"btnlogin"		=> "Log ind",
	"btnlogout"		=> "Log ud",
	"btnadd"		=> "Tilføj",
	"btnedit"		=> "Ret",
	"btnremove"	=> "Fjern",
	
	// actions
	"actdir"		=> "Mappe",
	"actperms"		=> "Ret tilladelser",
	"actedit"		=> "Ret fil",
	"actsearchresults"	=> "Søge resultater",
	"actcopyitems"	=> "Kopier emne(r)",
	"actcopyfrom"	=> "Kopier fra /%s til /%s ",
	"actmoveitems"	=> "Flyt emne(r)",
	"actmovefrom"	=> "Flyt fra /%s til /%s ",
	"actlogin"		=> "Log ind",
	"actloginheader"	=> "Log ind til QuiXplorer",
	"actadmin"		=> "Administration",
	"actchpwd"		=> "Ret kodeord",
	"actusers"		=> "Brugere",
	"actarchive"	=> "Arkiver emne(r)",
	"actupload"	=> "Tilføj fil(er)",
	
	// misc
	"miscitems"	=> "Emner(r)",
	"miscfree"		=> "Resterende plads",
	"miscusername"	=> "Brugernavn",
	"miscpassword"	=> "Kodeord",
	"miscoldpass"	=> "Gamle Kodeord",
	"miscnewpass"	=> "Nyt kodeord",
	"miscconfpass"	=> "Gentag kodeord",
	"miscconfnewpass"	=> "Bekræft nyt kodeord",
	"miscchpass"	=> "Rette kodeord",
	"mischomedir"	=> "Hjem mappe",
	"mischomeurl"	=> "Hjem URL",
	"miscshowhidden"	=> "Vis skjulte emne(r)",
	"mischidepattern"	=> "Skjul koder",
	"miscperms"	=> "Tilladelser",
	"miscuseritems"	=> "(navn, hjem mappe, vis skjulte emner, tilladelser, aktiv)",
	"miscadduser"	=> "Tilføj bruger",
	"miscedituser"	=> "Ret bruger '%s'",
	"miscactive"	=> "Aktiv",
	"misclang"		=> "Sprog",
	"miscnoresult"	=> "Ingen resultat.",
	"miscsubdirs"	=> "Søg i undermapper",
	"miscpermnames"	=> array("Vis kun","Ret","Ændre kodeord","Ret & Ændre kodeord",
					"Administrator"),
	"miscyesno"		=> array("Ja","Nej","J","N"),
	"miscchmod"		=> array("Ejer", "Gruppe", "Offentlig"),
);
?>
