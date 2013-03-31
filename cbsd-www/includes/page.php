<?php

/* page.php */

// start tags
$tags = array(
 'PAGE_TITLE'			=> @$guru['product_name'],
 'PRODUCT_NAME'			=> @$guru['product_name'],
 'PRODUCT_VERSION_STRING'	=> @$guru['product_version_string'],
 'PRODUCT_URL'			=> @$guru['product_url'],
);


// functions

function page_handle($content)
// process page
{
 global $guru, $tags;

 // set visual theme tag
 $theme = (@$guru['preferences']['theme'])
  ? $guru['preferences']['theme'] : 'default';

 // push content in tags array
 if (is_array($content))
  foreach ($content as $newtag => $value)
   $tags[$newtag] = $value;
 else
  $tags['CONTENT'] = $content;
 // grab layout
 $layout = content_handle_path('theme/'.$theme.'/layout', 'layout', 'layout', 
  false, true);
 // process tags
 $processed = page_processtags($layout);
 // output page as processed string
 echo($processed);
}

function page_processtags($source)
// returns processed source input string
{
 // get all tags
 $preg_tag = '/[^\%]?\%\%([^\%]+)\%\%[^\%]?/m';
 preg_match_all($preg_tag, $source, $rawtags);
 $requestedtags = @$rawtags[1];
 unset($rawtags);

 // process table tags
 foreach ($requestedtags as $id => $tag)
  if (substr($tag, 0, strlen('TABLE_')) == 'TABLE_')
   if (substr($tag, -4) != '_END')
  {
   $startpos = strpos($source, $tag);
   $endpos = strpos($source, $tag);
   $preg_table = '/\%\%('.$tag.')\%\%(.*)\%\%('.$tag.'_END)\%\%/sm';
   $source = preg_replace_callback($preg_table, 'page_callback_tableprocessor', $source);
   unset($requestedtags[$id]);
   $endid = @array_search($tag.'_END', $requestedtags);
   if (@is_int($endid))
    unset($requestedtags[$endid]);
  }

 // get all tags again (the table tags stripped away this time)
 preg_match_all($preg_tag, $source, $rawtags);
 $requestedtags = @array_unique($rawtags[1]);
 unset($rawtags);

 // page debug if ?pagedebug is appended to URL
 if (@isset($_GET['pagedebug']))
  viewarray($requestedtags);
 // start output string by copying the original
 $processed = $source; 
 // resolve and substitute all the tags
 foreach ($requestedtags as $tag)
 {
  // call function to get the contents of the tag
  $resolved = page_resolvetag($tag, $source);
  // now replace the tag with the contents instead
  $processed = str_replace('%%'.$tag.'%%', $resolved, $processed);
 }
 return $processed;
}

function page_injecttag($newtags)
// injects given tag in tags array
{
 global $tags;
 if (is_array($newtags))
  foreach ($newtags as $tagname => $tagvalue)
   $tags[$tagname] = $tagvalue;
}

function page_resolvetag($tag, $source)
// returns string that matches the given page tag
{
 global $tags;
 if (isset($tags[$tag]))
  return $tags[$tag];
 else
  return '';
}

function page_callback_tableprocessor($data)
{
 global $tags;
 $tabletag = @(string)$data[1];
 $tablebody = @(string)$data[2];
 // get all table tags
 $preg_tag = '/[^\%]?\%\%([^\%]+)\%\%[^\%]?/m';
 preg_match_all($preg_tag, $tablebody, $tabletags_raw);
 $tabletags = @array_unique($tabletags_raw[1]);

 // begin assembling the $output string row by row
 $output = '';
 if (@is_array($tags[$tabletag]))
  foreach ($tags[$tabletag] as $id => $rowtags)
  {
   // inject rowtags into tags for page_processtags function
   foreach ($rowtags as $tag => $value)
    $tags[$tag] = $value;
   // append processed row to output string
   $output .= page_processtags($tablebody);
  }
 return $output;
}

