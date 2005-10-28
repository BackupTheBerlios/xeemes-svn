<?php
/*
 * view.php - display a resource to a browser
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

require_once('src/utils.inc');
define_globals(0);

require_once('src/resources.inc');
require_once('src/authenticate.inc');

function view($location, $args) {
  $resource = get_resource($location, $args);

  verify_view_authorization($resource);

  global $xeemes_current_resource;
  $xeemes_current_resource = $resource;
  if (($resource->type() & XEEMES_RESOURCE_XML) > 0) {
    if (!$resource->exists()) {
      header('HTTP/1.1 404 Not found');
      print('<html><body>404 - not found</body></html>'); // TODO
    } else {
      header('Content-Type: text/html; charset=utf-8');
      print($resource->xmlContent());
    }
  } else {
    if (!$resource->exists())
      header('HTTP/1.1 404 Not found');
    else {
      // header Content-type ??
      $resource->outputContent();
    }
  }
}

view($_SERVER['PATH_INFO'], $_GET);

?>
