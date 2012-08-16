<?php

// Bulgarian Language Module for v2.3.2

$GLOBALS["charset"] = "UTF-8";
$GLOBALS["text_dir"] = "ltr"; // ('ltr' for left to right, 'rtl' for right to left)
$GLOBALS["date_fmt"] = "Y/m/d H:i";
$GLOBALS["error_msg"] = array(
      // error
      "error"                  => "ГРЕШКА(И)",
      "back"                  => "Назад",
      
      // root
      "home"                  => "Началната директория не съществува, проверете вашите настройки.",
      "abovehome"            => "Текущата директория не може да бъде преди началната.",
      "targetabovehome"      => "Целевата директория не може да бъде преди началната.",

      // exist
      "direxist"            => "Директорията не съществува",
      //"filedoesexist"      => "Файл с това име вече съществува",
      "fileexist"            => "Такъв файл не съществува",
      "itemdoesexist"            => "Такъв обект вече съществува",
      "itemexist"            => "Такъв обект не съществува",
      "targetexist"            => "Целевата директория не съществува",
      "targetdoesexist"      => "Целевият обект не съшествува",
      
      // open
      "opendir"            => "Директорията не може да бъде отворена",
      "readdir"            => "Директорията не може да бъде прочетена",

      // access
      "accessdir"            => "Нямате достъп до тази директория",
      "accessfile"            => "Нямате достъп до този файл",
      "accessitem"            => "Нямате достъп до този обект",
      "accessfunc"            => "Нямате право да ползвате тази функция",
      "accesstarget"            => "Нямате достъп до целевата директория",

      // actions
      "permread"            => "Грешка при получаване на права за достъп",
      "permchange"            => "Грешка при смяна права за достъп",
      "openfile"            => "Грешка при отваряне на файл",
      "savefile"            => "Грешка при запис на файл",
      "createfile"            => "Грешка при създаване на файл",
      "createdir"            => "Грешка при създаване на директория",
      "uploadfile"            => "Грешка при качване на файл",
      "copyitem"            => "Грешка при копиране",
      "moveitem"            => "Грешка при преименуване",
      "delitem"            => "Грешка при изтриване",
      "chpass"            => "Грешка при промяна на парола",
      "deluser"            => "Грешка при изтриване на потребител",
      "adduser"            => "Грешка при създаване на потребител",
      "saveuser"            => "Грешка при запис на потребител",
      "searchnothing"            => "Попълнете полето за търсене",
      
      // misc
      "miscnofunc"            => "Недостъпна функция",
      "miscfilesize"            => "Превишен максимален размер на файла",
      "miscfilepart"            => "Файла е качен частично",
      "miscnoname"            => "Трябва да въведете име",
      "miscselitems"            => "Не сте избрали обект(и)",
      "miscdelitems"            => "Сигурни ли сте че искате да изтриете тези \"+num+\" обект(а)?",
      "miscdeluser"            => "Сигурни ли сте че искате да изтриете потребител '\"+user+\"'?",
      "miscnopassdiff"      => "Новата парола не се отличава от предишната",
      "miscnopassmatch"      => "Паролите не съвпадат",
      "miscfieldmissed"      => "Пропуснали сте да попълните важно поле",
      "miscnouserpass"      => "Грешно име или парола",
      "miscselfremove"      => "Не можете да изтриете собственият си акаунт",
      "miscuserexist"            => "Потребителят вече съществува",
      "miscnofinduser"      => "Потребителят не може да бъде открит",
);
$GLOBALS["messages"] = array(
      // links
      "permlink"            => "ПРОМЕНИ ПРАВА НА ДОСТЪП",
      "editlink"            => "РЕДАКТИРАЙ",
      "downlink"            => "ИЗТЕГЛИ",
      "uplink"            => "НАГОРЕ",
      "homelink"            => "НАЧАЛО",
      "reloadlink"            => "ОБНОВИ",
      "copylink"            => "КОПИРАЙ",
      "movelink"            => "ПРЕМЕСТИ",
      "dellink"            => "ИЗТРИЙ",
      "comprlink"            => "АРХИВИРАЙ",
      "adminlink"            => "АДМИНИСТРИРАНЕ",
      "logoutlink"            => "ИЗХОД",
      "uploadlink"            => "ПРИКАЧИ",
      "searchlink"            => "ТЪРСИ",
      
      // list
      "nameheader"            => "Файл",
      "sizeheader"            => "Размер",
      "typeheader"            => "Тип",
      "modifheader"            => "Променен",
      "permheader"            => "Права",
      "actionheader"            => "Действия",
      "pathheader"            => "Път",
      
      // buttons
      "btncancel"            => "Отмени",
      "btnsave"            => "Съхрани",
      "btnchange"            => "Промени",
      "btnreset"            => "Изчисти",
      "btnclose"            => "Затвори",
      "btncreate"            => "Създай",
      "btnsearch"            => "Търси",
      "btnupload"            => "Прикачи",
      "btncopy"            => "Копирай",
      "btnmove"            => "Премести",
      "btnlogin"            => "Вход",
      "btnlogout"            => "Изход",
      "btnadd"            => "Добави",
      "btnedit"            => "Редактирай",
      "btnremove"            => "Изтрий",
      
      // actions
      "actdir"            => "Папка",
      "actperms"            => "Промяна на права",
      "actedit"            => "Редактирай файл",
      "actsearchresults"      => "Резултати от търсене",
      "actcopyitems"            => "Копирай обект(и)",
      "actcopyfrom"            => "Копирай от /%s в /%s ",
      "actmoveitems"            => "Премести обект(и)",
      "actmovefrom"            => "Премести от /%s в /%s ",
      "actlogin"            => "Вход",
      "actloginheader"      => "Вход за да ползваш QuiXplorer",
      "actadmin"            => "Администриране",
      "actchpwd"            => "Смени парола",
      "actusers"            => "Потребители",
      "actarchive"            => "Архивирай объект(и)",
      "actupload"            => "Прикачи файл(ове)",
      
      // misc
      "miscitems"            => "Обект(и)",
      "miscfree"            => "Свободно",
      "miscusername"            => "Потребител",
      "miscpassword"            => "Парола",
      "miscoldpass"            => "Стара парола",
      "miscnewpass"            => "Нова парола",
      "miscconfpass"            => "Потвърдете парола",
      "miscconfnewpass"      => "Потвърдете нова парола",
      "miscchpass"            => "Промени парола",
      "mischomedir"            => "Начална директория",
      "mischomeurl"            => "Начален URL",
      "miscshowhidden"      => "Показвай скрите обекти",
      "mischidepattern"      => "Скрий файлове",
      "miscperms"            => "Права",
      "miscuseritems"            => "(име, начална директория, показвай скрити обекти, права за достъп, активен)",
      "miscadduser"            => "добави потребител",
      "miscedituser"            => "редактирай потребител '%и'",
      "miscactive"            => "Активен",
      "misclang"            => "Език",
      "miscnoresult"            => "Няма резултати",
      "miscsubdirs"            => "Търси в поддиректории",
      "miscpermnames"            => array("Само да разглежда","Редактиране","Смяна на парола","Права и смяна на парола",
                              "Администратор"),
      "miscyesno"            => array("Да","Не","Д","Н"),
      "miscchmod"            => array("Притежател", "Група", "Общодостъпен"),
);
?>
