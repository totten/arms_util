<?php
/**
 * @file
 *
 * Helper functions for working with custom-data meta-data
 */

/**
 * Scan the available custom data groups/fields and record a cached variation.
 */
function arms_util_cdmd_update($dir, $file, $logger = ARMS_UTIL_LOG_WATCHDOG) {
  $cdmd =& arms_util_cdmd_build($logger);
  if ($cdmd === FALSE) {
    arms_util_log($logger, WATCHDOG_ERROR, 'Failed to query CiviCRM custom data group data');
    return;	
  }

  arms_util_cdmd_write($cdmd, $dir, $file, $logger);
  global $_arms_util_groups_cache;
  $_arms_util_groups_cache = NULL;
}

/**
 * Determine a "standard" field name for the given column.
 *
 * In the normal case, we use the old rule that "standard field name is
 * column name minus the last lexeme" (where lexemes are delimited by "_").
 * However, there is no "_", then we truncate nothing. Also, if a duplicate is
 * detected, we append the field ID.
 *
 * This should minimize accidental blanks or duplicates. However, it does not
 * ensure that names are _stable_ -- i.e. two different calls to this function
 * could yield different results. This is mitigated somewhat by generating names
 * in a chosen order (notice "order by" in "arms_util_cdmd_build"). However, it
 * is not possible to guarantee that names are unique, stable, and reasonably
 * portable while preserving the current CDMD lifecycle.
 *
 * @param $cdmdGroup array The data group which contains the field; may be FALSE
 * @param $columnName string The SQL column name
 * @param $columnId long The Civi column ID#
 * @return string, a valid field name
 */
function arms_util_cdmd_field_name($cdmdGroup, $columnName, $columnId) {
  $last_ = strrpos($columnName, '_');
  if ($last_ === FALSE || $last_ === 0) {
    // column name will lead to bad results
    $fieldName = $columnName;
  }
  else {
    $fieldName = substr($columnName, 0, strrpos($columnName, '_'));
  }
  
  if ($cdmdGroup && isset($cdmdGroup['fields'][$fieldName])) {
    // Proposed field name is not unique; try to find an alternative
    if (!isset($cdmdGroup['fields'][$columnName])) {
      // fall back on column name
      $fieldName = $columnName;
    }
    elseif (!isset($cdmdGroup['fields']['custom_' . $columnId])) {
      // yowza
      $fieldName = 'custom_' . $columnId;
    }
    else {
      // double yowza
      $fieldName = '__invalid__';
    }
  }
  
  return $fieldName;
}

/**
 * Creates an ARMS custom field record by merging in
 * additional information about the CiviCRM field.
 *
 * @param $groupRow stdClass; table civicrm_custom_group
 * @param $fieldRow stdClass; table civicrm_custom_field
 * @param $cdmdGroup array (derived from $groupRow)
 */
function arms_util_cdmd_wrap_field($groupRow, $fieldRow, $cdmdGroup = FALSE) {
  $groupName = strtolower($groupRow->name);
  $fieldName = arms_util_cdmd_field_name($cdmdGroup, $fieldRow->column_name, $fieldRow->id);
  $cdmdField = array(
              '_full_name'  => $groupName . ':' . $fieldName,
              '_group_id'   => $groupRow->id, // CDMD group id
              '_group_name' => $groupName, // CDMD group name
              '_name'       => $fieldName, // CDMD field name
              '_table_name' => $groupRow->table_name, // CDMD group table
              '_param'      => 'custom_' . $fieldRow->id, // Civi API field parameter
              '_is_active'  => ($groupRow->is_active && $fieldRow->is_active) ? 1 : 0,
              'id'          => $fieldRow->id, // Civi ID#
              'label'       => $fieldRow->label,
              'data_type'   => $fieldRow->data_type,
              'html_type'   => $fieldRow->html_type,
              'default_value' => $fieldRow->default_value,
              'text_length' => $fieldRow->text_length,
              'option_group_id' => $fieldRow->option_group_id,
              'column_name' => $fieldRow->column_name // SQL table name
  );
  return $cdmdField;          
}

/**
 * Creates the custom_data_meta_data.ser file in the arms_util files 
 * directory as a serialized array of civicrm custom data information for
 * the site.
 *    
 * @return void
 */ 
