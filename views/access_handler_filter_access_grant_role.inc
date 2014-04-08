<?php

/**
 * @file
 * Definition of access_handler_filter_access_grant_role.
 */

/**
 * Filter by an access grant's role ID.
 *
 * @ingroup views_filter_handlers
 */
class access_handler_filter_access_grant_role extends views_handler_filter_in_operator {

  function init(&$view, &$options) {
    parent::init($view, $options);
    $this->definition['allow empty'] = FALSE;
  }

  /**
   * Overrides views_handler_filter_in_operator::get_value_options().
   */
  function get_value_options() {
    $this->value_options = user_roles(TRUE);
    unset($this->value_options[DRUPAL_AUTHENTICATED_RID]);
  }

}
