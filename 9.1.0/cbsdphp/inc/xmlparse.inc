<?php
/*
	xmlparse.inc
	functions to parse/dump configuration files in XML format

	Part of NAS4Free (http://www.nas4free.org).
	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
	All rights reserved.

	Portions of freenas (http://www.freenas.org).
	Copyright (C) 2005-2011 by Olivier Cochard (olivier@freenas.org).
	All rights reserved.

	Portions of m0n0wall (http://m0n0.ch/wall).
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

/* tags that are always to be handled as lists */
/* This are TAGS that can be multiple in the XML config file */
$listtags = explode(" ", "dnsserver winsserver disk vdisk diskr sharetosync " .
	"encryption-algorithm-option hash-algorithm-option hosts onetoone hidemount " .
	"staticmap route pipe queue shellcmd cacert earlyshellcmd user group " .
	"servernat nfsnetworks passthrumac allowedip mount vlan domainoverrides " .
	"minute hour day month weekday content rsyncclient rsynclocal package " .
	"ipv6dnsserver share auxparam extent device target storage group job report " .
	"cmd module selftest url param lagg laggport vdevice pool dataset volume " .
	"snapshot autosnapshot rule " .
	"portalgroup portal initiatorgroup iginitiatorname ignetmask authgroup " .
	"agauth pgigmap agmap lunmap");

function startElement($parser, $name, $attrs) {
	global $depth, $curpath, $config, $havedata, $listtags;

	array_push($curpath, strtolower($name));

	$ptr =& $config;
	foreach ($curpath as $path) {
		$ptr =& $ptr[$path];
	}

	/* is it an element that belongs to a list? */
	if (in_array(strtolower($name), $listtags)) {
			/* is there an array already? */
		if (!is_array($ptr)) {
			/* make an array */
			$ptr = array();
		}

		array_push($curpath, count($ptr));

	} else if (isset($ptr)) {
		/* multiple entries not allowed for this element, bail out */
		die(sprintf("XML error: %s at line %d cannot occur more than once\n",
				$name,
				xml_get_current_line_number($parser)));
	}

	$depth++;
	$havedata = $depth;
}

function endElement($parser, $name) {
	global $depth, $curpath, $config, $havedata, $listtags;

	if ($havedata == $depth) {
		$ptr =& $config;
		foreach ($curpath as $path) {
			$ptr =& $ptr[$path];
		}
		$ptr = "";
	}

	array_pop($curpath);

	if (in_array(strtolower($name), $listtags))
		array_pop($curpath);

	$depth--;
}

function cData($parser, $data) {
	global $depth, $curpath, $config, $havedata;

	$data = trim($data, "\t\n\r");

	if ($data != "") {
		$ptr =& $config;
		foreach ($curpath as $path) {
			$ptr =& $ptr[$path];
		}

		if (is_string($ptr)) {
			$ptr .= $data;
		} else {
			if (trim($data, " ") != "") {
				$ptr = $data;
				$havedata++;
			}
		}
	}
}

function parse_xml_config($cffile, $rootobj) {
	global $depth, $curpath, $config, $havedata, $listtags;

	$config = array();
	$curpath = array();
	$depth = 0;
	$havedata = 0;
	$encoding = "UTF-8"; // PHP default

	// Create XML parser and initialize handler.
	$xml_parser = xml_parser_create();
	xml_set_element_handler($xml_parser, "startElement", "endElement");
	xml_set_character_data_handler($xml_parser, "cdata");

	// Read configuration file.
	$data = file_get_contents($cffile);
	if (FALSE === $data)
		die("Error: could not open XML input\n");

	// Detect and set output character encoding. Default output encoding is UTF-8
	if (preg_match('/<?xml.*encoding=[\'"](.*?)[\'"].*?>/m', $data, $m)) {
		$encoding = strtoupper($m[1]);
		xml_parser_set_option($xml_parser, XML_OPTION_TARGET_ENCODING, $encoding);
	}

	// Parse configuration.
	if (!xml_parse($xml_parser, $data)) {
		die(sprintf("XML error: %s at line %d\n",
			xml_error_string(xml_get_error_code($xml_parser)),
			xml_get_current_line_number($xml_parser)));
	}

	xml_parser_free($xml_parser);

	if (!$config[$rootobj]) {
		die("XML error: no $rootobj object found!\n");
	}

	// Remember encoding
	$config[$rootobj]['encoding'] = $encoding;

	return $config[$rootobj];
}

