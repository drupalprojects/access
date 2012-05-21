<?php
/**
 * @file
 * Access grants administrative UI for the access control kit module.
 */

/**
 * Menu page callback; the access grants overview page.
 */
function access_overview_grants() {
  $form['admin'] = access_admin_grants();

  return $form;
}

/**
 * Form constructor for the grant admin form.
 */
function access_admin_grants() {
  // Build the sortable table header.
  $header = array(
    'username' => array(
      'data' => t('Username'),
      'field' => 'u.name',
    ),
    'role' => array(
      'data' => t('Role'),
      'field' => 'r.name',
    ),
    'realms' => t('Realms'),
    'scheme' => array(
      'data' => t('Scheme'),
      'field' => 's.name',
    ),
    'operations' => array(
      'data' => t('Operations'),
      'colspan' => '3',
    ),
  );

  $query = db_select('access_grant', 'g')
    ->extend('PagerDefault')
    ->extend('TableSort');
  $query->join('users', 'u', 'g.uid = u.uid');
  $query->join('role', 'r', 'g.rid = r.rid');
  $query->join('access_scheme', 's', 'g.scheme = s.scheme');
  $query->addField('g', 'gid', 'gid');
  $query->addField('r', 'name', 'role');
  $query->addField('s', 'name', 'scheme');
  $result = $query
    ->fields('u', array('uid', 'name', 'status', 'created', 'access'))
    ->limit(50)
    ->orderByHeader($header)
    ->execute()
    ->fetchAllAssoc('gid');
  $grants = access_grant_load_multiple(array_keys($result));

  // Prepare the list of grants.
  $destination = drupal_get_destination();
  $options = array();
  foreach ($result as $gid => $fields) {
    // Get the realms list, truncated for length.
    $realms = implode(', ', access_grant_get_realms($grants[$gid]));
    if (drupal_strlen($realms) > 30) {
      $realms = drupal_substr($realms, 0, 25) . '...';
    }

    // Add the main columns.
    $options[$gid] = array(
      'username' => theme('username', array('account' => $fields)),
      'role' => check_plain($fields->role),
      'realms' => $realms,
      'scheme' => check_plain($fields->scheme),
    );

    // Add the operations links.
    $url = 'admin/access/grant/' . $gid;
    $options[$gid][] = array('data' => l(t('view'), $url));
    $options[$gid][] = array('data' => l(t('edit'), $url . '/edit', array('query' => $destination)));
    $options[$gid][] = array('data' => l(t('delete'), $url . '/delete', array('query' => $destination)));
  }

  // Format as a table with pager.
  $form['grants'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $options,
    '#empty' => t('No access grants available.'),
  );
  $form['pager'] = array('#markup' => theme('pager'));
  return $form;
}

/**
 * Menu page callback; the access grant add list page.
 */
function access_grant_add_list() {
  $schemes = access_scheme_get_schemes();
  $content = array();

  foreach ($schemes as $key => $scheme) {
    $content[$key] = array(
      'title' => $scheme->name,
      'description' => $scheme->description,
      'href' => 'admin/access/add/' . str_replace('_', '-', $scheme->scheme),
      'localized_options' => array(),
    );
  }

  if (empty($content)) {
    return t('You have not created any access schemes yet. Go to the <a href="@create-scheme">access scheme creation page</a> to add a new access scheme.', array('@create-scheme' => url('admin/structure/access/add')));
  }
  // Bypass this listing page if only one scheme is available.
  if (count($content) == 1) {
    $item = array_shift($content);
    drupal_goto($item['href']);
  }
  return theme('admin_block_content', array('content' => $content));
}

/**
 * Menu page callback; add an access grant for a given scheme.
 *
 * @param $scheme
 *   The access scheme in which to create a new grant.
 */
function access_grant_add($scheme) {
  $grant = (object) array('scheme' => $scheme->scheme);
  return drupal_get_form('access_grant_form', $grant);
}

