<?php
/*
 * utils.inc - various independent utilities
 * Copyright (c) 2005 David Frese
 */

/*
 * This file is part of Xeemes.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

// SCRIPT_DISTANCE specifies the distance to the root directory of the
// current script.
function define_globals($script_distance) {
  $info1 = pathinfo($_SERVER['SCRIPT_NAME']);
  $info2 = pathinfo($_SERVER['SCRIPT_FILENAME']);
  while ($script_distance > 0) {
    $info1 = pathinfo($info1['dirname']);
    $info2 = pathinfo($info2['dirname']);
    $script_distance--;
  }
  define('XEEMES_BASE_URL', $info1['dirname'].'/');
  define('XEEMES_BASE_DIR', $info2['dirname']);
}

function canonic_path($relative_path) {
  // Remove any ./
  $relative_path = str_replace('/./', '/', $relative_path);

  /*
  // Remove trailing /
  if ($relative_path[strlen($relative_path)-1] == '/')
    $relative_path = substr($relative_path, 0, -1);
  */

  $realpath = '';
  $q = 0;
  $p = strpos($relative_path, '/..', $q);
  while ($p !== false) {
    // Get the next part of the path
    if ($p != $q)
      $realpath .= substr($relative_path, $q, $p);

    // Find the edge of the previous directory
    $i = strrpos($realpath, '/');
    if ($i === false)
      return false;  // Not enough directories to go up any further.

    // Remove the directory
    $realpath = substr($realpath, 0, $i);
    // Take out the "/.."
    $relative_path = substr($relative_path, 0, $p) .
      substr($relative_path, $p+3);

    // Find the next "/.."
    $q = $p;
    $p = strpos($relative_path, '/..', $q);
  }
  // Get the rest of the relative path.
  $realpath .= substr($relative_path, $q);
  return $realpath;
}

function strpos_nested($text, $end_str, $start_str, $offset = 0) {
  do {
    $end_pos = strpos($text, $end_str, $offset);
    $start_pos = strpos($text, $start_str, $offset);
    if ($start_pos === false)
      return $end_pos;
    else if ($start_pos < $end_pos) {
      $rec_pos = strpos_nested($text, $end_str, $start_str,
			       $start_pos+strlen($start_str));
      if ($rec_pos === false)
	return false;
      else
	$offset = $rec_pos+strlen($end_str);
    } else
      return $end_pos;
  } while ($offset < strlen($text));
  return false;
}

function nodelist2array($nodelist) {
  $result = array();
  for ($i = 0; $i < $nodelist->length; $i++)
    $result []= $nodelist->item($i);
  return $result;
}

function replace_node($oldnode, $newnodes) {
  if (!is_array($newnodes))
    $newnodes = array($newnodes);
  foreach ($newnodes as $child) {
    $oldnode->parentNode->insertBefore($child, $oldnode);
  }
  $oldnode->parentNode->removeChild($oldnode);
}

function replace_process_instructions($doc, $defs, $node = false) {
  if ($node === false) $node = $doc->documentElement;
  if ($node->nodeType == XML_PI_NODE) {
    if (array_key_exists($node->target, $defs)) {
      $fun = $defs[$node->target];
      $node->parentNode->replaceChild($fun($doc, $node->data), $node);
    } else {
      // error, ignore, keep?
    }
  } else if ($node->nodeType == XML_ELEMENT_NODE) {
    foreach (nodelist2array($node->childNodes) as $child)
      replace_process_instructions($doc, $defs, $child);
  }
}

function replace_php_instructions($doc) {
  replace_process_instructions($doc, array('php' => 'eval_php'));
}

function eval_php($doc, $program) {
  ob_start();
  eval($program);
  $text = ob_get_contents();
  ob_end_clean();

  return import_html_text($doc, $text);
}

function import_html_text($doc, $text) {
  $xml_text = '<?xml version="1.0" encoding="UTF-8"?>'."\n".
    '<span>'.$text.'</span>';
  $xml = @DOMDocument::loadXML($xml_text);
  if (!$xml) {
    $span = $doc->createElement('span');
    $span->appendChild($doc->createComment('Failed to load text as XML: '));
    $span->appendChild($doc->createTextNode($text));
    return $span;
  } else
    return $doc->importNode($xml->documentElement, true);
}

?>
