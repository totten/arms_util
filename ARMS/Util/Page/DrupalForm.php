<?php

require_once 'CRM/Core/Page.php';

/**
 * Render a page using the Drupal Form API
 *
 * For example, if the module "foo" includes a file "foo.admin.inc" with
 * a form named "foo_admin", then set these options in the menu XML:
 *
 * <page_type>1</page_type>
 * <page_callback>ARMS_Util_Page_DrupalForm</page_callback>
 * <page_arguments>d.module=arms_actlog,d.file=arms_actlog.logs.inc,d.form=arms_actlog_logs_listing</page_arguments>
 */
class ARMS_Util_Page_DrupalForm extends CRM_Core_Page {
    public function run($path, $args) {
        arms_util_include_api('array');
        $args = arms_util_explode_tree('.', $args);

        if ($args['d']['module'] && $args['d']['file']) {
          module_load_include($args['d']['file'], $args['d']['module']);
        }
        print theme('page',
          drupal_get_form($args['d']['form'])
        );
        drupal_page_footer();
        exit();
    }
}
