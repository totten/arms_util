<?php
/**
 * ARMS utility functions for manipulating option groups and option values
 */

/**
 * Get a list of all option groups
 *
 * @return array; keys are option group ID's and values are OptionGroup defintions, encoded as arrays
 */
function arms_util_option_groups($fresh = FALSE) {
  static $cache = -1;
  if ($cache == -1 || $fresh) {
    $cache = array();
    $q = db_query('select * from {civicrm_option_group}');
    while ($optionGroup = db_fetch_array($q)) {
      $cache[$optionGroup['id']] = $optionGroup;
    }
  }
  return $cache;
}

/** Lookup a single option group with the given name. 
 *
 * @return An array representing the group; false if unavailable
 */
function arms_util_option_group_by_name($optionGroupName) {
  $params = array();
  $params['name'] = $optionGroupName;

  require_once 'api/v2/Options.php';
  $groups = civicrm_option_group_find($params);
  
  # dpm(array('groups' => $groups));
  if (empty($groups)) {
    return FALSE;
  }
  else {
    return $groups[0];
  }
}

/**
 * Determine the next available value in an option group
 *
 * @param $optionGroup An array which includes the "id" of the option group
 * @return an array in which keys "value" and "weight" provide for the next available value and weight in the option group; false if unavailable
 */
function arms_util_option_value_next($optionGroup) {
  if (empty($optionGroup['id'])) {
    return FALSE;
  }

  $sql = "select max(value * 1) + 1 as value, max(weight) + 1 as weight " . 
    "from {civicrm_option_value} " .
    "where option_group_id = %d";
  $result = db_query($sql, $optionGroup['id']);

  if ($max = db_fetch_array($result)) {
    return $max;
  }
  else {
    return array('value' => 1, 'weight' => '1');
  }
}

/** 
 * Register a new OptionValue whose value is part of an incrementing sequence
 *
 * @param $optionGroup An array representing the option group to which we will add a value. Must define 'id'
 * @param $optionValue An array of properties for the optionValue (e.g. name, label, description). The value, weight, and option_group_id will be set automatically.
 * @return The result of calling civicrm_option_value_create
 */
function arms_util_option_value_create_seq($optionGroup, $optionValue) {
  // find the next value/weight in the sequence  
  $nextValue = arms_util_option_value_next($optionGroup);
  if (! $nextValue) {
    return civicrm_create_error('Failed to determine next value/weight in option group');
  }
  
  // merge calculated info with $optionValue
  $params = $optionValue;
  $params['option_group_id'] = $optionGroup['id'];
  $params['value'] = $nextValue['value'];
  $params['weight'] = $nextValue['weight'];
  
  // persist
  # dpm(array('new option value' => $params));
  require_once 'api/v2/Options.php';
  return civicrm_option_value_create($params);
}

/** Remove an option value
 * 
 * @param $optionGroup An array representing the option group with the value to delete
 * @param $optionValue Reference to the option value(s) to delete. Will be passed through civicrm_option_value_find
 */
function arms_util_option_value_delete(&$optionValue) {
  require_once 'api/v2/Options.php';
  $ovresult = civicrm_option_value_find($optionValue);
  $ids = array();
  foreach ($ovresult as $ov) {
    $ids []= $ov['id'];
  }

  $idstring = implode(',', $ids);
  $delresult = db_query('delete from {civicrm_option_value} where id in ('.$idstring.')');
}

/**
 * Lookup an option value (by matching value and option_group_id); if none is found, create one
 *
 * @param $optionValue array with keys:
 *  - option_group_id: required
 *  - value: required
 *  - name: optional
 *  - label: optional
 *  - description: optional
 *  - weight: optional
 * @return array with:
 *   - is_error: boolean
 *   - result: array-encoded optionvalue
 */
function arms_util_option_value_findcreate($optionValue) {
  require_once 'api/v2/Options.php';
  
  foreach (array('option_group_id','value') as $arg) {
    if (empty($optionValue[$arg])) return civicrm_create_error('Missing parameter: ' . $arg);
  }
  
  $result = db_fetch_array(db_query('select * from {civicrm_option_value} where option_group_id = %d and value = "%s"',
    $optionValue['option_group_id'], $optionValue['value']));
  if (isset($result['id'])) return civicrm_create_success($result);
  
  foreach (array('name','label','description') as $arg) {
    if (empty($optionValue[$arg])) $optionValue[$arg] = $optionValue['value'];
  }
  
  if (empty($optionValue['weight'])) {
    $next = arms_util_option_value_next(array('id' => $optionValue['option_group_id']));
    $optionValue['weight'] = $next['weight'];
  }
  
  return civicrm_option_value_create($optionValue);
}

/**
 * Lookup the option values for a given group
 *
 * @param $optionGroup; an ID# (int), a group name (string), or array-encoded object
 * @return array; keys are civicrm_option_value.value strings, and values are civicrm_option_value records (array-encoded)
 */
function arms_util_option_values($optionGroup) {
  if (is_array($optionGroup)) {
    $optionGroupId = $optionGroup['id'];
  } elseif (is_numeric($optionGroup)) {
    $optionGroupId = $optionGroup;
  } else {
    $arr = arms_util_option_group_by_name($optionGroup);
    $optionGroupId = $arr['id'];
  }
  $option_values = array();
  $q = db_query('select * from {civicrm_option_value} where option_group_id = %d', $optionGroupId);
  while ($option_value = db_fetch_array($q)) {
    $option_values[] = $option_value;
  }
  return $option_values;
}

/**
 * Convert the OptionValue.values in an OptionGroup
 *
 * @param $option_group_id Unique ID of an option group
 * @param $option_map array keys are the old 'OptionValue.value', and array values are the new 'OptionValue.value'
 */
