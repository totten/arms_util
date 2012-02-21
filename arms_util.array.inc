<?php

/**
 * Clone each item in an array
 *
 * @param array(object)
 * @return array(object)
 */
function arms_util_array_cloneAll($array) {
  $result = array();
  foreach ($array as $key => $value) {
    if (is_object($value)) {
      $result[$key] = clone $value;
    } else {
      $result[$key] = $value;
    }
  }
  return $result;
}

/**
 * Determine if an array is hollow. An array is hallow if it
 * contains no values or, if there are values, all values are
 * empty.
 *
 * A value is considered empty if it is NULL, '', or an
 * empty array. It is not empty if it is FALSE, '0' or 0.
 */
function arms_util_array_hollow(&$arr, $recursive = FALSE) {
  if (empty($arr)) {
    return TRUE;
  }
  foreach ($arr as $key => $value) {
    if ($value === NULL) continue;
    if ($value === '') continue;
    if (is_array($value)) {
      if (count($value) == 0) continue;
      if ($recursive && arms_util_array_hollow($value, $recursive)) continue;
    }
    return FALSE;
  }
  return TRUE;
}

/**
 * Build an array-tree which indexes the records in an array
 *
 * @param $keys array of string (properties by which to index)
 * @param $records array of records (assoc-arrays)
 * @return array; multi-dimensional, with one layer for each key
 */
function arms_util_array_index($keys, $records) {
  $final_key = array_pop($keys);
  
  $result = array();
  foreach ($records as $record) {
    $node = &$result;
    foreach ($keys as $key) {
      if (is_array($record)) {
        $keyvalue = $record[$key];
      } else {
        $keyvalue = $record->{$key};
      }
      if (!is_array($node[$keyvalue])) {
        $node[$keyvalue] = array();
      }
      $node = &$node[$keyvalue];
    }
    if (is_array($record)) {
      $node[ $record[$final_key] ] = $record;
    } else {
      $node[ $record->{$final_key} ] = $record;
    }
  }
  return $result;
}

/**
 * Create a derivative array in which all of the keys have been changed.
 *
 * Example:
 *
 * $keys = array('un', 'deux', 'trois);
 * $original = array('a', 'b', 'c');
 * $modified = _arms_import_array_rekey($keys, $original);
 * // Produces array('un' => 'a', 'deux' => 'b', 'trois' => 'c');
 */
function arms_util_array_rekey(&$keys, &$arr) {
  $result = array();
  foreach ($arr as $key => $value) {
    $result[$keys[$key]] = $value;
  }
  return $result;
}

function arms_util_array_rekey_func($func, &$arr) {
  $result = array();
  foreach ($arr as $key => $value) {
    $result[call_user_func($func, $key, &$value)] = $value;
  }
  return $result;
}

/**
 * Translate the keys of an array from one domain to another using a "Rosetta stone" array
 *
 * @param $rosetta an array of words; each word is an array which lists its variations in different languages
 * @param $old_lang the name of the language used in the inputted array
 * @param $new_lang the name of the language used in the outputted array
 * @param $arr An array in which keys match words from the old language
 * @return array in which keys match words from the new language
 */
function arms_util_array_rekey_rosetta(&$rosetta, $old_lang, $new_lang, &$arr) {
  $result = array();
  foreach ($rosetta as &$word) {
    if (array_key_exists($word[$old_lang], $arr)) {
      $result[$word[$new_lang]] = $arr[$word[$old_lang]];
    }
  }
  return $result;
}

/**
 * Iterate over a collection of objects/arrays and collect a given property.
 *
 * e.g.
 * <pre>
 * $contacts = array(
 *   '12' => array('firstname' => 'John', 'lastname' => 'Doe'),
 *   '34' => array('firstname' => 'Jane', 'lastname' => 'Doe'),
 * );
 * $firstnames = arms_util_array_collect($contacts, 'firstname');
 * # assert $firstnames == array('12' => 'John', '34' => 'Jane');
 * </pre>
 *
 */