/**
 * Menu page callback; edit an access grant.
 *
 * @param $grant
 *   The access grant to edit.
 */
function access_grant_edit($grant) {
  $scheme = access_scheme_get_scheme($grant);
  drupal_set_title(t('<em>Edit grant by @scheme</em> @grant', array('@scheme' => $scheme->name, '@grant' => access_grant_label($grant, $grant->scheme))), PASS_THROUGH);
  return drupal_get_form('access_grant_form', $grant);
}

/**
 * Form constructor for the access grant add/edit form.
 *
 * @param $grant
 *   The access grant to edit.
 *
 * @see access_grant_form_validate()
 * @see access_grant_form_submit()
 */
function access_grant_form($form, &$form_state, $grant) {
  // During initial form build, add the grant entity to the form state for use
  // during form building and processing. During a rebuild, use what is in the
  // form state.
  if (!isset($form_state['grant'])) {
    $form_state['grant'] = $grant;
  }
  else {
    $grant = $form_state['grant'];
  }

  // Basic grant information.  Not user-configurable.
  foreach (array('gid', 'uid', 'rid', 'scheme') as $key) {
    $form[$key] = array(
      '#type' => 'value',
      '#value' => isset($grant->$key) ? $grant->$key : NULL,
    );
  }
  $is_new = empty($grant->gid);

  // The ACK user reference field.
  $account = empty($grant->uid) ? FALSE : user_load($grant->uid);
  $form['user'] = array(
    '#type' => 'textfield',
    '#title' => t('User'),
    '#maxlength' => 60,
    '#weight' => -5,
    '#required' => $is_new,
    '#disabled' => !$is_new,
  );
  if ($is_new) {
    $form['user']['#autocomplete_path'] = 'user/autocomplete';
  }
  if (!empty($account)) {
    $form['user']['#default_value'] = check_plain($account->name);
  }

  // The ACK role reference field.
  $role = empty($grant->rid) ? FALSE : user_role_load($grant->rid);
  $form['role'] = array(
    '#type' => 'select',
    '#title' => t('Role'),
    '#weight' => -4,
    '#options' => access_realm_roles(),
    '#required' => $is_new,
    '#disabled' => !$is_new,
  );
  if (!empty($role)) {
    $form['role']['#default_value'] = $role->rid;
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
  );
  if (!empty($grant->gid)) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 10,
    );
  }

  field_attach_form('access_grant', $grant, $form, $form_state);
  return $form;
}

/**
 * Form validation handler for access_grant_form().
 *
 * @see access_grant_form_submit()
 */
function access_grant_form_validate($form, &$form_state) {
  $grant = (object) $form_state['values'];

  // On new grants, we must validate that the user reference is valid, and that
  // the uid-rid-scheme combination is unique. This is not needed for editing
  // an existing grant, because the user and role fields cannot be changed.
  if (empty($grant->gid)) {
    $account = empty($grant->user) ? FALSE : user_load_by_name($grant->user);
    $role = empty($grant->role) ? FALSE : user_role_load($grant->role);

    // Validate the user reference.
    if (empty($account)) {
      form_set_error('user', t('The username %name does not exist.', array('%name' => $grant->user)));
    }

    // Validate the uniqueness of the user-role-scheme combination.
    if (isset($account->uid) && isset($role->rid)) {
      $existing = access_grant_load_by_condition(array('uid' => $account->uid, 'rid' => $role->rid, 'scheme' => $grant->scheme));
      if (!empty($existing)) {
        $scheme = access_scheme_load($grant->scheme);
        $existing = reset($existing);
        form_set_error('role', t('User %user has already been granted the %role role within the %scheme access scheme. Duplicate access grants are not permitted. Please <a href="@url">edit the existing grant</a> instead.', array('%user' => $account->name, '%role' => $role->name, '%scheme' => $scheme->name, '@url' => url('admin/access/grant/' . $existing->gid . '/edit'))));
      }
    }
  }

  entity_form_field_validate('access_grant', $form, $form_state);
}