function arms_util_option_convert_values($option_group_id, $option_map) {
  foreach ($option_map as $old => $new) {
    db_query('update {civicrm_option_value} set value = "%s" where option_group_id = %d and value = "%s"',
      $new,
      $option_group_id,
      $old);
  }
}

/**
 * Convert stored values for this field.
 *
 * @param $field array-encoded CustomField
 * @param $option_map array keys are the old encoding, and values are the new encoding
 * @param $preserve_unknown boolean; whether unknown values (which don't appear in the $option_map) should be preserved
 */
function arms_util_option_convert_field($field, $option_map, $preserve_unknown = FALSE) {
  $group = arms_util_group($field['_group_name']);
  $sql = strtr('select id, @column as value from @table', array(
    '@column' => $field['column_name'],
    '@table' => $group['table_name'],
  ));
  $q = db_query($sql);
  $todos = array();
  while ($rec = db_fetch_object($q)) {
    if ($rec->value) {
      if ($rec->value == CRM_Core_DAO::VALUE_SEPARATOR . CRM_Core_DAO::VALUE_SEPARATOR) {
        // noop
      } elseif (substr($rec->value, 0,1) == CRM_Core_DAO::VALUE_SEPARATOR) {
        $values = explode(CRM_Core_DAO::VALUE_SEPARATOR, trim($rec->value, CRM_Core_DAO::VALUE_SEPARATOR));
        $new_values = array();
        foreach ($values as $value) {
          if (isset($option_map[$value])) {
            $new_values[] = $option_map[$value];
          } elseif ($preserve_unknown) {
            $new_values[] = $value;
          } else {
            drupal_set_message(t('Failure processing @table.@column: Record #@id contains unknown value (@value)', array(
              '@table' => $group['table_name'],
              '@column' => $field['column_name'],
              '@id' => $rec->id,
              '@value' => $value,
            )), 'warning');
          }
        }
        $todos[$rec->id] = CRM_Core_DAO::VALUE_SEPARATOR . implode(CRM_Core_DAO::VALUE_SEPARATOR, $new_values) . CRM_Core_DAO::VALUE_SEPARATOR;
      } elseif (!empty($rec->value)) {
        if (isset($option_map[$rec->value])) {
          $todos[$rec->id] = $option_map[$rec->value];
        } elseif ($preserve_unknown) {
          $todos[$rec->id] = $rec->value;
        } else {
          drupal_set_message(t('Failure processing @table.@column: Record #@id contains unknown value (@value)', array(
            '@table' => $group['table_name'],
            '@column' => $field['column_name'],
            '@id' => $rec->id,
            '@value' => $rec->value,
          )), 'warning');
        }
      }
      if ($todos[$rec->id] == $rec->value) {
        unset($todos[$rec->id]); // unnecessary
      }
    }
  }
  // dpm($todos);
  $update_sql = strtr('update @table set @column = "%s" where id = %d', array(
    '@column' => $field['column_name'],
    '@table' => $group['table_name'],
  ));
  // dpm($update_sql);
  foreach ($todos as $id => $value) {
    db_query($update_sql, $value, $id);
  }
}

/**
 * Convert a list of option values from Civi's serialized array format to a proper array
 *
 * @param >>, >1>, >1>2>, etc.
 * @return array(), array(1), array(1,2), etc.
 */
function arms_util_option_explode($str) {
  civicrm_initialize();
  if (empty($str) || $str == CRM_Core_DAO::VALUE_SEPARATOR . CRM_Core_DAO::VALUE_SEPARATOR) {
    return array();
  } else {
    return explode(CRM_Core_DAO::VALUE_SEPARATOR, trim($str, CRM_Core_DAO::VALUE_SEPARATOR));
  }
}

/**
 * Convert a list of option values from an array to Civi's serialized array format
 *
 * @param array(1,2,3)
 * @return >1>2>3>
 */
function arms_util_option_implode($arr) {
  civicrm_initialize();
  return CRM_Core_DAO::VALUE_SEPARATOR . implode(CRM_Core_DAO::VALUE_SEPARATOR, $arr) . CRM_Core_DAO::VALUE_SEPARATOR;
}

/**
 * (Drush callback)
 *
 * Merge two option-values, eliminating one of them
 *
 * @param $optionGroupId int
 * @param $oldValue scalar, the deprecated value
 * @param $newValue scalar, the preferred value
 */
function _arms_util_option_drush_mergeinto($optionGroupId, $oldValue, $newValue) {
  civicrm_initialize();
  
  $oldOptionValue = db_fetch_array(db_query('select * from {civicrm_option_value} where option_group_id = %d and value = "%s"',
      $optionGroupId, $oldValue));
  if (empty($oldOptionValue)) {
    printf("failed to locate deprecated option value (group=[%d] value=[%s])\n", $optionGroupId, $oldValue);
    return;
  }
  $newOptionValue = db_fetch_array(db_query('select * from {civicrm_option_value} where option_group_id = %d and value = "%s"',
      $optionGroupId, $newValue));
  if (empty($newOptionValue)) {
    printf("failed to locate preferred option value (group=[%d] value=[%s])\n", $optionGroupId, $newValue);
    return;
  }

  $optionMap = array($oldValue => $newValue);
  $q = db_query('SELECT id FROM {civicrm_custom_field} WHERE option_group_id = %d', $optionGroupId);
  while ($fieldId = db_result($q)) {
    $field = arms_util_field_by_id($fieldId);
    printf("update values of %s (%s.%s)\n", $field['_full_name'], $field['_table_name'], $field['column_name']);
    arms_util_option_convert_field($field, $optionMap, TRUE);
  }

  printf("delete deprecated option value (group=[%d] value=[%s])\n", $optionGroupId, $oldValue);
  arms_util_option_value_delete($oldOptionValue);
}
