<?php

/**
 * Search for activities;
 *
 * @param $params Accepts the same params as _arms_util_activity_search_prepare, plus:
 *   - return.custom_data               A boolean indicating whether to return custom data fields; default FALSE
 *   - return.custom_data_simple        A boolean indicating whether to return custom data fields; default FALSE; guaranteed to return "custom_123" not "custom_123_45"
 *   - return.source_contact		A boolean indicating whether to return the (abridged) source contact record
 *   - return.target_contact		A boolean indicating whether to return an array of (abridged) target contact records
 *   - return.target_contact_id         A boolean indicating whether to return an array of target contact ID's
 *   - return.assignee_contact		A boolean indicating whether to return an array of (abridged) assignee contacts records
 *   - return.assignee_contact_id       A boolean indicating whether to return an array of assignee contact ID's
 *   - return.activity_type_name        A boolean indicating whether to return the type as a name (string)
 *   - return.activity_status_name      A boolean indicating whether to return the status as a name (string)
 *
 * If "target_contact" and/or "assignee_contact" is returned, each will be an array keyed by contact ID. The values will
 * be abridged contact records with keys (contact_id, contact_type, display_name, sort_name).
 *
 * @see _arms_util_activity_search_prepare()
 */
function arms_util_activity_search($params) {
  $result = array();
  $sql = _arms_util_activity_search_prepare($params);
  if (!$sql) {
    return array();
  }
  $q = db_query($sql);
  while ($activity = db_fetch_array($q)) {
    if (@$params['return.custom_data']) {
      $cd_params = array(
        'activity_id' => $activity['id'],
        'activity_type_id' => $activity['activity_type_id']
      );
      require_once 'api/v2/Activity.php';
      $cd = civicrm_activity_custom_get($cd_params);
      $activity = array_merge($activity, civicrm_activity_custom_get($cd_params));
    }
    if (@$params['return.custom_data_simple']) {
      $cd_params = array(
        'activity_id' => $activity['id'],
        'activity_type_id' => $activity['activity_type_id']
      );
      require_once 'api/v2/Activity.php';
      $custom = civicrm_activity_custom_get($cd_params);
      foreach ($custom as $key => $value) {
        $new = preg_replace('/^(custom_[0-9]+)_.*$/', '$1', $key);
        $activity[$new] = $custom[$key];
      }
    }
    if (@$params['return.source_contact']) {
      $source_q = db_query('select contact.id as contact_id, contact.contact_type, contact.display_name, contact.sort_name
        from {civicrm_contact} contact
        where id = %d',
        $activity['source_contact_id']);
      while ($source = db_fetch_array($source_q)) {
        $activity['source_contact'] = $source;
      }
    }
    if (@$params['return.target_contact']) {
      $activity['target_contact'] = array();
      $target_q = db_query('select contact.id as contact_id, contact.contact_type, contact.display_name, contact.sort_name
        from {civicrm_activity_target} cat 
        inner join {civicrm_contact} contact on cat.target_contact_id = contact.id
        where activity_id = %d',
        $activity['id']);
      while ($target = db_fetch_array($target_q)) {
        $activity['target_contact'][$target['contact_id']] = $target;
      }
    }
    if (@$params['return.target_contact_id']) {
      $activity['target_contact_id'] = array();
      $target_q = db_query('select cat.target_contact_id contact_id
        from {civicrm_activity_target} cat 
        where activity_id = %d',
        $activity['id']);
      while ($target = db_fetch_array($target_q)) {
        $activity['target_contact_id'][] = $target['contact_id'];
      }
    }
    if (@$params['return.assignee_contact']) {
      $activity['assignee_contact'] = array();
      $assignee_q = db_query('select contact.id as contact_id, contact.contact_type, contact.display_name, contact.sort_name
        from {civicrm_activity_assignment} cat 
        inner join {civicrm_contact} contact on cat.assignee_contact_id = contact.id
        where activity_id = %d',
        $activity['id']);
      while ($assignee = db_fetch_array($assignee_q)) {
        $activity['assignee_contact'][$assignee['contact_id']] = $assignee;
      }
    }
    if (@$params['return.assignee_contact_id']) {
      $activity['assignee_contact_id'] = array();
      $assignee_q = db_query('select cat.assignee_contact_id contact_id
        from {civicrm_activity_assignment} cat 
        where activity_id = %d',
        $activity['id']);
      while ($assignee = db_fetch_array($assignee_q)) {
        $activity['assignee_contact_id'][] = $assignee['contact_id'];
      }
    }
    if (@$params['return.activity_type_name']) {
      $activity['activity_type_name'] = arms_util_activity_type_by_id($activity['activity_type_id']);
    }
    if (@$params['return.activity_status_name']) {
      $activity['activity_status_name'] = arms_util_activity_status_by_id($activity['status_id']);
    }
    $result[$activity['id']] = $activity;
  }
  
  return $result;
}

