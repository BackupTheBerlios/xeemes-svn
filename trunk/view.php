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
require_once('src/transformation.inc');
require_once('src/cache.inc');

function view($location, $args) {
  $resource = get_resource($location, $args);

  global $xeemes_current_resource;
  $xeemes_current_resource = $resource;

  if ($resource->exists()) {
    $auth_info = verify_view_authorization($resource);

    $cache = new XeemesResourceCache('cache/');
    header('Content-Type: '.$resource->getContentType());

    // Problem: authenticate doesn't work then, because the browser
    // thinks it's a different directory.
    if ((!$auth_info) && $resource->isUnmodified()) {
      header('Location: '.XEEMES_BASE_URL.'data'.$location);
    } else {
      if (!$cache->output(array($location, $args))) {
	$output = $resource->stringContent();
	$inv = $resource->getCacheInvalidator();
	$cache->insert(array($location, $args), $output, $inv);
	print($output);
      }
    }
  } else {
    header('HTTP/1.1 404 Not found');
    print($resource->getNotFoundData());
  }
}

view(isset($_SERVER['PATH_INFO']) ? $_SERVER['PATH_INFO'] : $_SERVER['ORIG_PATH_INFO'],
     $_GET);

?>