function page_rawfile($filepath)
// outputs raw file
{
 readfile($filepath);
}

/* content handler */

function content_handle($cat, $pagename, $data = false, $skip_submit = false)
{
 $pagepath = 'pages/'.$cat.'/'.$pagename;
 $c = content_handle_path($pagepath, $cat, $pagename, $data, $skip_submit);
 return $c;
}

function content_handle_path($pagepath, $cat, $pagename, 
                             $data = false, $skip_submit = false)
// processes specific page into content string
// TODO: SECURITY
// TODO: translation page select
{
 global $guru, $tags;

 // set visual theme tag
 $theme = (@$guru['preferences']['theme'])
  ? $guru['preferences']['theme'] : 'default';
 $tags['THEME'] = $theme;

 // page name
// $pagename = basename($pagepath);

 // read page file
 $page = @file_get_contents($pagepath.'.page');

 // process content file
 $contentfile = $pagepath.'.php';
 $contenttags = array();
 if (@is_readable($contentfile))
 {
  // include contentpage (.php)
  include_once($contentfile);
 }

 // call submit function if applicable
 if (@isset($_POST['handle']))
  if (@function_exists('submit_'.$_POST['handle']))
   if (!$skip_submit)
    if ($data === false)
     $submittags = call_user_func('submit_'.$_POST['handle']);
    else
     $submittags = call_user_func('submit_'.$_POST['handle'], $data);
 // inject all content tags into main $tags array
 if (@is_array($submittags))
  foreach ($submittags as $newtag => $value)
   $tags[$newtag] = $value;

 // call page function if applicable
 $pagefunction = 'content_'.$cat.'_'.$pagename;
 if (@function_exists($pagefunction))
  if ($data)
   $contenttags = call_user_func($pagefunction, $data);
  else
   $contenttags = call_user_func($pagefunction);

 // inject all content tags into main $tags array
 if (@is_array($contenttags))
  foreach ($contenttags as $newtag => $value)
   $tags[$newtag] = $value;

 // process stylesheet if existent
 $stylepath = $pagepath.'.css';
 if (@is_readable($stylepath))
 {
  if ($stylepath{0} == '/')
  {
   // use inline CSS for absolute pathnames
   $css_code = @file_get_contents($stylepath);
   page_register_inlinestyle($css_code);
  }
  else
   page_register_stylesheet($stylepath);
 }

 // process tags
 $processed = page_processtags($page);
 // output processed string handled by page handler as %%CONTENT%% tag
 return $processed;
}


/* head and body handlers */

function page_register_headelement($element_raw)
{
 global $tags;
 $newtag = @$tags['HEAD'] . $element_raw . chr(10);
 $tags['HEAD'] = $newtag;
}

function page_register_bodyelement($element_raw)
{
 global $tags;
 $newtag = @$tags['BODY'] . ' ' . $element_raw;
 $tags['BODY'] = $newtag;
}

function page_register_stylesheet($relative_path)
// adds external stylesheet
{
 $str = '<link rel="stylesheet" tyle="text/css" href="'.$relative_path.'" />';
 page_register_headelement($str);
}

function page_register_inlinestyle($css_code)
// adds inline stylesheet (embedded into HTML document)
{
 $str = '<style type="text/css">'.chr(10);
 $str .= $css_code;
 $str .= chr(10).'</style>'.chr(10);
 page_register_headelement($str);
}

function page_feedback($feedback, $style = 'c_notice')
// register feedback message, style given changes visual appearance
{
 if (!@in_array($_SESSION['feedback'][$style], $feedback))
  $_SESSION['feedback'][$style][] = $feedback;
}

function page_refreshinterval($seconds)
// sets refresh interval which reloads current page
{
 if ((int)$seconds > 0)
 {
  $newelement = '<meta http-equiv="refresh" content="'.(int)$seconds.'" />';
  page_register_headelement($newelement);
 }
}

?>
