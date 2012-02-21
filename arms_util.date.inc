<?php

/**
 * Determine the boundaries of the week which includes the given timestamp
 *
 * @param $time int Seconds since Unix epoch
 * @return array with keys 'start_time' (Mon 0:0:0) and 'end_time' (Sun 23:59:59) (ints)
 */
function arms_util_date_week($time) {
  $offset = date('Z', $time);
  $time = (floor(($time + $offset) / (24*60*60)) * 24 * 60 * 60) - $offset; // today 0:0:0
  $day_of_week = date('N', $time);

  $start = $time - ($day_of_week-1)*24*60*60;
  $end = $time + (24*60*60) - 1 + (7-$day_of_week)*24*60*60;

  return array(
    'start_time' => $start,
    'end_time' => $end,
  );
}

/**
 * Format a time so that it can be included in an SQL query
 * 
 * @param $time int, seconds since epoch
 * @return string, MySQL date expression
 */
function arms_util_date_format_sql($time) {
  return date('Y-m-d H:i:s', $time);
}

/**
 * Convert a CiviCRM form element of type 'date' to a PHP-style date
 *
 * @param $arr array with keys 'Y', 'M', and 'd'
 * @return int or NULL; seconds since epoch
 */
function arms_util_date_arrayToEpoch($arr) {
  if (empty($arr) || ($arr['M'] == 0 && $arr['d'] == 0 && $arr['Y'] == 0)) {
    return NULL;
  } else {
    $arr2 = $arr;
    if (!isset($arr2['A'])) {
      // leave hour alone
    } elseif ($arr2['h'] == 12 && $arr2['A'] == 'AM') {
      $arr2['h'] = 0;
    } elseif ($arr2['h'] == 12 && $arr2['A'] != 'AM') {
      $arr2['h'] = 12;
    } elseif ($arr2['A'] == 'AM') {
      // leave hour alone
    } elseif ($arr2['A'] != 'AM') {
      $arr2['h'] = $arr2['h'] + 12;
    }

    if (!isset($arr2['i'])) { $arr2['i'] = 0; }
    if (!isset($arr2['s'])) { $arr2['s'] = 0; }

    return mktime($arr2['h'], $arr2['i'], $arr2['s'], $arr['M'], $arr['d'], $arr['Y']);
  }
}

