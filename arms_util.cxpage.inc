<?php

function _arms_util_cxpage_init() {
  civicrm_initialize();
  arms_util_include_api('cx');
  require_once 'CX/CustomizationExporter.php';
  require_once 'CX/UFGroup/Export.php';
  require_once 'CX/OptionGroup/Export.php';
  require_once 'CX/CustomGroup/Export.php';
  require_once 'api/v2/UFGroup.php';
  require_once 'api/v2/CustomGroup.php';
}

/**
 * (Form API)
 *
 * Render a configurable export form. The form displays a list of all
 * available CustomData and UF groups (each with a checkbox). After
 * submission, all the CustomData/UF widgets are collapsed, and a
 * new section is displayed with the export-XML.
 */
function arms_util_cxpage_export(&$form_state) {
  _arms_util_cxpage_init();
  $cx = new CX_CustomizationExporter();

  $customGroupsParams = array();
  $customGroups = civicrm_custom_group_find($customGroupsParams);
  $selectedCustomGroups = array();
  foreach ($customGroups as $customGroup) {
    if ($form_state['post']['cg'][$customGroup['id']]) {
      $selectedCustomGroups[] = $customGroup;
    }
  }
  if (!empty($selectedCustomGroups)) {
    _arms_cx_enqueue_all($cx, 'CX_CustomGroup_Export', $selectedCustomGroups);
  }
  
  $ufGroupsParams = array();
  $ufGroups = civicrm_uf_group_find($ufGroupsParams);
  $selectedUfGroups = array();
  foreach ($ufGroups as $ufGroup) {
    if ($form_state['post']['uf'][$ufGroup['id']]) {
      $selectedUfGroups[] = $ufGroup;
    }
  }
  if (!empty($selectedUfGroups)) {
    _arms_cx_enqueue_all($cx, 'CX_UFGroup_Export', $selectedUfGroups);
  }
  
  $has_selections = !empty($selectedCustomGroups) || !empty($selectedUfGroups);
  // dpm(array('form_state' => $form_state, 'customGroups' => $customGroups, 'selectedCustomGroups' => $selectedCustomGroups, 'ufGroups' => $ufGroups, 'selectedUfGroups' => $selectedUfGroups));

  // build form

  $form = array(
    '#tree' => TRUE,
    '#redirect' => FALSE,
  );
  
  $form['intro'] = array(
    '#value' => t('<p>Please select any custom-data groups and/or profile forms which you wish to export and then submit. After submission, you will see an XML representation of your selection(s). The XML can be copied for use on another site.</p>'),
  );
  
  if ($has_selections) { 
    $xmlNode =& $cx->createXml();
    $dom = dom_import_simplexml($xmlNode)->ownerDocument;
    $dom->formatOutput = true;
    
    $form['xml'] = array(
      '#type' => 'textarea',
      // '#disabled' => TRUE, // breaks highlight/copy functionality in Firefox 3.6 on OS X
      '#title' => t('XML'),
      '#wysiwyg' => FALSE,
      '#rows' => 25,
      '#value' => $dom->saveXML(),
    );
  }
  
  $form['cg'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE, // $has_selections,
    '#title' => t('Custom Data'),
  );
  if ($has_selections) {
    $form['cg']['#title'] .= ' (' . count($selectedCustomGroups) . ')'; 
  }
  
  foreach ($customGroups as $customGroup) {
    $form['cg'][$customGroup['id']] = array(
      '#type' => 'checkbox',
      '#title' => t('(@extends) @title', array(
        '@extends' => $customGroup['extends'],
        '@title' => $customGroup['title']
      )),
      '#default_value' => FALSE,
    );
  }

  $form['uf'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE, // $has_selections,
    '#title' => t('Profiles'),
  );
  if ($has_selections) {
    $form['uf']['#title'] .= ' (' . count($selectedUfGroups) . ')'; 
  }
  
  foreach ($ufGroups as $ufGroup) {
    $form['uf'][$ufGroup['id']] = array(
      '#type' => 'checkbox',
      '#title' => t('(@type) @title [#@id]', array(
        '@type' => $ufGroup['group_type'],
        '@title' => $ufGroup['title'],
        '@id' => $ufGroup['id'],
      )),
      '#default_value' => FALSE,
    );
  }
    
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Export',
  );
  
  return $form;
}

/**
 * (Form API)
 */
function arms_util_cxpage_import() {
  _arms_util_cxpage_init();
  $form = array(
    '#tree' => TRUE,
    '#redirect' => FALSE,
  );

  $form['intro'] = array(
    '#value' => t('<p>If you intend to import the customizations on a new site, then the process can be fairly straight-forward. If you intend to import on an existing site, then you should review the XML before importing.</p>'),
  );

  $form['xml'] = array(
    '#type' => 'textarea',
    '#title' => t('XML'),
    '#wysiwyg' => FALSE,
    '#rows' => 25,
    '#default_value' => '',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Import',
  );
  
  return $form;
}

function arms_util_cxpage_import_submit(&$form, &$form_state) {

  $xml = simplexml_load_string($form_state['values']['xml']);
  if (!$xml) {
    $result = arms_cx_error('simplexml_load_string', '(omitted for brevity)', libxml_get_errors());
  } else {
    $result = arms_cx_import($xml);
  }
  
  if (@$result['is_error']) {
    drupal_set_message(t('<p>Error while processing %op</p> <pre>INPUT: @input</pre> <pre>RESULT: @result</pre>', array(
      '%op' => $result['op'],
      '@input' => print_r($result['input'], TRUE),
      '@result' => print_r($result['result'], TRUE),
    )), 'error');
  } else {
    drupal_set_message(t('Import executed.'));
  }
}
