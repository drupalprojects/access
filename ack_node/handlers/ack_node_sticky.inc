<?php

/**
 * @file
 * Contains the handler class for the "sticky" flag on nodes.
 */

/**
 * Controls access to a node based on its "sticky" property.
 */
class ACKNodeSticky extends AccessControlKitHandler {

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    return t('Access will be based on whether or not the content is sticky.');
  }

  /**
   * Overrides AccessControlKitHandler::objectRealms().
   */
  public function objectRealms($object_type, $node) {
    return array($node->sticky);
  }
}
