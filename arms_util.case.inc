<?php

/**
 * @file
 *
 * Helpers, adapters, and other sugar for process-automation involving cases
 */

define(ARMS_UTIL_CASE_STATUS_OPEN, 1);
define(ARMS_UTIL_CASE_STATUS_CLOSED, 2);

/**
 * (Delegated) Implementation of hook_civicrm_post
 */
function _arms_util_case_civicrm_post($op, $objectName, $objectId, &$objectRef) {
  switch ($objectName) {
    case 'Activity':
      $caseId = db_result(db_query('SELECT case_id
        FROM {civicrm_case_activity}
        WHERE activity_id in (%d,%d)',
        $objectId, @$objectRef->original_id));
      // dpm(array('_arms_util_case_civicrm_post', $op, $objectName, $objectId, $objectRef, 'case id ['.$caseId.']'));
      // dpm(sprintf('_arms_util_case_civicrm_post: %s.%s objectId=%d case=%d', $objectName, $op, $objectId, $caseId));
      if ($caseId) {
        arms_util_case_refresh($caseId);
      } else {
        // dpm(array('failed to lookup case id', $activityId, $caseId, $objectId, $objectRef));
      }
      break;
    case 'Case':
      // dpm(sprintf('_arms_util_case_civicrm_post: %s.%s objectId=%d', $objectName, $op, $objectId));
      arms_util_case_refresh($objectId);
      break;
    default:
  }
}

/**
 * Schedule a refresh
 *
 * @param $caseId int
 */
function arms_util_case_refresh($caseId) {
  // PRECAUTION: avoid recursive refreshes -- reduce mental effort for downstream
  global $_arms_util_case_refresh_lock;
  if ($_arms_util_case_refresh_lock[$caseId]) {
    return;
  }
  
  arms_util_post_commit(array(
    'name' => 'case_refresh_' . $caseId,
    'func' => '_arms_util_case_refresh',
    'args' => array($caseId),
  ));
}

/**
 * Look up a case using an activity ID
 *
 * @param $activity_id
 * @return int, case ID
 */
