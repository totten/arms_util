<?php

/**
 * (Form API)
 *
 * Generate an advanced-settings form for a particular field
 *
 * @param $field_id int
 */
function arms_util_field_form(&$form_state, $field_id) {
  arms_util_include_api('fext');
  
  $field = arms_util_field_by_id($field_id);
  if (! $field) {
    drupal_set_message(t('Invalid field ID'), 'error');
    return array();
  }
  $fexts = arms_util_fext_find_by_field_id($field_id);
  $uf_groups = _arms_util_field_find_profiles($field);
  $smart_groups = _arms_util_field_find_smart_groups($field);

  $values_sql = strtr('
    select count(*) as row_count, count(@col) as cell_count, count(distinct @col) as distinct_count
    from @table', array(
    '@table' => $field['_table_name'],
    '@col' => $field['column_name'],
  ));
  $values = db_fetch_object(db_query($values_sql));

  // build form  
  drupal_set_title(t('Field: @label (@full_name)', array(
    '@label' => $field['label'],
    '@full_name' => $field['_full_name'],
  )));
  $form = array(
    '#tree' => TRUE,
    '#arms_field_id' => $field_id,
  );
  
  $form['naming'] = array(
    '#type' => 'fieldset',
    '#title' => 'Naming',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 0,
  );
  $form['naming']['label'] = array(
    '#type' => 'item',
    '#title' => 'Label',
    '#value' => check_plain($field['label']),
  );
  $form['naming']['arms_full_name'] = array(
    '#type' => 'item',
    '#title' => 'ARMS Full Name',
    '#value' => check_plain($field['_full_name']),
  );
  $form['naming']['civi_token'] = array(
    '#type' => 'item',
    '#title' => 'CiviCRM Token',
    '#value' => check_plain('{contact.' . $field['_param'] . '}'),
  );
  $form['naming']['civi_param'] = array(
    '#type' => 'item',
    '#title' => 'CiviCRM API Parameter',
    '#value' => check_plain($field['_param']),
  );
  $form['naming']['sql_table'] = array(
    '#type' => 'item',
    '#title' => 'SQL Table',
    '#value' => check_plain($field['_table_name']),
  );
  $form['naming']['sql_column'] = array(
    '#type' => 'item',
    '#title' => 'SQL Column',
    '#value' => check_plain($field['column_name']),
  );
  $form['naming']['civi_group'] = array(
    '#type' => 'item',
    '#title' => 'Group Name',
    '#value' => check_plain($field['_group_name']) . ' ' . l('(Move)', 'civicrm/admin/custom/group/field/move', array(
      'query' => array(
        'reset' => 1,
        'fid' => $field['id'],
      ),
    )),
  );
  
  $form['type'] = array(
    '#type' => 'fieldset',
    '#title' => 'Type',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 4,
  );
  $form['type']['data_type'] = array(
    '#type' => 'item',
    '#title' => 'Data Type',
    '#value' => check_plain($field['data_type']),
  );
  $change_type_link = l('(Change)', 'civicrm/admin/custom/group/field/changetype', array(
    'query' => array(
      'reset' => 1,
      'fid' => $field['id']
    )
  ));
  $form['type']['html_type'] = array(
    '#type' => 'item',
    '#title' => 'HTML Type',
    '#value' => check_plain($field['html_type']) . ' ' . $change_type_link,
  );
  
  $form['values'] = array(
    '#type' => 'fieldset',
    '#title' => t('Values (@row/@cell/@distinct)', array(
      '@row' => $values->row_count,
      '@cell' => $values->cell_count,
      '@distinct' => $values->distinct_count,
    )),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 5,
  );
  $form['values']['row_count'] = array(
    '#type' => 'item',
    '#title' => t('Records'),
    '#value' => $values->row_count,
  );
  $form['values']['cell_count'] = array(
    '#type' => 'item',
    '#title' => t('Defined Values'),
    '#value' => $values->cell_count,
  );
  $form['values']['distinct_count'] = array(
    '#type' => 'item',
    '#title' => t('Distinct Values'),
    '#value' => $values->distinct_count,
  );
  $form['values']['review'] = array(
    '#value' => l('Review all values', sprintf('admin/arms/field/%d/values', $field['id']))
  );
  
  $form['profiles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Profile Forms (@count)', array('@count' => count($uf_groups))),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 7,
  );
  $form['profiles']['links'] = array(
    '#type' => 'markup',
    '#value' => empty($uf_groups) ? '<p>(Not referenced in any profile forms)</p>' : theme('item_list', _arms_util_field_profile_links($uf_groups)),
  );
  
  $form['smart_groups'] = array(
    '#type' => 'fieldset',
    '#title' => t('Smart Groups (@count)', array('@count' => count($smart_groups))),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 7,
  );
  $form['smart_groups']['links'] = array(
    '#type' => 'markup',
    '#value' => empty($smart_groups) ? '<p>(Not referenced in any smart groups)</p>' : theme('item_list', _arms_util_field_group_links($smart_groups)),
  );
  
  $form['extensions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Extensions (@count)', array('@count' => count($fexts))),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 10,
  );
  $form['extensions']['table'] = array(
    '#type' => 'markup',
    '#value' => empty($fexts) ? '<p>(Not referenced in any field extensions)</p>' : _arms_util_field_extensions_table($field, $fexts),
  );
  $form['extensions']['see_also'] = array(
    '#type' => 'item',
    '#title' => 'See also',
    '#value' => l('ARMS Settings', 'admin/arms/settings'),
  );
  
  $form['submit'] =  array(
    '#weight' => 18,
    '#type' => 'submit',
    '#value' => t('Save') 
  );
  return $form;
}