/**
 * Form submission handler for access_grant_form().
 *
 * @see access_grant_form_validate()
 */
function access_grant_form_submit($form, &$form_state) {
  // Check for the Delete button.
  if ($form_state['triggering_element']['#value'] == t('Delete')) {
    $form_state['redirect'] = 'admin/access/grant/' . $form_state['values']['gid'] . '/delete';
    return;
  }

  $grant = access_grant_form_submit_build_grant($form, $form_state);
  $status = access_grant_save($grant);

  $account = user_load($grant->uid);
  $role = user_role_load($grant->rid);
  $scheme = access_scheme_load($grant->scheme);
  $t_args = array(
    '%user' => $account->name,
    '%role' => $role->name,
    '%scheme' => $scheme->name,
  );
  switch ($status) {
    case SAVED_NEW:
      drupal_set_message(t('Granted %user access to the %role role within the %scheme access scheme.', $t_args));
      watchdog('access', 'Granted %role to %user in %scheme.', $t_args, WATCHDOG_NOTICE, l(t('edit'), 'admin/access/grant/' . $grant->gid . '/edit'));
      break;
    case SAVED_UPDATED:
      drupal_set_message(t("Updated %user's access to the %role role within the %scheme access scheme.", $t_args));
      watchdog('access', 'Updated %role realms for %user in %scheme.', $t_args, WATCHDOG_NOTICE, l(t('edit'), 'admin/access/grant/' . $grant->gid . '/edit'));
      break;
  }

  $form_state['values']['gid'] = $grant->gid;
}

/**
 * Updates the form state's grant entity by processing the submission values.
 */
function access_grant_form_submit_build_grant($form, &$form_state) {
  $grant = $form_state['grant'];
  entity_form_submit_build_entity('access_grant', $grant, $form, $form_state);

  if (empty($grant->gid)) {
    $account = user_load_by_name($form_state['values']['user']);
    $grant->uid = $account->uid;
    $grant->rid = $form_state['values']['role'];
  }

  return $grant;
}

/**
 * Form constructor for the access grant delete confirmation form.
 *
 * @see access_grant_delete_confirm_submit()
 */
function access_grant_delete_confirm($form, &$form_state, $grant) {
  $form_state['grant'] = $grant;

  $account = user_load($grant->uid);
  $role = user_role_load($grant->rid);
  $scheme = access_scheme_load($grant->scheme);
  $t_args = array(
    '%user' => $account->name,
    '%role' => $role->name,
    '%scheme' => $scheme->name,
  );

  $message = t("Are you sure you want to revoke %user's access to the %role role in the %scheme access scheme?", $t_args);
  $caption = '<p>' . t('If this is the last access grant giving %user access to the %role role, then %user will be removed from the role entirely.', $t_args) . '</p>';
  $caption .= '<p>' . t('This action cannot be undone.') . '</p>';

  return confirm_form($form, $message, 'admin/access/grant/' . $grant->gid, $caption, t('Delete'));
}

/**
 * Form submission handler for access_grant_delete_confirm().
 */
function access_grant_delete_confirm_submit($form, &$form_state) {
  $grant = $form_state['grant'];
  access_grant_delete($grant->gid);

  $account = user_load($grant->uid);
  $role = user_role_load($grant->rid);
  $scheme = access_scheme_load($grant->scheme);
  $t_args = array(
    '%user' => $account->name,
    '%role' => $role->name,
    '%scheme' => $scheme->name,
  );
  drupal_set_message(t('%user has been removed from the %role role for the %scheme access scheme.', $t_args));
  watchdog('access', 'Revoked %role for %user in %scheme.', $t_args, WATCHDOG_NOTICE);

  $form_state['redirect'] = 'admin/access';
  return;
}

/**
 * Menu page callback; view a single access grant.
 */
function access_grant_view($grant) {
  return '@todo grant view page';
}
