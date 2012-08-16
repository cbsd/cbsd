<?php

// Japanese Language Module for v2.3.2

$GLOBALS["charset"] = "utf-8";
$GLOBALS["text_dir"] = "ltr"; // ('ltr' for left to right, 'rtl' for right to left)
$GLOBALS["date_fmt"] = "Y/m/d H:i";
$GLOBALS["error_msg"] = array(
	// error
	"error"			=> "エラー",
	"back"			=> "戻る",
	
	// root
	"home"			=> "ホームディレクトリがありません。設定を確認してください。",
	"abovehome"		=> "カレントディレクトリはホームディレクトリの中にありません。",
	"targetabovehome"	=> "ターゲットディレクトリはホームディレクトリの中にありません。",
	
	// exist
	"direxist"		=> "このディレクトリは存在していません。",
	//"filedoesexist"	=> "このファイルは既に存在しています。",
	"fileexist"		=> "このファイルは存在していません。",
	"itemdoesexist"		=> "この項目は既に存在しています。",
	"itemexist"		=> "この項目は存在していません。",
	"targetexist"		=> "指定されたディレクトリは存在していません。",
	"targetdoesexist"	=> "指定された項目は既に存在しています。",
	
	// open
	"opendir"		=> "ディレクトリを開くことができません。",
	"readdir"		=> "ディレクトリを読むことができません。",
	
	// access
	"accessdir"		=> "このディレクトリへのアクセスが許可されていません。",
	"accessfile"		=> "このファイルへのアクセスが許可されていません。",
	"accessitem"		=> "この項目へのアクセスが許可されていません。",
	"accessfunc"		=> "この機能の利用が許可されていません。",
	"accesstarget"		=> "指定されたディレクトリへのアクセスが許可されていません。",
	
	// actions
	"chmod_not_allowed"  => 'Changing Permissions to NONE is not allowed!',
	"permread"		=> "権限の取得ができません。",
	"permchange"		=> "権限の変更ができません。",
	"openfile"		=> "ファイルのオープンができません。",
	"savefile"		=> "ファイルの保存ができません。",
	"createfile"		=> "ファイルの作成ができません。",
	"createdir"		=> "ディレクトリの作成ができません。",
	"uploadfile"		=> "ファイルのアップロードができません。",
	"copyitem"		=> "コピーすることができません。",
	"moveitem"		=> "移動することができません。",
	"delitem"		=> "削除することができません。",
	"chpass"		=> "パスワードの変更ができません。",
	"deluser"		=> "ユーザの削除ができません。",
	"adduser"		=> "ユーザの追加ができません。",
	"saveuser"		=> "ユーザの保存ができません。",
	"searchnothing"		=> "検索するものを入力する必要があります。",
	
	// misc
	"miscnofunc"		=> "機能は利用できません。",
	"miscfilesize"		=> "ファイルは最大サイズを超えています。",
	"miscfilepart"		=> "ファイルは一部だけアップロードされました。",
	"miscnoname"		=> "名前を入力する必要があります。",
	"miscselitems"		=> "項目を選択していません。",
	"miscdelitems"		=> "この \"+num+\" 項目を本当に削除しますか？",
	"miscdeluser"		=> "この '\"+user+\"' ユーザを本当に削除しますか？",
	"miscnopassdiff"	=> "新パスワードが現在と同じです。",
	"miscnopassmatch"	=> "パスワードが一致しません。",
	"miscfieldmissed"	=> "重要なフィールドが入力されていません。",
	"miscnouserpass"	=> "ユーザ名またはパスワードが正しくありません。",
	"miscselfremove"	=> "自分自身は削除できません。",
	"miscuserexist"		=> "ユーザは既に存在しています。",
	"miscnofinduser"	=> "ユーザが見つかりません。",
);
$GLOBALS["messages"] = array(
	// links
	"permlink"		=> "権限変更",
	"editlink"		=> "編集",
	"downlink"		=> "ダウンロード",
	"uplink"		=> "上へ",
	"homelink"		=> "ホーム",
	"reloadlink"		=> "リロード",
	"copylink"		=> "コピー",
	"movelink"		=> "移動",
	"dellink"		=> "削除",
	"comprlink"		=> "アーカイブ",
	"adminlink"		=> "管理",
	"logoutlink"		=> "ログアウト",
	"uploadlink"		=> "アップロード",
	"searchlink"		=> "検索",
	
	// list
	"nameheader"		=> "名前",
	"sizeheader"		=> "サイズ",
	"typeheader"		=> "タイプ",
	"modifheader"		=> "変更日",
	"permheader"		=> "権限",
	"actionheader"		=> "動作",
	"pathheader"		=> "パス",
	
	// buttons
	"btncancel"		=> "キャンセル",
	"btnsave"		=> "保存",
	"btnchange"		=> "変更",
	"btnreset"		=> "リセット",
	"btnclose"		=> "閉じる",
	"btncreate"		=> "作成",
	"btnsearch"		=> "検索",
	"btnupload"		=> "アップロード",
	"btncopy"		=> "コピー",
	"btnmove"		=> "移動",
	"btnlogin"		=> "ログイン",
	"btnlogout"		=> "ログアウト",
	"btnadd"		=> "追加",
	"btnedit"		=> "編集",
	"btnremove"		=> "削除",
	
	// actions
	"actdir"		=> "ディレクトリ",
	"actperms"		=> "権限変更",
	"actedit"		=> "ファイル編集",
	"actsearchresults"	=> "検索結果",
	"actcopyitems"		=> "コピー項目",
	"actcopyfrom"		=> "/%s から /%s にコピー",
	"actmoveitems"		=> "移動項目",
	"actmovefrom"		=> "/%s から /%s に移動",
	"actlogin"		=> "ログイン",
	"actloginheader"	=> "QuiXplorerにログイン",
	"actadmin"		=> "管理",
	"actchpwd"		=> "パスワード変更",
	"actusers"		=> "ユーザ",
	"actarchive"		=> "アーカイブ項目",
	"actupload"		=> "アップロードファイル",
	
	// misc
	"miscitems"		=> "項目",
	"miscfree"		=> "フリー",
	"miscusername"		=> "ユーザ名",
	"miscpassword"		=> "パスワード",
	"miscoldpass"		=> "旧パスワード",
	"miscnewpass"		=> "新パスワード",
	"miscconfpass"		=> "パスワード確認",
	"miscconfnewpass"	=> "新パスワード確認",
	"miscchpass"		=> "パスワード変更",
	"mischomedir"		=> "ホームディレクトリ",
	"mischomeurl"		=> "ホームURL",
	"miscshowhidden"	=> "隠し項目表示",
	"mischidepattern"	=> "隠しパターン",
	"miscperms"		=> "権限",
	"miscuseritems"		=> "(名前, ホームディレクトリ, 隠し項目, 権限, 有効)",
	"miscadduser"		=> "ユーザ追加",
	"miscedituser"		=> "'%s'ユーザ編集",
	"miscactive"		=> "有効",
	"misclang"		=> "言語",
	"miscnoresult"		=> "検索結果がありません。",
	"miscsubdirs"		=> "サブディレクトリの検索",
	"miscpermnames"		=> array("表示のみ","修正","パスワード変更","修正とパスワード変更",
					"管理者"),
	"miscyesno"		=> array("はい","いいえ","Y","N"),
	"miscchmod"		=> array("所有者", "グループ", "その他"),
);
?>