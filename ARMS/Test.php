<?php

define(ARMS_MAIL_MOCK_TRANSPORT, 1);

/**
 * Base class for ARMS tests
 *
 */
class ARMS_Test extends DrupalTestCase {
  var $contacts;
  var $activities;
  var $user;
  var $fexts;
  var $snapshot_tables;

  function setUp() {
    parent::setUp();
    // drupalLoginUser doesn't work with "drush.php -l sitename". Might be
    // related to http://drupal.org/node/282712
    
    // The Civi API requires authentication for some calls (e.g.
    // civicrm_contact_delete) but not others (e.g. civicrm_contact_add).
    // So testing will suck.
    
    // $this->user = $this->drupalCreateUserRolePerm(array('access CiviCRM', 'administer CiviCRM'));
    // $this->drupalGet('logout');
    // $this->drupalLoginUser($this->user);
    
    civicrm_initialize();
    arms_util_include_api('fext');
    arms_util_include_api('option');
    arms_util_include_api('array');
    arms_util_time(time()); // real time

    $this->contacts = array();
    $this->activities = array();
    $this->groups = array();
    $this->fexts = new ARMS_Test_Fext($this);

    // hacky snapshot of db
    $this->snapshot_tables = array(
      'arms_actlog_policy' => array('all' => TRUE),
      'arms_actlog' => array('all' => TRUE),
      'arms_actlog_signature' => array('all' => TRUE),
      'arms_actlog_filter_type' => array('all' => TRUE),
      'arms_actlog_activity' => array('all' => TRUE),
      'arms_actlog_activity_target' => array('all' => TRUE),
      'arms_actlog_activity_assignment' => array('all' => TRUE),
      'arms_warnings' => array('column' => 'id'),
      'arms_recur_activity' => array('all' => TRUE),
      'arms_recur_sched' => array('all' => TRUE),
      'arms_warning_evaldays' => array('all' => TRUE),
      'arms_warning_contactdays' => array('all' => TRUE),
      'arms_import_rows'=> array('column' => 'id'),
      'arms_import_jobs' => array('column' => 'id'),
      'arms_tournament_contacts' => array('column' => 'id'),
      'arms_tournaments' => array('column' => 'id'),
      'civicrm_activity_target' => array('column' => 'id'),
      'civicrm_activity_assignment' => array('column' => 'id'),
      'civicrm_activity' => array('column' => 'id'),
      'civicrm_participant' => array('column' => 'id'),
      'civicrm_relationship' => array('column' => 'id'),
      'civicrm_phone' => array('column' => 'id'),
      'civicrm_email' => array('column' => 'id'),
      'civicrm_address' => array('column' => 'id'),
      'civicrm_note' => array('column' => 'id'),
      'civicrm_contact' => array('column' => 'id'),
      'civicrm_group' => array('column' => 'id'),
      'civicrm_custom_field' => array('column' => 'id'),
      'civicrm_custom_group' => array('column' => 'id'),
      'civicrm_option_value' => array('column' => 'id'),
      'civicrm_option_group' => array('column' => 'id'),
      'arms_sync_devices' => array('all' => TRUE),
      'arms_sync_states' => array('all' => TRUE),
      'yass_ace' => array('all' => TRUE),
      'yass_conflict' => array('all' => TRUE),
      'yass_mergelog' => array('all' => TRUE),
      'yass_log' => array('all' => TRUE),
      'yass_archive' => array('all' => TRUE),
      'yass_datastore' => array('all' => TRUE),
      'yass_syncstore_state' => array('all' => TRUE),
      'yass_syncstore_seen' => array('all' => TRUE),
      'yass_guidmap' => array('all' => TRUE),
      'yass_syncstatus' => array('all' => TRUE),
      // 'yass_replicas' => array('all' => TRUE),
      'arms_sync_hashes' => array('all' => TRUE), // last
      'arms_comp_program_log' => array('column' => 'id'), // last
    );
    foreach ($this->snapshot_tables as $table => &$table_spec) {
      if (db_table_exists($table)) {
        if ($table_spec['column']) {
          $q = db_query('select max(' . $table_spec['column'] . ') as max from {' . $table . '}');
          $row = db_fetch_object($q);
          $table_spec['max_id'] = $row->max;
        }
      }
    }
    
    if (class_exists('arms_mail_MockTransport')) {
      arms_mail_MockTransport::singleton()->reset();
    }
  }
  
