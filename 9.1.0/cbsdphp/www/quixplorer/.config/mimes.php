<?php
/*
	mimes.php
	
	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met: 

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer. 
	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those
	of the authors and should not be interpreted as representing official policies, 
	either expressed or implied, of the NAS4Free Project.
*/
// editable files:
$GLOBALS["editable_ext"]=array(
	"\.txt$|\.php$|\.php3$|\.phtml$|\.inc$|\.sql$|\.pl$",
	"\.htm$|\.html$|\.shtml$|\.dhtml$|\.xml$",
	"\.js$|\.css$|\.cgi$|\.cpp$\.c$|\.cc$|\.cxx$|\.hpp$|\.h$",
	"\.pas$|\.p$|\.java$|\.py$|\.sh$\.tcl$|\.tk$",
	"\.conf$|\.subr$"
);
//------------------------------------------------------------------------------
// image files:
$GLOBALS["images_ext"]="\.png$|\.bmp$|\.jpg$|\.jpeg$|\.gif$";
//------------------------------------------------------------------------------
// mime types: (description,image,extension)
$GLOBALS["super_mimes"]=array(
	// dir, exe, file
	"dir"	=> array($GLOBALS["mimes"]["dir"],"dir.gif"),
	"exe"	=> array($GLOBALS["mimes"]["exe"],"exe.gif","\.exe$|\.com$|\.bin$"),
	"file"	=> array($GLOBALS["mimes"]["file"],"file.gif")
);
$GLOBALS["used_mime_types"]=array(
	// text
	"text"	=> array($GLOBALS["mimes"]["text"],"txt.gif","\.txt$"),
	
	// programming
	"php"	=> array($GLOBALS["mimes"]["php"],"php.gif","\.php$|\.php3$|\.phtml$|\.inc$"),
	"sql"	=> array($GLOBALS["mimes"]["sql"],"src.gif","\.sql$"),
	"perl"	=> array($GLOBALS["mimes"]["perl"],"pl.gif","\.pl$"),
	"html"	=> array($GLOBALS["mimes"]["html"],"html.gif","\.htm$|\.html$|\.shtml$|\.dhtml$|\.xml$"),
	"js"	=> array($GLOBALS["mimes"]["js"],"js.gif","\.js$"),
	"css"	=> array($GLOBALS["mimes"]["css"],"src.gif","\.css$"),
	"cgi"	=> array($GLOBALS["mimes"]["cgi"],"exe.gif","\.cgi$"),
	//"py"	=> array($GLOBALS["mimes"]["py"],"py.gif","\.py$"),
	//"sh"	=> array($GLOBALS["mimes"]["sh"],"sh.gif","\.sh$"),
	// C++
	"cpps"	=> array($GLOBALS["mimes"]["cpps"],"cpp.gif","\.cpp$|\.c$|\.cc$|\.cxx$"),
	"cpph"	=> array($GLOBALS["mimes"]["cpph"],"h.gif","\.hpp$|\.h$"),
	// Java
	"javas"	=> array($GLOBALS["mimes"]["javas"],"java.gif","\.java$"),
	"javac"	=> array($GLOBALS["mimes"]["javac"],"java.gif","\.class$|\.jar$"),
	// Pascal
	"pas"	=> array($GLOBALS["mimes"]["pas"],"src.gif","\.p$|\.pas$"),
	
	// images
	"gif"	=> array($GLOBALS["mimes"]["gif"],"image.gif","\.gif$"),
	"jpg"	=> array($GLOBALS["mimes"]["jpg"],"image.gif","\.jpg$|\.jpeg$"),
	"bmp"	=> array($GLOBALS["mimes"]["bmp"],"image.gif","\.bmp$"),
	"png"	=> array($GLOBALS["mimes"]["png"],"image.gif","\.png$"),
	
	// compressed
	"zip"	=> array($GLOBALS["mimes"]["zip"],"zip.gif","\.zip$"),
	"tar"	=> array($GLOBALS["mimes"]["tar"],"tar.gif","\.tar$"),
	"gzip"	=> array($GLOBALS["mimes"]["gzip"],"tgz.gif","\.tgz$|\.gz$"),
	"bzip2"	=> array($GLOBALS["mimes"]["bzip2"],"tgz.gif","\.bz2$"),
	"rar"	=> array($GLOBALS["mimes"]["rar"],"tgz.gif","\.rar$"),
	//"deb"	=> array($GLOBALS["mimes"]["deb"],"package.gif","\.deb$"),
	//"rpm"	=> array($GLOBALS["mimes"]["rpm"],"package.gif","\.rpm$"),
	
	// music
	"mp3"	=> array($GLOBALS["mimes"]["mp3"],"mp3.gif","\.mp3$"),
	"wav"	=> array($GLOBALS["mimes"]["wav"],"sound.gif","\.wav$"),
	"midi"	=> array($GLOBALS["mimes"]["midi"],"midi.gif","\.mid$"),
	"real"	=> array($GLOBALS["mimes"]["real"],"real.gif","\.rm$|\.ra$|\.ram$"),
	"flac"	=> array($GLOBALS["mimes"]["flac"],"flac.gif","\.flac$"),
	//"play"	=> array($GLOBALS["mimes"]["play"],"mp3.gif","\.pls$|\.m3u$"),
	
	// movie
	"mpg"	=> array($GLOBALS["mimes"]["mpg"],"video.gif","\.mpg$|\.mpeg$"),
	"mov"	=> array($GLOBALS["mimes"]["mov"],"video.gif","\.mov$"),
	"avi"	=> array($GLOBALS["mimes"]["avi"],"video.gif","\.avi$"),
	"flash"	=> array($GLOBALS["mimes"]["flash"],"flash.gif","\.swf$"),
	
	// Micosoft / Adobe
	"word"	=> array($GLOBALS["mimes"]["word"],"word.gif","\.doc$"),
	"excel"	=> array($GLOBALS["mimes"]["excel"],"spread.gif","\.xls$"),
	"pdf"	=> array($GLOBALS["mimes"]["pdf"],"pdf.gif","\.pdf$")
);
//------------------------------------------------------------------------------
?>
