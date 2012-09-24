#!/usr/local/bin/php
<?php
/*
	system_edit.php

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
require("auth.inc");
require("guiconfig.inc");

$pgtitle = array(gettext("Advanced"), gettext("File Editor"));

$savetopath = "";
if (isset($_POST['savetopath']))
	$savetopath = htmlspecialchars($_POST['savetopath']);

if (($_POST['submit'] === gettext("Load")) && file_exists($savetopath) && is_file($savetopath)) {
	$content = file_get_contents($savetopath);
	$edit_area = "";
	if (stristr($savetopath, ".php") == true)
		$language = "php";
	else if (stristr($savetopath, ".inc") == true)
		$language = "php";
	else if (stristr($savetopath, ".sh") == true)
		$language = "core";
	else if (stristr($savetopath, ".xml") == true)
		$language = "xml";
	else if (stristr($savetopath, ".js") == true)
		$language = "js";
	else if (stristr($savetopath, ".css") == true)
		$language = "css";
} else if (($_POST['submit'] === gettext("Save"))) {
	conf_mount_rw();
	$content = ereg_replace("\r","",$_POST['code']) ;
	file_put_contents($savetopath, $content);
	$edit_area = "";
	$savemsg = gettext("Saved file to") . " " . $savetopath;
	if ($savetopath === "{$g['cf_conf_path']}/config.xml")
		unlink_if_exists("{$g['tmp_path']}/config.cache");
	conf_mount_ro();
} else if (($_POST['submit'] === gettext("Load")) && (!file_exists($savetopath) || !is_file($savetopath))) {
	$savemsg = gettext("File not found") . " " . $savetopath;
	$content = "";
	$savetopath = "";
}

if($_POST['highlight'] <> "") {
	if($_POST['highlight'] == "yes" or
	  $_POST['highlight'] == "enabled") {
		$highlight = "yes";
	} else {
		$highlight = "no";
	}
} else {
	$highlight = "no";
}

if($_POST['rows'] <> "")
	$rows = $_POST['rows'];
else
	$rows = 30;

if($_POST['cols'] <> "")
	$cols = $_POST['cols'];
else
	$cols = 66;
?>
<?php include("fbegin.inc");?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabcont">
			<form action="system_edit.php" method="post">
				<?php if ($savemsg) print_info_box($savemsg);?>
				<table width="100%" cellpadding='9' cellspacing='9' bgcolor='#eeeeee'>
					<tr>
						<td>
							<span class="label"><?=gettext("File path");?></span>
							<input size="42" id="savetopath" name="savetopath" value="<?=$savetopath;?>" />
							<input name="browse" type="button" class="formbtn" id="Browse" onclick='ifield = form.savetopath; filechooser = window.open("filechooser.php?p="+escape(ifield.value), "filechooser", "scrollbars=yes,toolbar=no,menubar=no,statusbar=no,width=550,height=300"); filechooser.ifield = ifield; window.ifield = ifield;' value="..." />
							<input name="submit" type="submit" class="formbtn" id="Load" value="<?=gettext("Load");?>" />
							<input name="submit" type="submit" class="formbtn" id="Save" value="<?=gettext("Save");?>" />
							<hr noshade="noshade" />
							<?php if($_POST['highlight'] == "no"): ?>
							<?=gettext("Rows"); ?>: <input size="3" name="rows" value="<? echo $rows; ?>" />
							<?=gettext("Cols"); ?>: <input size="3" name="cols" value="<? echo $cols; ?>" />
							|
							<?php endif; ?>
							<?=gettext("Highlighting"); ?>:
							<input id="highlighting_enabled" name="highlight" type="radio" value="yes" <?php if($highlight == "yes") echo " checked=\"checked\""; ?> />
							<label for="highlighting_enabled"><?=gettext("Enabled"); ?></label>
							<input id="highlighting_disabled" name="highlight" type="radio" value="no"<?php if($highlight == "no") echo " checked=\"checked\""; ?> />
							<label for="highlighting_disabled"><?=gettext("Disabled"); ?></label>
						</td>
					</tr>
				</table>
				<table width='100%'>
					<tr>
						<td valign="top" class="label">
							<div style="background: #eeeeee;" id="textareaitem">
							<!-- NOTE: The opening *and* the closing textarea tag must be on the same line. -->
							<textarea style="width: 98%; margin: 7px;" class="<?php echo $language; ?>:showcolumns" rows="<?php echo $rows; ?>" cols="<?php echo $cols; ?>" name="code"><?php echo htmlspecialchars($content);?></textarea>
							</div>
						</td>
					</tr>
				</table>
				<?php include("formend.inc");?>
			</form>
		</td>
	</tr>
</table>
<script type="text/javascript" src="syntaxhighlighter/shCore.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushCSharp.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushPhp.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushJScript.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushJava.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushVb.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushSql.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushXml.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushDelphi.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushPython.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushRuby.js"></script>
<script type="text/javascript" src="syntaxhighlighter/shBrushCss.js"></script>
<script type="text/javascript">
<!--
  // Set focus.
  document.forms[0].savetopath.focus();

  // Append css for syntax highlighter.
  var head = document.getElementsByTagName("head")[0];
  var linkObj = document.createElement("link");
  linkObj.setAttribute("type","text/css");
  linkObj.setAttribute("rel","stylesheet");
  linkObj.setAttribute("href","syntaxhighlighter/SyntaxHighlighter.css");
  head.appendChild(linkObj);

  // Activate dp.SyntaxHighlighter?
  <?php
  if($_POST['highlight'] == "yes") {
    echo "dp.SyntaxHighlighter.HighlightAll('code', true, true);\n";
    // Disable 'Save' button.
    echo "document.forms[0].Save.disabled = 1;\n";
  }
?>
//-->
</script>
<?php include("fend.inc");?>
