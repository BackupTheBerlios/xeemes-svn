<?php

function mime_content_type_ex($filename) {
  $handle = @fopen(XEEMES_BASE_DIR."/src/mime.types", "r");
  //$filehandle = @fopen($filename, "r");
  $result = false;
  if ($handle) {
    while (!feof($handle)) {
      $buffer = trim(fgets($handle));
      /* a line is blank, a comment, or a rule line */
      if (($buffer != '') && ($buffer[0] != '#')) {
	if (preg_match('/([\S]+)\s+(.*)/', $buffer, $matches)) {
	  $type = $matches[1];
	  $rules = $matches[2];
	  if (mime_test_rules($filename, $filehandle, $rules)) {
	    $result = $type;
	    break;
	  }
	} // else syntax error
      }
    }
    fclose($handle);
  }
  //if ($filehandle) fclose($filehandle);
  return $result;
}

function mime_test_rules($filename, $fh, $rules_spec) {
  if (strpos($rules_spec, '(') !== false)
    die('Functions and grouping in mime-types not implemented');
  $rules = preg_split('/[\s,]+/', $rules_spec);
  $info = pathinfo($filename);
  foreach ($rules as $rule) {
    if (strcasecmp($rule, $info['extension']) == 0)
      return true;
  }
  return false;
}

?>
