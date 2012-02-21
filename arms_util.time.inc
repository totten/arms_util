<?php

/**
 * fext "update" callback
 *
 * Convert a free-form height field to a standard second measurement.
 */
function arms_util_time_normalize($data) {
  $sql = strtr('update @table set @column = "%s" where entity_id = %d', array(
    '@table' => $data->fext->field['_table_name'],
    '@column' => $data->fext->field['column_name']
  ));
  $value = arms_util_time_to_fmt(
    arms_util_time_to_sec($data->param['value']));
  if ($value != $data->param['value']) {
    require_once 'CRM/Core/Transaction.php';
    CRM_Core_Transaction::addCallback(CRM_Core_Transaction::PHASE_POST_COMMIT,
      arms_util_curry('db_query', $sql, $value, $data->entity_id));
 }
}    

/**
 * Convert a height measurement expressed with a mix of minutes or seconds to straight seconds
 *
 * @param $orig string, e.g. "05:10", "5 min 10 sec"
 * @return int, or string if the value was inconvertible
 */
function arms_util_time_to_sec($orig) {
  $min_pattern = '(m|min|minute|minutes|:)';
  $sec_pattern = '(s|sec|second|seconds)';
  $orig = strtolower(trim($orig));
  
  if (is_numeric($orig)) {
    return $orig;
  } else if (preg_match('/^([0-9\.]+) *' . $min_pattern . ' *([0-9\.]+) *' . $sec_pattern . '?$/', $orig, $matches)) {
    return (float) ((60.0 * $matches[1]) + $matches[3]);
  } else if (preg_match('/^([0-9\.]+) *' . $min_pattern . '$/', $orig, $matches)) {
    return (float) (60 * $matches[1]);
  } else if (preg_match('/^([0-9\.]+) *' . $sec_pattern . '$/', $orig, $matches)) {
    return (float) $matches[1];
  } else {
    return $orig;
  }
}

/**
 * Format a time measurement (seconds) as a string.
 *
 * @param $in integer; if non-numeric, then pass through
 * @return string; if non-numeric, then pass through
 */
function arms_util_time_to_fmt($raw) {
  if (is_numeric($raw)) {
    $min = (int) ($raw / 60);
    $sec = (int) ( ((int)$raw) % 60);
    $dec = ($raw * 100.0) - ($min * 60.0 * 100.0) - ($sec * 100.0);
    if (empty($dec)) {
      return sprintf("%02d:%02d", $min, $sec);
    } else {
      return sprintf("%02d:%02d.%02d", $min, $sec, $dec);
    }
  } else {
    return $raw;
  }
}

/**
 * fext "update" callback
 *
 * Copy a height value from one fext field to another
 *
 * Example:
 * $defn['#update'] = array();
 * $defn['#update'][] = arms_util_curry('arms_util_time_copy', DESTINATION_FEXT_VAR);
 */
function arms_util_time_copy($copyToVar, $data) {
    
  require_once 'api/v2/Contact.php';
  require_once 'CRM/Core/Transaction.php';
  arms_util_include_api('fext');
            
  $copyToCf = arms_util_field(arms_util_fext_get_field($copyToVar));
  if ($copyToCf) {
        
    $value = arms_util_time_to_sec($data->param['value']);

    $sql = strtr('update @table set @column = %d where entity_id = %d', array(
      '@table' => $copyToCf['_table_name'],
      '@column' => $copyToCf['column_name']
    ));

    // avoid deadlock
    CRM_Core_Transaction::addCallback(CRM_Core_Transaction::PHASE_POST_COMMIT,
      arms_util_curry('db_query', $sql, (float) $value, $data->entity_id)
    );
  }
}
