<?php
define('ARMS_UTIL_TALEND_EXPORT_NULL_TYPE', 'force-null');

/**
 * (Drush callback)
 *
 * Generate a list of entity records and dump them to a file
 *
 * @param $fieldsFile string, a flat file which lists expected custom data fields, one per line
 * @param $sqlFile string, an output file which records the SQL query used for fetching data
 * @param $csvFile string, an output file which lists data records
 */
function arms_util_talend_export_drush($entity, $fieldsFile, $sqlFile, $csvFile) {
  if (empty($entity) || empty($fieldsFile) || empty($sqlFile) || empty($csvFile)) {
    printf("params: <entity> <fields-in-file> <sql-out-file> <rcsv-out-file>\n");
    return;
  }
  
  civicrm_initialize();
  $coreFields = arms_util_talend_getCoreFields($entity);
  $customFields = arms_util_talend_getCustomFields($entity, explode("\n", file_get_contents($fieldsFile)));
  $select = arms_util_talend_createQuery($entity, $coreFields, $customFields);
  file_put_contents($sqlFile, $select->toSQL());
  arms_util_talend_dumpCsv($select, $csvFile);
}

/**
 * (Drush callback)
 *
 * @param $fieldsFile string, a flat file which lists expected custom data fields, one per line
 * @param $csvFile string, an output file which lists data records
 * @param $xmlFile string, an output file which lists Talend-compatible schema elements
 */
function arms_util_talend_schema_drush($entity, $fieldsFile, $xmlFile) {
  civicrm_initialize();
  $customFields = arms_util_talend_getCustomFields($entity, 
    array_filter(explode("\n", file_get_contents($fieldsFile)), 'strlen'));
  $customFieldsXml = arms_util_talend_createCustomFieldsXml($customFields);

  // force pretty printing with extra encode/decode cycle
  $outXML = $customFieldsXml->saveXML();
  $xml = new DOMDocument();
  $xml->encoding = 'iso-8859-1';
  $xml->preserveWhiteSpace = false;
  $xml->formatOutput = true;
  $xml->loadXML($outXML);
  file_put_contents($xmlFile, $xml->saveXML());  
}

/**
 * Iterate through the query results and write to a CSV file
 *
 * @param $select ARMS_Util_Select
 * @param $csvFile string, an output file which lists data records
 */
function arms_util_talend_dumpCsv($select, $csvFile) {
  if ($fh = fopen($csvFile, 'w')) {
    $q = db_query($select->toSQL());
    $header = FALSE;
    while ($row = db_fetch_array($q)) {
      if ($header === FALSE) {
        $header = array_keys($row);
        fputcsv($fh, $header);
      }
      fputcsv($fh, $row);
    }
    fclose($fh);
  } else {
    throw new Exception("Failed to open " . $csvFile);
  }
}

/**
 * @param $entity string
 * @param $coreFields array($fieldName)
 * @param $customFields array($header => $fieldSpec)
 * @return ARMS_Util_Select
 */
function arms_util_talend_createQuery($entity, $coreFields, $customFields) {
  // print_r(array('entity'=>$entity, 'corefields'=>$coreFields, 'customfields'=>$customFields));
  arms_util_include_api('query');
  $select = arms_util_query($entity);
  foreach ($coreFields as $fieldName) {
    $select->addSelect($entity . '.' . $fieldName);
  }
  foreach ($customFields as $header => $field) {
    if ($field['data_type'] == ARMS_UTIL_TALEND_EXPORT_NULL_TYPE) {
      $select->addSelect('null as `' . $header.'`');
    } else {
      $select->addCustomField("{".$entity."}.id", $field, $header); 
    }
  }
  return $select;
}

function arms_util_talend_getCoreFields($entity) {
  arms_util_include_api('array');
  switch($entity) {
    case 'civicrm_contact':
      require_once 'CRM/Contact/DAO/Contact.php';
      return arms_util_array_collect(CRM_Contact_DAO_Contact::fields(), 'name');
    case 'civicrm_contact':
      require_once 'CRM/Activity/DAO/Activity.php';
      return arms_util_array_collect(CRM_Activity_DAO_Activity::fields(), 'name');
    default:
      throw new Exception('Unrecognized entity: ' . $entity);
  }
}

