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
require_once('tags.inc');
require_once('cache.inc');
require_once('mime-types.inc');

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

  if ($info['basename'][0] == '.') {
    // dot-files
    $einfo = pathinfo(substr($info['basename'], 1));
    if (isset($einfo['extension']))
      $info['extension'] = $einfo['extension'];
    else
      unset($info['extension']);
  }

  if (isset($info['extension']))
    $info['name'] = substr($info['basename'], 0,
			   strlen($info['basename'])
			   - strlen($info['extension']) - 1);
  else
    $info['name'] = $info['basename'];

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

  $info = mypathinfo($location);
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

  function isUnmodified() {
    return true;
  }

  function stringContent() {
    ob_start();
    $this->outputContent();
    $result = ob_get_contents();
    ob_end_clean();
    return $result;
  }

  function outputContent() {
    readfile($this->filename);
  }

  function getContentType() {
    /* this all doesn't work that well
    // try to get mime type from 'file -b -i' (Linux only!)
    $res = @exec('file -b -i '.escapeshellarg($this->filename), $lines);
    if (($res == 0) && (count($lines) > 0))
      return $lines[0];
    else if (file_exists($this->filename))
      return mime_content_type($this->filename);
    else
      return 'application/octet-stream';
    */
    $type = mime_content_type_ex($this->filename);
    if ($type)
      return $type;
    else
      return 'application/octet-stream';
  }

  function getLocation() {
    return $this->location;
  }

  function getName() {
    // TODO: what about title tags in html-files?
    $name = $this->get_meta('display-name');
    if ($name)
      return $name;
    else {
      $info = mypathinfo($this->location);
      $name = $info['name'];
      if ($name != '')
	return $name;
      else
	return 'Homepage'; // TODO config
    }
  }

  function getParentResource() {
    return get_resource(dirname($this->location), array());
  }

  function getCacheInvalidator() {
    return new XeemesCacheInvalidator();
  }

  // return something if this resource doesn't exist
  function getNotFoundData() {
    // TODO: override for image resources
    return '<html><body><h1>Not Found</h1><p>The requested location '.$this->location.' was not found.</p><hr>xeemes</body></html>';
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
    return array('xml', 'xsl', 'xhtml');
  }

  function outputContent() {
    $doc = $this->domContent();

    //$this->process_instructions($doc); // TODO
    XeemesTag::replaceTags($this, $doc->documentElement);

    $layout_name = $this->get_meta('layout-name');
    if (!$layout_name) $layout_name = 'default';
    $layout_class = $this->get_meta('layout-class');
    if ($layout_name) {
      // class can be false
      $layout = new Layout($layout_class, $layout_name);
      $doc = $layout->process($doc, array('base-url' => XEEMES_BASE_URL));
    }

    // again after the layout transformation
    //$this->process_instructions($doc); // TODO
    XeemesTag::replaceTags($this, $doc->documentElement);
    
    print($doc->saveXML());
  }

  function isUnmodified() {
    return false;
  }

  function domContent() {
    $doc = DOMDocument::load($this->filename);

    global $allow_php;
    if ($allow_php)
      replace_php_instructions($doc);

    XeemesTag::replaceTags($this, $doc->documentElement);
    // TODO transformations

    return $doc;
  }

  function getContentType() {
    return 'text/html; charset=utf-8';
  }

  /* TODO: plus layout, everything that is included etc.
  function getCacheInvalidator() {
    return new XeemesCacheFileInvalidator($this->filename);
  }
  */
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
				file_get_contents($this->filename)));
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

abstract class ImageResource extends Resource {
  /* display inline with <img> tag */
  /* conversions with GD library */

  function type() {
    return XEEMES_RESOURCE_IMAGE | parent::type();
  }

  abstract function outputSameType($image);
  abstract function imageContent();

  function outputContent() {
    // TODO: size args
    if ($this->args['thumbnail'] == '1') {
      $image = $this->imageContent();
      $ow = imagesx($image); $oh = imagesy($image);
      $width = 100;
      $height = 100;

      if ($ow > $oh)
	$height = $height * $oh / $ow;
      else
	$width = $width * $ow / $oh;
      $thumb = imagecreatetruecolor($width, $height);
      imagefilledrectangle($thumb, 0, 0, $width, $height, 16777215);
      imagecopyresampled($thumb, $image, 0, 0, 0, 0, $width, $height,
			 $ow, $oh);

      $this->outputSameType($thumb);
      imagedestroy($thumb);
      imagedestroy($image);
    } else
      parent::outputContent();
  }

  function isUnmodified() {
    return (count($this->args) == 0); // more exact?
  }

  function getCacheInvalidator() {
    if ($this->isUnmodified())
      return new XeemesCacheInvalidator();
    else
      return new XeemesCacheFileInvalidator($this->filename);
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

  function getContentType() {
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

  function getContentType() {
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

  function getContentType() {
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
	  ($file[0] != '_') &&
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

?>
