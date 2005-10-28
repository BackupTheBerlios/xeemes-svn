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
require_once('meta.inc');

define('XEEMES_RESOURCE_UNKNOWN',    1);
define('XEEMES_RESOURCE_XML',        2);
define('XEEMES_RESOURCE_IMAGE',      4);
define('XEEMES_RESOURCE_HTML',       8);
define('XEEMES_RESOURCE_DATA',      16);
define('XEEMES_RESOURCE_WIKIPEDIA', 32);
define('XEEMES_RESOURCE_DIRECTORY', 64);
define('XEEMES_RESOURCE_JPEG',      128);
define('XEEMES_RESOURCE_PNG',       256);
define('XEEMES_RESOURCE_GIF',       512);

$allow_php = true;
$directory_indices = array('Index', 'index', 'Index.xhtml', 'index.xhtml',
			   'Index.html', 'index.html', 'Index.php',
			   'index.php');
$filesystem_charset = 'iso-8859-1';

function mypathinfo($path) {
  $info = pathinfo($path);
  if ($path[strlen($path)-1] == '/')
    $info['pathname'] = $path;
  else
    $info['pathname'] = $info['dirname'].'/';
  return $info;
}

function get_resource($location, $args, $relation_location = '') {
  if (substr($location, 0, 1) != '/') {
    $info = mypathinfo($relation_location);
    if ($info['dirname'] == '/') $info['dirname'] = '';
    $location = $info['pathname'].$location;
  }
  $location = canonic_path($location); // TODO: can return false

  global $filesystem_charset;
  $filename = iconv($filesystem_charset, 'utf-8', $location);

  if (($location[strlen($location)-1] != '/') &&
      (file_exists(XEEMES_BASE_DIR.'/data'.$filename)) &&
      (is_dir(XEEMES_BASE_DIR.'/data'.$filename)))
    $location .= '/';

  if ($location[strlen($location)-1] == '/') {
    return new DirectoryResource($location, $args);
  }

  $info = pathinfo($location);
  $ext = strtolower($info['extension']);
  if (in_array($ext, JPEGResource::extensions()))
    return new JPEGResource($location, $args);
  else if (in_array($ext, PNGResource::extensions()))
    return new PNGResource($location, $args);
  else if (in_array($ext, GIFResource::extensions()))
    return new GIFResource($location, $args);
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
  var $location; // in utf-8 charset
  var $meta_data;

  var $filename;
  var $meta_filename;

  var $args;

  function Resource ($location, $args) {
    global $filesystem_charset;
    $this->location = $location;
    $this->meta_data = null;

    $this->filename = XEEMES_BASE_DIR.'/data'.
      iconv('utf-8', $filesystem_charset, $location); // TODO illegal chars
    $this->meta_filename = $this->filename.'.meta';
    $this->args = $args;
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
    if ($this->meta_data == null)
      if (file_exists($this->meta_filename))
	$this->meta_data = load_meta_data($this->meta_filename);
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

  function isOriginal() {
    return true;
  }

  function outputContent() {
    if ($this->isOriginal())
      // TODO: this is supposed to be slow; and not very suitable for
      // very large files, resp. low bandwidths
      readfile($this->filename);
    else
      print($this->stringContent());
  }

  function getLocation() {
    return $this->location;
  }

  function isDirectoryIndex() {
    global $directory_indices;
    return in_array(basename($this->location), $directory_indices);
  }

  function getName() {
    // TODO: what about title tags in html-files?
    $name = $this->get_meta('display-name');
    if ($name)
      return $name;
    else {
      $info = pathinfo($this->location);
      $name = $info['basename'];
      if ($this->isDirectoryIndex()) {
	$parent_location = $info['dirname'];
	if ($parent_location == '/')
	  return 'Homepage'; // TODO;
	else
	  return basename($parent_location);
      } else
	return $name;
    }
  }

  function getParentResource() {
    return get_resource(dirname($this->location), array());
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
    return array('xml', 'xsl');
  }

  function domContent() {
    return DOMDocument::load($this->filename);
  }

  function xmlContent() {
    $doc = $this->domContent();

    // The XSL-Transformation removes PIs, hence we have to do it here too.
    $this->process_instructions($doc);

    $layout_name = $this->get_meta('layout-name');
    if (!$layout_name) $layout_name = 'default';
    $layout_class = $this->get_meta('layout-class');
    if ($layout_name) {
      // class can be false
      $layout = new Layout($layout_class, $layout_name);
      $doc = $layout->process($doc, array()); // TODO parameters
    }
    $this->process_instructions($doc);
    $this->convert_special_tags($doc);
    // TODO transformations
    return $doc->saveXML();
  }

  function convert_special_tags($doc) {
    replace_special_tags($this, $doc->documentElement);
  }

  function process_instructions($doc, $node = false) {
    global $allow_php;
    if ($node === false) $node = $doc->documentElement;
    if ($node->nodeType == XML_PI_NODE) {
      if (($node->target == 'php') && $allow_php) {
	$text = eval_php($node->data);
	$nodes = import_html_text($doc, $text);
	replace_node($node, $nodes);
      }
    } else if ($node->nodeType == XML_ELEMENT_NODE) {
      foreach (nodelist2array($node->childNodes) as $child)
	$this->process_instructions($doc, $child);
    }
  }
}

class HTMLResource extends XMLResource {
  /* convert HTML -> DOM document */
  function extensions() {
    return array('htm', 'html', 'php');
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
    // TODO: look at charset meta-data
    return $parser->parse(iconv('iso-8859-1', 'utf-8',
				$this->stringContent()));
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

  function type() {
    return XEEMES_RESOURCE_IMAGE | parent::type();
  }

  function outputContent() {
    header('Content-Type: '.$this->contentType()); // this is abstract
    // TODO: size args
    if ($this->args['thumbnail'] == '1') {
      $image = $this->imageContent(); // abstract
      $ow = imagesx($image); $oh = imagesy($image);
      $width = 100;
      $height = 100;

      if ($ow > $oh)
	$height = $height * $oh / $ow;
      else
	$width = $width * $ow / $oh;
      $thumb = imagecreatetruecolor($width, $height);
      imagefilledrectangle($thumb, 0, 0, $width, $height, 16777215);
      imagecopyresampled($thumb, $image, 0, 0, 0, 0, $width, $height, $ow, $oh);

      $this->outputSameType($thumb); // abstract
      imagedestroy($thumb);
      imagedestroy($image);
    } else
      parent::outputContent();
  }
}

class JPEGResource extends ImageResource {
  function extensions() {
    return array('jpeg', 'jpg');
  }

  function type() {
    return XEEMES_RESOURCE_JPEG | parent::type();
  }

  function imageContent() {
    return imagecreatefromjpeg($this->filename);
  }

  function contentType() {
    return "image/jpeg";
  }

  function outputSameType($image) {
    imagejpeg($image);
  }
}

class PNGResource extends ImageResource {
  function extensions() {
    return array('png');
  }

  function type() {
    return XEEMES_RESOURCE_PNG | parent::type();
  }

  function imageContent() {
    return imagecreatefrompng($this->filename);
  }

  function contentType() {
    return "image/png";
  }

  function outputSameType($image) {
    imagepng($image);
  }
}

class GIFResource extends ImageResource {
  function extensions() {
    return array('gif');
  }

  function type() {
    return XEEMES_RESOURCE_GIF | parent::type();
  }

  function imageContent() {
    return imagecreatefromgif($this->filename);
  }

  function contentType() {
    return "image/gif";
  }

  function outputSameType($image) {
    imagegif($image);
  }
}

class DirectoryResource extends XMLResource {
  // a request for a directory
  var $index_resource;

  function DirectoryResource ($location, $args) {
    parent::XMLResource($location, $args);
    global $directory_indices;
    $this->index_resource = null;
    foreach ($directory_indices as $index) {
      $res = get_resource($location.$index, $args, $relation_location);
      if ($res->exists()) {
	$this->index_resource = $res;
	break;
      }
    }
    // no index file... TODO: create directory listing
  }

  function type() {
    return XEEMES_RESOURCE_DIRECTORY | parent::type();
  }

  function extensions() {
    return array();
  }

  function domContent() {
    if ($this->index_resource)
      return $this->index_resource->domContent();
    else
      return DOMDocument::loadXML('<?xml version="1.0" encoding="iso-8859-1"?>
<html><body>Automatic directory indexing not implemented</body></html>'); // TODO
  }

  function getSubresources() {
    global $directory_indices, $filesystem_charset;
    // TODO: override by meta-data
    $path = $this->getLocation();
    $locations = array();
    $dir = opendir(XEEMES_BASE_DIR.'/data'.$path);
    while (($file = readdir($dir)) !== false) {
      $filename = $path.$file;
      $pathinfo = pathinfo($filename);
      if (($file[0] != '.') &&
	  ($pathinfo['extension'] != 'meta') &&
	  (!in_array($pathinfo['basename'], $directory_indices)) &&
	  ($file[strlen($file)-1] != '~')) {
	// TODO: correct encoding of filesystem???
	if (is_dir(XEEMES_BASE_DIR.'/data'.$filename))
	  $filename .= '/';
	$locations []= iconv($filesystem_charset, 'utf-8', $filename);
      }
    }
    closedir($dir);
    $result = array();
    foreach ($locations as $location)
      $result []= get_resource($location, array());
    return $result;
  }
}

function include_resource($main_resource, $node,
			  $already_included_locations) {
  $doc = $node->ownerDocument;
  $result = array();

  $location = $node->attributes->getNamedItem('location')->nodeValue;
  // TODO: args: location, thumbnail, size, frame
  $args = array();
  
  $display_n = $node->attributes->getNamedItem('display');
  $inline = $display_n && $display_n->nodeValue;
  $resource = get_resource($location, $args,
			   $already_included_locations[0]);
  $name = $resource->getName();

  if (in_array($location, $already_included_locations)) {
    return $doc->createTextNode('[Error: recursive inclusion of '.
				$location.']');
  } else {
    $location = $resource->getLocation();
    array_unshift($already_included_locations, $location);
  }
  $type = $resource->type();
  if ($inline) {
    if (($type & XEEMES_RESOURCE_XML) > 0) {
      if ($resource->exists()) {
	$incl_doc = $resource->domContent();
	replace_special_tags($main_resource, $incl_doc->documentElement,
			     $already_included_locations);
	$bodies = $incl_doc->getElementsByTagName('body');
	$body = $bodies->item(0);
	// TODO: heads?
	$result = array();
	foreach (nodelist2array($body->childNodes) as $child)
	  $result []= $doc->importNode($child, true);
      } else {
	$err = $doc->createTextNode('['.$location.' not found]');
	$result = $err;
      }
    } else if (($type & XEEMES_RESOURCE_IMAGE) > 0) {
      $img = $doc->createElement('img');
      // TODO args
      $img->setAttribute('src', XEEMES_BASE_URL.'view.php'.$location);
      $result = $img;
    } else if (($type & XEEMES_RESOURCE_DATA) > 0) {
      $obj = $doc->createElement('object');
      $obj->setAttribute('data', XEEMES_BASE_URL.'data/'.$location);
      // TODO type, args
      $result = $obj;
    }
  } else {
    $link = $doc->createElement('a');
    if ($node->hasChildNodes()) {
      foreach (nodelist2array($node->childNodes) as $child)
	$link->appendChild($link, $doc->importNode($child, true));
    } else
      $link->appendChild($doc->createTextNode($name));
    $link->setAttribute('href', $resource->view_url()); // TODO args
    $result = $link;
  }

  return $result;
}

function replace_special_tags($resource, $nodes,
			      $already_included_locations = null) {
  if (!$already_included_locations)
    $already_included_locations = array($resource->location);

  if (!is_array($nodes))
    $nodes = array($nodes);

  foreach ($nodes as $node) {
    if ($node->nodeType == XML_ELEMENT_NODE) {
      if ($node->namespaceURI == XEEMES_NS_URI) {
	if ($node->localName == 'xsl-transformation') {
	  $new = make_xsl_transformation($resource, $node);
	  replace_node($node, $new);
	} else if ($node->localName == 'resource') {
	  $new = include_resource($resource, $node,
				  $already_included_locations);
	  replace_node($node, $new);
	} else {
	  $err = $node->ownerDocument->createTextNode('Unknown special tag: '.
						      $node->localName);
	  $node->parentNode->replaceChild($err, $node);
	}
      } else {
	foreach (nodelist2array($node->childNodes) as $child)
	  replace_special_tags($resource, $child, $already_included_locations);
      }
    }
  }
}

function make_xsl_transformation($resource, $node) {
  // TODO: Error checking
  // load stylesheet
  $doc = $node->ownerDocument;
  $stylesheet_res = get_resource($node->getAttribute('stylesheet'), array(),
				 $resource->getLocation());
  $stylesheet = new XSLTProcessor();
  $stylesheet->importStyleSheet($stylesheet_res->domContent());

  // find first element node child
  $docelement = null;
  foreach (nodelist2array($node->childNodes) as $child) {
    if ($child->nodeType == XML_ELEMENT_NODE) {
      $docelement = $child;
      break;
    }
  }
  $doc2 = new DomDocument();
  $doc2->appendChild($doc2->importNode($docelement, true));

  $result_doc = $stylesheet->transformToDoc($doc2);
  
  $result_nodes = array();
  foreach (nodelist2array($result_doc->childNodes) as $node) {
    $result_nodes []= $doc->importNode($node, true);
  }
  return $result_nodes;
  
  //return $doc->importNode($result_doc->documentElement, true);
  // TODO: recursive replace special tags?
}

?>