function arms_util_cdmd_build($logger = ARMS_UTIL_LOG_WATCHDOG) {
  $sql = "select id, name, title, table_name, extends, is_active " .
         "from {civicrm_custom_group}";
  $groupResult = db_query($sql);
  if ($groupResult !== FALSE) {
    
    $cdmd = array();
    while ($groupRow = db_fetch_object($groupResult)) {
      
      $groupName = strtolower($groupRow->name);
      $cdmdGroup = array(
          '_name'      => $groupName, // CDMD group name
          'id'         => $groupRow->id, // Civi ID#
          'title'      => $groupRow->title,
          'table_name' => $groupRow->table_name, // SQL table name
          'extends'    => $groupRow->extends,
          'default'    => array(),
      );
      
      $cdmdGroup['id'] = $groupRow->id;
      
      $sql = "select id, column_name, label, data_type, html_type, option_group_id, default_value, text_length, is_active " .
             "from {civicrm_custom_field} " .
             "where custom_group_id = %d " .
             "order by column_name";
      // order is significant; see arms_util_cdmd_field_name()
      $fieldResult = db_query($sql, $groupRow->id);
      if ($fieldResult !== FALSE) {
      
        while ($fieldRow = db_fetch_object($fieldResult)) {
          $cdmdField = arms_util_cdmd_wrap_field($groupRow, $fieldRow, $cdmdGroup);
          $cdmdGroup['fields'][$cdmdField['_name']] = $cdmdField;
          $defaultValue = _arms_util_cdmd_decode_value($cdmdField, $fieldRow->default_value);
          if ($defaultValue !== NULL) {
            $cdmdGroup['default']['custom_' . $fieldRow->id] = $defaultValue;
          }
        }
      }
      else {
        arms_util_log($logger, WATCHDOG_ERROR, 'Failed to query CiviCRM custom data field data for group ID @groupId', array('@groupId' => $groupRow->id));
      }
      
      $cdmd[$groupName] = $cdmdGroup;
    }
    return $cdmd;
  }
  else {
    return FALSE;
  }
}

/** Record the CDMD cache */
function arms_util_cdmd_write($cdmd, $dir, $file, $logger = ARMS_UTIL_LOG_WATCHDOG) {
  if (file_exists($dir)) {
    $path = $dir . '/' . $file;
    if (file_put_contents($path, serialize($cdmd)) > 0) {
      arms_util_log($logger, WATCHDOG_NOTICE, 'Wrote custom data meta data config to <code>@path</code>', array('@path' => $path));
      return TRUE;
    }
    else {
      arms_util_log($logger | ARMS_UTIL_LOG_SET_MESSAGE, WATCHDOG_ERROR, 'Failed to write custom data meta data config to "<code>@path</code>"', array('@path' => $path));
    }
  }
  else {
    arms_util_log($logger | ARMS_UTIL_LOG_SET_MESSAGE, WATCHDOG_ERROR, 'Failed to write custom data meta data config to "<code>@path</code>" - missing files directory', array('@path' => $path));
  }
  return FALSE;
}

/** Reset CiviCRM's cached list of custom data groups and fields */
function arms_util_cdmd_clearcivi() {
	db_query("delete from {civicrm_cache} where group_name  = 'contact fields'");
}

/**
 * Deserialize a field value
 *
 * For most fields, this just passes through $value. But some (e.g. checkbox and multiselect) require it.
 *
 * FIXME Only supports a handful of common field types. Need to test all others.
 *
 * @param $field The custom data field (encoded per arms_util_cdmd_build)
 * @param $value The value that will be stored in the table, or NULL if no value should be stored
 * @return $value, in a format that's compatible with civicrm_contact_add, or NULL if the format is unknown
 */
function _arms_util_cdmd_decode_value(&$field, $value) {
  if ($value === NULL) {
    return NULL;
  }
  else if ($field['data_type'] == 'String' && ($field['html_type'] == 'CheckBox' || $field['html_type'] == 'Multi-Select')) {
    // Checkbox fields require alternate encoding
    civicrm_initialize();
    require_once 'CRM/Core/DAO.php';
    $values = explode(CRM_Core_DAO::VALUE_SEPARATOR, substr($value, 1, -1));
    $dec_values = array();
    foreach ($values as $value) {
      $dec_values[$value] = 1;
    }
    return $dec_values;
  }
  else if (in_array($field['data_type'], array('String', 'Int', 'Boolean')) 
    && in_array($field['html_type'], array('Text', 'TextArea', 'Select', 'Radio'))) {
    return $value;
  }
  else {
    // Untested type
    return NULL;
  }
}

