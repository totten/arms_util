<?php

/**
 * (Drush callback)
 *
 * Prints contacts to STDOUT in a JSON-based format
 *
 * @param $contact_ids string CSV
 */
function arms_util_rawexpimp_drush_export($contact_ids) {
  civicrm_initialize();
  $exports = _arms_util_rawexpimp_export(explode(',', $contact_ids));
  // print_r($exports);
  print json_encode($exports);
}

/**
 * (Drush callback)
 *
 * Reads contacts from STDIN in a JSON-based format
 */
function arms_util_rawexpimp_drush_import() {
  $exports = json_decode(file_get_contents('php://stdin'), TRUE);
  civicrm_initialize();
  
  //print_r($exports);
  foreach ($exports as $export) {
    printf("Import contact #%d: start\n", $export['contact_id']);
    $result = _arms_util_rawexpimp_import($export);
    if (civicrm_error($result)) {
      print_r($result);
    }
    printf("Import contact #%d: finish\n", $export['contact_id']);
  }
}

/**
 * (Drush callback)
 *
 * @param $contact_ids array of int
 * @return array
 */
function _arms_util_rawexpimp_export($contact_ids) {
  $exports = array();

  foreach ($contact_ids as $contact_id) {
    $export = array(
      'contact_id' => $contact_id,
    );
    
    // get basic info
    $params = array(
      'contact_id' => $contact_id,
      'contact_type' => 'Individual',
    );
    require_once 'api/v2/Contact.php';
    $result = civicrm_contact_search($params);
    $export['civicrm_contact_search'] = $result[$contact_id];
  
    // get custom data fields
    $params = array_merge($params, _arms_util_rawexpimp_contact_fields());
    require_once 'api/v2/Contact.php';
    $result = civicrm_contact_search($params);
    $export['civicrm_contact_search'] = array_merge($result[$contact_id], $export['civicrm_contact_search']);
    // print_r(array('result' => $result, 'params' => $params));
    
    $params = array(
      'contact_id' => $contact_id,
      'contact_type' => 'Individual',
    );
    require_once 'api/v2/Location.php';
    $result = civicrm_location_get($params);
    $export['civicrm_location_get'] = $result;
    
    $params = array(
      'contact_id' => $contact_id,
      'contact_type' => 'Individual',
    );
    require_once 'api/v2/Activity.php';
    $result = civicrm_activities_get_contact($params);
    $export['civicrm_activities_get_contact'] = $result['result'];
    
    $exports[] = $export;
  }
  
  return $exports;
}

function _arms_util_rawexpimp_contact_fields() {
  require_once 'CRM/Core/BAO/CustomField.php';
  $fields = CRM_Core_BAO_CustomField::getFields('Individual');
  $result = array();
  foreach ($fields as $id => $field) {
    $result['return.custom_' . $id] = 1;
  }
  return $result;
}

/**
 * Process exported data
 *
 * @param $export 
 * @return TRUE or an array with error data
 */
function _arms_util_rawexpimp_import($export) {
  if (! db_result(db_query('select id from civicrm_contact where id = %d', $export['contact_id']))) {
    db_query('insert into civicrm_contact (id) values (%d)', $export['contact_id']);
  }
  
  $wtf_hacks_sql = array(); // array of SQL queries; each query is an array like ('update ... where id = %d', $some_id)

  require_once 'api/v2/Contact.php';
  require_once 'CRM/Core/BAO/CustomField.php';
  $contact_add = $export['civicrm_contact_search'];
  foreach ($contact_add as $key => $value) {
    $field = arms_util_field_by_param($key);
    if ($field['data_type'] == 'File') {
      // For Civi API outputs (like $exports), the Civi API provides a file
      // ID. For Civi API inputs, it expects an array (including file name
      // and type). For our use-case of re-importing deleted contacts, it's
      // better to use the file ID, but this requires hackery.
      unset($contact_add[$key]);
      $table = $field['_table_name'];
      $column = $field['column_name'];
      $wtf_hacks_sql[] = array("update {$table} set {$column} = %d where entity_id = %d", $value, $contact_add['contact_id']);
    }
  }
  //print_r(array('contact_add' => $contact_add));
  $result = civicrm_contact_add($contact_add);
  if (civicrm_error($result)) { return $result; }

  foreach ($wtf_hacks_sql as $sql_args) {
    call_user_func_array('db_query', $sql_args);
  }

  foreach ($export['civicrm_location_get'] as $loc) {
    db_query('delete from civicrm_phone where contact_id = %d', $export['contact_id']);
    require_once 'CRM/Core/DAO/Phone.php';
    foreach ($loc['phone'] as $phone) {
      $dao = new CRM_Core_DAO_Phone();
      $dao->copyValues($phone);
      $dao->insert();
    }
    
    db_query('delete from civicrm_email where contact_id = %d', $export['contact_id']);
    require_once 'CRM/Core/DAO/Email.php';
    foreach ($loc['email'] as $email) {
      $dao = new CRM_Core_DAO_Email();
      $dao->copyValues($email);
      $dao->insert();
    }

    db_query('delete from civicrm_address where contact_id = %d', $export['contact_id']);
    require_once 'CRM/Core/DAO/Address.php';
    $dao = new CRM_Core_DAO_Address();
    $dao->copyValues($loc['address']);
    $dao->insert();
  }
  
  foreach ($export['civicrm_activities_get_contact'] as $act_id => $activity) {
    $is_target = (in_array($export['contact_id'], $activity['targets']));
    $is_assignee = (in_array($export['contact_id'], $activity['assignees']));
    $is_source = ($export['contact_id'] == $activity['source_contact_id']);
    
    // note: we can't expect to handle all contacts on the activity
    // we're only concerned with this one contact
    if (! db_result(db_query('select id from civicrm_activity where id = %d', $act_id))) {
      $params = $activity;
      if ($is_target) {
         $params['targets'] = array($export['contact_id']); // this activity was deleted; don't re-associate with competing contacts
      }
      if ($is_assignee) {
        unset($params['assignees']); // this activity was deleted; don't re-associate with competing contacts
      }
      require_once 'api/v2/Activity.php';
      $result = civicrm_activity_create($params);
      if (civicrm_error($result)) { return $result; }
    }
    if ($is_target && ! db_result(db_query('select id from civicrm_activity_target where target_contact_id = %d and activity_id = %d', $export['contact_id'], $act_id))) {
      db_query('insert into civicrm_activity_target(target_contact_id,activity_id) values (%d,%d)',
        $export['contact_id'], $act_id);
    }
    if ($is_assignee && ! db_result(db_query('select id from civicrm_activity_assignment where assignee_contact_id = %d and activity_id = %d', $export['contact_id'], $act_id))) {
      db_query('insert into civicrm_activity_assignment (assignee_contact_id,activity_id) values (%d,%d)',
        $export['contact_id'], $act_id);
    }
  }
  
  return TRUE;
}
