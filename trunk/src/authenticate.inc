<?php
/*
 * authenticate.inc - control access to resources and scripts
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

function verify_view_authorization($resource) {
  $edir = dirname($resource->getLocation());
  if ($edir == '/') $edir = '';
  while (!file_exists(XEEMES_BASE_DIR.'/data'.$edir.'/.htaccess')) {
    if ($edir == '')
      return; // no htaccess file found
    else
      $edir = dirname($edir);
  }
  $htaccess = new htaccess(XEEMES_BASE_DIR.'/data'.$edir.'/.htaccess');
  $htaccess->load();

  // see if authorization is required
  $require = $htaccess->getRequire();
  if ($require == null)
    return;

  // check passed headers
  if (isset($_SERVER['PHP_AUTH_USER'])) {
    $htpasswd = $htaccess->getAuthUserFile();
    $htpasswd->load();
    $htgroups = $htaccess->getAuthGroupFile();

    /*$str = base64_decode($_SERVER['HTTP_AUTHORIZATION']);
    $pos = strpos($str, ':');
    $user = substr($str, 0, $pos);
    $pw = substr($str, $pos+1);*/
    $user = $_SERVER['PHP_AUTH_USER'];
    $pw = $_SERVER['PHP_AUTH_PW'];

    if (($htpasswd->validUser($user, $pw)) &&
	( ($require[0] == HTACCESS_REQUIRE_VALID_USER) ||
	  (($require[0] == HTACCESS_REQUIRE_USER) &&
	   in_array($user, $require[1])) ||
	  ((($htgroups->load() || true) &&
	    ($require[0] == HTACCESS_REQUIRE_GROUP) &&
	    in_array($user, $htgroups->getGroup($require[1]))) )))
      // OK
      return;
  }
  
  // send need for authorization requirement
  if ($htaccess->getAuthType() == HTACCESS_AUTHTYPE_BASIC) {
    header('HTTP/1.1 401 Unauthorized');
    $realm = $htaccess->getAuthName(); // TODO: quote
    header('WWW-Authenticate: Basic realm="'.$realm.'"');
    exit();
  } else {
    // TODO: internal error: type not supported
    return;
  }
}

function read_lines($fh) {
  $lines = array();
  while (!feof($fh)) {
    $str = trim(fgets($fh));
    if ($str != '') $lines []= $str;
  }
  return $lines;
}

function write_lines($fh, $lines) {
  foreach ($lines as $line)
    fputs($fh, $line."\n");
}

define('HTACCESS_AUTHTYPE_BASIC', 1);

define('HTACCESS_REQUIRE_VALID_USER', 1);
define('HTACCESS_REQUIRE_USER', 2);
define('HTACCESS_REQUIRE_GROUP', 3);

class htaccess {
  var $filename;
  var $lines;

  function htaccess ($filename) {
    $this->filename = $filename;
  }

  function load() {
    $fh = fopen($this->filename, 'r');
    $this->lines = read_lines($fh);
    fclose($fh);
  }

  function save() {
    $fh = fopen($this->filename, 'w'); // TODO chmod if create
    write_lines($fh, $this->lines);
    fclose($fh);
  }

  function getThing($regexp) {
    foreach ($this->lines as $line) {
      if (preg_match($regexp, $line, $matches))
	return $matches;
    }
    return null;
  }

  function setThing($regexp, $value) {
    for ($i = 0; $i < count($this->lines); $i++) {
      if (preg_match($regexp, $this->lines[$i])) {
	$this->lines[$i] = $value;
	return;
      }
    }
    $this->lines []= $value;
  }

  function getAuthType() {
    $matches = $this->getThing('/^authtype\s(.*)/i');
    if ($matches) {
      if (strcasecmp($matches[1], 'Basic') == 0)
	return HTACCESS_AUTHTYPE_BASIC;
      else
	return $matches[1];
    } else
      return null;
  }

  function setAuthType($value) {
    if ($value == HTACCESS_AUTHTYPE_BASIC)
      $value = 'Basic';
    $this->setThing('/^authtype\s(.*)/i', 'AuthType '.$value);
  }

  function getAuthName() {
    $matches = $this->getThing('/^authname\s(.*)/i');
    if ($matches)
      return $matches[1];
    else
      return null;
  }

  function setAuthName($value) {
    $this->setThing('/^authname\s(.*)/i', 'AuthName '.$value);
  }