/**
 * Fetch the custom data groups that apply to a given type
 *
 * @param $type A CiviCRM type, e.g. 'Individual', 'Organization', 'Activity'
 * @return array associative array of custom data groups
 */
function arms_util_cdmd_groups(&$config, $type = '*', $fresh = FALSE) {
  static $_types = FALSE;
  if ($_types == FALSE) {
    $_types = array();
  }
  if ($type == '*') {
    return $config;
  }
  if ($fresh || !isset($_types[$type])) {
    $_types[$type] = array();
    foreach ($config as $groupName => $group) {
      if ($group['extends'] == $type) {
        $_types[$type][$groupName] = $group;
      }
      else if ($group['extends'] == 'Contact' && ($type == 'Individual' || $type == 'Organization' || $type == 'Household')) {
        $_types[$type][$groupName] = $group;
      }
    }
  }
  return $_types[$type];
}

/**
 * Implementation of hook_civicrm_pre (see also: arms_util_civicrm_pre)
 */
function _arms_util_cdmd_civicrm_pre($op, $objectName, $objectId, &$objectRef) {
  $supportedObjectNames = array('Individual', 'Organization', 'Household');
  // Note: These hooks are not called when importing
  if ($op == 'create' && in_array($objectName, $supportedObjectNames)) {
    // Fix problem with defaults -- https://secure.bentonconsult.com/projects/arms/ticket/341
    // This code only applies defaults correctly if $objectRef contains a complete copy of the object's data.
    // This precondition holds when op = create, but it fails when op = edit.

    $config = &_arms_util_read_custom_data_config($fresh);
    $groups = &arms_util_cdmd_groups($config, $objectName);
    $encoding = __hack__arms_util_cdmd_pre_find_encoding($objectName, $objectRef);

    if (!$encoding) {
      watchdog('arms_util',
        'Failed to determine encoding for custom data for @objectName in hook_civicrm_pre', 
        array('@objectName' => $objectName),
        WATCHDOG_WARNING);
      return;
    }

    foreach ($groups as $group) {
      if (!is_array($group['fields'])) { continue; }
      foreach ($group['fields'] as $fieldName => &$cdmdField) {
        __hack__arms_util_cdmd_pre_default($objectName, &$objectRef, $encoding, &$cdmdField);
      } // each field
    } // each group
  }
}

/**
 * With hook_civicrm_pre, the $objectRef does not encode custom data in a reliable way; 
 * depending on how the hook is called, it could use key/value pairs ('custom_123' => 'foo')
 * or a tree that mixes custom data with schema. This function encapsulates the dirty
 * business of figuring out how to encode custom data.
 *
 * The return value should generally be considered opaque and passed through to the
 * related functions (e.g. __hack__arms_util_cdmd_pre_set)
 *
 * The return value will be false if the encoding cannot be determined.
 */
function __hack__arms_util_cdmd_pre_find_encoding($objectName, &$objectRef) {
  $backtrace = &debug_backtrace();
  foreach ($backtrace as $stackFrame) {
    // This matches genuine profile use-cases as well as imports and email2activity.
    if ($stackFrame['class'] == 'CRM_Contact_BAO_Contact' && $stackFrame['function'] == 'createProfileContact') {
      return  'TREE';
    }
    // This matches the "New Individual"-type pages (civicrm/contact/add)
    if ($stackFrame['class'] == 'CRM_Contact_Form_Edit' && $stackFrame['function'] == 'postProcess') {
      return 'KEYVALUE-1';
    }
    // This matches API call "civicrm_contact_add"
    if (empty($stackFrame['class']) && $stackFrame['function'] == 'civicrm_contact_add') {
      return 'TREE';
    }
  }
  return FALSE;
}

/**
 * Apply a default value for a custom data field. If a value is already defined or if
 * no default is available, then do nothing.
 *
 * @see __hack__arms_util_cdmd_pre_find_encoding
 */
