<?php
/*
 * meta.inc - load and save arbitrary meta data
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

function load_meta_data($filename) {
  $doc = domxml_open_file($filename);
  $root = $doc->document_element();
  $result = get_meta_data_entry($root);
  return $result;
}

function get_meta_data_entry($node) {
  $entry = false;
  foreach ($node->child_nodes() as $child) {
    if ($child->node_type() == XML_ELEMENT_NODE) {
      if (!is_array($entry)) $entry = array();
      $entry[$child->node_name()] = get_meta_data_entry($child);
    }
  }
  if (!$entry)
    $entry = $node->get_content();
  return $entry;
}

function save_meta_data($filename, $data) {
  $doc = domxml_new_doc('1.0');
  $doc->append_child(make_meta_data($doc, 'meta-data', $data));
  $doc->dump_file($filename, false, true);
}

function make_meta_data($doc, $name, $entry, $id = false) {
  $node = $doc->create_element($name);
  if (is_array($entry)) {
    foreach ($entry as $subname => $subentry) {
      $this_id = $id;
      if ($id) $down_id = $id.'00'; else $down_id = false;
      $d = make_meta_data($doc, $subname, $subentry, $down_id);
      if ($id) {
	$d->append_child($doc->create_attribute('id', $id));
	$id = $id+1;
      }
      $node->append_child($d);
    }
  } else {
    $node->append_child($doc->create_text_node($entry));
  }
  return $node;
}

?>
