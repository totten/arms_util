<?php

/**
 * Register a safe file extension
 *
 * @param $ext string e.g. "jpg", "docx"
 */
function arms_util_safe_file_create($ext) {
  civicrm_initialize();
  require_once 'api/v2/Options.php';
  arms_util_include_api('option');
  
  // get the activity status group id
  $params = array('name' => 'safe_file_extension');
  $group = civicrm_option_group_find($params);

  if (is_array($group) && count($group) > 0) {
    $params = arms_util_option_value_next($group[0]);
    $params['label'] = $ext;
    $params['option_group_id'] = $group[0]['id'];
    
    $result = civicrm_option_value_create($params);
  }
}

/**
 * Check whether an extension is currently used
 *
 * @param $ext string e.g. "jpg", "docx"
 * @return NULL or the "value" of the registration record
 */
function arms_util_safe_file_find($ext) {
  civicrm_initialize();
  require_once 'api/v2/Options.php';
  arms_util_include_api('option');
  
  // get the activity status group id
  $params = array('name' => 'safe_file_extension');
  $group = civicrm_option_group_find($params);
  if (is_array($group) && count($group) > 0) {
    $q = db_query('select * from {civicrm_option_value} where option_group_id = %d and label = "%s"',
      $group[0]['id'], $ext);
    $ov = db_fetch_array($q);
    if ($ov) {
      return $ov['value'];
    }
  }
  return NULL;
}
