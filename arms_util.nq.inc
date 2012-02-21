<?php

// Null Query utility -- see http://drupal.org/node/245335
//
// Note: The author in the thread doesn't explicitly state a license, but
// the work is a straight-up modification of core Drupal code, and it's hard
// to imagine a legal theory in which he could have distributed that code
// without licensing under GPL.

define('ARMS_UTIL_NQ_REGEXP', '/(%D|%S|%F|%N|%d|%s|%%|%f|%b|%n)/');

function arms_util_nq() {
  $args = func_get_args();
  return _db_query(call_user_func_array('arms_util_nq_prepare', $args));
}

function arms_util_nq_prepare($query) {
  $args = func_get_args();
  array_shift($args);
  $query = db_prefix_tables($query);
  if (isset($args[0]) and is_array($args[0])) { // 'All arguments in one array' syntax
    $args = $args[0];
  }
  _arms_util_nq_callback($args, TRUE);
  $query = preg_replace_callback(ARMS_UTIL_NQ_REGEXP, '_arms_util_nq_callback', $query);
  return $query;
}

function _arms_util_nq_callback($match, $init = FALSE) {
  static $args = NULL;
  if ($init) {
    $args = $match;
    return;
  }
  switch ($match[1]) {
    case '%d': // We must use type casting to int to convert FALSE/NULL/(TRUE?)
      return (int) array_shift($args); // We don't need db_escape_string as numbers are db-safe
    case '%s':
      return db_escape_string(array_shift($args));
    case '%n':
      // Numeric values have arbitrary precision, so can't be treated as float.
      // is_numeric() allows hex values (0xFF), but they are not valid.
      $value = trim(array_shift($args));
      return is_numeric($value) && !preg_match('/x/i', $value) ? $value : '0';
    case '%%':
      return '%';
    case '%f':
      return (float) array_shift($args);
    case '%b': // binary data
      return db_encode_blob(array_shift($args));
  }
  $d=array_shift($args);

  if ($d === NULL) {
    return 'NULL';
  }
  switch ($match[1]) {
    case '%D':
      return (int)$d;
    case '%S':
      return "'".db_escape_string($d)."'";
    case '%N':
      $value = trim(array_shift($d));
      return is_numeric($value) && !preg_match('/x/i', $value) ? $value : '0';
    case '%F':
  }
}