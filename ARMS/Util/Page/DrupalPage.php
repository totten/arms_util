<?php

require_once 'CRM/Core/Page.php';

/**
 * Render a page that was implemented as a Drupal callback
 *
 * For example, if the module "foo" includes a file "foo.admin.inc" with
 * a callback named "foo_admin", then set these options in the menu XML:
 *
 * <page_type>1</page_type>
 * <page_callback>ARMS_Util_Page_DrupalForm</page_callback>
 * <page_arguments>d.module=arms_actlog,d.file=arms_actlog.logs.inc,d.callback=arms_actlog_logs_listing</page_arguments>
 */
class ARMS_Util_Page_DrupalPage extends CRM_Core_Page {
    public function run($path, $args) {
        arms_util_include_api('array');
        $args = arms_util_explode_tree('.', $args);

        if ($args['d']['module'] && $args['d']['file']) {
          $file = './' . drupal_get_path('module', $args['d']['module']) . '/' . $args['d']['file']; 
          require_once $file;
        }
        print theme('page',
          call_user_func($args['d']['callback'])
        );
        drupal_page_footer();
        exit();
    }
}
