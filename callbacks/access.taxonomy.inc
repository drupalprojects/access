<?php

/**
 * @file
 * Callback functions for the taxonomy access scheme type.
 */

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
