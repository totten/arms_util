<?php

/**
 * This class manages a batch of import tasks. It processes each 
 * XML element and records a log of what it's done.
 */
class CX_CustomizationImporter {
  
  function CX_CustomizationImporter() {
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
  function processXml($xml) {
    require_once 'CX/OptionGroup/Import.php';
    require_once 'CX/CustomGroup/Import.php';
    require_once 'CX/UFGroup/Import.php';
    require_once 'XML/ArrayUtil.php';
  
    ## Import option groups/values
    $xmlOptionGroups = $xml->xpath('/customizations/option_group');
    foreach ($xmlOptionGroups as $xmlOptionGroup) {
      $result = CX_OptionGroup_Import::import($xmlOptionGroup, $this);
      if ($result['is_error']) { return $result; }
    }
      
    ## Import custom groups/fields
    $xmlCustomGroups = $xml->xpath('/customizations/custom_group');
    foreach ($xmlCustomGroups as $xmlCustomGroup) {
      $result = CX_CustomGroup_Import::import($xmlCustomGroup, $this);
      if ($result['is_error']) { return $result; }
    }
      
    ## Import UF groups/fields
    $xmlUfGroups = $xml->xpath('/customizations/uf_group');
    foreach ($xmlUfGroups as $xmlUfGroup) {
      $result = CX_UFGroup_Import::import($xmlUfGroup, $this);
      if ($result['is_error']) { return $result; }
    }
  
    require_once 'CRM/Core/BAO/Cache.php';
    CRM_Core_BAO_Cache::deleteGroup( 'contact fields' );
  
    cache_clear_all();
    drupal_get_schema(FALSE, TRUE);
  
    return array();
  }

  /**
   * Import an XML file
   *
   * @param $file string
   * @return array; if there's an error, then array includes 'is_error'
   */
  function processFile($file) {
    $xml = simplexml_load_file($file);
    if (!$xml) {
      return arms_cx_error('simplexml_load_file', $file, libxml_get_errors());
    }
    return $this->processXml($xml);
  }

  /**
   * Record a successfully imported value/entity
   *
   * @param $xml SimpleXML node representing the imported entity
   * @param $type string, e.g. 'ufGroup', 'customGroup', 'customField'
   * @param $id int, the numerical ID of the new or updated entity
   * @return void
   */
  function postProcess($xml, $type, $id) {
    foreach ($xml->children('urn:arms_util:cx:importer') as $key => $value) {
      switch ($key) {
        case 'drupal-var':
          //printf("variable_set($value,$id)\n");
          variable_set( (string) $value, $id);
          break;
        default:
          //print_r(array($key => $value, 'type' => $type, 'id' => $id));
      }
    }
  }

}
