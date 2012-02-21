<?php
/**
 * @file
 *
 * Field Extension API - Crude tools for binding logic to Civi custom
 * data fields. This should mitigate the problem in which attaching
 * logic/behavior to a field requires tracking the field name/ID
 * on each site.
 *
 * Each binding is identified by a programmatic name. The programmatic
 * name corresponds to a Drupal configuration variable (variable_get/set)
 * and is associated with logic (e.g. a callback to fill-in a drop-down
 * list). The Drupal configuration variable stores the name of a Civi/ARMS
 * field on the local site which should be extended.
 *
 * The idea for this module has gone through a few revisions. For my
 * (Tim) peace of mind, I want to record the evolution. However, I
 * don't know if these notes will mean much to anyone else.
 *
 * - We need the ability to activate certain hooks/callbacks for
 *   selected fields.
 * - We could make the core CustomField and UFField entities
 *   customizable. Use this to add metadata to each field which
 *   specifies when to enable/disable/select logic/behavior.
 *   Unfortunately, this sort of change would either be
 *   unmaintainable or would require negotiation with upstream.
 * - We should extend the idiom used in arms_team, arms_profile_status,
 *   et al -- i.e. store the names of any special fields in Drupal
 *   configuration variables, and then apply behavior to selected
 *   fields. Now the issue becomes how do we manage those variables.
 * - We could have arms_util_fext_register($var, $options) and a
 *   similar unregister function. Callers would be responsible for
 *   runinng the appropriate register/unregister function during, say,
 *   module installation.
 * - The "register" approach becomes a bit brittle because every
 *   module is responsible for managing that variable, and it's easy
 *   to screw it up. Instead, we'll imitate hook_menu (or hook_schema) --
 *   they can easily recover state even when modules are disabled,
 *   uninstalled, reinstalled, deleted, etc. As with the menu subsystem
 *   (or schema subsystem), we'll use the caching framework and build
 *   indices for performance purposes.
 * - We need to expose these options to the users, and the UI code is
 *   quite repetitive. How about we automatically add widgets on the
 *   "ARMS UI Settings" page?
 * - The signatures for the callback functions are a bit unruly. To
 *   make them a more manageable, we should put parameters in a single
 *   $data variable. The data variable should be typed, but we're too
 *   lazy to do that right now.
 *
 * At this point, my main complaints are:
 *
 * - I prefer the approach of "adding any behavior to any field" over
 *   the approach of "pre-defining a behavior and filling-in the field".
 *   However, the former requires more design/development, and the
 *   the latter resembles our pre-existing code.
 * - Even if we accept the above approach, this implementation is still
 *   too tightly coupled to the Drupal configuration framework. It needs
 *   the field identifiers and the callback lists (and some indices). It
 *   doesn't need to know how field identifiers or callbacks are chosen.
 * - The documentation for the callback parameters is a bit awkward to
 *   read. We should better formalize this contract by writing interfaces or
 *   classes for each callback's $data.
 */

/**
 * Hook stub
 *
 * Define field extensions. Each field extension has a symbolic name
 * and a list of callback functions. The callbacks will be invoked
 * whenever the field is processed. Several types of callback are available;
 * these generally correspond to hooks in CiviCRM (e.g. whenever
 * hook_civicrm_customFieldOptions is invoked, we copy the arguments
 * to a $data variable, lookup any '#options' callbacks, and then invoke
 * them.).
 *
 * @return array of definitions; each key is a Drupal configuration variable; each value is an array with keys:
 * - '#file': String; a file to include before invoking callbacks
 * - '#include': Array; similar to '#file', but the data is passed to module_load_include instead of require_once
 * - '#default_field': A default field name (optional)
 * - '#options': Array of callbacks; each callback may alter the list of options for a dropdown field; function($data), where data has 'fext', 'options', and 'detailedFormat'; see hook_civicrm_customFieldOptions
 * - '#validate': Array of callbacks; each callback may return an error message; function($data), where data has 'fext', 'form', 'formName', 'elementName', 'contactId', 'fields', 'files', 'value', 'error' [mutable]
 * - '#buildElement': Array of callbacks; each callback may modify the form element; function($data), where data has 'fext', 'form', 'formName', 'elementName', 'contactId'
 * - '#display': Array of callbacks; each callback may alter the display value of a custom data field; function($data), where data has 'fext', 'value', 'display', and 'options'; see CRM_Core_BAO_CustomField::getDisplayValue and hook_hack_customFieldValue_alter; do not set 'display' to markup -- this only works with some views
 * - '#update': Array of callbacks; each callback is run when field is updated; function($data), where data has 'fext', 'op', 'entity_id', 'params', and 'param'); see hook_civicrm_custom
 * - '#arms_ui_settings': FALSE or an array; if an array, then any options will be passed to Form API field in "ARMS UI Settings"
 */
