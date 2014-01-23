<?php

/**
 * @file
 * Contains the handler class for the author property on nodes.
 */

/**
 * Controls access to a node based on its author.
 */
class ACKNodeAuthor extends AccessControlKitHandler {

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    return t('Allows you to grant a user access to content authored by certain other users (for example, User A could be granted access to all content written by Users B, C, and D).');
  }

  /**
   * Overrides AccessControlKitHandler::objectRealms().
   */
  public function objectRealms($object_type, $node) {
    return array($node->uid);
  }
}
