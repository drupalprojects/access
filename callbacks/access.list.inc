<?php

/**
 * @file
 * Callback functions for the list field access scheme types.
 */

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
