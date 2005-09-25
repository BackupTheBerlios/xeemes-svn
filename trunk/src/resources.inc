<?php
/*
 * resources.inc - provides access to the resources of the site
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

require_once('transformation.inc');
require_once('wikipedia.inc');

define('XEEMES_RESOURCE_UNKNOWN',    1);
define('XEEMES_RESOURCE_XML',        2);
define('XEEMES_RESOURCE_IMAGE',      4);
define('XEEMES_RESOURCE_HTML',       8);
define('XEEMES_RESOURCE_DATA',      16);
define('XEEMES_RESOURCE_WIKIPEDIA', 32);

function get_resource($location, $args, $relation_location = '') {
  if (substr($location, 0, 1) != '/') {
    $info = pathinfo($relation_location);
    if ($info['dirname'] == '/') $info['dirname'] = '';
    $location = $info['dirname'].'/'.$location;
  }
  $info = pathinfo($location);
  $ext = strtolower($info['extension']);
  if (in_array($ext, ImageResource::extensions()))
    return new ImageResource($location, $args);
  else if (in_array($ext, DataResource::extensions()))
    return new DataResource($location, $args);
  else if (in_array($ext, WikipediaResource::extensions()))
    return new WikipediaResource($location, $args);
  else if (in_array($ext, HTMLResource::extensions()))
    return new HTMLResource($location, $args);
  else if (in_array($ext, XMLResource::extensions()))
    return new XMLResource($location, $args);
  else
    return new Resource($location, $args);
}

class Resource {
  /* location */
  /* meta data */
  /* content */
  var $location;
  var $meta_data;

  var $filename;
  var $meta_filename;

  function Resource ($location, $args) {
    $this->location = $location;
    $this->meta_data = null;

    $this->filename = XEEMES_BASE_DIR.'/data'.$location; // TODO illegal chars
    $this->meta_filename = $this->filename.'.meta';
  }

  function type() {
    return XEEMES_RESOURCE_UNKNOWN;
  }

  function url() {
    return XEEMES_BASE_URL.'data'.$this->location; // TODO illegal chars, encoding
  }

  function view_url() {
    return XEEMES_BASE_URL.'view.php'.$this->location;
  }

  function extensions() {
    return array();
  }

  function get_meta($name) {
    $this->maybe_load_meta();
    return $this->meta_data[$name];
  }

  function set_meta($name, $value) {
    $this->save_meta();
  }

  function maybe_load_meta() {
    if ($meta_data == null)
      if (file_exists($this->meta_filename))
	$meta_data = load_meta_data($this->meta_filename);
      else
	$this->meta_data = array();
  }

  function save_meta() {
    save_meta_data($this->meta_filename, $this->meta_data);
  }

  function exists() {
    return file_exists($this->filename);
  }

  function stringContent() {
    return file_get_contents($this->filename);
  }

  function original() {
    return true;
  }

  function get_location() {
    return $this->location;
  }

}

// TODO: maybe move inline-code into classes

class XMLResource extends Resource {
  /* DOM document */
  /* apply transformation */
  /* apply layout */
  /* convert xeemes tags */
  /* display inline (prevent mutual inclusions) */

  function type() {
    return XEEMES_RESOURCE_XML | parent::type();
  }

  function extensions() {
    return array('xml');
  }

  function domContent() {
    return DOMDocument::load($this->filename);
  }

  function xmlContent() {
    $doc = $this->domContent();
    if ($this->get_meta('layout')) {
      $layout = new Layout($this->get_meta('layout')); // class:name?
      $doc = $layout->process($doc, array()); // TODO parameters
    }
    $this->convert_special_tags($doc); // TODO prevent mutual incs
    // TODO transformations
    return $doc->saveXML();
  }

  function convert_special_tags($doc) {
    include_resources($doc, $doc->documentElement, array($this->location));
    replace_special_tags($doc, $doc->documentElement);
  }
}

