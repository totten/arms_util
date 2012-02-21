<?php
require_once 'XML/ArrayUtil.php';

class CX_OptionGroup_Import {

  /**
   * Import the <option_values> XML subtree
   *
   * @param $xmlOptionGroup SimpleXMLElement <option_values>
   * @return array; if an error occurred, includes key is_error
   */
  static function import($xmlOptionGroup, $cxImporter) {
    require_once 'api/v2/Options.php';
    
    ## Parse OptionGroup XML
    $optionGroup = xml_to_array($xmlOptionGroup, array('option_values'));
    
    ## Create or update OptionGroup
    #print_r(array('optionGroup' => $optionGroup));
    $result =& civicrm_option_group_create($optionGroup);
    if ($result['is_error']) {
      return arms_cx_error('civicrm_option_group_create', $optionGroup, $result);
    }
    $optionGroup =& $result['result'];
    $cxImporter->postProcess($xmlOptionGroup, 'optionGroup', $optionGroup['id']);
    
    foreach ($xmlOptionGroup->xpath('option_values/option_value') as $xmlOptionValue) {
      ## Parse OptionValue XML
      $optionValue = xml_to_array($xmlOptionValue);
      $optionValue['option_group_id'] = $optionGroup['id'];
      
      ## Create or update OptionValue
      #print_r(array('optionValue' => $optionValue));
      $result =& civicrm_option_value_create($optionValue);
      if ($result['is_error']) {
        return arms_cx_error('civicrm_option_value_create', $optionValue, $result);
      }
      $cxImporter->postProcess($xmlOptionValue, 'optionValue', $result['id']);
    }
    
    return array();
  }
}
?>
