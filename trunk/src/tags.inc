<?php
/*
 * tags.inc - what to do with the tags in the xeemes namespace
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

require_once('resources.inc');

function parse_url_arguments($spec) {
  $defs = explode('&', $spec);
  $result = array();
  foreach ($defs as $def) {
    $pos = strpos($def, '=');
    if ($pos === false)
      $result[$def] = $def;
    else
      $result[substr($def, 0, $pos)] = substr($def, $pos+1);
  }
  return $result;
}

function unparse_url_arguments($args) {
  $result = '';
  $first = true;
  foreach ($args as $name => $value) {
    if ($first) {
      $result .= $name.'='.$value;
      $first = false;
    } else
      $result .= '&'.$name.'='.$value;
  }
  return $result;
}


abstract class XeemesTag {
  /* returns a node, owned by the same document as $node */
  function fromNode($resource, $node) {
    $attrs = array();
    $content = array();
    $i = 0;
    while ($attrnode = $node->attributes->item($i)) {
      if ($attrnode->namespaceURI == XEEMES_NS_URI)
	$attrs[$attrnode->name] = $attrnode->value;
      else
	//$content []= $node->ownerDocument->importNode($attrnode, true);
	$content []= $attrnode;
      $i++;
    }
    $child = $node->firstChild;
    while ($child) {
      $content []= $child;
      //$content []= $node->ownerDocument->importNode($child, true);
      $child = $child->nextSibling;
    }
    return $this->toNode($resource, $node->ownerDocument, $attrs, $content);
  }

  /* return a single node */
  abstract function toNode($resource, $doc, $attrs, $content = array());

  static function error($doc, $message) {
    $span = $doc->createElement('span');
    $span->setAttribute('style', 'color: red;');
    $span->appendChild($doc->createTextNode('[Error: '.$message.']'));
    return $span;
  }

  private static $tags = array('resource' => 'XeemesResourceTag',
			       'xsl-transformation'
			       => 'XeemesXSLTransformationTag',
			       'gallery' => 'XeemesGalleryTag');

  static function replaceTags($resource, $node) {
    if ($node->nodeType == XML_ELEMENT_NODE) {
      if ($node->namespaceURI == XEEMES_NS_URI) {
	if (isset(self::$tags[$node->localName])) {
	  $class = self::$tags[$node->localName];
	  $obj = new $class();
	  $new = $obj->fromNode($resource, $node);
	  $node->parentNode->replaceChild($new, $node);
	} else {
	  $err = XeemesTag::error($node->ownerDocument, 'Unknown xeemes tag: '.
				  $node->localName);
	  $node->parentNode->replaceNode($err, $node);
	}
      } else {
	$child = $node->firstChild;
	while ($child) {
	  $next = $child->nextSibling;
	  self::replaceTags($resource, $child);
	  $child = $next;
	}
	//foreach (nodelist2array($node->childNodes) as $child)
      }
    }
  }
}

class XeemesResourceTag extends XeemesTag {
  public function toNode($resource, $doc, $attrs, $content = array()) {
    if (!isset($attrs['location']))
      return XeemesTag::error($doc, 'Location undefined in resource tag');
    $location = $attrs['location'];
    
    $inline = (isset($attrs['display']) && ($attrs['display'] == 'inline'))
      || false;
    if ($inline)
      return $this->toNodeInline($resource, $doc, $location, $attrs, $content);
    else
      return $this->toNodeLink($resource, $doc, $location, $attrs, $content);
  }

  private function toNodeInline($resource, $doc, $location, $attrs,
				$content) {
    // TODO: prevent recursive inclusions?
    $args = isset($attrs['arguments']) ?
      parse_url_arguments($attrs['arguments']) : array();
    $iresource = get_resource($location, $args, $resource->getLocation());
    $type = $iresource->type();
    if (($type & XEEMES_RESOURCE_XML) > 0) {
      if (!$iresource->exists())
	return XeemesTag::error($doc, 'Resource "'.$iresource->getLocation().
				'" not found');
      else {
	$idoc = $iresource->domContent();
	// TODO: extract body if existing
	$inode = $doc->importNode($idoc->documentElement, true);
	foreach ($content as $child)
	  $inode->appendChild($child);
	return $inode;
      }
    } else if (($type & XEEMES_RESOURCE_IMAGE) > 0) {
      $img = $doc->createElement('img');
      $url = XEEMES_BASE_URL.'view.php'.$iresource->getLocation();
      // TODO: size, frame, etc.
      // TODO: prepend or append args to not override user-defined args?
      if ($attrs['special'] == 'thumbnail')
	$args['thumbnail'] = '1';

      if (count($args) > 0)
	$url .= '?'.unparse_url_arguments($args);

      $img->setAttribute('src', $url);
      $img->setAttribute('alt', $iresource->getName());
      return $img;
    } else if (($type & XEEMES_RESOURCE_IMAGE) > 0) {
      $obj = $doc->createElement('object');
      $url = XEEMES_BASE_URL.'data/'.$iresource->getLocation();
      $obj->setAttribute('data', $url);
      // TODO type, args
      return $obj;
    } else
      return XeemesTag::error($doc, 'Don\'t know how to inline "'.$location.
			      '" ('.$type.')');
  }

  private function toNodeLink($resource, $doc, $location, $attrs,
			      $content) {
    $args = isset($attrs['arguments']) ?
      parse_url_arguments($attrs['arguments']) : array();
    $iresource = get_resource($location, $args, $resource->getLocation());
    $ilocation = $iresource->getLocation();

    if (self::onlyAttributes($content))
      $content []= $doc->createTextNode($iresource->getName());
    
    $link = $doc->createElement('a');
    $href = XEEMES_BASE_URL.'view.php'.$ilocation;
    if (count($args) > 0)
      $href .= '?'.unparse_url_arguments($args);
    $link->setAttribute('href', $href);
    $link->setAttribute('class', 'xeemes-internal');
    foreach ($content as $node)
      $link->appendChild($node);
    return $link;
  }