function hook_arms_fext() {
}

/**
 * Hook stub
 *
 * Alter the definitions of field extensions
 */
function hook_arms_fext_alter(&$defns) {
}

/**
 * Bind the logic for $var to field $field
 *
 * @param $var A Drupal configuration variable ('my_module_option')
 * @param $field An ARMS field full-name ('group:field')
 */
function arms_util_fext_set_field($var, $field) {
  variable_set($var, $field);
  arms_util_fext_index_clear();
}

/**
 * Determine which fields are bound to the variable
 *
 * @param $var A Drupal configuration variable ('my_module_option')
 * @return String; space-delimited list of ARMS fields ('group1:field1 group2:field2 group3:field3')
 */
function arms_util_fext_get_fields($var) {
  $defns = arms_util_fext_definitions();
  return variable_get($var, $defns[$var]['#default_field']);
}

/**
 * Determine which fields are bound to the variable
 *
 * @param $var A Drupal configuration variable ('my_module_option')
 * @return String; space-delimited list of ARMS fields ('group1:field1 group2:field2 group3:field3')
 */
function arms_util_fext_get_fields_array($var) {
  $value = arms_util_fext_get_fields($var);
  if (empty($value)) {
    return array();
  } else {
    return preg_split("/[ \r\n]+/", $value);
  }
}

/**
 * Determine which field is bound to the variable. If multiple fields are bound, return the first.
 *
 * @param $var A Drupal configuration variable ('my_module_option')
 * @return An ARMS field full-name ('group:field')
 */
function arms_util_fext_get_field($var) {
  $value = arms_util_fext_get_fields_array($var);
  if (empty($value)) {
    return '';
  } else {
    return array_shift($value);
  }
}

/**
 * Look up a defn by 'var'
 *
 * @return a single defn
 */
function arms_util_fext_find_by_var($var) {
  $defns = arms_util_fext_definitions();
  return $defns[$var];
}

/**
 * Look up all defns which match a field ID
 *
 * @return array of defns, keyed by var
 */
function arms_util_fext_find_by_field_id($field_id) {
  $idx = _arms_util_fext_index();
  return $idx[$field_id];
}

/**
 * Obtain a list of field extensions
 *
 * @return array(fextName => fextDefn)
 */
function arms_util_fext_definitions($fresh = FALSE) {
  global $_arms_util_fext_defns;
  if ($fresh || !isset($_arms_util_fext_defns)) {
    if (!$fresh && ($cache = cache_get('arms_util_fext_defns')) && !empty($cache->data)) {
      // dpm(array('cache hit', $cache));
      $_arms_util_fext_defns = $cache->data;
    }
    else {
      // dpm ('cache miss');
      $_arms_util_fext_defns = array(); // module_invoke_all('arms_fext');
      foreach (module_implements('arms_fext') as $module) {
        $defns = call_user_func($module . '_arms_fext');
        if (is_array($defns)) {
          foreach ($defns as $key => $value) {
            $defns[$key]['#module'] = $module;
          }
          $_arms_util_fext_defns = array_merge($_arms_util_fext_defns, $defns);
        }
      }
      drupal_alter('arms_fext', $_arms_util_fext_defns);
      cache_set('arms_util_fext_defns', $_arms_util_fext_defns);
      // clear index, which builds on definitions
      arms_util_fext_index_clear();
    }
    // dpm($_arms_util_fext_defns);
  }
  return $_arms_util_fext_defns;
}


