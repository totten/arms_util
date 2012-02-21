<?php

define(ARMS_UTIL_MENU_ROOT_ID, -1);
define(ARMS_UTIL_MENU_ROOT_NAME, 'Root');

/**
 * @file
 *
 * Helper functions for working CiviCRM menus
 */

/** Rebuild the CiviCRM menu cache */
function arms_util_menu_rebuild() {
  if (!module_exists('civicrm')) return;
  civicrm_initialize();
  require_once 'CRM/Core/Menu.php';
  CRM_Core_Menu::store();
  require_once 'CRM/Utils/System.php';
  if (version_compare(CRM_Utils_System::version(), '3.0.0') >= 0) {
    db_query('UPDATE {civicrm_preferences} SET navigation = null');
  }
  drupal_set_message(t('Rebuilt CiviCRM menu'));
}

/**
 * Implementation of hook_civicrm_navigationMenu
 *
 * Identify items in the xmlMenu which have a 'parent' tag and
 * automatically add them to the menu
 */
function _arms_util_menu_civicrm_navigationMenu(&$params) {
  $nav = _arms_util_menu_build($params);
  
  require_once 'CRM/Core/Menu.php';
  $xmlItems = CRM_Core_Menu::xmlItems();

  foreach ($xmlItems as $path => $xmlItem) {
    if ($xmlItem['page_type'] != 1) continue;
    if (!isset($xmlItem['parent'])) continue;

    $navItem = array(
      'attributes' => array(
        'label' => $xmlItem['title'],
        'name' => $path,
        'url' => $path . '?reset=1',
        'permission' => implode(',', $xmlItem['access_arguments'][0]),
        'operator' => strtoupper($xmlItem['access_arguments'][1]),
        'separator' => NULL,
        // parentID, // auto
        // navID, // auto
        'active' => 1,
      ),
      'child' => NULL,
    );
    _arms_util_menu_add($nav, $nav['byName'][$xmlItem['parent']], $navItem);
    _arms_util_menu_loosenAncestorPermissions($nav, $navItem);
    // dpm(array('path' => $path, 'xmlItem' => $xmlItem, 'navItem' => $navItem, 'parent' => $nav['byName'][$xmlItem['parent']]));
  }

  // dpm(array('nav' => $nav, 'xmlItems' => $xmlItems));
}

/**
 * Build indices by ID and name
 *
 * @return array, with keys 'root', 'byID', 'byName', 'maxID'
 */
function _arms_util_menu_build(&$params) {
  $nav = array(
    'root' => array(
      'attributes' => array(
        'name' => ARMS_UTIL_MENU_ROOT_NAME,
        'navID' => ARMS_UTIL_MENU_ROOT_ID,
      ),
      'child' => &$params,
    ),
    'byID' => array(),
    'byName' => array(),
    'maxID' => 0,
  );
  
  _arms_util_menu_buildIdx($nav, $nav['root']);
  
  return $nav;
}

function _arms_util_menu_buildIdx(&$nav, &$node) {
  if ($id = $node['attributes']['navID']) {
    $nav['byID'][$id] = &$node;
    if ($id > $nav['maxID']) {
      $nav['maxID'] = $id;
    }
  }
  if ($name = $node['attributes']['name']) {
    $nav['byName'][$name] = &$node;
  }
  if (is_array($node['child'])) {
    foreach ($node['child'] as &$child) {
      _arms_util_menu_buildIdx($nav, $child);
    }
  }
}

/**
 * Add a new navigation item
 *
 * @param $nav array per _arms_util_menu_build()
 * @param $parent array with keys 'attributes' and 'child'
 * @param $navItem array with keys 'attributes' and 'child'
 */
function _arms_util_menu_add(&$nav, &$parent, &$navItem) {
  $navItem['attributes']['navID'] = ++$nav['maxID'];

  $nav['byID'][$navItem['attributes']['navID']] = $navItem;
  $nav['byName'][$navItem['attributes']['name']] = $navItem;
  
  $navItem['attributes']['parentID'] = $parent['attributes']['navID'];
  $parent['child'][$navItem['attributes']['navID']] = $navItem;
}

/**
 * Modify the permissions of any ancestors to ensure that authorized
 * users can see $navItem
 *
 * FIXME ugly ugly killme killme wtf
 */
function _arms_util_menu_loosenAncestorPermissions(&$nav, &$navItem) {
  $targetPermission = empty($navItem['attributes']['permission']) ? 'access CiviCRM' : $navItem['attributes']['permission'];
  if ($navItem['attributes']['operator'] == 'AND' && strpos($targetPermission, ',') !== FALSE) {
     drupal_set_message(t('Failed to loosen permissions for ancestors: target is too complex'));
     if (function_exists('dpm')) dpm($navItem);
     return;
  }
  
  $nextAncestorID = $navItem['attributes']['parentID'];
  while ($nextAncestorID) {
    $ancestor = &$nav['byID'][$nextAncestorID];
    $nextAncestorID = $ancestor['attributes']['parentID'];

    if (empty($ancestor['attributes']['permission'])) {
      // already permissive
      continue;
    }
    
    if (empty($ancestor['attributes']['operator'])) {
      $ancestor['attributes']['operator'] = 'OR';
    }
    switch ($ancestor['attributes']['operator']) {
      case 'AND':
        drupal_set_message(t('Failed to loosen permissions for ancestors: ancestor is too complex'));
        if (function_exists('dpm')) dpm($ancestor);
        break;
      case 'OR':
        $ancestor['attributes']['permission'] .= ',' . $targetPermission;
        break;
      default:
    }
  }
}
