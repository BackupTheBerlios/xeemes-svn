<?php
/*
 * wikipedia.inc - transform a wikipedia-style document to XML
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

define('XEEMES_NS_URI', 'http://xeemes.org/schema');

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

class WikipediaParser {
  var $document;

  function WikipediaParser () {
  }

  function parse($text) {
    $lines = preg_replace('/[\r]?\n/', '</xeemes:line><xeemes:line>', $text);
    $xml_text = '<html xmlns:xeemes="'.XEEMES_NS_URI.'"><body><xeemes:line>'.
      $lines.'</xeemes:line></body></html>';
    $this->document = DOMDocument::loadXML($xml_text);

    $this->process_lines();
    $this->process_inliners($this->document->documentElement);
    $this->post_processing();

    // header element inside
    $docelem = $this->document->documentElement;
    $headers = $docelem->getElementsByTagName('head');
    if ($headers->length > 0) {
      $head = $headers->item(0);
      $docelem->insertBefore($head->cloneNode(true), $docelem->firstChild);
      $head->parentNode->removeChild($head);
    }

    $result = $this->document;
    $this->document = null;
    return $result;
  }

  function process_lines() {
    // TODO: enumerate lines for error messages?
    $next_line = $this->document->documentElement->firstChild->firstChild;
    while ($next_line) {
      $next_line = $this->process_next_line_thing($next_line);
    }
  }

  function process_next_line_thing($line) {
    // single liners
    if (preg_match('/^====[ ]?.*====$/', $line->textContent))
      return $this->process_header($line, 4);
    else if (preg_match('/^===[ ]?.*===$/', $line->textContent))
      return $this->process_header($line, 3);
    else if (preg_match('/^==[ ]?.*==$/', $line->textContent))
      return $this->process_header($line, 2);
    else if (preg_match('/^=[ ]?.*=$/', $line->textContent))
      return $this->process_header($line, 1);
    else if ($line->textContent == '----') {
      $hr = $this->document->createElement('hr');
      $line->parentNode->replaceChild($hr, $line);
      return $hr->nextSibling;
    } else if ($line->textContent == '') {
      $pbr = $this->document->createElement('pbr');
      $line->parentNode->replaceChild($pbr, $line);
      return $pbr->nextSibling;
    } else {
      foreach (nodelist2array($line->childNodes) as $child)
	$line->parentNode->insertBefore($child, $line);
      $next = $line->nextSibling;
      $line->parentNode->removeChild($line);
      return $next;
    }
    // TODO: multi-liners: lists, indentation, definition-lists, tables
  }

  function process_header($line, $depth) {
    $line->firstChild->splitText($depth);
    $line->removeChild($line->firstChild);
    $line->lastChild->splitText(strlen($line->lastChild->textContent)-$depth);
    $line->removeChild($line->lastChild);
    $header = $this->document->createElement('h'.$depth);
    foreach (nodelist2array($line->childNodes) as $child)
      $header->appendChild($child->cloneNode(true));
    $line->parentNode->replaceChild($header, $line);
    return $header->nextSibling;
  }

  function replace_inliners($text, $start = 0, $end = -1) {
    $result = array();
    $current_text = '';
    $text_end = false;
    if ($end == -1) $end = strlen($text);
    while ($start < $end) {
      $text_end = true;
      $new = null;
      // symmetric inliners
      if (substr($text, $start, 5) == '\'\'\'\'\'') {
	$pos = strpos($text, '\'\'\'\'\'', $start+5);
	if ($pos === false) $this->parse_error();
	$strong = $this->document->createElement('strong');
	$em = $this->document->createElement('em');
	foreach ($this->replace_inliners($text, $start+5, $pos) as $node)
	  $em->appendChild($node);
	$strong->appendChild($em);
	$start = $pos+5;
	$new = $strong;
      } else if (substr($text, $start, 3) == '\'\'\'') {
	$pos = strpos($text, '\'\'\'', $start+3);
	if ($pos === false) $this->parse_error();
	$em = $this->document->createElement('em');
	foreach ($this->replace_inliners($text, $start+3, $pos) as $node)
	  $em->appendChild($node);
	$start = $pos+3;
	$new = $em;
      } else if (substr($text, $start, 2) == '\'\'') {
	$pos = strpos($text, '\'\'', $start+2);
	if ($pos === false) $this->parse_error();
	$strong = $this->document->createElement('strong');
	foreach ($this->replace_inliners($text, $start+2, $pos) as $node)
	  $strong->appendChild($node);
	$start = $pos+2;
	$new = $strong;
      } // asymmetric inliners
      else if (substr($text, $start, 2) == '[[') {
	$pos = strpos_nested($text, ']]', '[[', $start+2);
	if ($pos === false) $this->parse_error();
	$res = $this->document->createElementNS(XEEMES_NS_URI,
						'xeemes:resource1');
	foreach ($this->replace_inliners($text, $start+2, $pos) as $node)
	  $res->appendChild($node);
	$start = $pos+2;
	$new = $res;
      } else if (substr($text, $start, 1) == '[') {
	$pos = strpos_nested($text, ']', '[', $start+1);
	if ($pos === false) $this->parse_error();
	$link = $this->document->createElementNS(XEEMES_NS_URI,
						 'xeemes:link1');
	foreach ($this->replace_inliners($text, $start+1, $pos) as $node)
	  $link->appendChild($node);
	$start = $pos+1;
	$new = $link;
      } else {
	$text_end = false;
	$current_text .= $text[$start];
	$start += 1;
      }

      if ($text_end) {
	if ($current_text != '')
	  $result []= $this->document->createTextNode($current_text);
	$result []= $new;
	$current_text = '';
      }
    } // while
    if ($current_text != '')
      $result []= $this->document->createTextNode($current_text);
    return $result;
  }

  function process_inliners($node) {
    if ($node->nodeType == XML_TEXT_NODE) {
      $parts = $this->replace_inliners($node->textContent);
      $last = null;
      foreach ($parts as $child) {
	$node->parentNode->insertBefore($child, $node);
	$last = $child;
      }
      $node->parentNode->removeChild($node);
      return $last->nextSibling;
    } else if (($node->nodeType == XML_ELEMENT_NODE) &&
	       ($node->nodeName != 'nowiki')) {
      $child = $node->firstChild;
      while ($child) {
	$child = $this->process_inliners($child);
      }
    }
    return $node->nextSibling;
  }

  /** */
  function post_processing($node = null) {
    if ($node == null) $node = $this->document->documentElement;
    // TODO: pbr
    if ($node->nodeType == XML_ELEMENT_NODE) {
      //if ($node->namespaceURI == XEEMES_NS_URI) { //?? TODO
      if ($node->prefix == 'xeemes') {
	if ($node->localName == 'resource1') {
	  foreach (nodelist2array($node->childNodes) as $child)
	    $this->post_processing($child);
	  $this->process_resource1($node);
	} else if ($node->localName == 'link1') {
	  foreach (nodelist2array($node->childNodes) as $child)
	    $this->post_processing($child);
	  $this->process_link1($node);
	}
      } else {
	foreach (nodelist2array($node->childNodes) as $child)
	  $this->post_processing($child);
      }
    }
  }

  /** input: a xeemes:resource1 node, containing text nodes and other tags
   *  is replaced by a xeemes:resource with appropriate attributes and content
   */
  function process_resource1($node) {
    $resource = $this->document->createElementNS(XEEMES_NS_URI,
						 'xeemes:resource');
    $first = true;
    foreach (nodelist2array($node->childNodes) as $child) {
      if ($child->nodeType == XML_TEXT_NODE) {
	$text_parts = explode('|', $child->textContent);
	foreach ($text_parts as $text) {
	  if ($first) { // the first text part is the location
	    $first = false;
	    if (($text != '') && ($text[0] == ':')) {
	      $resource->setAttribute('display', 'inline');
	      $text = substr($text, 1);
	    }
	    $resource->setAttribute('location', $text);
	  } else if (($text == 'left') ||
		     ($text == 'right') ||
		     ($text == 'center')) {
	    // TODO is center valid XHTML?
	    $resource->setAttribute('align', $text);
	  } else if ($text == 'none') {
	    // ignore ??
	  } else if (($text == 'thumb') ||
		     ($text == 'thumbnail')) {
	    $resource->setAttribute('special', 'thumbnail');
	  } else if (preg_match('/([0-9]+px)/', $text, $matches)) {
	    $resource->setAttribute('size', $matches[1]);
	  } else if ($text == 'frame') {
	    $resource->setAttribute('special', 'frame');
	  } else {
	    // the alternate text
	    $resource->appendChild($this->document->createTextNode($text));
	  }
	}
      } else {
	$resource->appendChild($child->cloneNode(true));
      }
    }
    $node->parentNode->replaceChild($resource, $node);
    //return $resource;
  }

  function process_link1($node) {
    $result = $this->document->createElement('a');
    $result->setAttribute('class', 'external');
    if ($node->firstChild->nodeType == XML_TEXT_NODE) {
      $pos = strpos($node->firstChild->textContent, ' ');
      if (!($pos === false)) {
	$url = substr($node->firstChild->textContent, 0, $pos);
	$rest = substr($node->firstChild->textContent, $pos+1);
	$node->replaceChild($this->document->createTextNode($rest),
			    $node->firstChild);
	$result->setAttribute('href', $url);
      }
    }
    foreach (nodelist2array($node->childNodes) as $child) {
      $result->appendChild($child->cloneNode(true));
    }
    $node->parentNode->replaceChild($result, $node);
  }
}

?>
