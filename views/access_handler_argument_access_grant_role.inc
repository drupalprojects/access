<?php

/**
 * @file
 * Definition of access_handler_argument_access_grant_role.
 */

/**
 * Allow an access grant's role ID as an argument.
 *
 * @ingroup views_argument_handlers
 */
class access_handler_argument_access_grant_role extends views_handler_argument_numeric {

  /**
   * Overrides views_handler_argument_numeric::title().
   */
  function title() {
    $title = db_query('SELECT name FROM {role} WHERE rid = :rid', array(':rid' => $this->argument))->fetchField();
    if (empty($title)) {
      return t('No role');
    }
    return check_plain($title);
  }

}
