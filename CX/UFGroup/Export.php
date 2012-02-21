<?php
require_once 'XML/ArrayUtil.php';

class CX_UFGroup_Export {
  var $ufGroup;

  function CX_UFGroup_Export(&$ufGroup) {
    $this->ufGroup = $ufGroup;
    
    $this->groupMapping = array(
      'attrs' => array('is_active','group_type', 'collapse_display', 'add_captcha', 'is_map', 'is_edit_link', 'is_uf_link', 'is_update_dupe', 'cancel_URL', 'is_cms_user', 'post_URL'),
      'elems' => array('title', 'help_pre', 'help_post', 'notify'),
      'excludes' => array('id','add_to_group_id', 'limit_listings_group_id'),
    );
  
    $this->fieldMapping = array(
      'attrs' => array('is_active','is_view', 'is_required', 'weight', 'in_selector', 'is_searchable', 'location_type_id', 'phone_type_id', 'field_type'),
      'elems' => array('help_post', 'visibility', 'label'),
      'excludes' => array('id','uf_group_id', 'field_name'), ## field_name requires special handling
    );
    
    $this->joinMapping = array(
      'attrs' => array('is_active', 'module', 'entity_table', 'entity_id','weight'),
      'elems' => array(),
      'excludes' => array('id','uf_group_id'),
    );
  }

  function export(&$cx) {
    require_once 'api/v2/UFGroup.php';
    require_once 'api/v2/UFJoin.php';
    require_once 'api/v2/CustomGroup.php';
  
    ## Add group to XML
    $xmlUfGroup = $cx->xml->addChild('uf_group');
    array_to_xml($xmlUfGroup, $this->ufGroup, $this->groupMapping);
  
    ## Fetch fields
    $ufFieldsParams = array('uf_group_id' => $this->ufGroup['id']);
    $ufFields =& civicrm_uf_field_find($ufFieldsParams);
  
    ## Add fields to XML
    $xmlUfFields = $xmlUfGroup->addChild('uf_fields');
    foreach ($ufFields as $ufField) {
      ## Add field to XML
      $xmlUfField = $xmlUfFields->addChild('uf_field');
      array_to_xml($xmlUfField, $ufField, $this->fieldMapping);
  
      ## $fieldMapping can't handle field_name. Let's do that now!
      if (substr($ufField['field_name'], 0, 7) == 'custom_') {
        ## This is a custom field whose name includes the surrogate key.
        ## Convert surrogate key to business key (customGroup[name] + customField[column_name]).
        
        ## Find custom field/group
        $customFieldParams = array('id' => substr($ufField['field_name'], 7));
        list($customField) = civicrm_custom_field_find($customFieldParams);
        if (empty($customField) || $customField['is_error']) { print "Unknown field name: " . $ufField['field_name']; exit(1); }
        $customGroupParams = array('id' => $customField['custom_group_id']);
        $customGroup = civicrm_custom_group_get($customGroupParams);
        if (empty($customGroup) || $customGroup['is_error']) { print "Unknown custom group: " . $customField['custom_group_id']; exit(1); }
  
        ## Clean column name -- make it more portable
        #$customField['column_name'] = preg_replace('/_' . $customField['id'] .'$/', '', $customField['column_name'] );
  
        ## Add field_name to XML
        $xmlFieldName = $xmlUfField->addChild('field_name');
        $xmlFieldName['custom_group'] = $customGroup['name'];
        $xmlFieldName['custom_field'] = $customField['column_name'];
      } else {
        ## Add field_name to XML
        $xmlFieldName = $xmlUfField->addChild('field_name', $ufField['field_name']);
      }
    } ## each ufField

    ## Fetch UFJoins
    ## Note that some UFJoins are generic and transferrable (e.g. joining a UFGroup to the generic "User Registrion" construct)
    ## Other UFJoins involve specific objects (e.g. joining a UFGroup to a specific event).
    $ufJoinParams = array(
      'uf_group_id' => $this->ufGroup['id'],
      'entity_id' => 'null' ## Only get portable UFJoins which don't use specific event/contribution page
    );
    $ufJoins = civicrm_uf_join_find($ufJoinParams);

    ## Add joins to XML
    $xmlUfJoins = $xmlUfGroup->addChild('uf_joins');
    foreach ($ufJoins as $ufJoin) {
      $xmlUfJoin = $xmlUfJoins->addChild('uf_join');
      array_to_xml($xmlUfJoin, $ufJoin, $this->joinMapping);
    }
  }

}

?>