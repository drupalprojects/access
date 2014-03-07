<?php

/**
 * @file
 * Callback functions for the user access scheme type.
 */

/**
 * Realms callback for the user scheme type.
 *
 * @see access_access_scheme_info()
 */
function access_scheme_user_realms($scheme) {
  return db_query('SELECT uid, name FROM {users} WHERE uid > 0')->fetchAllKeyed();
}
