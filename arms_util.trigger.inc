<?php

define(ARMS_UTIL_TRIGGER_PREFIX, 'arms_');

/**
 * Register any SQL triggers defined by other modules.
 */
function arms_util_trigger_rebuild() {
  arms_util_trigger_clear(ARMS_UTIL_TRIGGER_PREFIX);

  // get a list of triggers from other modules
  $module_triggers = module_invoke_all('arms_trigger');
  drupal_alter('arms_trigger', $module_triggers);
  usort($module_triggers, '_arms_util_trigger_sort');

  // consolidate the triggers 
  $sql_triggers = array();
  foreach ($module_triggers as $trigger) {
    foreach ($trigger['table'] as $table) {
      foreach ($trigger['event'] as $event) {
        foreach ($trigger['when'] as $when) {
          if (!is_array($sql_triggers[$table][$when][$event]['declare'])) {
            $sql_triggers[$table][$when][$event]['declare'] = array();
          }
          if (is_array($trigger['declare'])) {
            $sql_triggers[$table][$when][$event]['declare'] = array_merge($sql_triggers[$table][$when][$event]['declare'], $trigger['declare']);
          }
          $sql_triggers[$table][$when][$event]['sql'][] = $trigger['sql'];
        }
      }
    }
  }
  
  // add the triggers to the database
  foreach ($sql_triggers as $table => $table_v) {
    foreach ($table_v as $when => $when_v) {
      foreach ($when_v as $event => $event_v) {
        $sql = "begin\n";
        if (!empty($event_v['declare'])) {
          foreach ($event_v['declare'] as $var => $sig) {
            $sql .= 'DECLARE ' . $var . ' ' . $sig . ";\n";
          }
        }
        foreach ($event_v['sql'] as $fragment) {
          $fragment = rtrim($fragment,  " \r\n\t");
          if ($fragment{strlen($fragment)-1} != ';') $fragment .= ';';
          $sql .= $fragment;
          $sql .= "\n";
        }
        $sql .= "\nend\n";
        arms_util_trigger_add(ARMS_UTIL_TRIGGER_PREFIX, $when, $event, $table, $sql);
      }
    }
  }
}

function _arms_util_trigger_sort($left, $right) {
  if ($left['weight'] == $right['weight']) {
    return 0;
  } elseif ($left['weight'] < $right['weight']) {
    return -1;
  } else {
    return 1;
  }
}

function arms_util_trigger_clear($prefix) {
  $q = db_query('SELECT * FROM information_schema.TRIGGERS WHERE TRIGGER_NAME LIKE "' . $prefix .'%" AND TRIGGER_SCHEMA = SCHEMA()');
  $triggers = array();
  while ($row = db_fetch_array($q)) { 
    $triggers[] = $row['TRIGGER_NAME'];
  }
  
  foreach ($triggers as $trigger) {
    db_query('DROP TRIGGER IF EXISTS ' . $trigger);
  }
  
  variable_set('arms_util_trigger', 0);
}

function arms_util_trigger_add($prefix, $when, $event, $table, $source) {
  // $trigger_id = variable_get('arms_util_trigger', 0);
  // variable_set('arms_util_trigger', 1 + $trigger_id);
  // $name = sprintf("%s%03d", $prefix, $trigger_id);
  
  // In MySQL, triggers are unique for each ($when,$event,$table) tuple, so
  // this naming scheme is OK.
  $name = strtolower(implode('', array($prefix, $table, '_', $when{0}, $event{0})));
  $when = strtolower($when);
  $event = strtolower($event);
  if ($when != 'before' && $when != 'after') {
    return FALSE;
  }
  if ($event != 'insert' && $event != 'update' && $event != 'delete') {
    return FALSE;
  }
  
  switch ($event) {
    case 'insert':
    case 'update':
      $source = preg_replace('/{ACTIVE}\./', 'NEW.', $source);
      break;
    case 'delete':
      $source = preg_replace('/{ACTIVE}\./', 'OLD.', $source);
      break;
  }
  
  $sql = 'CREATE TRIGGER ' . $name . ' ' . $when . ' ' . $event . ' ON ' . $table . ' FOR EACH ROW ' . $source;
  // printf("arms_util_trigger_add: %s\n", $sql);
  return db_query($sql);
}
