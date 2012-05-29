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
      foreach ($terms['und'] as $term) {
        $realms[] = $term['tid'];
      }
    }
    return $realms;
  }
}
