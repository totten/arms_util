<?php

/**
 * Drush callback
 *
 * Import custom groups, UF groups, and/or option groups from XML
 */
function _arms_cx_drush_import() {
  #####################################################################
  $files = func_get_args();
  if (empty($files)) {
    print "Please specify an XML file\n";
  }
  
  #####################################################################
  civicrm_initialize();
  foreach ($files as $file) {
    print "Processing $file\n";
    $result = arms_cx_import_file($file);
    if ($result['is_error']) {
      print_r($result);
      return;
    }
  }
}

/**
 * Drush callback
 *
 * Export all custom groups to XML
 */
function _arms_cx_drush_export_customgroups() {
  civicrm_initialize();
  require_once 'CX/CustomizationExporter.php';
  require_once 'CX/UFGroup/Export.php';
  require_once 'CX/OptionGroup/Export.php';
  require_once 'CX/CustomGroup/Export.php';
  require_once 'api/v2/UFGroup.php';
  require_once 'api/v2/CustomGroup.php';
  
  $cx = new CX_CustomizationExporter();
  $customGroupsParams = array();
  $customGroups = civicrm_custom_group_find($customGroupsParams);
  _arms_cx_enqueue_all($cx, 'CX_CustomGroup_Export', $customGroups);
  
  $xmlNode =& $cx->createXml();
  $dom = dom_import_simplexml($xmlNode)->ownerDocument;
  $dom->formatOutput = true;
  echo $dom->saveXML();
}

/**
 * Drush callback
 *
 * Export all UF groups (profiles) to XML
 */
function _arms_cx_drush_export_ufgroups() {
  civicrm_initialize();
  require_once 'CX/CustomizationExporter.php';
  require_once 'CX/UFGroup/Export.php';
  require_once 'CX/OptionGroup/Export.php';
  require_once 'CX/CustomGroup/Export.php';
  require_once 'api/v2/UFGroup.php';
  require_once 'api/v2/CustomGroup.php';
  
  $cx = new CX_CustomizationExporter();
  $ufGroupsParams = array();
  $ufGroups = civicrm_uf_group_find($ufGroupsParams);
  _arms_cx_enqueue_all($cx, 'CX_UFGroup_Export', $ufGroups);  
  
  $xmlNode =& $cx->createXml();
  $dom = dom_import_simplexml($xmlNode)->ownerDocument;
  $dom->formatOutput = true;
  echo $dom->saveXML();
}

/**
 * Enqueue export tasks of type $clazz for all exportable
 * items in $arr.
 */
function _arms_cx_enqueue_all($cx, $clazz, $arr) {
  foreach ($arr as $obj) {
    $cx->enqueue(new $clazz($obj));
  }
}

/**
 * Import an XML document
 *
 * This function may or may not participate properly in transactions.
 * If the import only involves <uf_group> elements, then it's probably
 * safe. If the import involves <custom_group> elements, then the
 * import requires DDL which (in MySQL) breaks transactions.
 *
 * @param $xml SimpleXMLElement <customizations>
 * @return array; if there's an error, then array includes 'is_error'
 */
function arms_cx_import($xml) {
  require_once 'CX/CustomizationImporter.php';
  $cxImporter = new CX_CustomizationImporter();
  return $cxImporter->processXml($xml);
}

/**
 * Import an XML file
 *
 * @param $file string
 * @return array; if there's an error, then array includes 'is_error'
 */
function arms_cx_import_file($file) {
  require_once 'CX/CustomizationImporter.php';
  $cxImporter = new CX_CustomizationImporter();
  return $cxImporter->processFile($file);
}

/**
 * Return an error record
 */
function arms_cx_error($op, $input, $result) {
  return array(
    'op' => $op,
    'input' => $input,
    'result' => $result,
    'is_error' => 1,
  );
}

/**
 * (delegate) hook_requirements
 *
function arms_util_cx_requirements($phase) {
  arms_util_include_api('cx');
  $t = get_t();

  if ($phase == 'runtime') {
    $cx_files = module_invoke_all('arms_util_cx');
    foreach ($cx_files as $key => $cx) {
      $req = array();
      $req['title'] = $t($cx['title']),
      $req['description'] = $t('Load customizations in %file', array(
        '%file' => $cx['file']
      ));
      $req['value'] = false;
      $req['severity'] = false;

      $requirements['cx_' . $key] = $req;
    }
  }
}
*/