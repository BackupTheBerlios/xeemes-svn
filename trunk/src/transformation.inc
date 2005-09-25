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
    $logic_filename = XEEMES_BASE_DIR.'/layouts/'.$class.'.xsl';
    $layout_dir = '/layouts/'.$class.'/'.$name;
    $layout_filename = XEEMES_BASE_DIR.$layout_dir.'.xml';
    $this->document = DOMDocument::load($logic_filename);
    $this->parameters['layout-filename'] = $layout_filename;
  }
}

/*
function make_layout_logic($filename) {
  // TODO quote filename
  $xsl = '<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:variable name="page" select="/"/>
<xsl:variable name="layout" select="document(\''.$filename.'\')"/>
<xsl:template match="/">
    <xsl:apply-templates select="$layout/html"/>
</xsl:template>
<xsl:template match="insert-head">
    <!-- Get the head values here... -->
    <xsl:apply-templates select="$page/html/head/*"/>
</xsl:template>
<xsl:template match="insert-body">
    <!-- Get the body here -->
    <xsl:apply-templates select="$page/html/body/*"/>
</xsl:template>
<!-- Identity transformation -->
<xsl:template match="@*|*">
    <xsl:copy>
        <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
</xsl:template>
</xsl:stylesheet>';
  return DOMDocument::loadXML($xsl);
}
*/

?>
