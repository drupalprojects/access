<?php

/**
 * @file
 * Access control kit API hook implementations to integrate with core modules.
 */

/**
 * Implements hook_access_realm_info().
 */
function access_access_realm_info() {
  $info['boolean'] = array(
    'label' => t('Boolean'),
    'field_type' => 'access_boolean',
    'description' => t('A <em>boolean</em> scheme controls access based on a true/false value (or yes/no, checked/unchecked, etc.). For example, you could grant access to content based on the "sticky" property, or to user profiles based on the value of a boolean field provided by the list module.'),
  );

  if (module_exists('list')) {
    foreach (array('integer', 'float', 'text') as $type) {
      // See if a field exists of this type.
      $exists = db_select('field_config', 'f')
        ->condition('f.type', 'list_' . $type)
        ->condition('f.deleted', 0)
        ->fields('f', array('id'))
        ->range(0, 1)
        ->execute()
        ->fetchField();
      if ($exists) {
        $info['list_' . $type] = array(
          'field_type' => 'access_' . $type,
        );
        switch ($type) {
          case 'integer':
            $info['list_' . $type]['label'] = t('List (integer) field');
            $info['list_' . $type]['description'] = t('A <em>list (integer) field</em> scheme controls access based on the value of an integer field provided by the list module. The allowed values list of the selected field will become the access realms for the site, and you will be able to grant access based on the value of that field to any ACK-supported fieldable entity that uses the field. ACK modules that support non-fieldable objects (such as the ACK menu module, for menu links) may also provide the ability to map those objects to a list value.');
            break;

          case 'float':
            $info['list_' . $type]['label'] = t('List (float) field');
            $info['list_' . $type]['description'] = t('A <em>list (float) field</em> scheme controls access based on the value of a float field provided by the list module. The allowed values list of the selected field will become the access realms for the site, and you will be able to grant access based on the value of that field to any ACK-supported fieldable entity that uses the field. ACK modules that support non-fieldable objects (such as the ACK menu module, for menu links) may also provide the ability to map those objects to a list value.');
            break;

          case 'text':
            $info['list_' . $type]['label'] = t('List (text) field');
            $info['list_' . $type]['description'] = t('A <em>list (text) field</em> scheme controls access based on the value of a text field provided by the list module. The allowed values list of the selected field will become the access realms for the site, and you will be able to grant access based on the value of that field to any ACK-supported fieldable entity that uses the field. ACK modules that support non-fieldable objects (such as the ACK menu module, for menu links) may also provide the ability to map those objects to a list value.');
            break;
        }
      }
    }
  }

  if (module_exists('taxonomy')) {
    // See if any vocabularies exist.
    $exists = db_select('taxonomy_vocabulary', 't')
      ->fields('t', array('vid'))
      ->range(0, 1)
      ->execute()
      ->fetchField();
    if ($exists) {
      $info['taxonomy_term'] = array(
        'label' => t('Taxonomy'),
        'field_type' => 'access_integer',
        // @todo Support cascading hierarchical privileges as an option.
        'description' => t('A <em>taxonomy</em> scheme controls access based on the terms of a selected vocabulary. You will be able to grant access to any ACK-supported fieldable entity based on the terms associated with that entity through a taxonomy term reference field. ACK modules that support non-fieldable objects (such as the ACK menu module, for menu links) may also provide the ability to map those objects to a term.'),
      );
    }
  }

  $info['user'] = array(
    'label' => t('User account'),
    'field_type' => 'access_integer',
    'description' => t('A <em>user account</em> scheme controls access to ACK-supported objects based on the users associated with those objects. For example, you could grant User A access to all content authored by User B.'),
  );
  // @todo Support access to nodes based on authorship, menu items based on the authorship of the node it links to, etc. (ex. "You can edit any nodes written by Bob and any menu links that point to them.").

  return $info;
}

/**
 * Implements hook_access_realm_settings().
 */
function access_access_realm_settings($realm_type, $has_data, $values = array()) {
  $form = array();
  switch ($realm_type) {
    case 'list_integer':
    case 'list_float':
    case 'list_text':
      $fields = field_read_fields(array('type' => $realm_type));
      if ($fields) {
        $options = drupal_map_assoc(array_keys($fields));
        $form['field_name'] = array(
          '#type' => 'select',
          '#title' => t('List field'),
          '#description' => t('The allowed values list from this field will become the access realms for the scheme.'),
          '#default_value' => isset($values['field_name']) ? $values['field_name'] : NULL,
          '#options' => $options,
          '#required' => TRUE,
          '#disabled' => $has_data,
        );
      }
      break;

    case 'taxonomy_term':
      if (module_exists('taxonomy')) {
        $options = array();
        foreach (taxonomy_get_vocabularies() as $vocabulary) {
          $options[$vocabulary->machine_name] = $vocabulary->name;
        }
        $form['vocabulary'] = array(
          '#type' => 'select',
          '#title' => t('Vocabulary'),
          '#description' => t('The terms in this vocabulary will become the access realms for the scheme.'),
          '#default_value' => isset($values['vocabulary']) ? $values['vocabulary'] : NULL,
          '#options' => $options,
          '#required' => TRUE,
          '#disabled' => $has_data,
        );
      }
      break;
  }
  return $form;
}

/**
 * Implements hook_access_realms().
 */
function access_access_realms($realm_type, $settings = array()) {
  $values = array();
  switch ($realm_type) {
    case 'boolean':
      $values = array(0 => t('False'), 1 => t('True'));
      break;

    case 'list_integer':
    case 'list_float':
    case 'list_text':
      if (module_exists('list') && !empty($settings['field_name']) && $field = field_info_field($settings['field_name'])) {
        $values = list_allowed_values($field);
      }
      break;

    case 'taxonomy_term':
      if (module_exists('taxonomy') && !empty($settings['vocabulary'])) {
        // Re-use the allowed values function for term reference fields.
        $field = array();
        $field['settings']['allowed_values'][] = array('vocabulary' => $settings['vocabulary'], 'parent' => 0);
        $values = taxonomy_allowed_values($field);
      }
      break;

    case 'user':
      $values = db_query('SELECT uid, name FROM {users} WHERE uid > 0')->fetchAllKeyed();
      break;
  }
  return $values;
}

/**
 * Implements hook_access_handler_info().
 */
function access_access_handler_info() {
  // Generic Field API handler.
  $info['ACKEntityField'] = array(
    'label' => t('Field instance'),
    'realm types' => array(),
    'object types' => array('fieldable entity'),
  );
  if (module_exists('list')) {
    $info['ACKEntityField']['realm types'][] = 'list_integer';
    $info['ACKEntityField']['realm types'][] = 'list_float';
    $info['ACKEntityField']['realm types'][] = 'list_text';
  }

  // Handler for taxonomy term reference fields.
  if (module_exists('taxonomy')) {
    $info['ACKEntityTaxonomyTermReference'] = array(
      'label' => t('Term reference field'),
      'realm types' => array('taxonomy_term'),
      'object types' => array('fieldable entity'),
    );
  }

  return $info;
}
