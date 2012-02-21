<?php

/**
 * CRM_Core_PseudoConstant::locationType
 *
 * Get a list of all location types
 *
 * @return array; keys are names and values array-encoded location-types
 *
function arms_util_location_types($fresh = FALSE) {
  global $_arms_util_location_types;
  if ($fresh || ! $_arms_util_location_types) {
    $q = db_query('select * from {civicrm_location_type}');
    $_arms_util_location_types = array();
    while ($type = db_fetch_array($q)) {
      $_arms_util_location_types[$type['name']] = $type;
    }
  }
  return $_arms_util_location_types;
}*/

/**
 * Create a new location type
 *
 * @param $type an array-encoded location type; be sure to include 'name' and 'description'
 */
function arms_util_location_type_add($type) {
  $type['is_reserved'] = 0;
  $type['is_active'] = 1;
  db_query('insert into {civicrm_location_type} (name,description,is_reserved,is_active,is_default) values ("%s","%s",%d,%d,0)',
    $type['name'], $type['description'], $type['is_reserved'], $type['is_active']);

  //global $_arms_util_location_types;
  //$_arms_util_location_types = FALSE;
  require_once 'CRM/Core/PseudoConstant.php';
  CRM_Core_PseudoConstant::flush('locationType');
}

/**
 * Set the default location type
 */
function arms_util_location_type_set_default($type_id) {
  db_query('update {civicrm_location_type} set is_default = if(id = %d,1,NULL)', $type_id);

  require_once 'CRM/Core/PseudoConstant.php';
  CRM_Core_PseudoConstant::flush('locationType');
}

/**
 * Ensure that exactly one item in $rows has the is_primary flag and
 * that exactly one item (preferrably the same) has the is_billing flag.
 * (If there are zero rows, do nothing.)
 *
 * @param $rows array of locational records, such email records or phone records
 */
function arms_util_location_cleanFlags(&$rows) {
  $primary = FALSE; // row key
  $billing = FALSE; // row key

  // Determine which item in $rows, if any, is the first primary/billing
  foreach ($rows as $row_id => $row) {
    if ($primary === FALSE && $row['is_primary']) {
      $primary = $row_id;
    }
    if ($billing === FALSE && $row['is_billing']) {
      $billing = $row_id;
    }
  }

  // If $rows is missing a primary and/or billing, fill it in
  if ($primary !== FALSE && $billing === FALSE) {
    $billing = $primary;
  }
  if ($primary === FALSE && $billing !== FALSE) {
    $primary = $billing;
  }
  if ($primary === FALSE && $billing === FALSE) {
    foreach ($rows as $row_id => $row) {
      $primary = $billing = $row_id;
      break;
    }
  }

  // Enforce the selections
  foreach ($rows as $row_id => $row) {
    $rows[$row_id]['is_primary'] = ($row_id == $primary) ? 1 : 0;
    $rows[$row_id]['is_billing'] = ($row_id == $billing) ? 1 : 0;
  }
}