function arms_util_case_findByActivity($activity_id, $original_id = FALSE) {
  $original_id = db_result(db_query('SELECT original_id 
    FROM {civicrm_activity}
    WHERE id = %d', $activity_id));
  return db_result(db_query('SELECT case_id
    FROM {civicrm_case_activity}
    WHERE activity_id in (%d,%d)',
    $activity_id, $original_id));
}

/**
 * (Post-Commit Callback)
 *
 * Trigger any case-refresh logic using hook_arms_case_refresh
 *
 * Note: Any hook implementations will most likely need the case-type and a copy of the case record, so
 * we look those in a sane format in advance.
 *
 * @param $caseId int
 */
function _arms_util_case_refresh($caseId) {
  civicrm_initialize();
  $result = civicrm_api('case', 'get', array(
    'version' => 3,
    'id' => $caseId,
  ));
  
  require_once 'CRM/Case/PseudoConstant.php';
  $caseTypes = CRM_Case_PseudoConstant::caseType('name');
  
  // dpm(array('_arms_util_case_refresh', $caseId, $result));
  if (!civicrm_api3_error($result) && $result['count'] == 1) {
    // dpm('ok: [' . $caseId . ']');
    // PRECAUTION: avoid recursive refreshes -- reduce mental effort for downstream
    global $_arms_util_case_refresh_lock;
    $_arms_util_case_refresh_lock[$caseId] = TRUE;
    module_invoke_all('arms_case_refresh', $caseTypes[$result['values'][$caseId]['case_type_id']], $result['values'][$caseId]);
    unset($_arms_util_case_refresh_lock[$caseId]);
  } else {
    // dpm('fail: [' . $caseId .']');
    watchdog('arms_util', 'Failed to lookup case #@caseid', array('@caseid' => $caseId), WATCHDOG_ERROR);
  }
}

/**
 * (Optional helper for hook_arms_case_refresh) Refresh a case based on a linear pipeline
 *
 * This is somewhat similar to a "timeline" in CiviCase except, that (a) activity times
 * are not specified upfront, (b) the *completion* time of the previous activity determines
 * the *scheduled* time of the following activity and (c) default values can be supplied
 * for several different activity fields, incl. the assignee_contact_id.
 *
 * This is implemented as a helper function for two reason -- (1) so that it can be used as a building block
 * for more complex workflows and (2) because I don't want to commit to a UI yet.
 *
 * @param $caseType string
 * @param $case array-encoded entity, per hook_arms_case_refresh
 * @param $pipeline array; a general specification of the sequence of activities; keys are:
 *  - stages: array of stage-specifications, required; each includes 'activity_type' and 'defaults'
 *  - sync_status: boolean, optional; whether to update the case-status based on completion of the final stage
 * @return void
 */
function arms_util_case_refresh_pipeline($caseType, &$case, $pipeline) {
  if ($case['is_deleted']) { return; }

  arms_util_include_api('activity', 'array');

  // Index case activities so that we can do some quick existence-checking below
  // e.g. $acts['Phone Call']['Completed'][135] = array(...activity details...);
  $acts = arms_util_array_index(
    array('activity_type_name', 'activity_status_name', 'id'),
    arms_util_activity_search(array(
      'case_id' => array($case['id']),
      'return.activity_type_name' => 1,
      'return.activity_status_name' => 1,
      'return.assignee_contact_id' => 1,
      'return.custom_data_simple' => 1,
    ))
  );
  
  // Index case contacts so that we can do some quick existence-checking below
  // e.g. $contacts['Client'][246] = array(...contact details...)
  $contacts = arms_util_array_index(array('role', 'contact_id'), $case['contacts']);
  
  // dpm(array('refresh', 'type' => $caseType, 'case' => $case, 'acts' => $acts, 'contacts' => $contacts));

  // Determine case status: Has the final step been completed?
  if ($pipeline['sync_status'] && /* any manual overrides? */ empty($acts['Change Case Status']['Completed'])) {
    $final = arms_util_array_last($pipeline['stages']);
    $has_final_closed = !empty($acts[$final['activity_type']]['Completed']);
    $has_rejection = _arms_util_case_has_rejection($pipeline, $acts);
    $status_id = ($has_rejection || $has_final_closed) ? ARMS_UTIL_CASE_STATUS_CLOSED : ARMS_UTIL_CASE_STATUS_OPEN;
    if ($status_id != $case['status_id']) {
      $result = civicrm_api('Case', 'update', array(
        'version' => 3,
        'case_id' => $case['id'],
        'status_id' => $status_id,
      ));
      if (civicrm_api3_error($result)) {
        drupal_set_message(t('Failed to update case status: @error', array('@error' => $result['error_message'])), 'error');
        return;
      }
      module_invoke_all('arms_util_case_pipeline', array(
        'caseType' => $caseType,
        'case' => &$case,
        'pipeline' => $pipeline,
        'op' => 'status', 
        'old' => $case['status_id'], 
        'new' => $status_id,
        'activities' => &$acts,
        'contacts' => &$contacts,
        'outcome' => $has_rejection ? 'Reject' : 'Approve',
      ));
      $case['status_id'] = $status_id; 
    }
    if ($status_id == ARMS_UTIL_CASE_STATUS_CLOSED) {
      return;
    }
  }
    
  foreach ($pipeline['stages'] as $stage) {
    // Find any pre-existing, scheduled activities and softly apply defaults
    _arms_util_case_default_assignee($acts[$stage['activity_type']]['Scheduled'], $stage['defaults']['assignee_contact_id']);
    // TODO: Fill in any fields supplied in $stage['defaults']
    
    // If this activity hasn't been attempted yet, then auto-create an appropriate record
    if (arms_util_array_hollow($acts[$stage['activity_type']])) {
      $defaults = array(
        'version' => 3,
        'case_id' => $case['id'],
        'activity_type_id' => arms_util_activity_type($stage['activity_type']),
        'source_contact_id' => FALSE,
        'assignee_contact_id' => FALSE,
        'target_contact_id' => FALSE,
        'status_id' => array_search('Scheduled', CRM_Core_PseudoConstant::activityStatus('name')),
        'medium_id' => array_search('not_applicable', CRM_Case_PseudoConstant::encounterMedium('name')),
        'activity_date_time' => date('YmdHis', arms_util_time()),
      );
      $params = array_merge($defaults, $stage['defaults']);
      $result = civicrm_api('Case', 'activity_create', $params);
      if (civicrm_api3_error($result)) {
        if (function_exists('dpm')) { dpm(array('activity_create', 'stage' => $stage, 'params' => $params, 'result' => $result)); }
        drupal_set_message(t('Failed to generate @type: @error', array(
          '@type' => $stage['activity_type'],
          '@error' => $result['error_message'],
        )), 'error');
        return;
      } 
    }
    
    if (empty($acts[$stage['activity_type']]['Completed'])) { 
      break;
    }
  }
  
  // FIXME address the case where an earlier step was reopened after scheduling a later step
  // if (empty($acts['Intake Form']['Completed']) && !empty($acts['Final Review']['Scheduled'])) {
  //  foreach ($acts['Final Review']['Scheduled'] as $act_id => $act) {
  //    $result = civicrm_api('Activity', 'update', array(
  //      'version' => 3,
  //      'id' => $act_id,
  //      'status_id' => array_search('Cancelled', CRM_Core_PseudoConstant::activityStatus('name')),
  //    ));
  //  }
  //}
}

/**
 * Determine the outcome of the series of reviews.
 *
 * If any 'Completed' activity within any stage is 'Reject'ed, then
 * the overall case is 'Reject'ed
 *
 * @param $acts array of activities, indexed by (type,status,id)
 */
function _arms_util_case_has_rejection($pipeline, $acts) {
  foreach ($pipeline['stages'] as $stage) {
    if (!$stage['outcome_field']) { 
      continue;
    }
    $param = arms_util_field_param($stage['outcome_field']);
    if (is_array($acts[$stage['activity_type']]['Completed'])) {
      foreach ($acts[$stage['activity_type']]['Completed'] as $id => $act) {
        if ($act[$param] == 'Reject') {   
          return TRUE;
        }
      }
    } 
  } 
  return FALSE;
}

/**
 * If any of the listed activities are unassigned, then assign them
 */
function _arms_util_case_default_assignee(&$acts, $contact_ids) {
  if (!empty($acts)) {
    foreach ($acts as $id => &$act) {
      if (empty($act['assignee_contact_id'])) {
        $act['assignee_contact_id'] = $contact_ids;
        arms_util_activity_assignments_set($act['id'], $act['assignee_contact_id']);
      }
    }
  }
}
