<?php
/*
 * transformation.inc - wrappers for style and layout transformations
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

class Transformation {
  var $document;
  var $parameters;

  function Transformation ($name) {
    $filename = XEEMES_BASE_DIR.'/styles/'.$name;
    $this->document = DOMDocument::load($filename);
    $this->parameters = array();
  }

  function process($doc, $parameters) {
    $proc = new XSLTProcessor();
    $proc->registerPHPFunctions();
    $proc->importStyleSheet($this->document);
    foreach ($this->parameters as $name => $value)
      $proc->setParameter('', $name, $value);
    foreach ($parameters as $name => $value)
      $proc->setParameter('', $name, $value);
    //$proc->registerPHPFunctions(); (restrict-array) >=PHP-5.1.0
    return $proc->transformToDoc($doc->documentElement);
  }
}

class Layout extends Transformation {
  function Layout ($class, $name) {
    if ($class) {
      $logic_filename = XEEMES_BASE_DIR.'/layouts/'.$class.'.xsl';
      $layout_dir = 'layouts/'.$class.'/'.$name;
      $layout_filename = XEEMES_BASE_DIR.'/'.$layout_dir.'.xml';
      $this->document = DOMDocument::load($logic_filename);
      $this->parameters['layout-filename'] = $layout_filename;
      $this->parameters['layout-url'] = XEEMES_BASE_URL.$layout_dir.'/';
    } else {
      $layout_dir = 'layouts/'.$name;
      $layout_filename = XEEMES_BASE_DIR.'/'.$layout_dir.'.xsl';
      $this->document = DOMDocument::load($layout_filename);
      $this->parameters['layout-url'] = XEEMES_BASE_URL.$layout_dir.'/';
    }      
  }
}

/* utility functions, intended to be called from inside an xsl */

function resource_xml_representation($resource, $doc, $include_subs) {
  $node = $doc->createElement('resource');
  $node->setAttribute('location', $resource->getLocation());
  $node->setAttribute('name', $resource->getName());
  $is_directory = ($resource->type() & XEEMES_RESOURCE_DIRECTORY) > 0;
  $node->setAttribute('is-directory', $is_directory ? '1' : '0');
  if (($include_subs > 0) &&
      (($resource->type() & XEEMES_RESOURCE_DIRECTORY) > 0)) {
    $subs = $node->appendChild($doc->createElement('subresources'));
    foreach ($resource->getSubresources() as $sub)
      $subs->appendChild(resource_xml_representation($sub, $doc,
						     $include_subs-1));
  }
  return $node;
}

function xeemes_current_resource($include_subs = 0) {
  global $xeemes_current_resource;
  $doc = new DOMDocument('1.0', 'utf-8');
  $doc->appendChild(resource_xml_representation($xeemes_current_resource,
						$doc, $include_subs));
  return $doc;
}

function xeemes_parent_resource($include_subs = 0) {
  global $xeemes_current_resource;
  $doc = new DOMDocument('1.0', 'utf-8');
  $parent = $xeemes_current_resource->getParentResource();
  $doc->appendChild(resource_xml_representation($parent, $doc,
						$include_subs));
  return $doc;
}

function xeemes_current_resource_path($include_subs = 0) {
  global $xeemes_current_resource;
  $doc = new DOMDocument('1.0', 'utf-8');
  $root = $doc->createElement('resource-path');
  $root->setAttribute('order', 'ascending');
  $resources = array($xeemes_current_resource);
  $next = $xeemes_current_resource;
  while ($next->getLocation() != '/') {
    $next = $next->getParentResource();
    array_unshift($resources, $next);
  }
  foreach ($resources as $res)
    $root->appendChild(resource_xml_representation($res, $doc, $include_subs));
  $doc->appendChild($root);
  return $doc;
}

?>