/**
 * Determine the number of activities which match the given criteria.
 *
 * @param $params See _arms_util_activity_search_prepare,
 * @return FALSE if there is an error; otherwise, a number
 * @see _arms_util_activity_search_prepare()
 */
function arms_util_activity_count($params) {
  $params['return.count'] = TRUE;
  $result = array();
  $sql = _arms_util_activity_search_prepare($params);
  if (!$sql) {
    return FALSE;
  }
  $q = db_query($sql);
  $cnt = db_fetch_object($q);
  return $cnt->cnt;
}

/**
 * Prepare a search for activities.
 *
 * @param $params accepts the following keys:
 *   - activity_id           Array of acceptable activity ID's
 *   - activity_type_id      Array of acceptable type ID's
 *   - activity_type_name    Array of acceptable type names
 *   - activity_status_id    Array of acceptable status ID's
 *   - activity_status_name  Array of acceptable status names
 *   - activity_date_time    Array of acceptable date/times (note: don't mix with "date_range" -- that can be confusing)
 *   - case_id               Array of acceptable case ID's
 *   - contact_id            Array of acceptable contact ID's
 *   - contact_role          Array of acceptable relations between contact and activity; "source", "target", "assignee"
 *   - source_contact_id     Array of acceptable source contact IDs; (note: don't mix with "contact_id"/"contact_role" -- that can be confusing)
 *   - target_contact_id     Array of acceptable target contact IDs; (note: don't mix with "contact_id"/"contact_role" -- that can be confusing)
 *   - assignee_contact_id   Array of acceptable assignee contact IDs; (note: don't mix with "contact_id"/"contact_role" -- that can be confusing)
 *   - location              Array of acceptable location strings/patterns
 *   - subject               Array of acceptable subject strings/patterns
 *   - filter                Array of advanced filter options. See below. (EGAD)
 *   - date_range            Array whihch sets timestamp filters; optional keys 'low' and 'high'; values in the array are seconds since epoch
 *   - is_current_revision   0, 1, or FALSE (disabled); default to "1"
 *   - is_deleted            0, 1, or FALSE (disabled); default to "0"
 *   - sort                  A string expression used for sorting
 *   - rowCount              An integer specifying max #rows to return
 *   - offset                An integer
 *   - return.count          A boolean indicating whether to return a count or a list of records
 *   - return.select         An array of SQL string
 *
 * This is core of arms_util_activity_search() and arms_util_activity_count().
 *
 * The 'filter' format is opaque. To create a filter, use one of these utility functions:
 *  - arms_util_activity_filter_name($role, $name)
 *  - arms_util_activity_filter_custom_multi($role, $field, $or_values)
 *
 * TODO adapt date filters from arms_comp_comm_get_activities to be part of $activityParams
 * Note: Does not retrieve custom data fields. api/v2/Activity should provide a call for that.
 */
