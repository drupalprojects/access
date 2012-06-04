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
    // Make sure the object is correctly configured for a list field.
    if (empty($this->settings['field_name'])) {
      return array();
    }

    // Return the values of the selected list field, if any.
    $field_name = $this->settings['field_name'];
    $realms = array();
    if (isset($node->$field_name)) {
      $list = $node->$field_name;
      if (!empty($list['und'])) {
        foreach ($list['und'] as $item) {
          $realms[] = $item['value'];
        }
      }
    }
    return $realms;
  }

  /**
   * Implements AccessControlKitInterface::settingsForm().
   */
  public function settingsForm($realm_type, array $realm_settings = array()) {
    // Inherit the realm's list field as the handler's connection to the node.
    if (isset($realm_settings['field'])) {
      $form['field_name'] = array('#type' => 'value', '#value' => $realm_settings['field']);
      return $form;
    }
  }
}