class HTMLResource extends XMLResource {
  /* convert HTML -> DOM document */
  function extensions() {
    return array('htm', 'html');
  }

  function type() {
    return XEEMES_RESOURCE_HTML | parent::type();
  }
}

class WikipediaResource extends XMLResource {
  /* convert Wikitext -> DOM document */
  function extensions() {
    return array('', 'wiki');
  }

  function type() {
    return XEEMES_RESOURCE_WIKIPEDIA | parent::type();
  }

  function domContent() {
    $parser = new WikipediaParser();
    return $parser->parse($this->stringContent());
  }
}

class DataResource extends Resource {
  /* display inline with <object> tag */
  function extensions() {
    return array('mpg', 'mpeg', 'avi', 'wmv');
  }

  function type() {
    return XEEMES_RESOURCE_DATA | parent::type();
  }
}

class ImageResource extends Resource {
  /* display inline with <img> tag */
  /* conversions with GD library */
  function extensions() {
    return array('jpg', 'jpeg', 'png', 'gif');
  }

  function type() {
    return XEEMES_RESOURCE_IMAGE | parent::type();
  }
}

function include_resources($doc, $node, $already_included_locations) {
  if ($node->nodeType == XML_ELEMENT_NODE) {
    if ($node->nodeName == 'xeemes:resource') {
      $location = $node->attributes->getNamedItem('location')->nodeValue;
      $name = $location;
      // TODO: args: location, thumbnail, size, frame
      $args = array();

      $display_n = $node->attributes->getNamedItem('display');
      $inline = $display_n && $display_n->nodeValue;
      $resource = get_resource($location, $args,
			       $already_included_locations[0]);
      if (in_array($location, $already_included_locations)) {
	replace_node($node,
		     $doc->createTextNode('[Error: recursive '.
					  'inclusion of '.$location.']'));
	return;
      } else {
	$location = $resource->get_location();
	array_unshift($already_included_locations, $location);
      }
      $type = $resource->type();
      if ($inline) {
	if (($type & XEEMES_RESOURCE_XML) > 0) {
	  if ($resource->exists()) {
	    $incl_doc = $resource->domContent();
	    $incl_node = $doc->importNode($incl_doc->documentElement, true);
	    $bodies = $incl_node->getElementsByTagName('body');
	    replace_node($node, nodelist2array($bodies->item(0)->childNodes));
	  } else {
	    replace_node($node, $doc->createTextNode('['.$location.
						     ' Not found]'));
	  }
	} else if (($type & XEEMES_RESOURCE_IMAGE) > 0) {
	  $img = $doc->createElement('img');
	  // TODO args
	  $img->setAttribute('src', XEEMES_BASE_URL.'view.php'.$location);
	  replace_node($node, $img);
	} else if (($type & XEEMES_RESOURCE_DATA) > 0) {
	  $obj = $doc->createElement('object');
	  $obj->setAttribute('data', XEEMES_BASE_URL.'data/'.$location);
	  // TODO type, args
	}
      } else {
	$a = $doc->createElement('a');
	if ($node->hasChildNodes()) {
	  foreach (nodelist2array($node->childNodes) as $child)
	    $a->appendChild($a, $doc->importNode($child, true));
	} else
	  $a->appendChild($doc->createTextNode($name));
	$a->setAttribute('href', $resource->view_url()); // TODO args
	replace_node($node, $a);
      }
    }
    $child = $node->firstChild;
    while ($child) {
      $next = $child->nextSibling;
      include_resources($doc, $child, $already_included_locations);
      $child = $next;
    }
  }
}

function replace_node($oldnode, $newnodes) {
  if (!is_array($newnodes))
    $newnodes = array($newnodes);
  foreach ($newnodes as $child) {
    $oldnode->parentNode->insertBefore($child, $oldnode);
  }
  $oldnode->parentNode->removeChild($oldnode);
}

function replace_special_tags($doc, $node) {
}

?>
