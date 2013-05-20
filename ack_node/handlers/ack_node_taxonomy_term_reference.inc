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
    // Return the values of the selected term reference field.
    if ($field_name = $this->getSetting('field_name')) {
      return $this->getFieldValues('tid', 'node', $node, $field_name, $node->language);
    }
    return array();
  }

  /**
   * Implements AccessControlKitInterface::settingsForm().
   */
  public function settingsForm($realm_type, array $realm_settings = array()) {
    $options = array();
    if (isset($realm_settings['vocabulary'])) {
      // Only support single-value term fields for now. This may change in the
      // future, if we can figure out a sane way to handle it on object forms.
      $fields = field_read_fields(array('type' => 'taxonomy_term_reference', 'cardinality' => 1));
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
      '#default_value' => $this->getSetting('field_name', ''),
    );
    return $form;
  }

  /**
   * Implements AccessControlKitInterface::formAlterNew().
   */
  public function formAlterNew(&$form, &$form_state, $form_id, $realms = array()) {
    $field_name = $this->getSetting('field_name');
    if (!empty($field_name) && !empty($form[$field_name])) {
      $language = $form[$field_name]['#language'];
      $this->filterElementOptions($form[$field_name][$language], $realms, '_none');
    }
  }

  /**
   * Implements AccessControlKitInterface::formAlterExisting().
   */
  public function formAlterExisting(&$form, &$form_state, $form_id, $realms = array()) {
    $field_name = $this->getSetting('field_name');
    if (!empty($field_name)) {
      // Lock the term reference field.
      if (!empty($form[$field_name])) {
        $form[$field_name]['#disabled'] = TRUE;
      }
      // Hide the Delete button if the user does not have delete access.
      $node = $form_state['node'];
      if (isset($form['actions']['delete'])) {
        $access = FALSE;
        foreach ($this->getRealms($node) as $value) {
          if (array_key_exists($value, $realms)) {
            $access = TRUE;
          }
        }
        $form['actions']['delete']['#access'] = $access;
      }
    }
  }
}
