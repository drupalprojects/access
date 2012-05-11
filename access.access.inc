<?php
/**
 * @file
 * Implements the access control kit API to integrate with core modules.
 */

/**
 * Implements hook_access_info().
 */
function access_access_info() {
  $modules = array('list', 'taxonomy', 'user');
  $info = array();
  foreach ($modules as $module) {
    if (module_exists($module)) {
      $function = '_access_access_info_' . $module;
      $info += $function();
    }
  }
  return $info;
}

/**
 * Adds support for user.module to access_access_info().
 */
function _access_access_info_user() {
  $info['user'] = array(
    'label' => t('a user account'),
    'type' => 'integer',
    'group' => t('User'),
  );
  // @todo Support access to nodes based on authorship, menu items based on the
  // authorship of the node it links to, etc. (ex. "You can edit any nodes
  // written by Bob and any menu links that point to them.").
  return $info;
}

/**
 * Adds support for list.module to access_access_info().
 */
function _access_access_info_list() {
  $info['list_text'] = array(
    'label' => t("a text list field's allowed values"),
    'type' => 'text',
    'group' => t('List'),
  );
  // @todo Support the other list field types.
  return $info;
}

/**
 * Add support for taxonomy.module to access_access_info().
 */
function _access_access_info_taxonomy() {
  $info['taxonomy_term'] = array(
    'label' => t('a term'),
    'type' => 'integer',
    'group' => t('Taxonomy'),
  );
  // @todo Add a 'settings' key to declare defaults for 'boundary arguments', so we don't have to keep checking if they're empty.
  // @todo Support cascading hierarchical privileges as an option.
  // @todo Support relationships based on fields and on taxonomy_index.
  return $info;
}

/**
 * Implements hook_access_field_settings_form().
 */
function access_access_field_settings_form($type, $values = array()) {
  $form = array();

  switch ($type) {
    case 'list_text':
      $fields = field_read_fields(array('type' => $type));
      $options = drupal_map_assoc(array_keys($fields));
      $form['field'] = array(
        '#type' => 'select',
        '#title' => t('Use allowed values list from'),
        '#default_value' => $values['field'],
        '#options' => $options,
        '#required' => TRUE,
      );
      break;

    case 'taxonomy_term':
      $vocabularies = taxonomy_get_vocabularies();
      $options = array();
      foreach ($vocabularies as $vocabulary) {
        $options[$vocabulary->machine_name] = $vocabulary->name;
      }
      $form['vocabulary'] = array(
        '#type' => 'select',
        '#title' => t('Vocabulary'),
        '#default_value' => $values['vocabulary'],
        '#options' => $options,
        '#required' => TRUE,
      );
      break;
  }

  return $form;
}

/**
 * Implements hook_access_field_allowed_values().
 */
function access_access_field_allowed_values($type, $arguments = array()) {
  switch ($type) {
    // User accounts
    case 'user':
      return db_query('SELECT uid, name FROM {users} WHERE uid > 0')->fetchAllKeyed();

    // List field allowed values
    case 'list_text':
      $field = field_info_field($arguments['field']);
      return list_allowed_values($field);

    case 'taxonomy_term':
      // Re-use the allowed values function for term reference fields.
      $field = array();
      $field['settings']['allowed_values'][] = array('vocabulary' => $arguments['vocabulary'], 'parent' => 0);
      return taxonomy_allowed_values($field);
  }
}
