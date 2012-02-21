<?php

require_once 'CRM/Core/Page.php';

/**
 * Stub page controller; simply redirect to a preconfigured URL.
 *
 * For example, to create a stub which redirects to "/civicrm/activities/search?reset=1",
 * put this in the menu XML:
 *
 * <page_type>1</page_type>
 * <page_callback>ARMS_Util_Page_Redir</page_callback>
 * <page_arguments>path=civicrm/activities/search,query.reset=1</page_arguments>
 */
class ARMS_Util_Page_Redir extends CRM_Core_Page {
    public function run($path, $args) {
        arms_util_include_api('array');
        foreach ($args as $k => $v) {
          $args[$k] = $this->replace($v, $_REQUEST);
        }
        $args = arms_util_explode_tree('.', $args);
        drupal_goto($args['path'], $args['query']);
    }

    function replace( $str, $values ) {
        foreach ( $values as $n => $v ) {
            $str = str_replace( "%%$n%%", $v, $str );
        }
        return $str;
    }
}
