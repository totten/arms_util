<?php
require_once 'XML/ArrayUtil.php';

class CX_CustomGroup_Export {
  var $customGroup;

  static function createByName($customGroupName) {
        require_once 'api/v2/CustomGroup.php';

        $customGroupParams = array('name' => $customGroupName);
        $customGroup =& civicrm_custom_group_get($customGroupParams);
        return (new CX_CustomGroup_Export($customGroup));  
  }
  
  function CX_CustomGroup_Export(&$customGroup) {
    $this->customGroup = $customGroup;
    $this->mappings = array();
    $this->mappings['custom_group'] = array(
      'attrs' => array('name', 'extends', 'style', 'weight', 'collapse_display', 'is_active', 'is_multiple'), // , 'min_multiple', 'max_multiple'
      'elems' => array('title', 'help_pre', 'help_post'),
      'excludes' => array('id', 'extends_entity_column_name', 'extends_entity_column_value', 'table_name', 'is_error'),
    );
    
    $this->mappings['custom_field'] = array(
      'attrs' => array('column_name', 'data_type', 'html_type', 'weight', 'is_active', 'is_required', 'is_searchable', 'is_search_range', 'is_view', 'options_per_line', 'text_length'),
      'elems' => array('label', 'default_value', 'help_pre', 'help_post', 'javascript', 'attributes', 'mask'),
      'excludes' => array('id', 'custom_group_id', 'option_group_id', 'date_parts', 'start_date_years', 'end_date_years', 'note_columns', 'note_rows'),
    );
    
    $this->mappings['custom_field#Memo'] = array(
      'attrs' => array_merge($this->mappings['custom_field']['attrs'], array('note_columns', 'note_rows')),
      'elems' => $this->mappings['custom_field']['elems'],
      'excludes' => array_diff($this->mappings['custom_field']['excludes'], array('note_columns', 'note_rows')),
    );
    
    $this->mappings['custom_field#Date'] = array(
      'attrs' => array_merge($this->mappings['custom_field']['attrs'], array('start_date_years', 'end_date_years')),
      'elems' => array_merge($this->mappings['custom_field']['elems'], array('date_parts')),
      'excludes' => array_diff($this->mappings['custom_field']['excludes'], array('date_parts', 'start_date_years', 'end_date_years')),
    );

  }

  function export(&$cx) {
    require_once 'api/v2/Options.php';
    require_once 'api/v2/CustomGroup.php';

    ## Add custom group to XML
    $xmlCustomGroup =& $cx->xml->addChild('custom_group');
    array_to_xml($xmlCustomGroup, $this->customGroup, $this->mappings['custom_group']);
    
    ## Load custom fields for custom group
    $customFieldParams = array('custom_group_id' => $this->customGroup['id']);
    $customFields =& civicrm_custom_field_find($customFieldParams);
  
    ## Add custom fields to XML
    $xmlCustomFields =& $xmlCustomGroup->addChild('custom_fields');
    foreach ($customFields as $customField) {
      ## Tweak array
      # $customField['column_name'] = preg_replace('/_' . $customField['id'] .'$/', '', $customField['column_name'] );
    
      ## Add custom field to XML; note that we may tweak mappings based on data type
      if (isset($this->mappings['custom_field#' . $customField['data_type']])) {
        $fieldMapping = $this->mappings['custom_field#' . $customField['data_type']];
      } else {
        $fieldMapping = $this->mappings['custom_field'];
      }
      $xmlCustomField =& $xmlCustomFields->addChild('custom_field');
      array_to_xml($xmlCustomField, $customField, $fieldMapping);
  
      if (! empty($customField['option_group_id'])) {
        ## We'll store a reference to the option group's name and then queue it for export
        $optionGroupParams = array('id' => $customField['option_group_id']);
        list($optionGroup) = civicrm_option_group_find($optionGroupParams);
        
        ## Add option group to XML
        $xmlOptionGroup = $xmlCustomField->addChild('option_group');
        $xmlOptionGroup->addAttribute('name', $optionGroup['name']);
        
        ## Queue for full export
        $cx->enqueue(new CX_OptionGroup_Export($optionGroup));
      }
    } ## each customField

  }

}

?>