/**
 * Clear the cache of fext definitions
 */
function arms_util_fext_definitions_clear() {
  global $_arms_util_fext_defns;
  unset($_arms_util_fext_defns);
  cache_clear_all('arms_util_fext_defns', 'cache', TRUE);
}

/**
 * Obtain a list of defns, indexed by the ID number of any
 * bound fields.
 */
function _arms_util_fext_index($fresh = FALSE) {
  global $_arms_util_fext_idx;
  if ($fresh || !isset($_arms_util_fext_idx)) {
    if (!$fresh && ($cache = cache_get('arms_util_fext_idx')) && !empty($cache->data)) {
      $_arms_util_fext_idx = $cache->data;
    }
    else {
      $defns = arms_util_fext_definitions();
      $_arms_util_fext_idx = array();
      foreach ($defns as $var => $defn) {
        $field_names = arms_util_fext_get_fields_array($var);
        foreach ($field_names as $field_name) {
          if ($field_name) {
            $field = arms_util_field($field_name);
            if ($field) {
              $_arms_util_fext_idx[$field['id']][$var] = $defn;
            }
          }
        }
      }
      cache_set('arms_util_fext_idx', $_arms_util_fext_idx);
    }
    // dpm(array('idx' => $_arms_util_fext_idx));
  }
  return $_arms_util_fext_idx;
}

/**
 * Clear the fext indices
 */
function arms_util_fext_index_clear() {
  global $_arms_util_fext_idx;
  unset($_arms_util_fext_idx);
  cache_clear_all('arms_util_fext_idx', 'cache');
}

/**
 * Quasi-implementation of hook_civicrm_buildForm
 *
 * On any forms that are driven by custom data, identify custom-data fields
 * and call the appropriate one.
 */
function arms_util_fext_civicrm_buildForm($formName, &$form) {
  $elem_pattern = FALSE;
  $contact_id = FALSE;
  $contact_elem_pattern = FALSE;
  
  switch ($formName) {
    case 'CRM_Contact_Form_Contact':
    case 'CRM_Contact_Form_CustomData':
    case 'CRM_Contact_Form_Edit':
      $elem_pattern = '/custom_(\d+)(_\d+)?/';
      $contact_id = $form->_contactId;
      break;
      
    case 'CRM_Contact_Form_Search_Advanced':
      $elem_pattern = '/custom_(\d+)(_\d+)?/';
      $contact_id = FALSE;
      break;
      
    case 'CRM_Profile_Form_Edit':
      $elem_pattern = '/custom_(\d+)(_\d+)?/';
      $contact_id = $form->get('id');
      break;
      
    case 'CRM_Contact_Form_Task_Batch':
      $contact_elem_pattern = '/field\[([0-9]+)\]\[custom_(\d+)(_\d+)?\]/';
      $contact_id = FALSE; // FIXME
      break;      
  }
  
  if ($elem_pattern !== FALSE) {
    $idx = _arms_util_fext_index();
    foreach ($form->_elementIndex as $elementName => $elementId) {
      if (preg_match($elem_pattern, $elementName, $matches)) {
        $field_id = $matches[1];
        if (!isset($idx[$field_id])) continue;
        
        $data = new stdClass();
        $data->formName = $formName;
        $data->form = &$form;
        $data->elementName = $elementName;
        $data->contactId = $contact_id;
        _arms_util_fext_invoke_all('#buildElement', $field_id, $data);
      }
    }
  } elseif ($contact_elem_pattern !== FALSE) {
    $idx = _arms_util_fext_index();
    foreach ($form->_elementIndex as $elementName => $elementId) {
      if (preg_match($contact_elem_pattern, $elementName, $matches)) {
        $field_id = $matches[2];
        if (!isset($idx[$field_id])) continue;
        
        $data = new stdClass();
        $data->formName = $formName;
        $data->form = &$form;
        $data->elementName = $elementName;
        $data->contactId = $matches[1];
        _arms_util_fext_invoke_all('#buildElement', $field_id, $data);
      }
    }
  }  
}

