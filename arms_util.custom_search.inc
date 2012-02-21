<?php
/**
 * ARMS Utility - Custom Search Registration
 *
 * The "custom search" mechanism in Civi is a semi-documented/semi-supported API
 * for implementing special-purposes search pages. It provides support for
 * executing tasks, saving search results, etc. However, it also requires manually
 * registering and tracking a "search ID". This file defines a helper mechanism
 * to simplify that.
 *
 * In the initial implementation, we will auto-create but not auto-destroy
 * custom search records. There may be "saved searches" which reference the
 * custom search ID, and it's important to preserve these ID's even when
 * the module is disabled-and-re-enabled.
 */

/**
 * Register a custom search form 
 *
 * @return The ID for the search form (i.e. the "value" of the OptionValue for the search form)
 */
function arms_util_custom_search_register($searchClass, $description) {
  arms_util_include_api('option');
  $optionValue = array();
  $optionValue['name'] = $searchClass;
  $optionValue['label'] = $searchClass;
  $optionValue['description'] = $description;

  arms_util_include_api('option');
  $result = arms_util_option_value_create_seq(
    arms_util_option_group_by_name('custom_search'),
    $optionValue);

  arms_util_log(ARMS_UTIL_LOG_TMPFILE, WATCHDOG_INFO, $result);
  
  if (civicrm_error($result)) {
    drupal_set_message(
      t('Failed to correctly load custom search form, @class', array('@class' => $searchClass)), 
      'error');
    return;
  }
  else {
    variable_set(ARMS_TEAM_CUSTOM_SEARCH_VAR, $result['result']['value']);
    return $result['result']['value'];
  }
}

/**
 * Determine the custom search record for the given class
 *
 * @return array with the custom search record (OptionValue), or NULL if not registered
 */
function arms_util_custom_search_lookup($searchClass) {
  arms_util_include_api('option');
  $optionGroup = arms_util_option_group_by_name('custom_search');
  $query = db_query('select * from {civicrm_option_value} cov'
    . ' where cov.option_group_id = %d'
    . ' and cov.name = "%s"',
    $optionGroup['id'],
    $searchClass);
  if ($optionValue = db_fetch_array($query)) {
    return $optionValue;
  } else {
    return NULL;
  }
}

/**
 * Remove a custom search page
 *
 * @param $csid The unique ID of the form (as produced by arms_util_custom_search_register)
 */
function arms_util_custom_search_unregister($csid) {
  arms_util_include_api('option');
  
  if (! $csid) {
    return;
  }
  
  $optionGroup = arms_util_option_group_by_name('custom_search');

  $optionValue = array();
  $optionValue['option_group_id'] = $optionGroup['id'];
  $optionValue['value'] = $csid;
  return arms_util_option_value_delete($optionValue);
}