function __hack__arms_util_cdmd_pre_default($objectName, &$objectRef, $encoding, &$cdmdField) {
  if (!isset($cdmdField['default_value'])) {
    return;
  }

  switch ($encoding) {
    case 'TREE':
      if (!isset($objectRef['custom'][$cdmdField['id']])) {
        CRM_Core_BAO_CustomField::formatCustomField(
          $cdmdField['id'],
          $objectRef['custom'],
          $cdmdField['default_value'],
          $objectName
        );
      }    
      break;
    case 'KEYVALUE-1':
      $key = 'custom_' . $cdmdField['id'] . '_-1';
      if (!isset($objectRef[$key])) {
        $objectRef[$key] = $cdmdField['default_value'];
      }
      break;
    case 'KEYVALUE':
      $key = 'custom_' . $cdmdField['id'];
      if (!isset($objectRef[$key])) {
        $objectRef[$key] = $cdmdField['default_value'];
      }
      break;
  }
}

/**
 * Fetch an array which has been initialized with default values for all custom fields
 *
 * @param $type A CiviCRM type, e.g. 'Individual', 'Organization', 'Activity'
 * @return Array of default values, indexed by field name. The array encoding is suitable for submission to, e.g., civicrm_contact_add
 */
function arms_util_cdmd_all_defaults(&$config, $type, $fresh = FALSE) {
  $defaults = array();
  $groups =& arms_util_cdmd_groups($config, $type, $fresh);
  foreach ($groups as $group) {
    $defaults = $defaults + $group['default'];
  }
  return $defaults;
}

/**
 * Scan the database for records that have undefined values and apply the default values.
 *
 * FIXME Only supports a handful of common field types. Need to test all others.
 *
 * @param $group The group for which we want to apply defaults (encoded per arms_util_cdmd_build)
 */
function arms_util_cdmd_reapply_defaults(&$group) {
  // List of data types and html types for which we apply defaults; all others are skipped
  $supported_field_types = array(
    'String' => array('Text' => 1, 'Select' => 1, 'Radio' => 1, 'CheckBox' => 1, 'Multi-Select' => 1),
    'Int' => array('Text' => 1, 'Select' => 1, 'Radio' => 1), 
    'Memo' => array('TextArea' => 1, 'RichTextEditor' => 1),
    'Boolean' => array('Radio' => 1)
  );
  
  // Determine how to query for custom data
  if (! in_array($group['extends'], array('Contact', 'Individual', 'Organization', 'Household'))) {
    arms_util_log(ARMS_UTIL_LOG_WATCHDOG, WATCHDOG_ERROR,
     'Failed to re-apply defaults for group @name',
      array('@name' => $group['table_name'])
      );
    return;
  }
  
  $rowTemplate = array();
  foreach ($group['fields'] as $field) {
    if (1 == @$supported_field_types[$field['data_type']][$field['html_type']]) {
      if ($field['default_value'] !== NULL) {
        $rowTemplate[$field['column_name']] = $field['default_value'];
      }
    }
  }

  // Insert CD for contacts who don't have any values in this custom data group
  $sql = 'select c.id as contact_id, g.id as group_row_id from civicrm_contact c'
    . ' left outer join ' . $group['table_name'] . ' g on c.id = g.entity_id';
  $where_contact_type = ($group['extends'] == 'Contact') ? '' : (' and c.contact_type = "' . $group['extends'] .'"');
  $q = db_query($sql . ' where g.id is null' . $where_contact_type);
  // dpm(array('sql' => $sql . ' where g.id is null' . $where_contact_type));
  while ($row = db_fetch_object($q)) {
    $rowTemplate['entity_id'] = $row->contact_id;
    drupal_write_record($group['table_name'], $rowTemplate);
  }
  
  // Update existing rows, field-by-field; this code does not depend on the type of contact being manipulated
  foreach ($group['fields'] as $field) {
    if (1 == @$supported_field_types[$field['data_type']][$field['html_type']]) {
      $sql = sprintf('update %s set %s = "%%s" where %s is null',
        $group['table_name'], $field['column_name'], $field['column_name']);
      // dpm(array('sql' => $sql, 'value' => $field['default_value']));
      db_query($sql, $field['default_value']);
    }
  }
}

/**
 * Get CDMD for the given group
 *
 * @param $gid The CiviCRM numeric ID of the group
 * @return FALSE or the array (encoded per arms_util_cdmd_build)
 */
function arms_util_cdmd_group_by_id(&$config, $gid) {
  if (is_numeric($gid)) {
    foreach ($config as $group_name => $group) {
      if ($group['id'] == $gid) {
        return $group;
      }
    }
  }
  return FALSE;
}
