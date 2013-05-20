<?php
/**
 * @file
 * Contains the node list field access handler.
 */

/**
 * Controls access to a node based on a list field.
 */
class ACKNodeList extends AccessControlKit {
  /**
   * Implements AccessControlKitInterface::getRealms().
   */
  public function getRealms($node) {
    // Return the selected values of the list field.
    if ($field_name = $this->getSchemeSetting('field')) {
      return $this->getFieldValues('value', 'node', $node, $field_name, $node->language);
    }
    return array();
  }

  /**
   * Implements AccessControlKitInterface::formAlterNew().
   */
  public function formAlterNew(&$form, &$form_state, $form_id, $realms = array()) {
    $field_name = $this->getSchemeSetting('field');
    if (!empty($field_name) && !empty($form[$field_name])) {
      $language = $form[$field_name]['#language'];
      $this->filterElementOptions($form[$field_name][$language], $realms, '_none');
    }
  }

  /**
   * Implements AccessControlKitInterface::formAlterExisting().
   */
  public function formAlterExisting(&$form, &$form_state, $form_id, $realms = array()) {
    $field_name = $this->getSchemeSetting('field');
    if (!empty($field_name)) {
      // Lock the list field.
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
