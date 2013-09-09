<?php

/**
 * @file
 * Contains the handler class for term reference fields on entities.
 */

/**
 * Controls access to an entity based on a taxonomy term reference field.
 */
class ACKEntityTaxonomyTermReference extends ACKEntityField {

  /**
   * The machine name of the vocabulary that defines the realms.
   *
   * @var string
   */
  protected $vocabulary;

  /**
   * Overrides ACKEntityField::__construct().
   */
  public function __construct($scheme, array $settings = array()) {
    parent::__construct($scheme, $settings);
    // The choice of field is a handler-level setting.
    $this->field_name = isset($settings['field_name']) ? $settings['field_name'] : NULL;
    // Taxonomy term reference fields use 'tid' as the value key.
    $this->field_value_key = 'tid';
    // Get the vocabulary from the scheme settings.
    $this->vocabulary = isset($scheme->settings['vocabulary']) ? $scheme->settings['vocabulary'] : NULL;
  }

  /**
   * Overrides ACKEntityField::description().
   */
  public function description() {
    return t('The value of the selected term reference field will determine realm membership.');
  }

  /**
   * Overrides AccessControlKitHandler::settingsForm().
   */
  public function settingsForm() {
    // Find all term reference fields that work with the scheme's vocabulary.
    $options = array();
    if (!empty($this->vocabulary)) {
      $fields = field_read_fields(array('type' => 'taxonomy_term_reference'));
      foreach ($fields as $field_name => $field) {
        if (!empty($field['settings']['allowed_values']) && $field['settings']['allowed_values'][0]['vocabulary'] == $this->vocabulary) {
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
