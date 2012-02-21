<?php
require_once 'XML/ArrayUtil.php';

class CX_CustomGroup_Import {

  /**
   * Import the <custom_fields> XML subtree
   *
   * @param $xmlOptionGroup SimpleXMLElement <custom_fields>
   * @return array; if an error occurred, includes key is_error
   */
  static function import($xmlCustomGroup, $cxImporter) {
    require_once 'api/v2/CustomGroup.php';
    require_once 'api/v2/Options.php';
    
    ## Parse CustomGroup XML
    $customGroup = xml_to_array($xmlCustomGroup, array('custom_fields'));
    $customGroup['extends'] = array($customGroup['extends']);
    
    ## Create or update CustomGroup
    $result =& civicrm_custom_group_add($customGroup);
    if ($result['is_error']) {
      return arms_cx_error('civicrm_custom_group_add', $customGroup, $result);
    }
    $customGroup = $result;
    $cxImporter->postProcess($xmlCustomGroup, 'customGroup', $customGroup['id']);
    
    foreach ($xmlCustomGroup->xpath('custom_fields/custom_field') as $xmlCustomField) {
      ## Parse CustomField XML
      $customField = xml_to_array($xmlCustomField, array('option_group'));
      $customField['custom_group_id'] = $customGroup['id'];
      if (! empty($xmlCustomField->option_group['name'])) {
        $optionGroupSearch = array('name' => (string)$xmlCustomField->option_group['name']);
        $result =& civicrm_option_group_find($optionGroupSearch);
        if ($result['is_error']) {
          return arms_cx_error('civicrm_option_group_find', $optionGroupSearch, $result);
        }
        if (sizeof($result) != 1) {
          return arms_cx_error('civicrm_option_group_find -- wrong count', $optionGroupSearch, $result);
        }
        $customField['option_group_id'] = $result[0]['id'];
      }
      
      ## Cleanup CustomField
      $bak = $customField;
      if ($customField['data_type'] == 'String' && empty($customField['text_length'])) {
        $customField['text_length'] = 255;
      }
      if (empty($customField['name']) && version_compare(CRM_Utils_System::version(), '3.3.0') >= 0) {
        $customField['name'] = $customField['column_name'];
      }
      
      ## Create or update CustomField
      $result =& civicrm_custom_field_add($customField);
      if ($result['is_error']) {
        return arms_cx_error('civicrm_custom_field_add', $customField, $result);
      }
      $cxImporter->postProcess($xmlCustomField, 'customField', $result['id']);
    }
    
    return array();
  }
}
?>