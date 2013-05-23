<?php
/**
 * @file
 * Contains the entity list field access handler.
 */

/**
 * Controls access to an entity based on a list field.
 */
class ACKEntityList extends ACKEntityField {
  /**
   * Overrides ACKEntityField::__construct().
   */
  public function __construct($scheme = '', $object_type = '', array $settings = array()) {
    parent::__construct($scheme, $object_type, $settings);
    // Use the realm field from the scheme as the access control field.
    $this->field_name = $this->getSchemeSetting('field');
  }
}