  function tearDown() {
    $q = db_query('select id as contact_id, contact_type from civicrm_contact where id > %d',
      $this->snapshot_tables['civicrm_contact']['max_id']);
    while ($civicrm_contact = db_fetch_array($q)) {
      require_once 'api/v2/Contact.php';
      $result = civicrm_contact_delete($civicrm_contact);
      $this->assertFalse(civicrm_error($result), 'Delete temporary contact');
      // print_r(array('delete', $civicrm_contact, $result));
    }
    $q = db_query('select * from civicrm_custom_field where id > %d', $this->snapshot_tables['civicrm_custom_field']['max_id']);
    while ($civicrm_custom_field = db_fetch_array($q)) {
      //$result = civicrm_custom_field_delete($civicrm_custom_field);
      //$this->assertFalse(civicrm_error($result), 'Delete custom field');

      require_once 'CRM/Core/DAO/CustomField.php';
      $field = new CRM_Core_DAO_CustomField();
      $field->copyValues($civicrm_custom_field);
      require_once 'CRM/Core/BAO/CustomField.php';
      $result = CRM_Core_BAO_CustomField::deleteField($field);
      
      // print_r(array('delete field', $civicrm_custom_field, $result));
    }
    $q = db_query('select id from civicrm_custom_group where id > %d', $this->snapshot_tables['civicrm_custom_group']['max_id']);
    while ($civicrm_custom_group = db_fetch_array($q)) {
      $result = civicrm_custom_group_delete($civicrm_custom_group);
      $this->assertFalse(civicrm_error($result), 'Delete custom group');
      // print_r(array('delete group', $civicrm_custom_group, $result));
    }

    require_once 'CRM/Core/BAO/Cache.php';
    CRM_Core_BAO_Cache::deleteGroup( 'contact fields' );
  
    cache_clear_all();
    drupal_get_schema(FALSE, TRUE);
    
    foreach ($this->snapshot_tables as $table => $table_spec) {
      if (db_table_exists($table)) {
        if ($table_spec['column']) {
          db_query('delete from {' . $table . '} where ' . $table_spec['column'] . ' > %d', $table_spec['max_id']);
        } else if ($table_spec['all']) {
          db_query('delete from {' . $table . '}');
        }
      }
    }
  }
  
  /**
   * Delete all status options in the status custom data field 
   *
   * @return void
   */
  function clearStatuses() {
    $this->assertTrue(isset($this->fexts[ARMS_WARNING_STATUS_FIELD]), 'Status field must exist');
    db_query('delete from {civicrm_option_value} where option_group_id = %d', $this->fexts[ARMS_WARNING_STATUS_FIELD]['option_group_id']);
    variable_set(ARMS_WARNING_RECRUIT_STATUSES_VAR, array());
  }
  
  /**
   * Add a new status option to the status custom data field
   *
   * @param $status array; keys may be:
   *  - value: string
   *  - is_recruit: boolean
   *  - is_default: boolean
   * @return void
   */
  function createStatus($status) {
    arms_util_include_api('option');
    
    $this->assertTrue(isset($this->fexts[ARMS_WARNING_STATUS_FIELD]), 'Status field must exist');
    $q = db_query('select * from {civicrm_option_group} where id = %d', $this->fexts[ARMS_WARNING_STATUS_FIELD]['option_group_id']);
    $option_group = db_fetch_array($q);
    $this->assertTrue(isset($option_group['name']), 'Option group for status is defined');

    $status['option_group_id'] = $option_group['id'];
    if (! $status['label']) {
      $status['label'] = $status['value'];
    }
    require_once 'api/v2/Options.php';
    civicrm_option_value_create($status);

    if ($status['is_recruit']) {
      $recruit_statuses = variable_get(ARMS_WARNING_RECRUIT_STATUSES_VAR, '');
      $recruit_statuses[$status['value']] = $status['label'];
      variable_set(ARMS_WARNING_RECRUIT_STATUSES_VAR, $recruit_statuses);
    }
  }

  /**
   * Replace the current list of statuses with a new list of statuses
   *
   * @param $statuses array; each item is a suitable parameter to createStatus
   * @return void
   */  
  function setStatuses($statuses) {
    $this->clearStatuses();
    foreach ($statuses as $status) {
      $this->createStatus($status);
    }
  }
  
  /**
   * Create an individual record.
   *
   * If any of these fields are omitted, then defaults will be supplied:
   * - contact_type
   * - first_name
   * - last_name
   * - email
   *
   * @param $params See civicrm_contact_add. In addition, adds special support for "email" and "phone" and "phone_type"
   * @return array $params plus contact_id
   */
  function createIndividual($contact_params = array()) {
    $idx = count($this->contacts);
    $contact_defaults = array();
    $contact_defaults['contact_type'] = 'Individual';
    $contact_defaults['first_name'] = 'First_' . $idx;
    $contact_defaults['last_name'] = 'Last_' . $idx;
    $contact_defaults['email'] = 'test' . $idx . '@example.com';
    $contact_params = array_merge($contact_defaults, $contact_params);
    return $this->createContact($contact_params);
  }
  
