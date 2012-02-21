<?php

/**
 * Drupal variable containgin a list of previously generated procedures
 */
define(ARMS_UTIL_PROCEDURE_VAR, 'arms_util_procedures');

/**
 * Register any SQL procedures defined by other modules.
 */
function arms_util_procedure_rebuild() {
  $procedures = module_invoke_all('arms_procedure');
  // FIXME consider using pattern-based deletion instead of storing list of old procs
  $drop_procedures = array_merge(
    variable_get(ARMS_UTIL_PROCEDURE_VAR, array()),
    array_keys($procedures)
  );
  foreach ($drop_procedures as $drop_procedure) {
    db_query('DROP PROCEDURE IF EXISTS ' . $drop_procedure);
  }
  foreach ($procedures as $procName => $procSpec) {
    db_query($procSpec['full_sql']);
  }
  variable_set(ARMS_UTIL_PROCEDURE_VAR, array_keys($procedures));
}
