<?php

/**
 * @file
 * Contains the ACK menu handler interface.
 */

/**
 * Interface for a menu link access handler.
 */
interface AckMenuHandlerInterface extends AccessControlKitHandlerInterface {

  /**
   * Finds all links for the given realm.
   *
   * @param int $realm
   *   A realm value.
   *
   * @return array
   *   The realm's menu links, sorted by depth. Each value is an array defining
   *   a link, similar to what would be returned by menu_link_load().
   *
   * @see menu_overview_form()
   */
  public function realmLinks($realm);

  /**
   * Finds the default menu to which new links for a realm can be added.
   *
   * @param int $realm
   *   A realm value.
   *
   * @return string|false
   *   The menu name, or FALSE if no usable menu found.
   */
  public function realmMenu($realm);

}
