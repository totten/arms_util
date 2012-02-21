<?php

/**
 * Create or update an existing mailing-component
 *
 * @param $comp array{civicrm_mailing_component}
 * @return array{civicrm_mailing_component}
 */
function arms_util_mailing_component_add($comp) {
  $existing = db_fetch_array(db_query('SELECT id, name, component_type, subject, body_html, body_text, is_default, is_active FROM {civicrm_mailing_component} WHERE name = "%s" OR id = %d', @$comp['name'], @$comp['id']));
  if (empty($existing)) {
    return arms_util_mailing_component_create($comp);
  } else {
    $comp = array_merge($existing, $comp);
    return arms_util_mailing_component_update($comp);
  }
}

/**
 * Add a new mailing-component
 *
 * @param $comp array{civicrm_mailing_component}
 * @return array{civicrm_mailing_component}
 */
function arms_util_mailing_component_create($comp) {
  db_query('INSERT INTO {civicrm_mailing_component} (name, component_type, subject, body_html, body_text, is_default, is_active)
    VALUES ("%s", "%s", "%s", "%s", "%s", %d, %d)',
    $comp['name'], $comp['component_type'], $comp['subject'], $comp['body_html'], $comp['body_text'], $comp['is_default'], $comp['is_active']
  );
  // FIXME // $comp['id'] = db_last_insert_id('civicrm_mailing_component', 'id');
  
  if ($comp['is_default']) {
    db_query('UPDATE {civicrm_mailing_component} SET is_default = 0 WHERE component_type = "%s" AND name != "%s"', $comp['component_type'], $comp['name']);
  }
  
  return $comp;
}

/**
 * Update an existing mailing-component
 *
 * @param $comp array{civicrm_mailing_component}
 * @return array{civicrm_mailing_component}
 */
function arms_util_mailing_component_update($comp) {
  db_query('UPDATE {civicrm_mailing_component} SET 
           component_type = "%s",   name = "%s",   subject = "%s",   body_html = "%s",   body_text = "%s",   is_default = %d,     is_active = %d
           WHERE id = %d',
    $comp['component_type'], $comp['name'], $comp['subject'], $comp['body_html'], $comp['body_text'], $comp['is_default'], $comp['is_active'],
    $comp['id']
  );
  
  if ($comp['is_default']) {
    db_query('UPDATE {civicrm_mailing_component} SET is_default = 0 WHERE component_type = "%s" AND name != "%s"', $comp['component_type'], $comp['name']);
  }
  
  return $comp;
}
