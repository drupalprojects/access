<?php

/**
 * @file
 * Callback functions for the boolean access scheme type.
 */

/**
 * Realms callback for the boolean scheme type.
 *
 * @see access_access_scheme_info()
 */
function access_scheme_boolean_realms($scheme) {
  return array(0 => t('False'), 1 => t('True'));
}
