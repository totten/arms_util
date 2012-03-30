<?php

/**
 * Drush callback
 */
function _arms_util_drush_clear() {
  cache_clear_all();
  drupal_get_schema(FALSE, TRUE);
  drupal_flush_all_caches();

  module_invoke_all('arms_clear');
  
  if (function_exists('_block_rehash')) {
    _block_rehash();
  } else {
    print "Failed to update blocks\n";
  }
}

/**
 * Drush callback
 */
function _arms_util_drush_execute($file, $sitename = 'unknown') {
  global $arms_util_drush_sitename;
  global $arms_util_drush_execute;
  $args = func_get_args();
  array_shift($args);
  array_shift($args);
  
  $arms_util_drush_execute = array(
    'file' => $file,
    'sitename' => $sitename,
    'args' => $args,
  );
  require_once $file;
}

/**
 * Drush callback
 *
 * Print all contacts in CSV format
 */
function arms_util_drush_contacts() {
  civicrm_initialize();
  arms_util_include_api('query');
  
  // Build the query
  $select = arms_util_query('civicrm_contact');
  $select->addSelect("civicrm_contact.*");
  foreach (array('Contact','Individual','Household','Organization') as $entity) {
    $groups = arms_util_groups($entity);
    foreach ($groups as $groupName => $group) {
      if ($groupName == 'engagement') continue;
      foreach ($group['fields'] as $field) {
        $select->addCustomField('civicrm_contact.id', $field, $field['_group_name'] .'_'. $field['column_name']);
      }
    }
  }
  $locTypes = CRM_Core_PseudoConstant::locationType();
  foreach ($locTypes as $locTypeId => $locTypeName) {
    $mungedName = 'loc_'.strtolower(preg_replace('/[^a-zA-Z0-9]/', '_', $locTypeName));
    $select->addJoinf("left join civicrm_email email_{$locTypeId} on (civicrm_contact.id = email_{$locTypeId}.contact_id and email_{$locTypeId}.location_type_id=%d)", $locTypeId);
    $select->addSelect("email_{$locTypeId}.email as {$mungedName}_email");
    $select->addJoinf("left join civicrm_phone phone_{$locTypeId} on (civicrm_contact.id = phone_{$locTypeId}.contact_id and phone_{$locTypeId}.location_type_id=%d)", $locTypeId);
    $select->addSelect("phone_{$locTypeId}.phone as {$mungedName}_phone");
    $select->addJoinf("left join civicrm_address address_{$locTypeId} on (civicrm_contact.id = address_{$locTypeId}.contact_id and address_{$locTypeId}.location_type_id=%d)", $locTypeId);
    $select->addSelect("address_{$locTypeId}.street_address as {$mungedName}_street_address");
    $select->addSelect("address_{$locTypeId}.city as {$mungedName}_city");
    $select->addSelect("address_{$locTypeId}.postal_code as {$mungedName}_postal_code");
    $select->addSelect("address_{$locTypeId}.country_id as {$mungedName}_country");
    $select->addSelect("address_{$locTypeId}.state_province_id as {$mungedName}_state_province");
  }
  
  // Run the query
  $fh = fopen('php://stdout','w');
  if (!$fh) {
    printf("Error opening output\n");
    exit(1);
  }
  $header = FALSE;
  $q = db_query($select->toSQL());
  $countries = CRM_Core_PseudoConstant::country();
  $states = CRM_Core_PseudoConstant::stateProvince();
  while ($row = db_fetch_array($q)) {
    unset($row['hash']);
    if (!$header) {
      $header = array_keys($row);
      fputcsv($fh, $header);
    }
    
    foreach (array_keys($row) as $key) {
      $value = $row[$key];
      if (preg_match('/loc_.*_country/', $key)) {
        $row[$key] = $countries[$value];
      } elseif (preg_match('/loc_.*_state_province/', $key)) {
        $row[$key] = $states[$value];
      } else {
        $row[$key] = strtr($value, CRM_Core_DAO::VALUE_SEPARATOR, '|');
      }
    }
    fputcsv($fh, $row);
  }
  
  fclose($fh);
}