function _arms_util_activity_search_prepare($params) {
  // This would be so much easier with something like Hibernate Critera or
  // D7 DatabaseCondition
  arms_util_include_api('query');

  $wheres = array(); // list SQL fragments; these will be AND'd  
  $joinsAvail = array(); // list of SQL fragments, keyed by table alias
  $joinsActive = array(); // keys are table aliases, values are 1
  $groupBys = array(); // list of SQL fragments; these will be implode(',')ed

  $joinsAvail['civicrm_activity_target'] = 'left join {civicrm_activity_target} civicrm_activity_target on civicrm_activity_target.activity_id = civicrm_activity.id';
  $joinsAvail['civicrm_activity_assignment'] = 'left join {civicrm_activity_assignment} civicrm_activity_assignment on civicrm_activity_assignment.activity_id = civicrm_activity.id';
  $joinsAvail['civicrm_case_activity'] = 'left join {civicrm_case_activity} civicrm_case_activity on civicrm_case_activity.activity_id = civicrm_activity.id';

  if (is_array($params['contact_id'])) {
    $params['contact_id'] = array_filter($params['contact_id'], 'is_numeric');
    if (empty($params['contact_id'])) {
      return FALSE;
    }

    $contactWheres = array(); // list of SQL fragments; these will be OR'd
    if (is_array($params['contact_role'])) {
      $roles = drupal_map_assoc($params['contact_role']);
    } else {
      $roles = drupal_map_assoc(array('source', 'target', 'assignee'));
    }
  
    if (isset($roles['source'])) {
      $ids = implode(',', $params['contact_id']);
      $contactWheres[] = 'source_contact_id IN (' . $ids . ')';
    }
    
    if (isset($roles['target'])) {
      $ids = implode(',', $params['contact_id']);
      $contactWheres[] = 'civicrm_activity_target.target_contact_id IN (' . $ids . ')';
      $joinsActive['civicrm_activity_target'] = 1;
    }
  
    if (isset($roles['assignee'])) {
      $ids = implode(',', $params['contact_id']);
      $contactWheres[] = 'civicrm_activity_assignment.assignee_contact_id IN (' . $ids . ')';
      $joinsActive['civicrm_activity_assignment'] = 1;
    }
    
    $wheres []= '(' . join(' OR ', $contactWheres) . ')';
  }

  if (is_array($params['source_contact_id'])) {
    $params['source_contact_id'] = array_filter($params['source_contact_id'], 'is_numeric');
    if (empty($params['source_contact_id'])) {
      return FALSE;
    }

    $ids = implode(',', $params['source_contact_id']);
    $wheres[] = 'source_contact_id IN (' . $ids . ')';
  }

  if (is_array($params['target_contact_id'])) {
    $params['target_contact_id'] = array_filter($params['target_contact_id'], 'is_numeric');
    if (empty($params['target_contact_id'])) {
      return FALSE;
    }

    $ids = implode(',', $params['target_contact_id']);
    $wheres[] = 'civicrm_activity_target.target_contact_id IN (' . $ids . ')';
    $joinsActive['civicrm_activity_target'] = 1;
  }

  if (is_array($params['assignee_contact_id'])) {
    $params['assignee_contact_id'] = array_filter($params['assignee_contact_id'], 'is_numeric');
    if (empty($params['assignee_contact_id'])) {
      return FALSE;
    }

    $ids = implode(',', $params['assignee_contact_id']);
    $wheres[] = 'civicrm_activity_assignment.assignee_contact_id IN (' . $ids . ')';
    $joinsActive['civicrm_activity_assignment'] = 1;
  }

  if (is_array($params['activity_id'])) {
    $params['activity_id'] = array_filter($params['activity_id'], 'is_numeric');
    if (empty($params['activity_id'])) {
      return FALSE;
    }
    $ids = implode(',', $params['activity_id']);
    $wheres[] = 'civicrm_activity.id IN (' . $ids . ')';
  }

  if (is_array($params['activity_type_name'])) {
    if (empty($params['activity_type_name'])) {
      return FALSE;
    }
    // translate names into id's
    $params['activity_type_id'] = array();
    foreach ($params['activity_type_name'] as $activity_type_name) {
      $params['activity_type_id'][] = arms_util_activity_type($activity_type_name);
    }
    unset($params['activity_type_name']);
  }
  
  if (is_array($params['activity_type_id'])) {
    $params['activity_type_id'] = array_filter($params['activity_type_id'], 'is_numeric');
    if (empty($params['activity_type_id'])) {
      return FALSE;
    }
    $ids = implode(',', $params['activity_type_id']);
    $wheres[] = 'activity_type_id IN (' . $ids . ')';
  }
  
  if (is_array($params['activity_status_name'])) {
    if (empty($params['activity_status_name'])) {
      return FALSE;
    }
    // translate names into id's
    $params['activity_status_id'] = array();
    foreach ($params['activity_status_name'] as $activity_status_name) {
      $params['activity_status_id'][] = arms_util_activity_status($activity_status_name);
    }
    unset($params['activity_status_name']);
  }

  if (is_array($params['activity_status_id'])) {
    $params['activity_status_id'] = array_filter($params['activity_status_id'], 'is_numeric');
    if (empty($params['activity_status_id'])) {
      return FALSE;
    }
    $ids = implode(',', $params['activity_status_id']);
    $wheres[] = 'status_id IN (' . $ids . ')';
  }
  
  if (is_array($params['activity_date_time'])) {
    $params['activity_date_time'] = array_filter($params['activity_date_time'], 'is_numeric');
    
    arms_util_include_api('date');
    $times = implode('","', array_map('arms_util_date_format_sql', $params['activity_date_time']));
    $wheres[] = 'activity_date_time IN ("' . $times . '")';
  }

  if (is_array($params['case_id'])) {
    $params['case_id'] = array_filter($params['case_id'], 'is_numeric');
    if (empty($params['case_id'])) {
      return FALSE;
    }

    $ids = implode(',', $params['case_id']);
    $wheres[] = 'civicrm_case_activity.case_id IN (' . $ids . ')';
    $joinsActive['civicrm_case_activity'] = 1;
  }
  
  if (isset($params['date_range']['low'])) {
    if (!is_numeric($params['date_range']['low'])) {
      return FALSE;
    }
    arms_util_include_api('date');
    $wheres[] = 'activity_date_time >= "' . arms_util_date_format_sql($params['date_range']['low']) . '"';
  }
  
  if (isset($params['date_range']['high'])) {
    if (!is_numeric($params['date_range']['high'])) {
      return FALSE;
    }
    arms_util_include_api('date');
    $wheres[] = 'activity_date_time <= "' . arms_util_date_format_sql($params['date_range']['high']) . '"';
  }
  
  if (is_array($params['location'])) {
    $location_sqls = array();
    foreach ($params['location'] as $location) {
      $location_sqls[] = arms_util_query_sprintf('location like "%s"', $location);
    }
    $wheres[] = arms_util_query_or($location_sqls);
  }
  if (is_array($params['subject'])) {
    $subject_sqls = array();
    foreach ($params['subject'] as $subject) {
      $subject_sqls[] = arms_util_query_sprintf('subject like "%s"', $subject);
    }
    $wheres[] = arms_util_query_or($subject_sqls);
  }
  if (!isset($params['is_current_revision'])) $params['is_current_revision'] = 1;
  if ($params['is_current_revision'] !== FALSE) {
    $wheres[] = arms_util_query_sprintf('is_current_revision = %d', $params['is_current_revision']);
  }
  if (!isset($params['is_deleted'])) $params['is_deleted'] = 0;
  if ($params['is_deleted'] !== FALSE) {
    $wheres[] = arms_util_query_sprintf('is_deleted = %d', $params['is_deleted']);
  }
  if (!$params['return.count']) {
    $groupBys[] = 'civicrm_activity.id';
  }
  
  if (is_array($params['filter'])) {
    foreach ($params['filter'] as $filter) {
      $wheres = array_merge($wheres, $filter['wheres']);
      $joinsActive = array_merge($joinsActive, $filter['joinsActive']);
      $joinsAvail = array_merge($joinsAvail, $filter['joinsAvail']);
      if ($filter['groupBys']) {
        $groupBys = array_merge($groupBys, $filter['groupBys']);
      }
    }
  }
  
  if (isset($params['return.select'])) { 
    $query = 'SELECT ' . implode(', ', $params['return.select']) . ' ';
  } elseif ($params['return.count'] == 1) { 
    $query = 'SELECT count(civicrm_activity.id) as cnt';
  } else {
    $query = 'SELECT civicrm_activity.*';
  }
  $query .= ' FROM {civicrm_activity} civicrm_activity';
  foreach ($joinsActive as $alias => $active) {
    if ($active) {
      $query .= ' ' . $joinsAvail[$alias];
    }
  }

  if (!empty($wheres)) {
    $query .= ' WHERE ';
    $query .= implode(' AND ', $wheres);
  }
  if (!empty($groupBys)) {
    $query .= ' GROUP BY ' . implode(', ', $groupBys);
  }
  if (!$params['return.count']) {
    if (isset($params['sort']) && preg_match('/^[a-zA-Z0-9_, ]+$/', $params['sort'])) {
      $query .= ' ORDER BY ' . $params['sort'];
    }
    if (isset($params['rowCount']) && is_numeric($params['rowCount'])) {
      $query .= ' LIMIT ' . $params['rowCount'];
      if (isset($params['offset']) && is_numeric($params['offset'])) {
        $query .= ' OFFSET ' . $params['offset'];
      }
    }
  }
  
  return $query;
}

