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
    'realms callback' => 'access_scheme_boolean_realms',
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
      $info[$field_type] = array(
        'data_type' => $data_type,
        'realms callback' => 'access_scheme_list_field_realms',
        'settings callback' => 'access_scheme_list_field_settings',
      );
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
        'realms callback' => 'access_scheme_taxonomy_term_realms',
        'settings callback' => 'access_scheme_taxonomy_term_settings',
      );
    }
  }

  $info['user'] = array(
    'label' => t('User account'),
    'data_type' => 'integer',
    'description' => t('A <em>user account</em> scheme controls access to ACK-supported objects based on the users associated with those objects. For example, you could grant User A access to all content authored by User B.'),
    'realms callback' => 'access_scheme_user_realms',
  );
  return $info;
}

/**
 * Realms callback for the boolean scheme type.
 *
 * @see access_access_scheme_info()
 */
function access_scheme_boolean_realms($scheme) {
  return array(0 => t('False'), 1 => t('True'));
}

/**
 * Realms callback for the list field scheme types.
 *
 * @see access_access_scheme_info()
 */
function access_scheme_list_field_realms($scheme) {
  if (!empty($scheme->settings['field_name']) && $field = field_info_field($scheme->settings['field_name'])) {
    return list_allowed_values($field);
  }
  return array();
}

/**
 * Settings callback for the list field scheme types.
 *
 * @see access_access_scheme_info()
 */
function access_scheme_list_field_settings($scheme, $has_data) {
  // Find all available fields of the given type.
  $fields = field_read_fields(array('type' => $scheme->type));
  // Exclude other realm fields from the list.
  foreach ($fields as $field_name => $field) {
    if (!empty($field['settings']['allowed_values_function']) && $field['settings']['allowed_values_function'] == '_access_field_allowed_values') {
      unset($fields[$field_name]);
    }
  }

  // Build the form.
  $form = array();
  if (!empty($fields)) {
    $options = drupal_map_assoc(array_keys($fields));
    $form['field_name'] = array(
      '#type' => 'select',
      '#title' => t('List field'),
      '#description' => t('The allowed values list from this field will become the access realms for the scheme.'),
      '#default_value' => isset($scheme->settings['field_name']) ? $scheme->settings['field_name'] : NULL,
      '#options' => $options,
      '#required' => TRUE,
      '#disabled' => $has_data,
    );
  }
  return $form;
}

/**
 * Realms callback for the taxonomy term scheme type.
 *
 * @see access_access_scheme_info()
 */
function access_scheme_taxonomy_term_realms($scheme) {
  if (!empty($scheme->settings['vocabulary'])) {
    // Re-use the allowed values function for term reference fields.
    $field = array();
    $field['settings']['allowed_values'][] = array('vocabulary' => $scheme->settings['vocabulary'], 'parent' => 0);
    return taxonomy_allowed_values($field);
  }
  return array();
}

/**
 * Settings callback for the taxonomy term scheme type.
 *
 * @see access_access_scheme_info()
 */
function access_scheme_taxonomy_term_settings($scheme, $has_data) {
  $options = array();
  foreach (taxonomy_get_vocabularies() as $vocabulary) {
    $options[$vocabulary->machine_name] = $vocabulary->name;
  }
  $form['vocabulary'] = array(
    '#type' => 'select',
    '#title' => t('Vocabulary'),
    '#description' => t('The terms in this vocabulary will become the access realms for the scheme.'),
    '#default_value' => isset($scheme->settings['vocabulary']) ? $scheme->settings['vocabulary'] : NULL,
    '#options' => $options,
    '#required' => TRUE,
    '#disabled' => $has_data,
  );
  return $form;
}

/**
 * Realms callback for the user scheme type.
 *
 * @see access_access_scheme_info()
 */
function access_scheme_user_realms($scheme) {
  return db_query('SELECT uid, name FROM {users} WHERE uid > 0')->fetchAllKeyed();
}

/**
 * Implements hook_access_scheme_presave().
 */
function access_access_scheme_presave($scheme) {
  // Prevent a blank realm_field_name property.
  if (empty($scheme->realm_field_name)) {
    $scheme->realm_field_name = 'ack_' . $scheme->machine_name;
  }
  // Make sure we have the scheme type definition on the object.
  if (empty($scheme->info)) {
    $scheme->info = access_scheme_info($scheme->type);
  }
  if (!empty($scheme->info)) {
    // If the realm field doesn't exist, create it.
    $field = field_info_field($scheme->realm_field_name);
    if (empty($field)) {
      $field = array(
        'field_name' => $scheme->realm_field_name,
        'type' => 'list_' . $scheme->info['data_type'],
        'entity_types' => array('access_grant'),
        'cardinality' => FIELD_CARDINALITY_UNLIMITED,
        'locked' => TRUE,
        'settings' => array(
          'allowed_values' => array(),
          'allowed_values_function' => '_access_field_allowed_values',
        ),
      );
      field_create_field($field);
    }
  }
}

/**
 * Implements hook_access_scheme_insert().
 */
function access_access_scheme_insert($scheme) {
  // Create the realm field instance.
  $instance = field_info_instance('access_grant', $scheme->realm_field_name, $scheme->machine_name);
  if (empty($instance)) {
    $instance = array(
      'field_name' => $scheme->realm_field_name,
      'entity_type' => 'access_grant',
      'bundle' => $scheme->machine_name,
      'label' => check_plain($scheme->name),
      'description' => filter_xss($scheme->description),
      'required' => TRUE,
      'widget' => array(
        'type' => 'options_buttons',
        'settings' => array(),
      ),
    );
    field_create_instance($instance);
  }
  // Save the roles.
  if (empty($scheme->roles)) {
    $scheme->roles = array();
  }
  variable_set('access_scheme_roles_' . $scheme->machine_name, $scheme->roles);
}

/**
 * Implements hook_access_scheme_update().
 */
function access_access_scheme_update($scheme) {
  access_access_scheme_insert($scheme);
}

/**
 * Implements hook_access_scheme_delete().
 */
function access_access_scheme_delete($scheme) {
  // Delete the scheme's role configuration.
  variable_del('access_scheme_roles_' . $scheme->machine_name);
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

/**
 * Implements hook_access_grant_view().
 */
function access_access_grant_view($grant) {
  $account = user_load($grant->uid);
  $grant->content['user'] = array(
    '#type' => 'item',
    '#title' => t('User:'),
    '#markup' => theme('username', array('account' => $account)),
    '#weight' => -5,
  );

  $role = user_role_load($grant->rid);
  $grant->content['role'] = array(
    '#type' => 'item',
    '#title' => t('Role:'),
    '#markup' => check_plain($role->name),
    '#weight' => -4,
  );
}