function arms_util_array_collect(&$collection, $properties) {
  $source = $collection;
  if (!is_array($properties)) {
    $properties = array($properties);
  }
  foreach ($properties as $property) {
    $result = array();
    foreach ($source as $key => $value) {
      if (is_array($value)) {
        $result[$key] = $value[$property];
      }
      else if (is_object($value)) {
        $result[$key] = $value->{$property};
      }
    }
    $source = $result;
  }
  return $result;
}

/**
 * Look up a particular item in a tree
 *
 * @param $tree An tree of arrays or objects
 * @param $path An array of keys
 */
function arms_util_array_resolve($tree, $path) {
  $tmp = $tree;
  while (!empty($path)) {
    $key = array_shift($path);
    if (is_array($tmp)) {
      $tmp = $tmp[$key];
    } else if (is_object($tmp)) {
      $tmp = $tmp->{$key};
    } else {
      return NULL;
    }
  }
  return $tmp;
}

/**
 * Set a particular item in a tree
 *
 * @param $tree array-based tree structure
 * @param $path array of path-elements
 * @param $value the item to put in to the tree
 * @return $value
 */
function arms_util_array_set(&$tree, $path, $value) {
  $tmp = &$tree;
  while (count($path) > 1) {
    $key = array_shift($path);
    if (!is_array($tmp[$key])) {
      $tmp[$key] = array();
    }
    $tmp = &$tmp[$key];
  }
  
  $key = array_shift($path);
  $tmp[$key] = $value;
  return $value;
}

/**
 * Unset a particular item in a tree
 *
 * @param $tree array-based tree structure
 * @param $path array of path-elements
 */
function arms_util_array_unset(&$tree, $path) {
  $tmp = &$tree;
  while (count($path) > 1) {
    $key = array_shift($path);
    if (!is_array($tmp[$key])) {
      return; // nothing to do!
    }
    $tmp = &$tmp[$key];
  }
  
  $key = array_shift($path);
  unset($tmp[$key]);
}

/**
 * Convert an array with path-like keys into a tree of arrays
 *
 * @param $delim A path delimiter
 * @param $arr A one-dimensional array indexed by string keys
 */
function arms_util_explode_tree($delim, &$arr) {
  $result = array();
  foreach ($arr as $key => $value) {
    $path = explode($delim, $key);
    $node =& $result;
    while (count($path) > 1) {
      $key = array_shift($path);
      if (!isset($node[$key])) {
        $node[$key] = array();
      }
      $node =& $node[$key];
    }
    $key = array_shift($path); // last part of path
    $node[$key] = $value;
  }
  return $result;
}

/**
 * Convert a tree-like array into an array with path-like keys
 *
 * @param $delim A path delimiter
 * @param A multi-demensional, acyclic array
 */
function arms_util_implode_tree($delim, &$arr) {
  $result = array();
  foreach ($arr as $key => &$value) {
    if (is_array($value)) {
      $temp = &arms_util_implode_tree($delim, $value);
      foreach ($temp as $key2 => $value2) {
        $result[$key . $delim . $key2] = $value2;
      }
    } else {
      $result[$key] = $value;
    }
  }
  return $result;
}


/**
 * Delete a column by removing it from each row and renumbering columns
 *
 * This helper is not used in the core import logic, but it's useful
 * in unit testing
 *
 * @param $rows A two-dimensional array, indexed by row number then column number
 * @param $col A column number
 */
function arms_util_array_delete_column(&$rows, $col) {
  foreach ($rows as $key => $row) {
    unset($row[$col]);
    $rows[$key] = array_values($row);
  }
  return $rows;
}

/**
 * Rename a column
 *
 * @param $rows A two-dimensional array, indexed by row name/number then column name/number
 * @param $old A column name/number
 * @param $new A column name/number
 */
