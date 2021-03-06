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
   * Finds all links in a menu for the given realm.
   *
   * @param int $realm
   *   A realm value.
   * @param string $menu_name
   *   The machine-readable name of the menu.
   *
   * @return array
   *   The realm's menu links, sorted by depth. Each value is an array defining
   *   a link, similar to what would be returned by menu_link_load().
   *
   * @see menu_overview_form()
   */
  public function realmLinks($realm, $menu_name);

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

  /**
   * Returns the list of menus managed by this handler.
   *
   * @return array
   *   An array with the machine-readable names as the keys, and human-readable
   *   titles as the values.
   */
  public function managedMenus();

}