/**
 * Quasi-implementation of hook_civicrm_validate
 */
function arms_util_fext_civicrm_validate($formName, &$fields, &$files, &$form) {
  $errors = array();
  $elem_pattern = FALSE;
  $contact_id = FALSE;
  $contact_elem_pattern = FALSE;
  
  switch ($formName) {
    case 'CRM_Contact_Form_Contact':
    case 'CRM_Contact_Form_CustomData':
    case 'CRM_Contact_Form_Edit':
      $elem_pattern = '/custom_(\d+)(_\d+)?/';
      $contact_id = $form->_contactId;
      break;
    
    case 'CRM_Contact_Form_Search_Advanced':
      $elem_pattern = '/custom_(\d+)(_\d+)?/';
      $contact_id = FALSE;
      break;
      
    case 'CRM_Profile_Form_Edit':
      $elem_pattern = '/custom_(\d+)(_\d+)?/';
      $contact_id = $form->get('id');
      break;
      
    case 'CRM_Contact_Form_Task_Batch':
      $contact_elem_pattern = '/field\[([0-9]+)\]\[custom_(\d+)(_\d+)?\]/';
      $contact_id = FALSE; // FIXME
      break;
  }

  // Single-contact forms  
  if ($elem_pattern !== FALSE) {
    $idx = _arms_util_fext_index();
    foreach ($form->_elementIndex as $elementName => $elementId) {
      if (preg_match($elem_pattern, $elementName, $matches)) {
        $field_id = $matches[1];
        if (!isset($idx[$field_id])) continue;
        
        $data = new stdClass();
        $data->formName = $formName;
        $data->form = &$form;
        $data->fields = &$fields;
        $data->files = &$files;
        $data->elementName = $elementName;
        $data->contactId = $contact_id;
        $data->errors = &$errors;
        _arms_util_fext_invoke_all('#validate', $field_id, $data);
      }
    }
  // Multi-contact form
  } elseif ($contact_elem_pattern !== FALSE) {
    $idx = _arms_util_fext_index();
    foreach ($form->_elementIndex as $elementName => $elementId) {
      if (preg_match($contact_elem_pattern, $elementName, $matches)) {
        $field_id = $matches[2];
        if (!isset($idx[$field_id])) continue;
        
        $data = new stdClass();
        $data->formName = $formName;
        $data->form = &$form;
        $data->fields = &$fields;
        $data->files = &$files;
        $data->elementName = $elementName;
        $data->contactId = $matches[1];
        $data->errors = &$errors;
        _arms_util_fext_invoke_all('#validate', $field_id, $data);
      }
    }
  }  
  
  #arms_util_include_api('array');
  #return (arms_util_array_hollow($errors) ?  TRUE : $errors);
  return (empty($errors) ?  TRUE : $errors);
}

/**
 * Quasi-implementation of hook_civicrm_customFieldOptions which
 * delegates to any appropriate '#options' callbacks.
 *
 * @see arms_util_civicrm_customFieldOptions
 * @see hook_civicrm_customFieldOptions
 */
function arms_util_fext_civicrm_customFieldOptions($field_id, &$options, $detailed_format) {
  $defns = arms_util_fext_find_by_field_id($field_id);
  if (!is_array($defns)) {
    return;
  }
  $data = new stdClass();
  $data->options = &$options;
  $data->detailedFormat = $detailed_format;
  _arms_util_fext_invoke_all('#options', $field_id, $data);
}

/**
 * Quasi-implementation of hook_civicrm_custom which delegates
 * to any appropriate '#update' callbacks.
 *
 * @see arms_util_civicrm_custom
 * @see hook_civicrm_custom
 */