  private static function onlyAttributes($arr) {
    foreach ($arr as $node) {
      if ($node->nodeType != XML_ATTRIBUTE_NODE)
	return false;
    }
    return true;
  }
}

class XeemesXSLTransformationTag extends XeemesTag {
  public function toNode($resource, $doc, $attrs, $content = array()) {
    if (!isset($attrs['stylesheet']))
      return XeemesTag::error($doc, 'Stylesheet undefined in xsl-transformation tag');

    /* load the stylesheet */
    $sresource = get_resource($attrs['stylesheet'], array(),
			      $resource->getLocation());
    $stylesheet = new XSLTProcessor();
    $stylesheet->importStyleSheet($sresource->domContent());

    /* only one element allowed in content */
    $docelement = null;
    foreach ($content as $node) {
      if ($node->nodeType == XML_ELEMENT_NODE) {
	$docelement = $node;
	break;
      }
    }
    if ($docelement == null)
      return XeemesTag::error($doc, 'No content specified for xsl-transformation');

    /* content to be transformed */
    $doc2 = new DomDocument();
    $doc2->appendChild($doc2->importNode($docelement, true));

    /* transformation */
    $result_doc = $stylesheet->transformToDoc($doc2);
    
    /* import to main doc */
    return $doc->importNode($result_doc->documentElement, true);
  }
}

class XeemesGalleryTag extends XeemesTag {
  public function toNode($resource, $doc, $attrs, $content = array()) {
    /* collect image locations */
    // TODO: error checks
    // TODO: single resources
    if (!isset($attrs['location']))
      return XeemesTag::error($doc, 'Gallery location not defined');
    $slocation = $attrs['location'];

    $res = get_resource($slocation, array(), $resource->getLocation());
    if (!$res->exists())
      return XeemesTag::error('Gallery location not found: '.
			      $res->getLocation());
    $location = $res->getLocation();
  
    $locations = self::find_locations($location, true);

    if (isset($resource->args['image'])) { // TODO: maybe separate
					   // resource and request args?
      /* view single image */
      $index = $resource->args['image'];
      $rt = new XeemesResourceTag();
      $res = $rt->toNode($resource, $doc,
			 array('location' => $locations[$index],
			       'display' => 'inline'));
      // TODO: navigation
      $nexti = ($index + 1) % count($locations);
      $previ = (count($locations) + $index - 1) % count($locations);

      return $res;
    } else {
      /* overview page */
      $columns = max(isset($attrs['columns']) ? $attrs['columns'] : 6, 1);
      $per_page = max(isset($attrs['images-per-page']) ?
	$attrs['images-per-page'] : 30, 1);

      if (isset($resource->args['page']))
	$page = $resource->args['page'];
      else
	$page = 0;

      $num_pages = count($locations)/$per_page;

      $results = array();
      /* links to the pages */
      $rt = new XeemesResourceTag();
      $links = $doc->createElement('div');
      $links->setAttribute('class', 'xeemes-gallery-page-links');
      for ($i = 0; $i < $num_pages; $i++) {
	$lnk = $rt->toNode($resource, $doc,
			   array('location' => $resource->getLocation(),
				 'arguments' => 'page='.$i),
			   array($doc->createTextNode($i+1)));
	$links->appendChild($lnk);
      }
      $results []= $links;
      
      /* thumbnails of images on this page */
      $thumbnails = array();
      for ($i = $page * $per_page;
	   $i < min($per_page, count($locations) - ($page * $per_page));
	   $i++) {
	$img = $rt->toNode($resource, $doc,
			   array('location' => $locations[$i],
				 'display' => 'inline',
				 'special' => 'thumbnail'));
	$lnk = $rt->toNode($resource, $doc,
			   array('location' => $resource->getLocation(),
				 'arguments' => 'image='.$i),
			   array($img));
	$thumbnails []= $lnk;
      }

      /* create a table with all thumbnails */
      $row = 0;
      $table = $doc->createElement('table');
      $tr = null;
      foreach ($thumbnails as $thumb) {
	if ($row == 0)
	  $tr = $table->appendChild($doc->createElement('tr'));
	$td = $tr->appendChild($doc->createElement('td'));
	$td->appendChild($thumb);

	$row = ($row+1) % ($per_page / $columns);
      }
      $results []= $table;
      
      /* div with results */
      $div = $doc->createElement('div');
      $div->setAttribute('class', 'xeemes-gallery');
      foreach ($results as $res)
	$div->appendChild($res);
      return $div;
    }
  }

  private static function find_locations($location, $include_dirs) {
    global $filesystem_charset;
    $path = iconv('utf-8', $filesystem_charset, $location);
    $locations = array();
    $dir = opendir(XEEMES_BASE_DIR.'/data'.$path);
    while (($file = readdir($dir)) !== false) {
      $filename = $path.$file;
      $pathinfo = pathinfo($filename);
      if (($file[0] != '.') &&
	  ($pathinfo['extension'] != 'meta') &&
	  ($file[0] != '_') &&
	  ($file[strlen($file)-1] != '~')) {
	// TODO: correct encoding of filesystem???
	if (is_dir(XEEMES_BASE_DIR.'/data'.$filename)) {
	  $filename .= '/';
	  if (!$include_dirs)
	    continue;
	}
	$locations []= iconv($filesystem_charset, 'utf-8', $filename);
      }
    }
    closedir($dir);
    return $locations;
  }
}

?>