  function createOrganization($contact_params = array()) {
    $idx = count($this->contacts);
    $contact_defaults = array();
    $contact_defaults['contact_type'] = 'Organization';
    $contact_defaults['organization_name'] = 'Org_' . $idx;
    $contact_defaults['email'] = 'test' . $idx . '@example.com';
    $contact_params = array_merge($contact_defaults, $contact_params);
    return $this->createContact($contact_params);
  }
  
  function createAddress($params) {
    // FIXME: when api/v3/Address becomes available,  use it
    if (!isset($params['id'])) {
      arms_util_insert('civicrm_address')->addValues($params)->toDrupalQuery();
      $params['id'] = db_last_insert_id('civicrm_address', 'id');
    } else {
      db_query(arms_util_update('civicrm_address')->addWheref("id=%d", $params['id'])->addValues($params)->toSQL());
    }
    return $params;
  }
  
  function createContact($contact_params = array()) {
    $location_params = array();
    // $location_params['location_type_id'] = 1;
    $location_params['location_type'] = 'Home';
    $location_params['is_primary'] = 1;
    if ($contact_params['email']) {
      $location_params['email'] = array();
      $location_params['email'][1] = array(
        'email' => $contact_params['email'],
        'is_primary' => 1,
      );
      unset($contact_params['email']);
    }

    if ($contact_params['phone']) {
      if (isset($contact_params['phone_type'])) {
        require_once 'CRM/Core/PseudoConstant.php';
        $phoneTypesByName = array_flip(CRM_Core_PseudoConstant::phoneType());
        $phoneTypeId = $phoneTypesByName[$contact_params['phone_type']];
        unset($contact_params['phone_type']);
      } else {
        $phoneTypeId = 1;
      }
    
      $location_params['phone'] = array();
      $location_params['phone'][] = array(
        'phone' => $contact_params['phone'],
        'phone_type_id' => $phoneTypeId,
        'is_primary' => 1,
      );
      unset($contact_params['phone']);
    }

    require_once 'api/v2/Contact.php';
    $contact_result = civicrm_contact_add($contact_params);
    $this->assertFalse(civicrm_error($contact_result), 'Add temporary contact');
    $contact_result = array_merge($contact_params, $contact_result);
    $contact_result = $contact_params;
    $this->contacts[] = $contact_result;
    
    if (isset($location_params['email']) || isset($location_params['phone'])) {
      $location_params['contact_id'] = $contact_result['contact_id'];
      require_once 'api/v2/Location.php';
      $location_result = civicrm_location_add($location_params);
      $this->assertFalse(civicrm_error($location_result), 'Add temporary location');
    }
    
    return $contact_result;
  }
  
  /**
   * Create an activity record.
   *
   * If any of these fields are omitted, then defaults will be supplied:
   * - subject
   *
   * @param $act_params array see civicrm_activity_create
   * @return see civicrm_activity_create
   */
  function createActivity($act_params) {
    $idx = count($this->activities);
    $act_defaults = array();
    $act_defaults['subject'] = 'Subject ' . $idx;
    $act_params = array_merge($act_defaults, $act_params);
    
    $act_params['activity_date_time'] = preg_replace('/[ \-:]/', '', $act_params['activity_date_time']);
    
    require_once 'api/v2/Activity.php';
    $act_result = civicrm_activity_create($act_params);
    $this->assertFalse(civicrm_error($act_result), 'Add temporary activity');
    if (civicrm_error($act_result)) {
      print_r($act_result);
    }
    
    return $act_result;
  }
  
  /**
   * Update 
   */
  function updateActivity($act_params) {
    require_once 'api/v2/Activity.php';
    $act_result = civicrm_activity_update($act_params);
    $this->assertFalse(civicrm_error($act_result), 'Add temporary activity');
    if (civicrm_error($act_result)) {
      print_r($act_result);
    }
    
    return $act_result;
  }
  
  /**
   * Format a given time for use in Civi activity API
   *
   * @return string
   */
  function createActivityTime($yyyy, $mmMon, $dd, $hh, $mmMin, $ss) {
    return sprintf('%04d%02d%02d%02d%02d%02d', $yyyy, $mmMon, $dd, $hh, $mmMin, $ss);
  }
  
  /**
   * Format a given time as "seconds since epoch" (similar to time())
   *
   * @return string
   */
  function createEpochTime($yyyy, $mmMon, $dd, $hh, $mmMin, $ss) {
    return mktime($hh, $mmMin, $ss, $mmMon, $dd, $yyyy);
  }
  