/**
 * Convert a list of activities into a list of contact+activity records
 *
 * @param $activities A list of activities in the format returned by arms_util_activity_search()
 * @return array of ('target' => $, 'activity' => $) tuples; each contact is an abridged-contact record;
 */
function arms_util_activity_by_target($activities) {
  if (empty($activities)) {
    return array();
  }
  $sql = 'select cat.activity_id, contact.id as contact_id, contact.contact_type, contact.display_name, contact.sort_name';
  $sql .= ' from {civicrm_activity_target} cat';
  $sql .= ' inner join {civicrm_contact} contact on contact.id = cat.target_contact_id';
  $sql .= ' where cat.activity_id in (' . implode(',', array_keys($activities)) . ')';
  $query = db_query($sql);
  $result = array();
  while ($obj = db_fetch_array($query)) {
    $tuple = array();
    $tuple['target'] = $obj;
    $tuple['activity'] = &$activities[$obj['activity_id']];
    $result[] = $tuple;
  }
  return $result;
}

/**
 * Create an activity type. If it already exists, do nothing.
 *
 * @param $params array. Mandatory keys: "label" and "description". Optional keys: weight, is_active, is_reserved
 * @return see civicrm_activity_type_create
 */
function arms_util_activity_type_create($params) {
  civicrm_initialize();
  if (empty($params['label']) || empty($params['description'])) {
    return array('is_error' => 1, 'message' => 'Missing label or description'); 
  }

  if (arms_util_activity_type($params['label']) != -1) {
    return array('is_error' => 0); 
  }

  if (!isset($params['is_active'])) {
    $params['is_active'] = 1;
  }
  if (!isset($params['is_reserved'])) {
    $params['is_reserved'] = 1;
  }
  if (!isset($params['weight'])) {
    $sql = 'select max(v.weight) as max_weight'
		. ' from civicrm_option_value v'
		. ' inner join civicrm_option_group g on v.option_group_id=g.id'
		. ' where g.name=\'activity_type\'';
    $q = db_query($sql);
    if ($res = db_fetch_object($q)) {
      $params['weight'] = 1 + $res->max_weight;
    }
  }

  require_once 'api/v2/Activity.php';
  $result = &civicrm_activity_type_create($params);
  
  // refresh
  arms_util_activity_types(TRUE);
  require_once 'CRM/Core/PseudoConstant.php';
  CRM_Core_PseudoConstant::flush('activityType');
  
  return $result;
}

