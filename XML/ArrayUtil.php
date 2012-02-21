<?php

require_once 'CRM/Core/DAO.php';

function array_to_xml(&$xml, &$arr, &$mapping) {
  $unhandled = array_diff(array_keys($arr), $mapping['attrs'], $mapping['elems'], $mapping['excludes']);

  if (!empty($unhandled)) { error_log(sprintf("Warning: Ignoring keys: %s\n", print_r($unhandled, true))); }

  foreach ($mapping['attrs'] as $attr) {
    if (!isset($arr[$attr])) { # || empty($arr[$attr])) {
      continue;
    }
    $xml->addAttribute($attr, $arr[$attr]);
  }
  foreach ($mapping['elems'] as $elem) {
    if (!isset($arr[$elem])) { # || empty($arr[$elem])) {
      continue;
    }
    $values = explode(CRM_Core_DAO::VALUE_SEPARATOR, $arr[$elem]);
    foreach ($values as $value) {
      $xml->addChild($elem, $value);
    }
  }
  return $xml;
}


function xml_to_array(&$xml, $excludes = array()) {
  $arr = array();
  foreach ($xml->attributes() as $key => $value) {
    if (in_array($key, $excludes)) {
      continue;
    }
    $value = (string)$value;
    $arr[$key] = $value;
  }
  foreach ($xml->children() as $key => $value) {
    if (in_array($key, $excludes)) {
      continue;
    }
    $value = (string)$value;
    if (isset($arr[$key])) {
      $arr[$key] = $arr[$key] . CRM_Core_DAO::VALUE_SEPARATOR . $value;
    } else {
      $arr[$key] = $value;
    }
  }
  return $arr;
}

?>