// Dump configuration.
// $arr - Array to be dumped
// $indent - Number of indent
// $in_charset - The input charset, e.g. ISO-8859-1
// $out_charset - The output charset, e.g. UTF-8
// A charset conversion may appear when switching from a ISO-8859-1 language
// (e.g. English) to a UTF-8 one (e.g. French). If this is not done
// we will get an parser error.
function dump_xml_config_sub($arr, $indent, $in_charset, $out_charset) {
	global $listtags;

	$xmlconfig = "";

	foreach ($arr as $ent => $val) {
		if (is_array($val)) {
			/* is it just a list of multiple values? */
			if (in_array(strtolower($ent), $listtags)) {
				foreach ($val as $cval) {
					if (is_array($cval)) {
						$xmlconfig .= str_repeat("\t", $indent);
						$xmlconfig .= "<$ent>\n";
						$xmlconfig .= dump_xml_config_sub($cval, $indent + 1, $in_charset, $out_charset);
						$xmlconfig .= str_repeat("\t", $indent);
						$xmlconfig .= "</$ent>\n";
					} else {
						$xmlconfig .= str_repeat("\t", $indent);
						if ((is_bool($cval) && ($cval == true)) ||
							($cval === ""))
							$xmlconfig .= "<$ent/>\n";
						else if (!is_bool($cval))
							$xmlconfig .= "<$ent>" . htmlspecialchars(iconv($in_charset, $out_charset, $cval)) . "</$ent>\n";
					}
				}
			} else {
				/* it's an array */
				$xmlconfig .= str_repeat("\t", $indent);
				$xmlconfig .= "<$ent>\n";
				$xmlconfig .= dump_xml_config_sub($val, $indent + 1, $in_charset, $out_charset);
				$xmlconfig .= str_repeat("\t", $indent);
				$xmlconfig .= "</$ent>\n";
			}
		} else {
			if ((is_bool($val) && ($val == true)) || ($val === "")) {
				$xmlconfig .= str_repeat("\t", $indent);
				$xmlconfig .= "<$ent/>\n";
			} else if (!is_bool($val)) {
				$xmlconfig .= str_repeat("\t", $indent);
				$xmlconfig .= "<$ent>" . htmlspecialchars(iconv($in_charset, $out_charset, $val)) . "</$ent>\n";
			}
		}
	}

	return $xmlconfig;
}

function dump_xml_config($arr, $rootobj, $encoding) {
	// Set in/out encoding
	$in_charset = $arr['encoding'];
	$out_charset = $encoding;

	// Do not dump temporary encoding attribute
	unset($arr['encoding']);

	$xmlconfig = "<?xml version=\"1.0\" encoding=\"{$encoding}\"?>\n";
	$xmlconfig .= "<$rootobj>\n";
	$xmlconfig .= dump_xml_config_sub($arr, 1, $in_charset, $out_charset);
	$xmlconfig .= "</$rootobj>\n";

	return $xmlconfig;
}

/**
 * Validate the configuration xml file.
 * @param[in] cffile Path to the file to validate.
 * @param[in] rootobj The name of the root object.
 * @return TRUE if valid, otherwise FALSE.
 */
function validate_xml_config($cffile, $rootobj) {
	// Parse the configuration
	$xmlconfig = parse_xml_config($cffile, $rootobj);

	// Due the configuration file does not have a DTD, we can only do some
	// simple checks, e.g. if version is set.
	if (!isset($xmlconfig['version']))
		return FALSE;

	// Add more checks, or implement a DTD...
	return TRUE;
}
?>
