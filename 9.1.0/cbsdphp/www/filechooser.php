<?
/*
	filechooser.php

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
	All rights reserved.
	
	Parts of code are take from 'File Browser Class'
	Copyright @ 2005 Gabriel Dunne <gdunne@quilime.com>.
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
require("auth.inc");
require("guiconfig.inc");

class FileChooser
{
	var	$cfg = array();

	function FileChooser()
	{
		// Settings.
		$this->cfg['footer'] = true; // show footer
		$this->cfg['sort'] = true; // show sorting header
		$this->cfg['lineNumbers'] = true; // show/hide column
		$this->cfg['showFileSize'] = true; // show/hide column
		$this->cfg['showFileModDate'] = true; // show/hide column
		$this->cfg['showFileType'] = true; // show/hide column
		$this->cfg['calcFolderSizes'] = false; // calculate folder sizes (increases processing time)
		$this->cfg['simpleType'] = true; // display MIME type, or "simple" file type (MIME type increases processing time)
		$this->cfg['separateFolders'] = true; // sort folders on top of files
		$this->cfg['naturalSort'] = true; // natural sort files, as opposed to regular sort (files with capital letters get sorted first)
		$this->cfg['filterShowFiles'] = '*';
		$this->cfg['filterHideFiles'] = '.*';
		$this->cfg['filterShowFolders'] = '*';
		$this->cfg['filterHideFolders'] = '.,..,.*';
		$this->cfg['dateFormat'] = 'F d, Y g:i A'; // date format.
		$this->cfg['startDirectory'] = "/";

		// Get path if browsing a tree.
		$path = (isset($_GET['p'])) ? urldecode(htmlspecialchars($_GET['p'])) : FALSE;

		// If no path is available, set it to root.
		if (!$path) {
			$path = (isset($_GET['sd'])) ? htmlspecialchars($_GET['sd']) : $this->cfg['startDirectory'];
    }

    // Check if file exists.
    if(!file_exists($path)) {
    	echo "<tr><td class=\"addrbar\">";
		  print_info_box("File not found $path");
		  echo "</tr></td>";
      $path = $this->get_valid_parent_dir($path);
    }

    $dir = $path;

		// Extract path if necessary.
    if(is_file($dir)) {
      $dir = dirname($dir).'/';
    }

		// Check if directory string end with '/'. Add it if necessary.
		if ("/" !== substr(strrev($dir),0,1)) {
			$dir .= "/";
		}

		// Get sorting vars from URL, if nothing is set, sort by N [file Name].
		$this->cfg['sortMode'] = (isset($_GET['N']) ? 'N' :
			        				 (isset($_GET['S']) ? 'S' :
			         				 (isset($_GET['T']) ? 'T' :
			         				 (isset($_GET['M']) ? 'M' : 'N' ))));

		// Get sort ascending or descending.
		$this->cfg['sortOrder'] =
			isset($_GET[$this->cfg['sortMode']]) ?
			$_GET[$this->cfg['sortMode']] : 'A';

		// Create array of files in tree.
		$files = $this->make_file_array($dir);

		// Get size of arrays before sort.
		$totalFolders = sizeof($files['folders']);
		$totalFiles = sizeof($files['files']);

		// Sort files.
		$files = $this->sort_files($files);

    // Display navigation bar.
    echo $this->navigation_bar($path);

		// Display file list.
		echo $this->file_list($dir, $files);
	}

  function make_file_array($dir)
	{
		if(!function_exists('mime_content_type'))
		{
		   function mime_content_type($file)
		   {
			   $file = escapeshellarg($file);
			   $type = `file -bi $file`;
		   	   $expl = explode(";", $type);
		   	   return $expl[0];
		   }
		}

		$dirArray	= array();
		$folderArray = array();
		$folderInfo = array();
		$fileArray = array();
		$fileInfo = array();

		$content = $this->get_content($dir);

		foreach($content as $file)
    {
			if(is_dir("{$dir}/{$file}")) // is a folder
			{
				// store elements of folder in sub array
				$folderInfo['name']	= $file;
				$folderInfo['mtime'] = @filemtime("{$dir}/{$file}");
				$folderInfo['type'] = 'Folder';
				// calc folder size ?
				$folderInfo['size'] =
					$this->cfg['calcFolderSizes'] ?
					$this->get_folder_size("{$dir}/{$file}") : '-';
				$folderInfo['rowType'] = 'fr';
				$folderArray[] = $folderInfo;
			}
			else // is a file
			{
				// store elements of file in sub array
				$fileInfo['name'] = $file;
				$fileInfo['mtime'] = @filemtime("{$dir}/{$file}");
				$fileInfo['type'] = $this->cfg['simpleType'] ?
					$this->get_extension("{$dir}/{$file}") :
					mime_content_type("{$dir}/{$file}");
				$fileInfo['size'] = @get_filesize("{$dir}/{$file}");
				$fileInfo['rowType'] = 'fl';
				$fileArray[] = $fileInfo;
			}
		}

		$dirArray['folders'] = $folderArray;
		$dirArray['files'] = $fileArray;

		return $dirArray;
  }

  function get_content($dir)
  {
    $folders = array();
    $files = array();

    $handle = @opendir($dir);
    while($file = @readdir($handle)) {
      if(is_dir("{$dir}/{$file}"))
        $folders[] = $file;
      elseif(is_file("{$dir}/{$file}"))
        $files[] = $file;
    }
    @closedir($handle);

    $folders = $this->filter_content($folders, $this->cfg['filterShowFolders'], $this->cfg['filterHideFolders']);
    $files = $this->filter_content($files, $this->cfg['filterShowFiles'], $this->cfg['filterHideFiles']);

    return array_merge($folders, $files);
  }

  function filter_content($arr, $allow, $hide)
  {
    $allow = $this->make_regex($allow);
    $hide = $this->make_regex($hide);

    $ret = array();
    $ret = preg_grep("/$allow/", $arr);
    $ret = preg_grep("/$hide/",  $ret, PREG_GREP_INVERT);

    return $ret;
  }

  function make_regex($filter)
  {
    $regex = str_replace('.', '\.', $filter);
    $regex = str_replace('/', '\/', $regex);
    $regex = str_replace('*', '.+', $regex);
    $regex = str_replace(',', '$|^', $regex);
    return "^$regex\$";
  }

  function get_extension($filename)
	{
		$justfile = explode("/", $filename);
		$justfile = $justfile[(sizeof($justfile)-1)];
    	$expl = explode(".", $justfile);
		if(sizeof($expl)>1 && $expl[sizeof($expl)-1])
		{
    		return $expl[sizeof($expl)-1];
    	}
		else
		{
			return '?';
		}
	}

  function get_valid_parent_dir($path)
  {
    if(strcmp($path,"/") == 0) // Is it already root of filesystem?
      return false;

  	$expl = explode("/", substr($path, 0, -1));
  	$path = substr($path, 0, -strlen($expl[(sizeof($expl)-1)].'/'));

  	if(!(file_exists($path) && is_dir($path)))
      $path = $this->get_valid_parent_dir($path);

    return $path;
  }

	function format_size($bytes)
	{
		if(is_numeric($bytes) && $bytes > 0)
		{
			$formats = array("%d Bytes","%.1f KB","%.1f MB","%.1f GB","%.1f TB");
			$logsize = min(intval(log($bytes)/log(1024)), count($formats)-1);
			return sprintf($formats[$logsize], $bytes/pow(1024, $logsize));
		}
		// is a folder without calculated size
		else if(!is_numeric($bytes) && $bytes == '-')
		{
			return '-';
		}
		else
		{
			return '0 bytes';
		}
	}

	function get_folder_size($dir)
	{
		$size = 0;
		if ($handle = opendir($dir)) {
			while (false !== ($file = readdir($handle))) {
				if ($file != '.' && $file != '..') {
					if(is_dir("{$dir}/{$file}")) {
						$size += $this->get_folder_size("{$dir}/{$file}");
					} else {
						$size += @get_filesize("{$dir}/{$file}");
					}
				}
			}
		}
		return $size;
	}

	function sort_files($files)
	{
		// sort folders on top
		if($this->cfg['separateFolders'])
		{
			$sortedFolders = $this->order_by_column($files['folders'], '2');

			$sortedFiles = $this->order_by_column($files['files'], '1');

			// sort files depending on sort order
			if($this->cfg['sortOrder'] == 'A')
			{
				ksort($sortedFolders);
				ksort($sortedFiles);
				$result = array_merge($sortedFolders, $sortedFiles);
			}
			else
			{
				krsort($sortedFolders);
				krsort($sortedFiles);
				$result = array_merge($sortedFiles, $sortedFolders);
			}
		}
		else
		// sort folders and files together
		{
			$files = array_merge($files['folders'], $files['files']);
			$result = $this->order_by_column($files,'1');

			// sort files depending on sort order
			$this->cfg['sortOrder'] == 'A' ? ksort($result):krsort($result);
		}
		return $result;
	}

	function order_by_column($input, $type)
	{
		$column = $this->cfg['sortMode'];

		$result = array();

		// available sort columns
		$columnList = array('N'=>'name',
							'S'=>'size',
							'T'=>'type',
							'M'=>'mtime');

		// row count
		// each array key gets $rowcount and $type
		// concatinated to account for duplicate array keys
		$rowcount = 0;

		// create new array with sort mode as the key
		foreach($input as $key=>$value)
		{
			// natural sort - make array keys lowercase
			if($this->cfg['naturalSort'])
			{
				$col = $value[$columnList[$column]];
				$res = strtolower($col).'.'.$rowcount.$type;
				$result[$res] = $value;
			}
			// regular sort - uppercase values get sorted on top
			else
			{
				$res = $value[$columnList[$column]].'.'.$rowcount.$type;
				$result[$res] = $value;
			}
			$rowcount++;
		}
		return $result;
	}

	function file_list($dir, $files)
	{
    $ret .= '<tr>';
    $ret .= '<td class="filelist">';
    $ret .= '<table cellspacing="0" border="0">';
		$ret .= ($this->cfg['sort']) ? $this->row('sort', $dir) : '';
		$ret .= ($this->get_valid_parent_dir($dir)) ? $this->row('parent', $dir) : '';

		// total number of files
		$rowcount  = 1;
		// total byte size of the current tree
		$totalsize = 0;

		// rows of files
		foreach($files as $file)
		{
			$ret .= $this->row($file['rowType'], $dir, $rowcount, $file);
			$rowcount++;
			$totalsize += $file['size'];
		}

		$this->cfg['totalSize'] = $this->format_size($totalsize);

		$ret .= ($this->cfg['footer']) ? $this->row('footer') : '';

		$ret .= '</table>';
		$ret .= '</td>';
		$ret .= '</tr>';

		return $ret;
	}

	function row($type, $dir=null, $rowcount=null, $file=null)
	{
		$scriptname = "filechooser.php";
		// alternating row styles
		$rnum = $rowcount ? ($rowcount%2 == 0 ? '_even' : '_odd') : null;

		// start row string variable to be returned
		$row = "\n".'<tr class="'.$type.$rnum.'">'."\n";

		switch($type)
		{
			// file / folder row
			case 'fl':
			case 'fr':
				// line number
				$row .= $this->cfg['lineNumbers'] ? '<td class="ln">'.$rowcount.'</td>' : '';

				// filename
				$row .= '<td class="nm"><a href="';
				$row .= ''.$scriptname.'?p=' . urlencode("{$dir}{$file['name']}") . (($type === "fr") ? "/" : "");
				$row .= '">'.$file['name'].'</a></td>';

				// file size
				$row .= $this->cfg['showFileSize'] ?
				        '<td class="sz">'.$this->format_size($file['size']).'
				         </td>' : '';

				// file type
				$row .= $this->cfg['showFileType'] ?
				        '<td class="tp">'.$file['type'].'</td>' : '';

				// date
				$row .= $this->cfg['showFileModDate'] ?
				        '<td class="dt">
				        '.date($this->cfg['dateFormat'], $file['mtime']).'
				         </td>' : '';
				break;

			// sorting header
			case 'sort':
				// sort order. Setting ascending or descending for sorting links
				$N = ($this->cfg['sortMode'] == 'N') ?
					 ($this->cfg['sortOrder'] == 'A' ? 'D' : 'A') : 'A';

				$S = ($this->cfg['sortMode'] == 'S') ?
					 ($this->cfg['sortOrder'] == 'A' ? 'D' : 'A') : 'A';

				$T = ($this->cfg['sortMode'] == 'T') ?
					 ($this->cfg['sortOrder'] == 'A' ? 'D' : 'A') : 'A';

				$M = ($this->cfg['sortMode'] == 'M') ?
					 ($this->cfg['sortOrder'] == 'A' ? 'D' : 'A') : 'A';

				$row .= $this->cfg['lineNumbers'] ?
				        '<td class="ln">&nbsp;</td>' : '';
				$row .= '<td><a href="'.$scriptname.'?N='.$N.'&amp;p=' . urlencode($dir) . '">Name</a></td>';
				$row .= $this->cfg['showFileSize'] ?
					    '<td class="sz">
						 <a href="'.$scriptname.'?S='.$S.'&amp;p=' . urlencode($dir) . '">Size</a>
						 </td>' : '';
				$row .= $this->cfg['showFileType'] ?
				        '<td class="tp">
				         <a href="'.$scriptname.'?T='.$T.'&amp;p=' . urlencode($dir) . '">Type</a>
				         </td>' : '';
				$row .= $this->cfg['showFileModDate'] ?
					    '<td class="dt">
					     <a href="'.$scriptname.'?M='.$M.'&amp;p=' . urlencode($dir) . '">Last Modified</a>
					     </td>' : '';
				break;

			// parent directory row
			case 'parent':
				$row .= $this->cfg['lineNumbers'] ?
				        '<td class="ln">&laquo;</td>' : '';
				$row .= '<td class="nm">
				         <a href="'.$scriptname.'?p=' . urlencode($this->get_valid_parent_dir($dir)) . '">';
				$row .= 'Parent Directory';
				$row .= '</a></td>';
				$row .= $this->cfg['showFileSize'] ?
				        '<td class="sz">&nbsp;</td>' : '';
				$row .= $this->cfg['showFileType'] ?
				        '<td class="tp">&nbsp;</td>' : '';
				$row .= $this->cfg['showFileModDate'] ?
				        '<td class="dt">&nbsp;</td>' : '';
				break;

			// footer row
			case 'footer':
				$row .= $this->cfg['lineNumbers'] ?
				        '<td class="ln">&nbsp;</td>' : '';
				$row .= '<td class="nm">&nbsp;</td>';
				$row .= $this->cfg['showFileSize'] ?
				        '<td class="sz">'.$this->cfg['totalSize'].'
				         </td>' : '';
				$row .= $this->cfg['showFileType'] ?
				        '<td class="tp">&nbsp;</td>' : '';
				$row .= $this->cfg['showFileModDate'] ?
				        '<td class="dt">&nbsp;</td>' : '';
				break;
		}

		$row .= '</tr>';
		return $row;
	}

  function navigation_bar($path)
	{
    $ret .= <<<EOD
	<tr>
	<td class="navbar">
	<form method="get" action="filechooser.php" onSubmit="onSubmit();" onReset="onReset();">
		<input class="input" name="p" value="{$path}" type="text">
		<input class="button" type="reset" value="Cancel">
		<input class="button" type="submit" value="Ok">
EOD;
    ob_start();
    include("formend.inc");
    $formend = ob_get_contents();
    ob_end_clean();
    $formend = str_replace('/>', '>', $formend);
    $ret .= $formend;
    $ret .= <<<EOD
	</form>
	</td>
	</tr>
EOD;
    return $ret;
  }
}
?>
<?php header("Content-Type: text/html; charset=" . system_get_language_codeset());?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html lang="<?=system_get_language_code();?>">
  <head>
	<title><?=htmlspecialchars(gettext("filechooser"));?></title>
	<meta http-equiv="Content-Type" content="text/html; charset=<?=system_get_language_codeset();?>">
	<meta http-equiv="Content-Script-Type" content="text/javascript">
	<meta http-equiv="Content-Style-Type" content="text/css">
  	<link href="gui.css" rel="stylesheet" type="text/css">
		<script type="text/javascript" src="niftycube/niftycube.js"></script>
		<script type="text/javascript" src="niftycube/niftylayout.js"></script>
		<style type="text/css">
		<!--
			body { background: #FFFFFF; min-width: 0px; }

			.filechooser { background-color: #fff; margin: 0px; padding: 0px; }
			.filechooser table { width: 100%; height: 100%; font-size: 11px; font-family: Tahoma, Verdana, Arial, sans-serif !important; }
			.filechooser td { padding: 10px; vertical-align: top; }

			.filechooser .filelist table { width:100%; }
			.filechooser .filelist table tr td { padding:1px; font-size:12px; }
			.filechooser .filelist table tr.fr_odd td,
			.filechooser .filelist table tr.fr_even td,
			.filechooser .filelist table tr.fl_odd td,
			.filechooser .filelist table tr.fl_even td { height: 18px; border-top:1px solid #fff; border-bottom:1px solid #ddd; }
			.filechooser .filelist table tr.fr_odd td,
			.filechooser .filelist table tr.fl_odd td { background:#eee; }
			.filechooser .filelist a { text-decoration:none; }
			.filechooser .filelist a:hover { text-decoration:underline; }
			/* sorting row */
			.filechooser .filelist table tr.sort td { height: 18px; border: 0; border-top: 1px solid #eee; border-bottom: 1px solid #eee; }
			.filechooser .filelist table tr.sort a { text-decoration:none; color:#9d9d9d; }
			.filechooser .filelist table tr.sort a:hover { text-decoration:underline; }
			/* parent row */
			.filechooser .filelist table tr.parent { font-weight:bold; }
			.filechooser .filelist table tr.parent td { height: 18px; border-bottom: 1px solid #eee; text-color: #ffffff; background: #435370 url(listtopic_bg.png); }
			.filechooser .filelist table tr.parent a { text-decoration:none; color:#ffffff; }
			.filechooser .filelist table tr.parent a:hover { text-decoration:underline; }
			/* filelist rows */
			/* line number column */
			.filechooser .filelist table tr td.ln { border-left:1px solid #eee; font-weight:normal; text-align:right; padding:0 10px 0 10px; width:10px; color: #999; }
			/* filename column */
			.filechooser .filelist table tr.fr_odd td.nm,
			.filechooser .filelist table tr.fr_even td.nm { font-weight:bold; }
			.filechooser .filelist table tr.fl_odd td.nm,
			.filechooser .filelist table tr.fl_even td.nm { font-weight:normal; }
			/* size column */
			.filechooser .filelist table tr td.sz { }
			/* type column */
			.filechooser .filelist table tr td.tp { }
			/* date column */
			.filechooser .filelist table tr td.dt { border-right:1px solid #eee; }
			/* footer row */
			.filechooser .filelist table tr.footer td { border:0; font-weight:bold; }

			/* Navigation bar */
			.filechooser .navbar { background-color: #eee; padding: 6px 9px; text-align:left; border-left:1px solid #eee; border-right:1px solid #eee; border-bottom:1px solid #eee; border-spacing:0; height: 40px }
			.filechooser .navbar .input { position:absolute; width:75%; top: 6px; left: 9px; }
			.filechooser .navbar .button { position:relative; float:right; }
		-->
		</style>
		<script type="text/javascript">
		<!--
			function onSubmit()
			{
				opener.ifield.value = document.forms[0].p.value;
				close();
			}
			function onReset()
			{
				close();
			}
		// -->
		</script>
  </head>
  <body class="filechooser">
  	<table cellspacing="0">
		<?php new FileChooser();?>
		</table>
  </body>
</html>