  /**
   * Create a group record.
   *
   * If any of these fields are omitted, then defaults will be supplied:
   * - subject
   *
   * @param $act_params array see civicrm_group_add
   * @return see civicrm_group_add
   */
  function createGroup($params = array()) {
    $idx = count($this->groups);
    $defaults = array();
    $defaults['subject'] = 'Subject-' . $idx;
    $defaults['name'] = 'Name-' . $idx;
    $defaults['title'] = 'Title-' . $idx;
    $defaults['description'] = 'Description-' . $idx;
    $defaults['is_active'] = 1;
    $defaults['visibility'] = 'User and User Admin Only';
    $defaults['group_type'] = array('2' => 1);
    $params = array_merge($defaults, $params);
    
    require_once 'api/v2/Group.php';
    $result = civicrm_group_add($params);
    $this->assertFalse(civicrm_error($result), 'Add temporary group');
    if (civicrm_error($result)) {
      print_r(array('params' => $params, 'result' => $result));
    } else {
      $this->groups[] = $result;
    }
    
    return $result;
  }
  
  /**
   * Assert that a SQL query returns a given value
   *
   * The first argument is an expected value. The remaining arguments are passed
   * to db_query, evaluated, and then compared to the expected value.
   *
   * assertSql should generally be used in one of two modes:
   *  - single value: the expected value is a scalar, and the query returns a single value
   *  - column: The expected value is an array of scalars, and the query returns a column
   *
   * Example: $this->assertSql(2, 'select count(*) from foo where foo.bar like "%s"', $value);
   * Example: $this->assertSql(array("foo", "bar", "whiz"), 'select word from dictionary');
   */
  protected function assertSql() {
    arms_util_include_api('query');
    $args = func_get_args();
    $expecteds = array_shift($args);
    if (!is_array($expecteds)) {
      $expecteds = array($expecteds);
    }
    
    $q = call_user_func_array('db_query', $args);
    $this->assertFalse(db_error(), 'Query should not yield DB error');
    $actuals = array();
    while ($row = db_fetch_array($q)) {
        $actuals[] = array_shift($row);
    }
    sort($actuals);
    sort($expecteds);
    $this->assertEqual($expecteds, $actuals,
      sprintf('expecteds=[%s] actuals=[%s] query=[%s]', 
        implode(',', $expecteds), implode(',', $actuals), call_user_func_array('arms_util_query_sprintf', $args))
    );
  }

  /**
   * Assert that a SQL query returns a given row
   *
   * The first argument is an expected row. The remaining arguments are passed
   * to db_query, evaluated, and then compared to the expected value.
   *
   * assertSqlRow should generally be used in one mode:
   *  - single row: the expected value is a row with several cells
   *
   * Example: $this->assertSql(
   *   array('first_name' => 'John', 'last_name' => 'Doe'),
   *   'select * from contact where id = %d', $contactId
   *   );
   *
   * The result-set should be designed to 
   */
  protected function assertSqlRow() {
    arms_util_include_api('query');
    $args = func_get_args();
    $expected = array_shift($args);
    
    $queryExpr = call_user_func_array('arms_util_query_sprintf', $args);
    $found = FALSE;
    
    $q = call_user_func_array('db_query', $args);
    $this->assertFalse(db_error(), 'Query should not yield DB error');
    while ($actual = db_fetch_array($q)) {
      $found = TRUE;
      foreach ($expected as $key => $expValue) {
        $this->assertEqual($expected[$key], $actual[$key], 
          sprintf('%s: expected=[%s] actual=[%s] query=[%s]',
            $key,
            $expected[$key], $actual[$key],
            $queryExpr
          ));
      }
    }
    
    if (!$found) {
      $this->fail(sprintf('Query did not return any results: %s', $queryExpr));
    }
  }
  
  /**
   * Assert that every key in $expected is matches the same key in $actual.
   *
   * If $actual contains extraneous keys, they are ignored.
   */
  protected function assertEqualByKey($expected, $actual, $prefix = '') {
    // $keys = array_intersect(array_keys($expected), array_keys($actual));
    foreach (array_keys($expected) as $key) {
      $this->assertEqual($expected[$key], $actual[$key], sprintf('%skey=[%s] expected=[%s] actual=[%s]', $prefix, $key, $expected[$key], $actual[$key]));
    }
  }
}

/**
 * A helper which which warps arms_util_fext_get_field, enabling some syntactic sugar
 */
class ARMS_Test_Fext implements ArrayAccess {
  var $_fexts;
  var $_test;
  
  function __construct($test) {
    $this->_test = $test;
    $this->_fexts = array();
  }
  
  function offsetGet($name) {
    if (!isset($this->_fexts[$name])) {
      $this->_fexts[$name] = arms_util_field(arms_util_fext_get_field($name));
    }
    return $this->_fexts[$name];
  }
  
  function offsetSet($name, $value) {
  }
  
  function offsetExists($name) {
    $field = $this->offsetGet($name);
    return isset($field['_param']);
  }
  
  function offsetUnset($name) {
    unset($this->_fexts[$name]);
  }
}
