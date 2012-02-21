<?php
require_once 'XML/ArrayUtil.php';

class CX_UFGroup_Import {

  /**
   * Import the <uf_fields> XML subtree
   *
   * @param $xmlOptionGroup SimpleXMLElement <uf_fields>
   * @return array; if an error occurred, includes key is_error
   */
  static function import($xmlUfGroup, $cxImporter) {
    require_once 'api/v2/UFGroup.php';
    require_once 'api/v2/UFJoin.php';
    require_once 'api/v2/CustomGroup.php';    
    
    ## Parse UFGroup XML
    $ufGroup = xml_to_array($xmlUfGroup, array('uf_fields'));
    
    ## Create or update UFGroup
    $result =& civicrm_uf_group_create($ufGroup);
    if ($result['is_error']) {
      return arms_cx_error('civicrm_uf_group_create', $ufGroup, $result);
    }
    $ufGroup = $result;
    $cxImporter->postProcess($xmlUfGroup, 'ufGroup', $ufGroup['id']);
    
    foreach ($xmlUfGroup->xpath('uf_fields/uf_field') as $xmlUfField) {
      $xmlUfFieldImporter = $xmlUfField->children('urn:arms_util:cx:importer');
      $ignoreError = ('true' == trim((string)$xmlUfFieldImporter->{'ignore-error'}));
      
      ## Parse UfField XML
      $ufField = xml_to_array($xmlUfField);
      $ufField['group_id'] = $ufGroup['id'];

      ## Determine field name; if it's a custom field, construct field_name with surrogate key
      if (empty($xmlUfField->field_name['custom_group'])) {
        $ufField['field_name'] = (string)$xmlUfField->field_name;
      } else {
        $customGroupParams = array('name' => (string)$xmlUfField->field_name['custom_group']);
        $customGroup = civicrm_custom_group_get($customGroupParams);
        if (empty($customGroup) || $customGroup['is_error']) {
          if ($ignoreError) { continue; }
          return arms_cx_error('civicrm_custom_group_get', $customGroupParams, $customGroup);
        }
        
        $customFieldParams = array('custom_group_id' => $customGroup['id'],  'column_name' => (string)$xmlUfField->field_name['custom_field']);
        $customFields = civicrm_custom_field_find($customFieldParams);
        if (empty($customFields) || $customFields['is_error']) {
          if ($ignoreError) { continue; }
          return arms_cx_error('civicrm_custom_field_find', $customFieldParams, $customFields);
        }
        $customField =& $customFields[0];
        
        #$customField = findCustomField($xmlUfField->field_name['custom_group'], $xmlUfField->field_name['custom_field']);
        $ufField['field_name'] = 'custom_' . $customField['id'];
      }
      
      ## Create or update UfField
      # printf("Adding ufField: %s\n", print_r($ufField, true));
      $result =& civicrm_uf_field_create($ufField['group_id'], $ufField);
      if ($result['is_error']) {
        return arms_cx_error('civicrm_uf_field_create', $ufField, $result);
      }
      $cxImporter->postProcess($xmlUfField, 'ufField', $result['id']);
    } ## each field
    
    foreach ($xmlUfGroup->xpath('uf_joins/uf_join') as $xmlUfJoin) {
      $ufJoin = xml_to_array($xmlUfJoin);
      $ufJoin['uf_group_id'] = $ufGroup['id'];
      
      $result = civicrm_uf_join_add($ufJoin);
      if (empty($result) || $result['is_error']) {
        return arms_cx_error('civicrm_uf_join_add', $ufJoin, $result);
      }
    } ## each join

    return array();
  }
}
?>