/**
 * Add filter based on a contact's name
 *
 * @param $role string ('target', 'source', 'assignee')
 * @param $name string
 * @return opaque
 */
function arms_util_activity_filter_name($role, $name) {
  $query = array(
    'wheres' => array(),
    'joinsActive' => array(),
    'joinsAvail' => array(),
  );

  $vars = array(
    '@table' => 'civicrm_contact',
    '@alias' => $role . '_contact',
    '@role_contact_id' => $role . '_contact_id',
    '@value' => db_escape_string(strtoupper($name)),
  );
  
  switch ($role) {
    case 'target':
      $query['joinsActive']['civicrm_activity_target'] = 1;
      break;
    case 'assignee':
      $query['joinsActive']['civicrm_activity_assignment'] = 1;
      break;
  }
  $query['joinsAvail'][$vars['@alias']] = strtr('inner join civicrm_contact @alias on @alias.id = @role_contact_id', $vars);
  $query['joinsActive'][$vars['@alias']] = 1;
  $query['wheres'][] = strtr('upper(@alias.display_name) like "%@value%"', $vars);
  
  return $query;
}

/**
 * Add filter which matches any contact record which has any of the given values
 * on a multi-select custom field
 *
 * @param $role string ('target', 'source', 'assignee')
 * @param $field See arms_util_field
 * @param $or_values An array; any value can be matched
 * @return opaque
 */
