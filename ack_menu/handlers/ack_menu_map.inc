<?php

/**
 * @file
 * Contains the handler class for mapping menu links to access realms.
 */

/**
 * Controls access to menu links based on realm mapping.
 */
class ACKMenuMap extends AccessControlKitHandler {

  /**
   * The machine name of the scheme to which this handler is attached.
   *
   * @var string
   */
  protected $schemeMachineName;

  /**
   * Overrides AccessControlKitHandler::__construct().
   */
  public function __construct($scheme, array $settings = array()) {
    parent::__construct($scheme, $settings);
    // Save the scheme machine name for later reference by the mapping function.
    $this->schemeMachineName = $scheme->machine_name;
  }

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    return t('A menu link will be considered a part of an access realm if an administrator has assigned it&mdash;or one of its parent links&mdash;to that realm on the link edit form.');
  }

  /**
   * Overrides AccessControlKitHandler::objectRealms().
   */
  public function objectRealms($object_type, $menu_link) {
    $map = db_query('SELECT map FROM {ack_menu_map} WHERE mlid = :mlid', array(':mlid' => $menu_link['mlid']))->fetchField();
    if (!empty($map)) {
      $map = unserialize($map);
      if (isset($map[$this->schemeMachineName])) {
        return $map[$this->schemeMachineName];
      }
    }
    return array();
  }

  /**
   * Overrides AccessControlKitHandler::objectFormAlter().
   */
  public function objectFormAlter($object_type, $menu_link, &$form, &$form_state, $form_id, $realms = NULL) {
    // @todo Filter the menu link form's parent field.
  }
}