  function getAuthUserFile() {
    $matches = $this->getThing('/^authuserfile\s(.*)/i');
    if ($matches) {
      $file = $matches[1];
      if ($file[0] != '/')
	$file = dirname($this->filename).'/'.$file;
      return new htpasswd($file);
    } else
      return null;
  }

  function setAuthUserFile($value) {
    // TODO: preserve relative/absolute paths
    $fname = $value->filename;
    $this->setThing('/^authuserfile\s(.*)/i', $fname);
  }

  function getAuthGroupFile() {
    $matches = $this->getThing('/^authgroupfile\s(.*)/i');
    if ($matches) {
      $file = $matches[1];
      if ($file[0] != '/')
	$file = dirname($this->filename).'/'.$file;
      return new htpasswd($file);
    } else
      return null;
  }

  function setAuthGroupFile($value) {
    $fname = $value->filename;
    $this->setThing('/^authgroupfile\s(.*)/i', $fname);
  }

  function getRequire() {
    $matches = $this->getThing('/^require\s([\S]*)\s?(.*)/i');
    if ($matches) {
      $typen = $matches[1];
      $more = preg_split('/[\s]+/', $matches[2]);
      if (strcasecmp($typen, 'valid-user') == 0)
	return array(HTACCESS_REQUIRE_VALID_USER, null);
      else if (strcasecmp($typen, 'user') == 0)
	return array(HTACCESS_REQUIRE_USER, $more);
      else if (strcasecmp($typen, 'group') == 0)
	return array(HTACCESS_REQUIRE_GROUP, $more);
      else
	return array($typen, $matches[2]);
    } else
      return null;
  }

  function setRequire($type, $names = array()) {
    if ($type == HTACCESS_REQUIRE_VALID_USER)
      $value = 'require valid-user';
    else if ($type == HTACCESS_REQUIRE_USER)
      $value = 'require user '.implode(' ', $names);
    else if ($type == HTACCESS_REQUIRE_GROUP)
      $value = 'require group '.implode(' ', $names);
    else
      $value = 'require '.$type.' '.$names; // TODO: array?
    $this->setThing('/^require\s([\S]*)\s?(.*)/i', $value);
  }
}

class htpasswd {
  var $filename;
  var $lines;

  function htpasswd($filename) {
    $this->filename = $filename;
  }

  function load() {
    $fh = fopen($this->filename, 'r');
    $this->lines = read_lines($fh);
    fclose($fh);
  }

  function save() {
    $fh = fopen($this->filename, 'w'); // TODO chmod if create
    write_lines($fh, $this->lines);
    fclose($fh);
  }

  function validUser($user, $password) {
    foreach ($this->lines as $line) {
      if (preg_match('/^'.preg_quote($user).':(.*)/', $line, $matches))
	return (crypt($password, substr($matches[1], 0, 2)) == $matches[1]);
    }
    return false;
  }

  function setUser($user, $password) {
    $str = $user.':'.crypt($password, 'ab'); // TODO: create salt randomly
    for ($i = 0; $i < count($this->lines); $i++) {
      $line = $this->lines[$i];
      if (preg_match('/^'.preg_quote($user).':.*/', $line)) {
	$this->lines[$i] = $str;
	return;
      }
    }
    $this->lines []= $str;
  }
}

class htgroupfile {
  var $filename;

  // TODO: is whitespace after ':' needed/checked/inserted?

  function htpasswd($filename) {
    $this->filename = $filename;
  }

  function load() {
    $fh = fopen($this->filename, 'r');
    $this->lines = read_lines($fh);
    fclose($fh);
  }

  function save() {
    $fh = fopen($this->filename, 'w'); // TODO chmod if create
    write_lines($fh, $this->lines);
    fclose($fh);
  }

  function getGroups() {
    $result = array();
    foreach ($this->lines as $line) {
      if (preg_match('/^([^:]+):.*/', $line, $matches))
	$result []= $matches[1];
      // TODO else syntax error/comment
    }
    return $result;
  }

  function getGroup($name) {
    foreach ($this->lines as $line) {
      if (preg_match('/^'.preg_quote($name).':(.*)/', $line, $matches)) {
	$users = preg_split('/[\s]+/', $matches[1]);
	return $users;
      }
    }
    return null;
  }

  function setGroup($name, $users) {
    $str = $name.':'.implode(' ', $users);
    for ($i = 0; $i < count($this->lines); $i++) {
      $line = $this->lines[$i];
      if (preg_match('/^'.preg_quote($name).':.*/', $line)) {
	$this->lines[$i] = $str;
	return;
      }
    }
    $this->lines []= $str;
  }
}

?>