function arms_util_activity_filter_custom_multi($role, $field, $or_values) {
  if (empty($or_values)) {
    return array(); 
  }
  
  $query = array(
    'wheres' => array(),
    'joinsActive' => array(),
    'joinsAvail' => array(),
  );
  
  $vars = array(
    '@table' => $field['_table_name'],
    '@column' => $field['column_name'],
    '@alias' => $role . '_' . $field['_table_name'],
    '@role_contact_id' => $role . '_contact_id',
  );

  $or_wheres = array();
  arms_util_include_api('option');
  foreach ($or_values as $value) {
    // Recall: Multi-select values are encoded as "SEP . join(SEP, $values) . SEP"
    $or_vars = $vars;
    $or_vars['@value'] = '%' . db_escape_string(arms_util_option_implode(array($value))) . '%';
    $or_wheres[] = strtr('@alias.@column like "@value"', $or_vars);
  }
  
  switch ($role) {
    case 'target':
      $query['joinsActive']['civicrm_activity_target'] = 1;
      break;
    case 'assignee':
      $query['joinsActive']['civicrm_activity_assignment'] = 1;
      break;
  }
  $query['joinsAvail'][$vars['@alias']] = strtr('inner join @table @alias on @alias.entity_id = @role_contact_id', $vars);
  $query['joinsActive'][$vars['@alias']] = 1;
  $query['wheres'][] = '(' . join(' or ', $or_wheres) . ')';
  
  return $query;
}

/**
 * Add filter which matches a custom field value on a contact record.
 *
 * @param $role string ('target', 'source', 'assignee')
 * @param $field See arms_util_field
 * @param $rel string; '=', '<', '<=', etc.
 * @param $value a value to plugin; if the field's datatype is non-numeric, this will be automatically escaped
 * @return opaque
 */
function arms_util_activity_filter_custom($role, $field, $rel, $value) {
  $query = array(
    'wheres' => array(),
    'joinsActive' => array(),
    'joinsAvail' => array(),
  );
  
  $vars = array(
    '@table' => $field['_table_name'],
    '@column' => $field['column_name'],
    '@alias' => $role . '_' . $field['_table_name'],
    '@role_contact_id' => $role . '_contact_id',
    '@rel' => $rel,
    '@value' => $field['data_type'] == 'Int' ? $value : '"' . db_escape_string($value) .'"',
  );
  
  switch ($role) {
    case 'target':
      $query['joinsActive']['civicrm_activity_target'] = 1;
      break;
    case 'assignee':
      $query['joinsActive']['civicrm_activity_assignment'] = 1;
      break;
  }
  $query['joinsAvail'][$vars['@alias']] = strtr('inner join @table @alias on @alias.entity_id = @role_contact_id', $vars);
  $query['joinsActive'][$vars['@alias']] = 1;
  $query['wheres'][] = strtr('@alias.@column @rel @value', $vars);
  
  return $query;
}

/**
 * Add filter which matches activities with cetain warnings
 *
 * @param $column string; The name of a column on the {arms_warnings} table
 * @param $rel string; e.g. "=", "IN", "LIKE"
 * @param $value string, escaped
 * @return opaque
 */
function arms_util_activity_filter_warning($column, $rel, $value) {
  $query = array(
    'wheres' => array(),
    'joinsActive' => array(),
    'joinsAvail' => array(),
  );
  
  $vars = array(
    '@column' => $column,
    '@rel' => $rel,
    '@value' => $value,
  );
  
  $query['joinsActive']['arms_warnings'] = 1;
  $query['joinsAvail']['arms_warnings'] = 'inner join arms_warnings warning on (warning.activity_id = civicrm_activity.id)';
  $query['wheres'][] = strtr('warning.@column @rel @value', $vars);
  
  return $query;
}

/**
 * Add filter which matches based on a literal expression
 *
 * @param $where SQL string
 * @param $joins array of join expressions, keyed by tables/aliases
 * @param $groupBy SQL string
 * @return opaque
 */
function arms_util_activity_filter_expr($where, $joins = FALSE, $groupBy = FALSE) {
  $query = array(
    'wheres' => array(),
    'joinsActive' => array(),
    'joinsAvail' => array(),
  );
  
  $vars = array(
    '@column' => $column,
  );
  
  if ($where) {
    $query['wheres'][] = $where;
  }
  if ($joins) {
    foreach ($joins as $alias => $expr) {
      $query['joinsActive'][$alias] = 1;
      $query['joinsAvail'][$alias] = $expr;
    }
  }
  if ($groupBy) {
    $query['groupBys'][] = $groupBy;
  }
  
  return $query;
}