function arms_util_fext_civicrm_custom($op, $group_id, $entity_id, &$params) {
  if ($op == 'create' || $op == 'edit') {
    $idx = _arms_util_fext_index();
    // find any $param fields that have bindings
    foreach ($params as $pid => $param) {
      if (isset($idx[$param['custom_field_id']])) {
        // this field has bindings; prepare data to pass to the callback

        $data = new stdClass();
        $data->op = $op;
        $data->group_id = $group_id;
        $data->entity_id = $entity_id;
        $data->params = &$params;
        $data->param = &$params[$pid];
        _arms_util_fext_invoke_all('#update', $param['custom_field_id'], $data);
      }
    }
  }
  // dpm(array('called arms_util_fext_civicrm_custom', 'idx' => $idx, 'args' => func_get_args(), 'log' => $log));
}

/**
 * Quasi-implementation of hook_hack_customFieldValue_alter which
 * delegates to any appropriate '#display' callbacks
 *
 * @see arms_util_hack_customFieldValue_alter
 */
function arms_util_fext_hack_customFieldValue_alter(&$hook_data) {
  $defns = arms_util_fext_find_by_field_id($hook_data['id']);
  if (!is_array($defns)) {
    return;
  }
  $data = new stdClass();
  $data->value = &$hook_data['value']; ## The internal field value
  $data->display = &$hook_data['display']; ## The displayable field value
  $data->options = &$hook_data['options']; ## Listing of data in the profile form
  _arms_util_fext_invoke_all('#display', $hook_data['id'], $data);
}

/**
 * Invoke all callbacks of a given type for a given field
 *
 * Each callback will receive a single parameter, $data. The parameter will be enhanced
 * with an extra property called 'fext' which defines the fext context -- i.e. the
 * the fext "defn" record, the Drupal config "var", and the ARMS "field" record
 *
 * @param $type string The type of callback (e.g. '#options' or '#update')
 * @param $field_id int The numeric field ID
 * @param $data An object with all the data that should be passed to clients
 *
 * TODO Use classes for $data to facilitate refactoring
 */
function _arms_util_fext_invoke_all($type, $field_id, $data) {
  $idx = _arms_util_fext_index();
  if (! isset($idx[$field_id])) {
    return;
  }
  foreach ($idx[$field_id] as $var => $defn) {
    if (isset($defn[$type])) {
      if (is_string($defn['#file'])) {
        require_once $defn['#file'];
      }
      if (is_array($defn['#include'])) {
        call_user_func_array('module_load_include', $defn['#include']);
      }
      $fext = new stdClass();
      $fext->defn = $defn;
      $fext->var = $var;
      $fext->field = arms_util_field_by_id($field_id);

      if (is_array($data)) {
        $arg = $data; // copy
        $arg['fext'] = $fext;
      } elseif (is_object($data)) {
        $arg = clone $data;
        $arg->fext = $fext;
      } else {
        $arg = $data;
      }

      foreach ($defn[$type] as $cb) {
        call_user_func($cb, $arg);
      }
    }
  }
}

/**
 * A helper for #options implementations which handles the vagaries of the
 * $options format -- i.e. respect detailedFormat parameter; preserve the
 * 'attributes' value.
 *
 * @param $data The $data params for the #option callback
 * @param $std_options Array of options; keys are codes and values are printable labels
 * @return Array of options; keys are id's; values are arrays with 'id', 'value', and 'label'
 *
 * @see http://issues.civicrm.org/jira/browse/CRM-5516
 */
function arms_util_fext_set_options($data, $std_options) {
  $attributes = $data->options['attributes']; // dear god man
  
  if ($data->detailedFormat) {
    $detailed_options = array();
    foreach ($std_options as $value => $label) {
      $detailed_options[$value] = array(
        'id' => $value,
        'value' => $value,
        'label' => $label,
      );
    }
    $data->options = $detailed_options;
  } else {
    $data->options = $std_options;
  }

  // if 'attributes' wasn't set in the original, then don't add it
  if ($attributes) {
    $data->options['attributes'] = $attributes; // dear god man
  }
}
