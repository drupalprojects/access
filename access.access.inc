<?php

/**
 * @file
 * Access control kit API hook implementations to integrate with core modules.
 */

/**
 * Implements hook_access_scheme_info().
 */
function access_access_scheme_info() {
  $info['boolean'] = array(
    'label' => t('Boolean'),
    'data_type' => 'boolean',
    'description' => t('A <em>boolean</em> scheme controls access based on a true/false value (or yes/no, checked/unchecked, etc.). For example, you could grant access to content based on the "sticky" property, or to user profiles based on the value of a boolean field provided by the list module.'),
  );

  foreach (array('integer', 'float', 'text') as $data_type) {
    // See if a list field exists of this type.
    $field_type = 'list_' . $data_type;
    $fields = field_read_fields(array('type' => $field_type));
    // Don't include other realm fields.
    foreach ($fields as $field_name => $field) {
      if (!empty($field['settings']['allowed_values_function']) && $field['settings']['allowed_values_function'] == '_access_field_allowed_values') {
        unset($fields[$field_name]);
      }
    }
    if (!empty($fields)) {
      $info[$field_type] = array('data_type' => $data_type);
      switch ($data_type) {
        case 'integer':
          $info[$field_type]['label'] = t('List (integer) field');
          $info[$field_type]['description'] = t('A <em>list (integer) field</em> scheme controls access based on the value of an integer field provided by the list module. The allowed values list of the selected field will become the access realms for the site, and you will be able to grant access based on the value of that field to any ACK-supported fieldable entity that uses the field. ACK modules that support non-fieldable objects (such as the ACK menu module, for menu links) may also provide the ability to map those objects to a list value.');
          break;

        case 'float':
          $info[$field_type]['label'] = t('List (float) field');
          $info[$field_type]['description'] = t('A <em>list (float) field</em> scheme controls access based on the value of a float field provided by the list module. The allowed values list of the selected field will become the access realms for the site, and you will be able to grant access based on the value of that field to any ACK-supported fieldable entity that uses the field. ACK modules that support non-fieldable objects (such as the ACK menu module, for menu links) may also provide the ability to map those objects to a list value.');
          break;

        case 'text':
          $info[$field_type]['label'] = t('List (text) field');
          $info[$field_type]['description'] = t('A <em>list (text) field</em> scheme controls access based on the value of a text field provided by the list module. The allowed values list of the selected field will become the access realms for the site, and you will be able to grant access based on the value of that field to any ACK-supported fieldable entity that uses the field. ACK modules that support non-fieldable objects (such as the ACK menu module, for menu links) may also provide the ability to map those objects to a list value.');
          break;
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
        'data_type' => 'integer',
        // @todo Support cascading hierarchical privileges as an option.
        'description' => t('A <em>taxonomy</em> scheme controls access based on the terms of a selected vocabulary. You will be able to grant access to any ACK-supported fieldable entity based on the terms associated with that entity through a taxonomy term reference field. ACK modules that support non-fieldable objects (such as the ACK menu module, for menu links) may also provide the ability to map those objects to a term.'),
      );
    }
  }

  $info['user'] = array(
    'label' => t('User account'),
    'data_type' => 'integer',
    'description' => t('A <em>user account</em> scheme controls access to ACK-supported objects based on the users associated with those objects. For example, you could grant User A access to all content authored by User B.'),
  );
  // @todo Support access to nodes based on authorship, menu items based on the authorship of the node it links to, etc. (ex. "You can edit any nodes written by Bob and any menu links that point to them.").

  return $info;
}

/**
 * Implements hook_access_realm_settings().
 */
function access_access_realm_settings($scheme_type, $has_data, $values = array()) {
  $form = array();
  switch ($scheme_type) {
    case 'list_integer':
    case 'list_float':
    case 'list_text':
      $fields = field_read_fields(array('type' => $scheme_type));
      // Don't include other realm fields.
      foreach ($fields as $field_name => $field) {
        if (!empty($field['settings']['allowed_values_function']) && $field['settings']['allowed_values_function'] == '_access_field_allowed_values') {
          unset($fields[$field_name]);
        }
      }
      if (!empty($fields)) {
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
function access_access_realms($scheme_type, $settings = array()) {
  $values = array();
  switch ($scheme_type) {
    case 'boolean':
      $values = array(0 => t('False'), 1 => t('True'));
      break;

    case 'list_integer':
    case 'list_float':
    case 'list_text':
      if (!empty($settings['field_name']) && $field = field_info_field($settings['field_name'])) {
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
    'scheme types' => array(
      'list_integer',
      'list_float',
      'list_text',
    ),
    'object types' => array('fieldable entity'),
  );

  // Handler for taxonomy term reference fields.
  if (module_exists('taxonomy')) {
    $info['ACKEntityTaxonomyTermReference'] = array(
      'label' => t('Term reference field'),
      'scheme types' => array('taxonomy_term'),
      'object types' => array('fieldable entity'),
    );
  }

  return $info;
}