/**
 * Add filter which matches based on a property of an activity
 *
 * @param $column string; The name of a column on the {civicrm_activity} table
 * @param $rel string; e.g. "=", "IN", "LIKE"
 * @param $value string, escaped
 * @return opaque
 */
function arms_util_activity_filter_activity($column, $rel, $value) {
  $query = array(
    'wheres' => array(),
    'joinsActive' => array(),
    'joinsAvail' => array(),
  );
  
  $vars = array(
    '@column' => $column,
    '@rel' => $rel,
    '@value' => $value,
  );
  
  $query['wheres'][] = strtr('civicrm_activity.@column @rel @value', $vars);
  
  return $query;
}

/**
 * Construct CRUD links to an activity
 *
 * @param $activity array-encoded activity record
 * @param $contact_id int 
 * @param $mask int e.g. CRM_Core_Action::mask(CRM_Core_Permission::EDIT), e.g. CRM_Core_Action::VIEW | CRM_CORE_Action::UPDATE
 * @return string (HTML)
 */
function arms_util_activity_links($activity, $contact_id, $mask, $context = 'activity') {
  require_once 'CRM/Core/Action.php';
  require_once 'CRM/Activity/Selector/Activity.php';
    
  $selector = new CRM_Activity_Selector_Activity($contact_id, $permission, FALSE, $context);
  $links = $selector->actionLinks($activity['activity_type_id'], $activity['source_record_id']);
  drupal_alter('hack_activity_links', $links, $activity);
  $concrete_mask = array_sum(array_keys($links)) & $mask;
  
  return CRM_Core_Action::formLink($links, $concrete_mask, array(
    'id' => $activity['id'],
    'cid' => $contact_id,
    'cxt' => $context,
    'caseid' => $activity['case_id'],
  ));
}

/**
 * Set the list of targets for an activity
 *
 * @param $activity_id int
 * @param $target_contact_ids array of int
 * @return void
 */
function arms_util_activity_targets_set($activity_id, $target_contact_ids) {
  $target_contact_ids = array_filter($target_contact_ids, 'is_numeric');
  if (empty($target_contact_ids)) {
    db_query('DELETE FROM {civicrm_activity_target} WHERE activity_id = %d', $activity_id);
  } else {
    $ids = implode(',', $target_contact_ids);
    db_query('DELETE FROM {civicrm_activity_target} WHERE activity_id = %d AND target_contact_id NOT IN ('.$ids.')', $activity_id);
  }
  $pending = drupal_map_assoc($target_contact_ids);
  $q = db_query('SELECT target_contact_id FROM {civicrm_activity_target} WHERE activity_id = %d', $activity_id);
  while ($row = db_fetch_object($q)) {
    unset($pending[$row->target_contact_id]);
  }
  foreach ($pending as $target_contact_id) {
    db_query('INSERT INTO {civicrm_activity_target} (activity_id,target_contact_id) VALUES (%d,%d)', $activity_id, $target_contact_id);
  }
}

/**
 * Set the list of assignees for an activity
 *
 * @param $activity_id int
 * @param $assignee_contact_ids array of int
 * @return void
 */
function arms_util_activity_assignments_set($activity_id, $assignee_contact_ids) {
  $assignee_contact_ids = array_filter($assignee_contact_ids, 'is_numeric');
  if (empty($assignee_contact_ids)) {
    db_query('DELETE FROM {civicrm_activity_assignment} WHERE activity_id = %d', $activity_id);
  } else {
    $ids = implode(',', $assignee_contact_ids);
    db_query('DELETE FROM {civicrm_activity_assignment} WHERE activity_id = %d AND assignee_contact_id NOT IN ('.$ids.')', $activity_id);
  }
  $pending = drupal_map_assoc($assignee_contact_ids);
  $q = db_query('SELECT assignee_contact_id FROM {civicrm_activity_assignment} WHERE activity_id = %d', $activity_id);
  while ($row = db_fetch_object($q)) {
    unset($pending[$row->assignee_contact_id]);
  }
  foreach ($pending as $assignee_contact_id) {
    db_query('INSERT INTO {civicrm_activity_assignment} (activity_id,assignee_contact_id) VALUES (%d,%d)', $activity_id, $assignee_contact_id);
  }
}
