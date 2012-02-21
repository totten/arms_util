<?php

/**
 * Create a new contribution type
 *
 * @param $type an array-encoded contribution type; be sure to include 'name'
 */
function arms_util_contribution_type_add($type) {
  $type['is_deductible'] = 0;
  $type['is_reserved'] = 0;
  $type['is_active'] = 1;
  db_query('insert into {civicrm_contribution_type} (name,is_deductible,is_reserved,is_active) values ("%s",%d,%d,%d)',
    $type['name'], $type['is_deductible'], $type['is_reserved'], $type['is_active']);

  require_once 'CRM/Contribute/PseudoConstant.php';
  CRM_Contribute_PseudoConstant::flush('contributionType');
}
