<?php
/**
 * @file
 * Contains the node term reference field access handler.
 */

/**
 * Controls access to a node based on a taxonomy term reference field.
 */
class ACKNodeTaxonomyTermReference extends AccessControlKit {
  /**
   * Implements AccessControlKitInterface::getRealms().
   */
  public function getRealms($node) {
    // Make sure the object is correctly configured for a term reference field.
    if (empty($this->settings['field_name'])) {
      return array();
    }

    // Return the values of the selected term reference field, if any.
    $field_name = $this->settings['field_name'];
    $realms = array();
    if (isset($node->$field_name)) {
      $terms = $node->$field_name;
      if (!empty($terms['und'])) {
        foreach ($terms['und'] as $term) {
          $realms[] = $term['tid'];
        }
      }
    }
    return $realms;
  }

  /**
   * Implements AccessControlKitInterface::settingsForm().
   */
  public function settingsForm($realm_type, array $realm_settings = array()) {
    $options = array();
    if (isset($realm_settings['vocabulary'])) {
      $fields = field_read_fields(array('type' => 'taxonomy_term_reference'));
      foreach ($fields as $field_name => $field) {
        if (!empty($field['settings']['allowed_values']) && $field['settings']['allowed_values'][0]['vocabulary'] == $realm_settings['vocabulary']) {
          $options[$field_name] = check_plain($field_name);
        }
      }
    }

    $form['field_name'] = array(
      '#title' => t('Term reference field'),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => empty($this->settings['field_name']) ? '' : $this->settings['field_name'],
    );
    return $form;
  }
}