/**
 * Find all profile forms which reference this field
 *
 * @param $field array-encoded field, see arms_util_field
 * @return array; keys are profile ID's, and values are profile names
 */
function _arms_util_field_find_profiles($field) {
  $result = array();
  
  $q = db_query('select uf_group.id, uf_group.title
    from {civicrm_uf_group} uf_group
    inner join {civicrm_uf_field} uf_field on uf_group.id = uf_field.uf_group_id
    where uf_field.field_name = "%s"
  ', $field['_param']);
  while ($uf_group = db_fetch_object($q)) {
    $result[$uf_group->id] = $uf_group->title;
  }
  
  return $result;
}

/**
 * Generate a list of links
 *
 * @param $uf_groups array; keys are profile ID's, and values are profile names
 * @return array of strings (HTML)
 */
function _arms_util_field_profile_links($uf_groups) {
  $result = array();
  foreach ($uf_groups as $id => $title) {
    $result[] = l($title . ' [#' . $id . ']', 'civicrm/admin/uf/group/field', array(
        'query' => array(
          'reset' => 1,
          'action' => 'browse',
          'gid' => $id,
        )
      )
    );
  }
  return $result;
}

/**
 * Find all smart groups which reference this field
 *
 * @return array; keys are group ID's, and values are group names
 */
function _arms_util_field_find_smart_groups($field) {
$result = array();
  
  $q = db_query('select smart_group.id, smart_group.title
    from {civicrm_group} smart_group
    inner join {civicrm_saved_search} saved_search on smart_group.saved_search_id = saved_search.id
    where saved_search.where_clause like "%s"
  ', ' %' . $field['_table_name'] . '.' . $field['column_name'] . ' %');
  while ($group = db_fetch_object($q)) {
    $result[$group->id] = $group->title;
  }
  
  return $result;
}

/**
 * Generate a list of links
 *
 * @param $groups array; keys are group ID's, and values are group names
 * @return array of strings (HTML)
 */
function _arms_util_field_group_links($groups) {
  $result = array();
  foreach ($groups as $id => $title) {
    $result[] = l($title . ' [#' . $id . ']', 'civicrm/group/search', array(
        'query' => array(
          'reset' => 1,
          'force' => 1,
          'context' => 'smog',
          'gid' => $id,
        )
      )
    );
  }
  return $result;
}

/**
 * Generate a table which summarizes the functionality of field extensions
 *
 * @param $field array-encoded field
 * @param $fexts array of fext definitions
 * @return string, HTML
 */
function _arms_util_field_extensions_table($field, $fexts) {
  $header = array(
    'Module',
    'Extension',
    'Data',
  );

  $rows = array();
  foreach ($fexts as $name => $defn) {
    $abbrdefn = $defn;
    unset($abbrdefn['#default_field']);
    unset($abbrdefn['#arms_ui_settings']);
    unset($abbrdefn['#module']);
    
    $names = array();
    if ($defn['#arms_ui_settings']['#title']) { $names[] = $defn['#arms_ui_settings']['#title']; }
    $names[] = '[' . $name . ']';
    
    $rows[] = array(
      array('data' => check_plain($defn['#module']), 'valign' => 'top'),
      array('data' => implode('<br/>', array_map('check_plain', $names)), 'valign' => 'top'),
      empty($abbrdefn) ? '' : ('<pre>' . check_plain(trim(print_r($abbrdefn, TRUE))) . '</pre>'),
    );
  }
  
  return theme('table', $header, $rows);
}

function arms_util_field_review_form(&$form_state, $field_id) {
  $field = arms_util_field_by_id($field_id);
  if (!isset($field['_param'])) {
    drupal_set_message(t('Error: Field %id not found', array(
      '%id' => $form['#arms_field_id'],
    )));
    return;
  }
  drupal_set_title(t('Field: @label (@full_name): Values', array(
    '@label' => $field['label'],
    '@full_name' => $field['_full_name'],
  )));
  
  $form = array(
    '#tree' => TRUE,
    '#redirect' => FALSE,
    '#arms_field_id' => $field_id,
  );
  
  $form['ignore_case'] = array(
    '#type' => 'checkbox',
    '#title' => 'Ignore case',
    '#default_value' => TRUE,
  );
  $form['trim'] = array(
    '#type' => 'checkbox',
    '#title' => 'Trim',
    '#default_value' => TRUE,
  );
  $form['order_by'] = array(
    '#type' => 'select',
    '#title' => 'Order By',
    '#default_value' => 'cnt',
    '#options' => array('value' => 'Value', 'cnt' => 'Record Count'),
  );
  $form['order_dir'] = array(
    '#type' => 'select',
    '#title' => 'Order',
    '#default_value' => 'desc',
    '#options' => array('desc' => 'Descending', 'asc' => 'Ascending'),
  );
  $form['limit'] = array(
    '#type' => 'textfield',
    '#title' => 'Max Display Values',
    '#default_value' => 200,
    '#element_validate' => array('_arms_util_field_validate_numeric'),
    '#required' => TRUE,
  );
  $form['submit'] =  array(
    '#weight' => 18,
    '#type' => 'submit',
    '#value' => t('Run') 
  );
  
  return $form;
}

function _arms_util_field_validate_numeric($element, &$form_state) {
  if (empty($element['#value']) || ! is_numeric($element['#value'])) {
    form_error($element, t('Must be a number'));
  }
}

function arms_util_field_review_form_submit(&$form, &$form_state) {
  // dpm(array('form' => $form, 'form_state' => $form_state));
  $field = arms_util_field_by_id($form['#arms_field_id']);
  $sql = arms_util_field_review_sql($field,
    $form_state['values']['ignore_case'],
    $form_state['values']['trim'],
    $form_state['values']['order_by'],
    $form_state['values']['order_dir'],
    (int) $form_state['values']['limit']
  );
  drupal_set_message('<pre>'.check_plain($sql) .'</pre>');
  drupal_set_message(arms_util_field_review_format($sql));
}

/**
 * Prepare a SQL query which summarizes the values of a field
 *
 * @param $field array-encoded field (see arms_util_field())
 * @param $is_ignore_case bool
 * @param $is_trim bool
 * @param $order_by string, either 'value' or 'cnt'
 * @param $order_dir string, either 'asc' or 'desc'
 * @param $limit int
 * @return string SQL
 */
function arms_util_field_review_sql($field, $is_ignore_case, $is_trim, $order_by, $order_dir, $limit) {
  $col = $field['column_name'];
  if ($is_ignore_case) {
    $col = 'lower(' . $col . ')';
  }
  if ($is_trim) {
    $col = 'trim(' . $col . ')';
  }
  
  $sql = strtr('
    select @col as value, count(*) as cnt
    from @table
    group by @col
    order by @order_by @order_dir 
    limit @limit', array(
    '@table' => $field['_table_name'],
    '@col' => $col,
    '@order_by' => $order_by,
    '@order_dir' => $order_dir,
    '@limit' => (int) $limit,
  ));
  return $sql;
}

/**
 * Prepare a formatted table to present the values of the field
 *
 * @param $sql A SQL expression which returns columns 'value' and 'cnt'
 * @return string HTML
 */
function arms_util_field_review_format($sql) {
  civicrm_initialize();
  
  $q = db_query($sql);
  $data = array();
  while ($row = db_fetch_array($q)) {
    if ($row['value'] === NULL || $row['value'] === FALSE) {
      $row['value'] = 'NULL';
    } elseif ($row['value'][0] == CRM_Core_DAO::VALUE_SEPARATOR) {
      arms_util_include_api('option');
      $options = arms_util_option_explode($row['value']);
      $row['value'] = 'LIST [' . implode('; ', array_map('check_plain', $options)) . ']';
    } else {
      $row['value'] = '"' . check_plain($row['value']) . '"';
    }
    $data[] = $row;
  }
  return theme('table', array('Value', 'Record Count'), $data);
}