/**
 * For each expected column, determine the field mapping
 *
 * @param $entity string
 * @param $expectedHeaders array(header) list of custom data columns which should be outputted
 * @return array($header => $fieldSpec); if a field is unavailable, then fieldSpec is substituted using ARMS_UTIL_TALEND_EXPORT_NULL_TYPE
 */
function arms_util_talend_getCustomFields($entity, $expectedHeaders) {
  arms_util_include_api('thinapi');
  $customFields = arms_util_thinapi_getFields($entity); // array(fieldSpec)
  $newCustomFields = arms_util_talend_filterCustomFields($entity, $customFields); // array(header => fieldSpec)
  
  $result = array(); // array(header => fieldSpec)
  foreach ($expectedHeaders as $expectedHeader) {
    if ($newCustomFields[$expectedHeader]) {
      $result[$expectedHeader] = $newCustomFields[$expectedHeader];
    } else {
      $result[$expectedHeader] = array(
        'data_type' => ARMS_UTIL_TALEND_EXPORT_NULL_TYPE,
      );
    }
  }
  
  return $result;
}

/**
 * Translate custom fields list to a Talend XML
 *
 * @return SimpleXMLElement <schema>
 */
function arms_util_talend_createCustomFieldsXml($customFields) {
  $xmlSchema = simplexml_load_string('<schema></schema>');
  foreach ($customFields as $header => $customField) {
    $xmlColumn = $xmlSchema->addChild('column');
    $xmlColumn->addAttribute('label', preg_replace('/[^a-zA-Z0-9_]/', '_', $header));
    $xmlColumn->addAttribute('originalDbColumnName', $header);
    $xmlColumn->addAttribute('default', ''); // $customField['default_value']);
    
    // TODO: multi-value fields are always strings
    
    switch ($customField['data_type']) {
      case 'Country':
      case 'StateProvince':
      case 'File':
        $xmlColumn->addAttribute('talendType', 'id_Long');
        $xmlColumn->addAttribute('type', 'BIGINT');
        $xmlColumn->addAttribute('key', 'true');
    
        $xmlColumn->addAttribute('length', '10');
        $xmlColumn->addAttribute('nullable', 'true');
        $xmlColumn->addAttribute('pattern', '');
        $xmlColumn->addAttribute('precision', '0');
        $xmlColumn->addAttribute('comment', '');
        break;
        
      case 'Boolean':
        $xmlColumn->addAttribute('talendType', 'id_Byte');
        $xmlColumn->addAttribute('type', 'TINYINT');
        $xmlColumn->addAttribute('key', 'false');
    
        $xmlColumn->addAttribute('length', '3');
        $xmlColumn->addAttribute('nullable', 'true');
        $xmlColumn->addAttribute('pattern', '');
        $xmlColumn->addAttribute('precision', '0');
        $xmlColumn->addAttribute('comment', '');
        break;
      
      case 'String':
      case 'Link':
      case ARMS_UTIL_TALEND_EXPORT_NULL_TYPE:
        $xmlColumn->addAttribute('talendType', 'id_String');
        $xmlColumn->addAttribute('type', 'VARCHAR');
        $xmlColumn->addAttribute('key', 'false');
        
        if (empty($customField['text_length'])) { // e.g. NULL_TYPE
          $xmlColumn->addAttribute('length', 255);
        } else {
          $xmlColumn->addAttribute('length', $customField['text_length']);
        }
        $xmlColumn->addAttribute('nullable', 'true');
        $xmlColumn->addAttribute('pattern', '');
        $xmlColumn->addAttribute('precision', '0');
        if ($customField['data_type'] == ARMS_UTIL_TALEND_EXPORT_NULL_TYPE) {
          $xmlColumn->addAttribute('comment', 'Unknown field. Fall back to string.');
        } else {
          $xmlColumn->addAttribute('comment', '');
        }
        break;

      case 'Memo':
        $xmlColumn->addAttribute('talendType', 'id_String');
        $xmlColumn->addAttribute('type', 'TEXT');
        $xmlColumn->addAttribute('key', 'false');
        
        $xmlColumn->addAttribute('length', 65535);
        $xmlColumn->addAttribute('nullable', 'true');
        $xmlColumn->addAttribute('pattern', '');
        $xmlColumn->addAttribute('precision', '0');
        $xmlColumn->addAttribute('comment', '');
        break;
        
      case 'Money':
        $xmlColumn->addAttribute('talendType', 'id_Float');
        $xmlColumn->addAttribute('type', 'FLOAT');
        $xmlColumn->addAttribute('key', 'false');
    
        $xmlColumn->addAttribute('length', '20');
        $xmlColumn->addAttribute('nullable', 'true');
        $xmlColumn->addAttribute('pattern', '');
        $xmlColumn->addAttribute('precision', '0');
        $xmlColumn->addAttribute('comment', '');
        break;
        
      case 'Float':
        $xmlColumn->addAttribute('talendType', 'id_Double');
        $xmlColumn->addAttribute('type', 'DOUBLE');
        $xmlColumn->addAttribute('key', 'false');
    
        $xmlColumn->addAttribute('length', '22');
        $xmlColumn->addAttribute('nullable', 'true');
        $xmlColumn->addAttribute('pattern', '');
        $xmlColumn->addAttribute('precision', '0');
        $xmlColumn->addAttribute('comment', '');
        break;
        
      case 'Int':
        $xmlColumn->addAttribute('talendType', 'id_Integer');
        $xmlColumn->addAttribute('type', 'INT');
        $xmlColumn->addAttribute('key', 'false');
    
        $xmlColumn->addAttribute('length', '10');
        $xmlColumn->addAttribute('nullable', 'true');
        $xmlColumn->addAttribute('pattern', '');
        $xmlColumn->addAttribute('precision', '0');
        $xmlColumn->addAttribute('comment', '');
        break;
        
      case 'Date':
        $xmlColumn->addAttribute('talendType', 'id_Date');
        $xmlColumn->addAttribute('type', 'DATETIME');
        $xmlColumn->addAttribute('key', 'false');
    
        $xmlColumn->addAttribute('length', '19');
        $xmlColumn->addAttribute('nullable', 'true');
        $xmlColumn->addAttribute('pattern', 'yyyy-MM-dd HH:mm:ss');
        $xmlColumn->addAttribute('precision', '0');
        $xmlColumn->addAttribute('comment', '');
        break;

      default:
        throw new Exception('Unrecognized field type: ' . $customField['data_type']);
    }
  }
  return $xmlSchema;
}

