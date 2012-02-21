<?php

// Note: Error detection is affected by http://drupal.org/node/195812 and can be patched by http://drupal.org/node/298768

/**
 * SQL Entity I/O with support for CiviCRM custom-data model
 *
 * @param $params array with keys:
 *   - entity: SQL table name
 *   - action: insert, update, insert-update, delete, select
 *   - data: key-value pairs, incl "id" and "custom_123" for custom data (action==insert/update/delete)
 * @return array with keys:
 *   - entity: sqlTableName
 *   - data: key-value-pairs; note: for 'insert', the 'data' will be updated with entity id (action==insert/update/delete)
 *   - select: ARMS_Util_Select with full complement of single-value custom data columns (action==select)
 * @throws Exception
 */
function _arms_util_thinapi($params) {
  arms_util_include_api('array');
  arms_util_include_api('query');
  
  $idColumn = 'id';
  
  switch ($params['action']) {
    case 'insert':
      $dataByGroup = arms_util_array_partition_func($params['data'], '_arms_util_thinapi_getFieldGroup');
      arms_util_insert($params['entity'])->addValues($dataByGroup['core'])->toDrupalQuery();
      if ($error = db_error()) { throw new Exception(sprintf('Failed to insert entity (%s): %s', $params['entity'], $error)); }
      $params['data'][$idColumn] = db_last_insert_id($params['entity'], $idColumn);
      _arms_util_thinapi_customSave($params['entity'], $params['data'][$idColumn], $dataByGroup);
      break;
    case 'update':
      $dataByGroup = arms_util_array_partition_func($params['data'], '_arms_util_thinapi_getFieldGroup');
      if (count(array_keys($dataByGroup['core'])) > 1) {
        db_query(arms_util_update($params['entity'])->addWheref("${idColumn}=%d", $params['data'][$idColumn])->addValues($dataByGroup['core'])->toSQL());
        if ($error = db_error()) { throw new Exception(sprintf('Failed to update entity (%s): %s', $params['entity'], $error)); }
      }
      _arms_util_thinapi_customSave($params['entity'], $params['data'][$idColumn], $dataByGroup);
      break;
    case 'insert-update':
      $dataByGroup = arms_util_array_partition_func($params['data'], '_arms_util_thinapi_getFieldGroup');
      if (empty($dataByGroup['core'][$idColumn])) {
        $params['action'] = 'insert';
        return _arms_util_thinapi($params);
      } else {
        $updates = $dataByGroup['core'];
        unset($updates[$idColumn]);
        
        $insert = arms_util_insert($params['entity'], 'update')
          ->addValue($idColumn, $dataByGroup['core'][$idColumn], 'insert-only')
          ->addValues($updates, 'insert-update');
        $insert->toDrupalQuery();
        if ($error = db_error()) { throw new Exception(sprintf('Failed to insert-update entity (%s): %s', $params['entity'], $error)); }
      }
      _arms_util_thinapi_customSave($params['entity'], $params['data'][$idColumn], $dataByGroup);

      break;
    case 'delete':
      $dataByGroup = arms_util_array_partition_func($params['data'], '_arms_util_thinapi_getFieldGroup');
      // With civicrm_contact, deletes cascade -- so we don't have to explicitly delete custom-data groups.
      db_query(sprintf('DELETE FROM %s WHERE %s = %d', $params['entity'], $idColumn, $params['data'][$idColumn]));
      if ($error = db_error()) { throw new Exception(sprintf('Failed to delete entity (%s): %s', $params['entity'], $error)); }
      break;
    case 'select':
      $select = arms_util_query($params['entity']);
      $select->addSelect('{'.$params['entity'].'}.*');
      $fields = arms_util_thinapi_getFields($params['entity']);
      foreach ($fields as $field) {
        $select->addCustomField("{".$params['entity']."}.id", $field, 'custom_' . $field['id']);
      }
      return array(
        'entity' => $params['entity'],
        'select' => $select,
      );
      break;
  }
  
  return array(
    'entity' => $params['entity'],
    'data' => $params['data'],
  );
}

/**
 * Insert or update any custom-value records
 *
 * @param $type string
 * @param $lid int the ID of the primary entity
 * @param $dataByGroup array(groupName => array(custom_123 => fieldValue))
 */
function _arms_util_thinapi_customSave($type, $lid, $dataByGroup) {
  foreach ($dataByGroup as $groupName => $groupValues) {
    if ($groupName == 'core') continue;
    $group = arms_util_group($groupName);
    $columnValues = arms_util_array_rekey_rosetta($group['fields'], '_param', 'column_name', $groupValues);

    $idColumn = 'entity_id';
    $insert = arms_util_insert($group['table_name'], 'update')
      ->addValue($idColumn, $lid, 'insert-only')
      ->addValues($columnValues, 'insert-update');
    
    $insert->toDrupalQuery();
    if ($error = db_error()) { throw new Exception(sprintf('Failed to insert-update custom data (%s:%s): %s', $type, $groupName, $error)); }
  }
}

/**
 * Determine which custom-data group stores a given field
 *
 * @param $key field name, e.g. 'first_name' or 'custom_123'
 * @param $value field value
 * @return FALSE, 'core', or group-name
 */
function _arms_util_thinapi_getFieldGroup($key, $value) {
  if (preg_match('/^custom_(\d+)$/', $key, $matches)) {
    $field = arms_util_field_by_id($matches[1]);
    if (is_array($field)) {
      return $field['_group_name'];
    } else {
      return FALSE;
    }
  } else {
    return 'core';
  }
}

/**
 * Lookup any single-value custom-data fields
 *
 * @param $entityType string
 * @return array(customFieldSpec), each customFieldSpec is formatted per arms_util_field
 */
function arms_util_thinapi_getFields($entityType) {
  $entityMap = array( // array(yassEntityType => array(civiEntityType))
    'civicrm_contact' => array('Contact','Individual','Household','Organization'),
    'civicrm_activity' => array('Activity'),
  );
  if (!is_array($entityMap[$entityType])) {
    return array();
  }
  $fields = array();
  foreach ($entityMap[$entityType] as $civiEntityType) {
    $groups = arms_util_groups($civiEntityType);
    if (empty($groups)) continue;
    foreach ($groups as $groupName => $group) {
      if ($groupName == 'engagement') continue; // FIXME ugly
      if (empty($group['fields'])) { continue;}
      $fields = array_merge($fields, array_values($group['fields']));
      // $fields = $fields + arms_util_array_index(array('_full_name'), $group['fields']);
      // $fields = $fields + arms_util_array_index(array('id'), $group['fields']);
    }
  }
  return $fields;
}
