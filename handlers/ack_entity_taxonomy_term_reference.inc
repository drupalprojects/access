<?php
/**
 * @file
 * Contains the entity term reference field access handler.
 */

/**
 * Controls access to an entity based on a taxonomy term reference field.
 */
class ACKEntityTaxonomyTermReference extends ACKEntityField {
  /**
   * Overrides ACKEntityField::__construct().
   */
  public function __construct($scheme = '', $object_type = '', array $settings = array()) {
    parent::__construct($scheme, $object_type, $settings);
    // Taxonomy term reference fields use 'tid' as the value key.
    $this->field_value_key = 'tid';
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
      '#default_value' => $this->field_name,
    );
    return $form;
  }
}
