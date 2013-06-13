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
  protected $scheme_machine_name;

  /**
   * Overrides AccessControlKitHandler::__construct().
   */
  public function __construct($scheme, array $settings = array()) {
    parent::__construct($scheme, $settings);
    $this->scheme_machine_name = $scheme->machine_name;
  }

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    return t('An administrator can map menu links to access realms using the ACK menu module settings page.  The mapped link and its child links will become members of the access realm.');
  }

  /**
   * Overrides AccessControlKitHandler::objectRealms().
   */
  public function objectRealms($object_type, $menu_link) {
    $map = db_query('SELECT map FROM {ack_menu_map} WHERE mlid = :mlid', array(':mlid' => $menu_link->mlid))->fetchField();
    if (!empty($map)) {
      $map = unserialize($map);
      if (isset($map[$this->scheme_machine_name])) {
        return $map[$this->scheme_machine_name];
      }
    }
    return array();
  }

  // @todo objectFormAlter() the menu link form's parent field?
}
