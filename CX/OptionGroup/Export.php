<?php
require_once 'XML/ArrayUtil.php';

class CX_OptionGroup_Export {
  var $optionGroup;

  static function createByName($optionGroupName) {
        require_once 'api/v2/Options.php';
        $optionGroupParams = array('name' => $optionGroupName);
        list($optionGroup) = civicrm_option_group_find($optionGroupParams);
        return (new CX_OptionGroup_Export($optionGroup));  
  }
  
  function CX_OptionGroup_Export(&$optionGroup) {
    $this->optionGroup = $optionGroup;
    $this->groupMapping = array(
      'attrs' => array('name', 'is_active', 'is_reserved'),
      'elems' => array('label', 'description'),
      'excludes' => array('id'),
    );
  
    $this->valueMapping = array(
      'attrs' => array('name', 'value', 'weight', 'is_active', 'is_default', 'grouping', 'filter', 'is_optgroup', 'is_reserved', 'component_id'),
      'elems' => array('label', 'description'),
      'excludes' => array('id', 'option_group_id'),  
    );
  }

  function export(&$cx) {
      require_once 'api/v2/Options.php';
      
      ## Add option group to XML
      $xmlOptionGroup = $cx->xml->addChild('option_group');
      array_to_xml($xmlOptionGroup, $this->optionGroup, $this->groupMapping);
    
      ## Fetch option values
      $optionValueParams = array('option_group_id' => $this->optionGroup['id']);
      $optionValues = civicrm_option_value_find($optionValueParams);
      #error_log(sprintf("Need to dump option group [%s]: %s\n", $customField['option_group_id'], print_r($options, true)));
      
      ## Add option values to XML
      $xmlOptionValues = $xmlOptionGroup->addChild('option_values');
      foreach ($optionValues as $optionValue) {
        $xmlOptionValue = $xmlOptionValues->addChild('option_value');
        array_to_xml($xmlOptionValue, $optionValue, $this->valueMapping);
      } ## each optionValue
  }

}

?>