function arms_util_array_rename_column(&$rows, $old, $new) {
  foreach ($rows as $key => $row) {
    if (isset($row[$old])) {
      $rows[$key][$new] = $row[$old];
      unset($rows[$key][$old]);
    }
  }
  return $rows;
}

/**
 * Perform an "array_merge" between $defaults and each item in $rows
 *
 * Put another way, given a two-dimensional table, apply columnar defaults
 *
 * @param $rows A two-dimensional array, indexed by row name/number then column name/number; will be modified in-place
 * @param $defaults Array of, indexed by column name/number
 * @return $rows
 */
function arms_util_array_defaults(&$rows, $defaults) {
  foreach ($rows as $row_id => $row) {
    foreach ($defaults as $col_id => $value) {
      if (is_array($row) && !isset($row[$col_id])) {
        $rows[$row_id][$col_id] = $value;
      } elseif (is_object($row) && !isset($row->{$col_id})) {
        $rows[$row_id]->{$col_id} = $value;
      }
    }
  }
  return $rows;
}

/**
 * Convert an array of (key,value) tuples into an associative array (key => value)
 *
 * @param $arr The array to convert
 * @param $key_prop A property shared by each item in $arr; each item should have a unique value for this property
 * @param $value_prop A property shared by each item in $arr
 */
function arms_util_array_combine_properties(&$arr, $key_prop, $value_prop) {
  $result = array();
  foreach ($arr as $item) {
    if (is_array($item)) {
      $result[$item[$key_prop]] = $item[$value_prop];
    } else if (is_object($item)) {
      $result[$item->{$key_prop}] = $item->{$value_prop};
    }
  }
  return $result;
}

/**
 * Return the first item in the (numerical or associative) array
 *
 * @param $arr array, may be numerical or associative
 * @return the first item in the array, or FALSE
 */
function arms_util_array_first($arr) {
  if (empty($arr)) {
    return FALSE;
  }
  foreach ($arr as $key => $value) {
    return $value;
  }
}

/**
 * Return the last item in the (numerical or associative) array
 *
 * @param $arr array, may be numerical or associative
 * @return the first item in the array, or FALSE
 */
function arms_util_array_last($arr) {
  // FIXME Ridiculous; there must be a more efficient way to do this
  if (empty($arr)) {
    return FALSE;
  }
  foreach ($arr as $key => $value) {
    $result = $value;
  }
  return $result;
}

/**
 * Returns the set of elements matching the given keys. Comparable to array_slice() but used with associate arrays instead of numeric ranges
 *
 * @param $array array(scalar => mixed)
 * @param $keys array(scalar)
 * @return array(scalar => mixed)
 */
function arms_util_array_keyslice($array, $keys) {
  $result = array();
  foreach ($keys as $key) {
    $result[$key] = $array[$key];
  }
  return $result;
}

/**
 * Generate a sequence of numbers
 *
 * @param $min int
 * @param $max int
 * @param $incr int
 * @return array of int
 */
function arms_util_array_seq($min, $max, $incr = 1) {
  $result = array();
  for ($n = $min; $n <= $max; $n += $incr) {
    $result[$n] = $n;
  }
  return $result;
}

/**
 * A variation of PHP's sort which can be called inline
 *
 * @param $array pass-by-value array
 * @param $func calblback
 */
function arms_util_array_sort($array, $func = 'sort', $arg = FALSE) {
  if ($arg) {
    $func($array, $arg);
  } else {
    $func($array);
  }
  return $array;
}

/**
 * Create an iterator for which breaks up a numerical range into manageable chunks
 *
 * <code>
 * $chunks = arms_util_array_chunks(1, 95, 10);
 * foreach ($chunks as $low => $high) {
 *   printf("Process items %d through %d\n", $low, $high);
 * }
 * </code>
 *
 * @param $min The lowest value in the range (inclusive)
 * @param $max The highest value in the range (inclusive)
 * @param $interval The difference between key() and current() for each iteration, except the last 
 */
