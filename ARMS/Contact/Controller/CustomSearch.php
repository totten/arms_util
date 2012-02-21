<?php
 
require_once 'CRM/Core/Controller.php';

/**
 * Custom Search Auto-Registration Adapter
 *
 * The "custom search" mechanism in Civi is a semi-documented/semi-supported API
 * for implementing special-purposes search pages. It provides support for
 * executing tasks, saving search results, etc. However, it also requires manually
 * registering and tracking a "search ID". This file defines a helper mechanism
 * to simplify that.
 *
 * To use this class, register a menu item in a menu .xml file with something like:
 *
 * <item>
 *    <path>civicrm/team</path>
 *    <title>Teams</title>
 *    <page_callback>ARMS_Contact_Controller_CustomSearch</page_callback>
 *    <page_arguments>mode=16384,searchClass=My_Custom_Search,description=Run a custom search,defaultForce=1</page_arguments>
 *    <access_arguments>access CiviCRM</access_arguments>
 *    <weight>20</weight>
 *    <page_type>1</page_type>
 * </item>
 *
 * In the initial implementation, we will auto-create OptionValue's for the custom
 * search class, but we will not auto-destroy them. There may be "saved searches"
 * which reference the custom search ID, and it's important to preserve these ID's
 * even when the module is disabled-and-re-enabled.
 *
 * Note that this class does _not_ extend CRM_Contact_Controller_Search; instead, it wraps
 * around it using dynamic delegation. This helps ensure that the custom-search controller
 * runs with its normal environment/settings. In particular, this affects how controller
 * state is stored in CRM_Core_Session -- With an inheritance strategy, the scope-key
 * would derive from the ARMS_* subclass name. With a delegation strategy, the scope-key
 * derives from the main CRM_* class. This effectively ensures that paths like
 * "civicrm/mysearch?force=1" and "/civicrm/contact/search/custom?force=1" are equivalent.
 * By equating those paths, we resolve ARMS-1157 and similar issues that arise
 * from using different paths/controllers.
 */
class ARMS_Contact_Controller_CustomSearch extends CRM_Core_Controller {

  /**
   * The real controller
   *
   * @var CRM_Contact_Controller_Search
   */
  var $_delegate;

  /**
   * The OptionValue which represents the custom search registration record.
   */
  var $_customSearch;
  
  function __construct() {
    $args = func_get_args();
    require_once 'CRM/Contact/Controller/Search.php';
    $ref = new ReflectionClass('CRM_Contact_Controller_Search');
    $this->_delegate = $ref->newInstanceArgs($args);
  }

  protected function getDelegate() {
    return $this->_delegate;
  }
  
  function run($newArgs = NULL, $pageArgs = NULL) {
    $delegate = $this->getDelegate();
    $delegate->_customSearch = self::lookupCustomSearch($pageArgs['searchClass'], $pageArgs['description']);
    if ($_SERVER['REQUEST_METHOD'] == 'GET') {
      $_GET['csid'] = $delegate->_customSearch['value'];
      if ($pageArgs['defaultForce']) {
        ## Skip the edit form -- go directly to the result page
        $_GET['force'] = 1;
      }
    }
    elseif ($_SERVER['REQUEST_METHOD'] == 'POST') {
      $_POST['csid'] = $delegate->_customSearch['value'];
    }
    
    $delegate->run($newArgs, $pageArgs);

    /*
    $delegate->_customSearch = self::lookupCustomSearch($pageArgs['searchClass'], $pageArgs['description']);
    $query = array();
    $query['csid'] = $delegate->_customSearch['value'];
    if ($pageArgs['defaultForce']) {
      $query['force'] = 1;
    }
    drupal_goto('civicrm/contact/search/custom', $query);
    */
  }

  function __get($property) {
    return $this->getDelegate()->{$property};
  }
   
  function __set($property, $value) {
    $this->getDelegate()->{$property} = $value;
  }
   
  function __call($method, $params) {
    return call_user_func_array(
      array($this->getDelegate(), $method),
      $params
      );
  }
  
  /**
   * Find or create the custom search record.
   *
   * @return array containing an OptionValue
   */
  static function lookupCustomSearch($searchClass, $description) {
    arms_util_include_api('custom_search');
    $optionValue = arms_util_custom_search_lookup($searchClass);
    if ($optionValue === NULL) {
      arms_util_custom_search_register($searchClass, $description);
      $optionValue = arms_util_custom_search_lookup($searchClass);
    }
    return $optionValue;
  }
}
