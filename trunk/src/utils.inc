<?php
/*
 * utils.inc - various independent utilities
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

// SCRIPT_DISTANCE specifies the distance to the root directory of the
// current script.
function define_globals($script_distance) {
  $info1 = pathinfo($_SERVER['SCRIPT_NAME']);
  $info2 = pathinfo($_SERVER['SCRIPT_FILENAME']);
  while ($script_distance > 0) {
    $info1 = pathinfo($info1['dirname']);
    $info2 = pathinfo($info2['dirname']);
    $script_distance--;
  }
  define('XEEMES_BASE_URL', $info1['dirname'].'/');
  define('XEEMES_BASE_DIR', $info2['dirname']);
}

?>
