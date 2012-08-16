<?php

// Polish Language Module for v2.3.2

$GLOBALS["charset"] = "UTF-8";
$GLOBALS["text_dir"] = "ltr"; // ('ltr' for left to right, 'rtl' for right to left)
$GLOBALS["date_fmt"] = "d-m-Y H:i";
$GLOBALS["error_msg"] = array(
	// error
	"error"			=> "B£¡D(ÊDY)",
	"back"			=> "Z Powrotem",
	
	// root
	"home"			=> "Katalog domowy nie istnieje. Sprawd¼ swoje ustawienia.",
	"abovehome"		=> "Obecny katalog nie mo¿e byæ powy¿ej katalogu domowego.",
	"targetabovehome"	=> "Katalog docelowy nie mo¿e byæ powy¿ej katalogu domowego.",
	
	// exist
	"direxist"		=> "Ten katalog nie istnieje.",
	//"filedoesexist"	=> "This file already exists.",
	"fileexist"		=> "Ten plik nie istnieje.",
	"itemdoesexist"		=> "Ta pozycja ju¿ istnieje.",
	"itemexist"		=> "Ta pozycja nie istnieje.",
	"targetexist"		=> "Katalog docelowy nie istnieje.",
	"targetdoesexist"	=> "Pozycja docelowa ju¿ istnieje.",
	
	// open
	"opendir"		=> "Nie mogê otworzyæ katalogu.",
	"readdir"		=> "Nie mogê odczytaæ katalogu.",
	
	// access
	"accessdir"		=> "Nie masz dostêpu do tego katalogu.",
	"accessfile"		=> "Nie masz dostêpu do tego pliku.",
	"accessitem"		=> "Nie masz dostêpu do tej pozycji.",
	"accessfunc"		=> "Nie masz dostêpu do tej funkcji.",
	"accesstarget"		=> "Nie masz dostêpu do katalogu docelowego.",
	
	// actions
	"chmod_not_allowed"  => 'Changing Permissions to NONE is not allowed!',
	"permread"		=> "Pobranie uprawnieñ nie uda³o siê.",
	"permchange"		=> "Zmiana uprawnieñ siê nie powiod³a.",
	"openfile"		=> "Otawrcie pliku siê nie powiod³o.",
	"savefile"		=> "Zapis pliku siê nie powiod³o.",
	"createfile"		=> "Utworzenie pliku siê nie powiod³o.",
	"createdir"		=> "Utworzenie katalogu siê nie powiod³o.",
	"uploadfile"		=> "Wrzucanie pliku na serwer siê nie powiod³o.",
	"copyitem"		=> "Kopiowanie siê nie powiod³o.",
	"moveitem"		=> "Przenoszenie siê nie powiod³o.",
	"delitem"		=> "Usuwanie siê nie powiod³o.",
	"chpass"		=> "Zmiana has³a nie powiod³a siê.",
	"deluser"		=> "Usuwanie u¿ytkowika siê nie powiod³o.",
	"adduser"		=> "Dodanie u¿ytkownika siê nie powiod³o.",
	"saveuser"		=> "Zapis u¿ytkownika siê nie powiod³o.",
	"searchnothing"		=> "Musisz dostarczyæ czego¶ do szukania.",
	
	// misc
	"miscnofunc"		=> "Funkcja niedostêpna.",
	"miscfilesize"		=> "Rozmiar pliku przekroczy³ maksymaln± warto¶æ.",
	"miscfilepart"		=> "Plik zosta³ za³adowany tylko czê¶ciowo.",
	"miscnoname"		=> "Musisz nadaæ nazwê.",
	"miscselitems"		=> "Nie zaznaczy³e¶ ¿adnej pozycji.",
	"miscdelitems"		=> "Jeste¶ pewny ¿e chcesz usun±æ te (\"+num+\") pozycje?",
	"miscdeluser"		=> "Jeste¶ pewny ¿e chcesz usun±æ u¿ytkownika '\"+user+\"'?",
	"miscnopassdiff"	=> "Nowe has³o nie ró¿ni siê od obecnego.",
	"miscnopassmatch"	=> "Podane has³a ró¿ni± siê.",
	"miscfieldmissed"	=> "Opuszczono wa¿ne pole.",
	"miscnouserpass"	=> "U¿ytkownik i has³o s± niezgodne.",
	"miscselfremove"	=> "Nie mo¿esz siebie usun±æ.",
	"miscuserexist"		=> "U¿ytkownik ju¿ istnieje.",
	"miscnofinduser"	=> "U¿ytkownika nie znaleziono.",
);
$GLOBALS["messages"] = array(
	// links
	"permlink"		=> "ZMIANA UPRAWNIEÑ",
	"editlink"		=> "EDYCJA",
	"downlink"		=> "DOWNLOAD",
	"uplink"		=> "KATALOG WY¯EJ",
	"homelink"		=> "KATALOG DOMOWY",
	"reloadlink"		=> "OD¦WIE¯",
	"copylink"		=> "KOPIUJ",
	"movelink"		=> "PRZENIE¦",
	"dellink"		=> "USUÑ",
	"comprlink"		=> "ARCHIWIZUJ",
	"adminlink"		=> "ADMINISTRUJ",
	"logoutlink"		=> "WYLOGUJ",
	"uploadlink"		=> "WRZUÆ PLIK NA SERWER - UPLOAD",
	"searchlink"		=> "SZUKAJ",
	
	// list
	"nameheader"		=> "Nazwa",
	"sizeheader"		=> "Rozmiar",
	"typeheader"		=> "Typ",
	"modifheader"		=> "Zmodyfikowano",
	"permheader"		=> "Prawa dostêpu",
	"actionheader"		=> "Akcje",
	"pathheader"		=> "¦cie¿ka",
	
	// buttons
	"btncancel"		=> "Zrezygnuj",
	"btnsave"		=> "Zapisz",
	"btnchange"		=> "Zmieñ",
	"btnreset"		=> "Reset",
	"btnclose"		=> "Zamknij",
	"btncreate"		=> "Utwórz",
	"btnsearch"		=> "Szukaj",
	"btnupload"		=> "Wrzuæ na serwer",
	"btncopy"		=> "Kopiuj",
	"btnmove"		=> "Przenie¶",
	"btnlogin"		=> "Zaloguj",
	"btnlogout"		=> "Wyloguj",
	"btnadd"		=> "Dodaj",
	"btnedit"		=> "Edycja",
	"btnremove"		=> "Usuñ",
	
	// actions
	"actdir"		=> "Katalog",
	"actperms"		=> "Zmiana uprawnieñ",
	"actedit"		=> "Edycja pliku",
	"actsearchresults"	=> "Rezultaty szukania",
	"actcopyitems"		=> "Kopiuj pozycje",
	"actcopyfrom"		=> "Kpiuj z /%s do /%s ",
	"actmoveitems"		=> "Przenie¶ pozycje",
	"actmovefrom"		=> "Przenie¶ z /%s do /%s ",
	"actlogin"		=> "Nazwa u¿ytkownika",
	"actloginheader"	=> "Zaloguj siê by u¿ywaæ QuiXplorer",
	"actadmin"		=> "Administracja",
	"actchpwd"		=> "Zmieñ has³o",
	"actusers"		=> "U¿ytkownicy",
	"actarchive"		=> "Pozycje zarchiwizowane",
	"actupload"		=> "Wrzucanie na serwer- Upload",
	
	// misc
	"miscitems"		=> " -Ilo¶c elementów",
	"miscfree"		=> "Wolnego miejsca",
	"miscusername"		=> "Nazwa u¿ytkownika",
	"miscpassword"		=> "Has³o",
	"miscoldpass"		=> "Stare has³o",
	"miscnewpass"		=> "Nowe has³o",
	"miscconfpass"		=> "Potwierd¼ has³o",
	"miscconfnewpass"	=> "Potwierd¼ nowe has³o",
	"miscchpass"		=> "Zmieñ has³o",
	"mischomedir"		=> "Katalog g³ówny",
	"mischomeurl"		=> "URL Katalogu domowego",
	"miscshowhidden"	=> "Show hidden items",
	"mischidepattern"	=> "Hide pattern",
	"miscperms"		=> "Uprawnienia",
	"miscuseritems"		=> "(nazwa, katalog domowy, poka¿ pozycje ukryte, uprawnienia, czy aktywny)",
	"miscadduser"		=> "dodaj u¿ytkownika",
	"miscedituser"		=> "edycja u¿ytkownika '%s'",
	"miscactive"		=> "Aktywny",
	"misclang"		=> "Jêzyk",
	"miscnoresult"		=> "Bez rezultatu.",
	"miscsubdirs"		=> "Szukaj w podkatalogach",
	"miscpermnames"		=> array("Tylko przegl±danie","Modyfikacja","Zmiana has³a","Modyfikacja i zmiana has³a",
					"Administrator"),
	"miscyesno"		=> array("Tak","Nie","T","N"),
	"miscchmod"		=> array("W³a¶ciciel", "Grupa", "Publiczny"),
);
?>