function arms_util_array_chunks($min, $max, $interval) {
  if ($min > $max) { return array(); }
  return new ArmsUtilArrayChunkIterator($min, $max, $interval);
}

/**
 * An iterator which breaks up a numerical range into manageable chunks
 */
class ArmsUtilArrayChunkIterator implements Iterator {
  var $min;
  var $max;
  var $interval;
  var $current_low;

  public function __construct($min, $max, $interval) {
    $this->min = (int) $min;
    $this->max = (int) $max;
    $this->interval = (int) $interval;
  }

  public function rewind() {
    $this->current_low = $this->min;
    $this->current_high = $this->current_low + $this->interval - 1;
    if ($this->current_high > $this->max) { $this->current_high = $this->max; }
  }

  public function current() {
    return $this->current_high;
  }
  
  public function key() {
    return $this->current_low;
  }
  
  public function next() {
    $this->current_low = $this->current_high + 1;
    $this->current_high = $this->current_low + $this->interval - 1;
    if ($this->current_high > $this->max) { $this->current_high = $this->max; }
  }
    
  public function valid() {
   return $this->current_low <= $this->max;
  }
}

/**
 * Split an array into partitions based on each attribute's values
 *
 * @param $array array; each item is a stdClass- or array-encoded record
 * @param $partition_attr
 * @return array of arrays
 */
function arms_util_array_partition_attr($array, $partition_attr, $default_partition = '_unknown_') {
  $result = array();
  foreach ($array as $key => &$value) {
    if (is_array($value) && !empty($value[$partition_attr])) {
      $partition = $value[$partition_attr];
    } elseif (is_object($value) && !empty($value->{$partition_attr})) {
      $partition = $value->{$partition_attr};
    } else {
      $partition = $default_partition;
    }
    if (!isset($result[$partition])) {
      $result[$partition] = array();
    }
    $result[$partition][$key] = $value;
  }
  return $result;
}

/**
 * Return the first item which matches $func
 */
function arms_util_array_firstMatch($callback, $values) {
  foreach ($values as $value) {
    if ($callback($value)) {
      return $value;
    }
  }
  return FALSE;
}

/**
 * Split an array into partitions based on a partitioning function
 *
 * @param $array array(key => value); each item is a stdClass- or array-encoded record
 * @param $partition_func callback($key,$value) which returns the name of the partition for this item
 * @return array(partitionName => array(key => value))
 */
function arms_util_array_partition_func($array, $partition_func) {
  $result = array();
  foreach ($array as $key => &$value) {
    $partition = call_user_func($partition_func, $key, $value);
    if ($partition === FALSE) {
      continue;
    }
    if (!isset($result[$partition])) {
      $result[$partition] = array();
    }
    $result[$partition][$key] = $value;
  }
  return $result;
}

/**
 * Walk through an array-tree, executing a callback for each branching-node
 *
 * This differs from PHP's array_walk_recursive, which runs the callback on each leaf-node
 */
function arms_util_array_tree_walk(&$node, $callback) {
  if (is_array($node)) {
    $callback($node);
    foreach ($node as &$child) {
      arms_util_array_tree_walk($child, $callback);
    }
  }
}

/**
 * Compare the contents of two array-trees
 *
 * @param $left array
 * @param $right array
 * @return array(fieldPath => array(0=>leftValue, 1=>rightValue))
 */
function arms_util_array_rdiff($delim, $left, $right) {
  arms_util_include_api('array');
  $left = arms_util_implode_tree($delim, $left);
  $right = arms_util_implode_tree($delim, $right);
  $keys = array_merge(array_keys($left), array_keys($right));
  
  $result = array();
  foreach ($keys as $key) {
    if ($left[$key] != $right[$key]) {
      $result[$key] = array($left[$key], $right[$key]);
    }
  }
  return $result;
}
