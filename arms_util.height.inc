<?php

/**
 * fext "update" callback
 *
 * Convert a free-form height field to a standard inch measurement.
 */
function arms_util_height_normalize($data) {
  $sql = strtr('update @table set @column = "%s" where entity_id = %d', array(
    '@table' => $data->fext->field['_table_name'],
    '@column' => $data->fext->field['column_name']
  ));
  $value = arms_util_height_to_fmt(
    arms_util_height_to_in($data->param['value']));
  if ($value != $data->param['value']) {
    require_once 'CRM/Core/Transaction.php';
    CRM_Core_Transaction::addCallback(CRM_Core_Transaction::PHASE_POST_COMMIT,
      arms_util_curry('db_query', $sql, $value, $data->entity_id));
 }
}    

/**
 * Convert a height measurement expressed with a mix of feet or inches to straight inches
 *
 * @param $orig string, e.g. "6' 6", "6.5 feet", "78"
 * @return int, or string if the value was inconvertible
 */
function arms_util_height_to_in($orig) {
  $ft_pattern = '(\'|\`|\-|f|ft|foot|fot|feet|fet)';
  $in_pattern = '("|\'\'|\`\`|”|i|in|inch|inches|inc|inche|inchs)';
  $orig = strtolower(trim($orig));
  
  if (is_numeric($orig)) {
    return (int) (12.0 * $orig);
    //if ($orig < 20) {
    //  return (int) (12.0 * $orig);
    //} else {
    //  return (int) $orig;
    //}
  } else if (preg_match('/^([0-9\.]+) *' . $ft_pattern . ' *([0-9\.]+) *' . $in_pattern . '?$/', $orig, $matches)) {
    return (int) ((12.0 * $matches[1]) + $matches[3]);
  } else if (preg_match('/^([0-9\.]+) *' . $ft_pattern . '$/', $orig, $matches)) {
    return (int) (12 * $matches[1]);
  } else if (preg_match('/^([0-9\.]+) *' . $in_pattern . '$/', $orig, $matches)) {
    return (int) $matches[1];
  } else if (preg_match('/^([0-9\.]+) *(m|meter|meters)$/', $orig, $matches)) {
    return (int) (39.3700787 * $matches[1]);
  } else if (preg_match('/^([0-9\.]+) *(cm|centimeter|centimeters)$/', $orig, $matches)) {
    return (int) (0.393700787 * $matches[1]);
  } else if (preg_match('/^([0-9\.]+) *(furlong|furlongs)$/', $orig, $matches)) { // :)
    return (int) (7920 * $matches[1]);
  } else {
    return $orig;
  }
  
  return ((int) ($in/12)) . "'" . ($in%12);
}

/**
 * Format a height measurement (inches) as a string.
 *
 * @param $in integer; if non-numeric, then pass through
 * @return string; if non-numeric, then pass through
 */
function arms_util_height_to_fmt($in) {
  if (is_numeric($in)) {
    return ((int) ($in/12)) . "' " . ($in%12);
  } else {
    return $in;
  }
}

/**
 * fext "update" callback
 *
 * Copy a height value from one fext field to another
 *
 * Example:
 * $defn['#update'] = array();
 * $defn['#update'][] = arms_util_curry('arms_util_height_copy', DESTINATION_FEXT_VAR);
 */
function arms_util_height_copy($copyToVar, $data) {
    
  require_once 'api/v2/Contact.php';
  require_once 'CRM/Core/Transaction.php';
  arms_util_include_api('fext');
            
  $copyToCf = arms_util_field(arms_util_fext_get_field($copyToVar));
  if ($copyToCf) {
        
    $value = arms_util_height_to_in($data->param['value']);
    $heightIn = 0;
    if ($value == 'low') {
        $heightIn = 59;
    }
    else if ($value == 'high') {
    	  $heightIn = 145;
    }
    else {
    	  $heightIn = $value;
    }

    $sql = strtr('update @table set @column = %d where entity_id = %d', array(
      '@table' => $copyToCf['_table_name'],
      '@column' => $copyToCf['column_name']
    ));

    // avoid deadlock
    CRM_Core_Transaction::addCallback(CRM_Core_Transaction::PHASE_POST_COMMIT,
      arms_util_curry('db_query', $sql, (int) $heightIn, $data->entity_id)
    );
  }
}
