<?php

/**
 * @file
 * Contains a dummy class for testing the access control kit handler API.
 */

/**
 * Controls access to the fictitious 'cat' and 'dog' object types.
 */
class ACKTestMeowHandler extends AccessControlKitHandler {

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    return t('The meow handler assigns the boolean TRUE realm to cats and the boolean FALSE realm to everything else.');
  }

  /**
   * Overrides AccessControlKitHandler::objectRealms().
   */
  public function objectRealms($object_type, $object) {
    return $object_type == 'cat' ? array(TRUE) : array(FALSE);
  }

  /**
   * Overrides AccessControlKitHandler::settingsForm().
   */
  public function settingsForm() {
    $form['access_test'] = array('#markup' => t('Meow handler settings would go here.'));
    return $form;
  }

}
