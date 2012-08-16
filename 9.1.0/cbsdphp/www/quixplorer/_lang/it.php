<?php

// Italian Language Module for v2.3.2

$GLOBALS["charset"] = "UTF-8";
$GLOBALS["text_dir"] = "ltr"; // ('ltr' for left to right, 'rtl' for right to left)
$GLOBALS["date_fmt"] = "d-m-Y H:i";
$GLOBALS["error_msg"] = array(
	// error
	"error"			=> "ERRORE(I)",
	"back"			=> "Torna indietro",
	
	// root
	"home"			=> "La directory home non esiste, controlla le impostazioni.",
	"abovehome"		=> "La directory corrente potrebbe non essere sopra al livello della directory home.",
	"targetabovehome"	=> "La directory di destinazione potrebbe non essere sopra al livello della directory home.",
	
	// exist
	"direxist"		=> "Questa cartella non esiste.",
	//"filedoesexist"	=> "Questo file esiste già.",
	"fileexist"		=> "Questo file non esiste.",
	"itemdoesexist"		=> "Questo elemento esiste già.",
	"itemexist"		=> "Questo elemento non esiste.",
	"targetexist"		=> "La directory di destinazione non esiste.",
	"targetdoesexist"	=> "L'elemento di destinazione esiste già.",
	
	// open
	"opendir"		=> "Impossibile aprire la directory.",
	"readdir"		=> "Impossibile leggere la directory.",
	
	// access
	"accessdir"		=> "Non hai il permesso di accedere a questa directory.",
	"accessfile"		=> "Non hai il permesso di accedere a questo file.",
	"accessitem"		=> "Non hai il permesso di accedere a questo elemento.",
	"accessfunc"		=> "Non hai il permesso di usare questa funzione.",
	"accesstarget"		=> "Non hai il permesso di accedere alla directory di destinazione.",
	
	// actions
	"chmod_not_allowed"  => 'Changing Permissions to NONE is not allowed!',
	"permread"		=> "Recupero dei permessi fallito.",
	"permchange"		=> "Cambiamento dei permessi fallito.",
	"openfile"		=> "Apertura del file fallita.",
	"savefile"		=> "Salvataggio del file fallito.",
	"createfile"		=> "Creazione del file fallita.",
	"createdir"		=> "Creazione della directory fallita.",
	"uploadfile"		=> "Caricamento del file fallito.",
	"copyitem"		=> "Copia del file fallita.",
	"moveitem"		=> "Spostamento del file fallito.",
	"delitem"		=> "Eliminazione del file fallita.",
	"chpass"		=> "Cambiamento della password fallito.",
	"deluser"		=> "Rimozione dell'utente fallita.",
	"adduser"		=> "Aggiunta dell'utente fallita.",
	"saveuser"		=> "Salvataggio dell'utente fallito.",
	"searchnothing"		=> "Devi fornire un criterio di ricerca.",
	
	// misc
	"miscnofunc"		=> "Funzione non disponibile.",
	"miscfilesize"		=> "Il file supera la dimensione massima.",
	"miscfilepart"		=> "Il file è stato caricato solo parzialmente.",
	"miscnoname"		=> "Devi fornire un nome.",
	"miscselitems"		=> "Non hai selezionato nessuno elemento(i).",
	"miscdelitems"		=> "Sei sicuro di voler eliminare questi \"+num+\" elemento(i)?",
	"miscdeluser"		=> "Sei sicuro di voler eliminare l'utente '\"+user+\"'?",
	"miscnopassdiff"	=> "La nuova password non è diversa da quella attualmente impostata.",
	"miscnopassmatch"	=> "La password non corrisponde.",
	"miscfieldmissed"	=> "Hai dimenticato un campo importante.",
	"miscnouserpass"	=> "Nome utente o password non corretti.",
	"miscselfremove"	=> "Non puoi rimuovere te stesso.",
	"miscuserexist"		=> "L'utente esiste già.",
	"miscnofinduser"	=> "Impossibile trovare l'utente.",
);
$GLOBALS["messages"] = array(
	// links
	"permlink"		=> "CAMBIA I PERMESSI",
	"editlink"		=> "MODIFICA",
	"downlink"		=> "SCARICA",
	"uplink"		=> "SU",
	"homelink"		=> "HOME",
	"reloadlink"		=> "RICARICA",
	"copylink"		=> "COPIA",
	"movelink"		=> "SPOSTA",
	"dellink"		=> "ELIMINA",
	"comprlink"		=> "COMPRIMI",
	"adminlink"		=> "ADMIN",
	"logoutlink"		=> "LOGOUT",
	"uploadlink"		=> "CARICA",
	"searchlink"		=> "CERCA",
	
	// list
	"nameheader"		=> "Nome",
	"sizeheader"		=> "Dimensione",
	"typeheader"		=> "Tipo",
	"modifheader"		=> "Modificato",
	"permheader"		=> "Permessi",
	"actionheader"		=> "Azioni",
	"pathheader"		=> "Percorso",
	
	// buttons
	"btncancel"		=> "Annulla",
	"btnsave"		=> "Salva",
	"btnchange"		=> "Cambia",
	"btnreset"		=> "Resetta",
	"btnclose"		=> "Chiudi",
	"btncreate"		=> "Crea",
	"btnsearch"		=> "Cerca",
	"btnupload"		=> "Carica",
	"btncopy"		=> "Copia",
	"btnmove"		=> "Sposta",
	"btnlogin"		=> "Login",
	"btnlogout"		=> "Logout",
	"btnadd"		=> "Aggiungi",
	"btnedit"		=> "Modifica",
	"btnremove"		=> "Rimuovi",
	
	// actions
	"actdir"		=> "Directory",
	"actperms"		=> "Cambia i permessi",
	"actedit"		=> "Modifica il file",
	"actsearchresults"	=> "Risultati della ricerca",
	"actcopyitems"		=> "Copia elemento(i)",
	"actcopyfrom"		=> "Copia da /%s a /%s ",
	"actmoveitems"		=> "Copia elemento(i)",
	"actmovefrom"		=> "Sposta da /%s a /%s ",
	"actlogin"		=> "Login",
	"actloginheader"	=> "Login per usare QuiXplorer",
	"actadmin"		=> "Amministrazione",
	"actchpwd"		=> "Cambia la password",
	"actusers"		=> "Utenti",
	"actarchive"		=> "Archivia elemento(i)",
	"actupload"		=> "Carica file(s)",
	
	// misc
	"miscitems"		=> "Elemento(i)",
	"miscfree"		=> "Liberi",
	"miscusername"		=> "Nome utente",
	"miscpassword"		=> "Password",
	"miscoldpass"		=> "Vecchia password",
	"miscnewpass"		=> "Nuova password",
	"miscconfpass"		=> "Conferma la password",
	"miscconfnewpass"	=> "Conferma la nuova password",
	"miscchpass"		=> "cambia la password",
	"mischomedir"		=> "Directory home",
	"mischomeurl"		=> "URL della home",
	"miscshowhidden"	=> "Mostra elementi nascosti",
	"mischidepattern"	=> "Nascondi il motivo",
	"miscperms"		=> "Permessi",
	"miscuseritems"		=> "(nome, directory home, mostra elementi nascosti, permessi, attivo)",
	"miscadduser"		=> "aggiungi utente",
	"miscedituser"		=> "modifica l'utente '%s'",
	"miscactive"		=> "Attivo",
	"misclang"		=> "Lingua",
	"miscnoresult"		=> "Nessun risultato disponibile.",
	"miscsubdirs"		=> "Cerca nelle subdirectories",
	"miscpermnames"		=> array("Solo visualizzazione","Modifica","Cambio della password","Modifica e cambio della password",
					"Amministratore"),
	"miscyesno"		=> array("Si","No","S","N"),
	"miscchmod"		=> array("Proprietario", "Gruppo", "Pubblico"),
);
?>