/**
 * Create field headers based on normalized table/column names
 *
 * @param $customFields array($fieldSpec)
 * @return array($header => $fieldSpec)
 */
function arms_util_talend_filterCustomFields($entity, $customFields, $tableColumnDelimiter = '.') {
  arms_util_include_api('fext');
  $tableMaps = array( // array(oddTableName => stdTableName)
    'info_hs_academics_N' => 'academics_N',
    'academic_info_N' => 'academics_N',
    'high_school_academic_N' => 'academics_N',
    'info_academic_information_N' => 'academics_N',
    'academic_information_N' => 'academics_N',
    'info_academic_information_N' => 'academics_N',
    'academic_information_N' => 'academics_N',
    'high_school_academics_N' => 'academics_N',
    
    'alumni_information_N' => 'alumni_N',
    'alumni_data_N' => 'alumni_N',
    
    'club_information_N' => 'club_N',
    'contact_club_information_N' => 'club_N',
    'contact_club_N' => 'club_N',
    
    'evaluation_info_N' => 'evaluation_N',
    
    'parents_information_N' => 'parents_N',
    
    'info_recruiting_information_N' => 'main_info_N',
    'misc__N' => 'main_info_N',
    'misc_information_N' => 'main_info_N',
  );
  
  $newCustomFields = array(); // array(header => fieldSpec)
  foreach ($customFields as $field) {
    $table =  preg_replace('/^civicrm_value_(.*)_[0-9]+$/', '$1_N', $field['_table_name']);
    if ($tableMaps[$table]) {
      $table = $tableMaps[$table];
    }
    $column = preg_replace('/_[0-9]+$/', '_N', $field['column_name']);
    $column = preg_replace('/(_ai|_cci|_csi|_ihsa|_ia|_ir|_ov|_chs)$/', '_N', $column);
    
    $fextDefns = arms_util_fext_find_by_field_id($field['id']);
    if (!empty($fextDefns)) {
      foreach ($fextDefns as $var => $fextDefn) {
        $newCustomFields['fext' . $tableColumnDelimiter . $var] = $field;
      }
    }
    // else {
      $newCustomFields[$table . $tableColumnDelimiter . $column] = $field;
    // }
  }
  
  return $newCustomFields;
}
