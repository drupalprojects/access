<?php
/**
 * @file
 * Contains the menu link-to-realm mapping access handler.
 */

/**
 * Controls access to menu links based on realm mapping.
 */
class ACKMenuMap extends AccessControlKit {
  /**
   * Implements AccessControlKitInterface::getRealms().
   */
  public function getRealms($menu_link) {
    $map = db_query('SELECT map FROM {ack_menu_map} WHERE mlid = :mlid', array(':mlid' => $menu_link->mlid))->fetchField();
    if (!empty($map)) {
      $map = unserialize($map);
      if (!empty($this->scheme) && isset($map[$this->scheme])) {
        return $map[$this->scheme];
      }
    }
    return array();
  }
}
