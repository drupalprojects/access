<?php

/**
 * @file
 * Access grants administrative UI for the access control kit module.
 */

/**
 * Menu page callback; the access grants overview page.
 */
function access_overview_grants($form, &$form_state) {
  // Check for the delete confirmation form.
  if (isset($form_state['values']['operation']) && $form_state['values']['operation'] == 'delete') {
    return access_grant_multiple_delete_confirm($form, $form_state, array_filter($form_state['values']['grants']));
  }
  $form['filter'] = access_admin_grants_filter_form();
  $form['#submit'][] = 'access_admin_grants_filter_form_submit';
  $form['admin'] = access_admin_grants();

  return $form;
}

/**
 * Form constructor for the grant admin form.
 *
 * @see access_admin_grants_validate()
 * @see access_admin_grants_submit()
 */
function access_admin_grants() {
  // Build the delete button.
  $form['actions'] = array('#type' => 'actions', '#weight' => -10);
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Delete selected grants'),
    '#validate' => array('access_admin_grants_validate'),
    '#submit' => array('access_admin_grants_submit'),
  );

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
    'operations' => array('data' => t('Operations')),
  );

  $query = db_select('access_grant', 'g')
    ->extend('PagerDefault')
    ->extend('TableSort');
  access_admin_grants_build_filter_query($query);

  $query->join('users', 'u', 'g.uid = u.uid');
  $query->join('role', 'r', 'g.rid = r.rid');
  $query->join('access_scheme', 's', 'g.scheme = s.machine_name');
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
    $realms = implode(', ', access_grant_realms($grants[$gid]));
    if (drupal_strlen($realms) > 30) {
      $realms = drupal_substr($realms, 0, 25) . '...';
    }

    // Add the main columns.
    $options[$gid] = array(
      'username' => theme('username', array('account' => $fields)),
      'role' => check_plain($fields->role),
      // We don't need to check_plain($realms) because access_grant_realms()
      // already sanitized the realm labels.
      'realms' => $realms,
      'scheme' => check_plain($fields->scheme),
    );

    // Add the operations links.
    $url = 'admin/access/grant/' . $gid;
    $operations = array();
    $operations['view'] = array(
      'title' => t('view'),
      'href' => $url,
    );
    $operations['edit'] = array(
      'title' => t('edit'),
      'href' => $url . '/edit',
      'query' => $destination,
    );
    $operations['delete'] = array(
      'title' => t('delete'),
      'href' => $url . '/delete',
      'query' => $destination,
    );
    $options[$gid]['operations'] = array(
      'data' => array(
        '#theme' => 'links',
        '#links' => $operations,
        '#attributes' => array('class' => array('links', 'inline')),
      ),
    );
  }

  // Format as a table with pager.
  $form['grants'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No access grants available.'),
  );
  $form['pager'] = array('#markup' => theme('pager'));
  return $form;
}

/**
 * Form validation handler for access_admin_grants().
 *
 * @see access_admin_grants()
 * @see access_admin_grants_submit()
 */
function access_admin_grants_validate($form, &$form_state) {
  // Error if there are no items to select.
  if (!is_array($form_state['values']['grants']) || !count(array_filter($form_state['values']['grants']))) {
    form_set_error('', t('No items selected.'));
  }
}

/**
 * Form submission handler for access_admin_grants().
 *
 * @see access_admin_grants()
 * @see access_admin_grants_validate()
 */
function access_admin_grants_submit($form, &$form_state) {
  // Filter out unchecked grants.
  array_filter($form_state['values']['grants']);

  if ($form_state['values']['op'] == t('Delete selected grants')) {
    $form_state['values']['operation'] = 'delete';
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * Form constructor for the access grant administration filter form.
 *
 * @see access_admin_grants_filter_form_submit()
 */
function access_admin_grants_filter_form() {
  $session = isset($_SESSION['access_overview_grant_filter']) ? $_SESSION['access_overview_grant_filter'] : array();
  $filters = access_admin_grants_filters();

  $i = 0;
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Show only access grants where'),
    '#theme' => 'exposed_filters__access_grant',
  );
  foreach ($session as $filter) {
    list($type, $value) = $filter;
    if (isset($filters[$type]['options'])) {
      $value = $filters[$type]['options'][$value];
    }
    else {
      $value = check_plain($value);
    }
    if (!empty($value)) {
      $t_args = array(
        '%property' => $filters[$type]['title'],
        '%value' => $value,
      );
      if ($i++) {
        $form['filters']['current'][] = array('#markup' => t('and where %property is %value', $t_args));
      }
      else {
        $form['filters']['current'][] = array('#markup' => t('where %property is %value', $t_args));
      }
      unset($filters[$type]);
    }
  }

  $form['filters']['available'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('clearfix')),
    '#prefix' => ($i ? '<div class="additional-filters">' . t('and where') . '</div>' : ''),
  );
  $form['filters']['available']['filters'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('filters')),
  );
  foreach ($filters as $key => $filter) {
    if (isset($filter['options'])) {
      $form['filters']['available']['filters'][$key] = array(
        '#type' => 'select',
        '#options' => $filter['options'],
        '#title' => check_plain($filter['title']),
        '#default_value' => '[any]',
      );
    }
    else {
      $form['filters']['available']['filters'][$key] = array(
        '#type' => 'textfield',
        '#size' => '31',
        '#title' => check_plain($filter['title']),
      );
    }
  }

  $form['filters']['available']['actions'] = array(
    '#type' => 'actions',
    '#attributes' => array('class' => array('container-inline')),
  );
  $available = element_children($form['filters']['available']['filters']);
  if (!empty($available)) {
    $form['filters']['available']['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => count($session) ? t('Refine') : t('Filter'),
    );
  }
  if (count($session)) {
    $form['filters']['available']['actions']['undo'] = array(
      '#type' => 'submit',
      '#value' => t('Undo'),
    );
    $form['filters']['available']['actions']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
    );
  }

  drupal_add_js('misc/form.js');

  return $form;
}

/**
 * Form submission handler for access_admin_grants_filter_form().
 *
 * @see access_admin_grants_filter_form()
 */
function access_admin_grants_filter_form_submit($form, &$form_state) {
  $filters = access_admin_grants_filters();
  switch ($form_state['values']['op']) {
    case t('Filter'):
    case t('Refine'):
      // Apply every filter that has a choice selected other than 'any'.
      foreach ($filters as $filter => $options) {
        if (!empty($form_state['values'][$filter])) {
          if (!isset($options['options'])) {
            // Filters lacking an 'options' key are text input.
            $_SESSION['access_overview_grant_filter'][] = array($filter, $form_state['values'][$filter]);
          }
          elseif ($form_state['values'][$filter] != '[any]') {
            // Flatten the  array to accommodate hierarchical/nested options.
            $flat_options = form_options_flatten($options['options']);
            // Only accept valid selections offered on the dropdown.
            if (isset($flat_options[$form_state['values'][$filter]])) {
              $_SESSION['access_overview_grant_filter'][] = array($filter, $form_state['values'][$filter]);
            }
          }
        }
      }
      break;

    case t('Undo'):
      array_pop($_SESSION['access_overview_grant_filter']);
      break;

    case t('Reset'):
      $_SESSION['access_overview_grant_filter'] = array();
      break;
  }
}

/**
 * Lists access grant administration filters that can be applied.
 *
 * @return array
 *   Returns a list of filter definitions for the grant administration form.
 *   Each key is filter identifier for access_admin_grants_build_filter_query(),
 *   and each value is an array with the following keys:
 *   - 'title': The translated title of the filter, to be shown on the form.
 *   - 'options': (optional) A list of options to populate the filter as a
 *     select field. If omitted, the filter will be rendered as a text field.
 */
function access_admin_grants_filters() {
  // By role.
  $roles = array();
  foreach (access_scheme_load_multiple() as $scheme) {
    $roles[$scheme->machine_name] = $scheme->roles;
  }
  $filters['rid'] = array(
    'title' => t('role'),
    'options' => array('[any]' => t('any')) + options_array_flatten($roles),
  );

  // By scheme.
  $filters['scheme'] = array(
    'title' => t('scheme'),
    'options' => array('[any]' => t('any')) + access_scheme_names(),
  );

  // By username.
  $filters['username'] = array(
    'title' => t('username'),
  );

  return $filters;
}

/**
 * Apply filters for the access grant administration page based on session.
 *
 * @param SelectQueryInterface $query
 *   A SelectQuery to which the filters should be applied.
 */
function access_admin_grants_build_filter_query(SelectQueryInterface $query) {
  $filter_data = isset($_SESSION['access_overview_grant_filter']) ? $_SESSION['access_overview_grant_filter'] : array();
  foreach ($filter_data as $filter) {
    list($key, $value) = $filter;
    switch ($key) {
      case 'rid':
      case 'scheme':
        $query->condition('g.' . $key, $value);
        break;

      case 'username':
        $query->condition('u.name', $value);
        break;
    }
  }
}

/**
 * Menu page callback; the add access grant list page.
 */
function access_grant_add_list() {
  $content = array();
  foreach (access_scheme_load_multiple() as $scheme) {
    $content[$scheme->machine_name] = array(
      'title' => $scheme->name,
      'description' => $scheme->description,
      'href' => 'admin/access/add/' . str_replace('_', '-', $scheme->machine_name),
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
 * @param object $scheme
 *   The access scheme in which to create a new grant.
 *
 * @return array
 *   A form for adding a new access grant.
 */
function access_grant_add($scheme) {
  $grant = entity_get_controller('access_grant')->create(array('scheme' => $scheme->machine_name));
  return drupal_get_form('access_grant_form', $grant);
}

/**
 * Menu page callback; edit an access grant.
 *
 * @param object $grant
 *   The access grant to edit.
 *
 * @return array
 *   The access grant form.
 */
function access_grant_edit($grant) {
  $scheme = access_scheme_machine_name_load($grant->scheme);
  drupal_set_title(t('<em>Edit @scheme for</em> @grant', array(
    '@scheme' => $scheme->name,
    '@grant' => entity_label('access_grant', $grant),
  )), PASS_THROUGH);
  return drupal_get_form('access_grant_form', $grant);
}

/**
 * Form constructor for the access grant add/edit form.
 *
 * @param object $grant
 *   The access grant to edit.
 *
 * @see access_grant_form_validate()
 * @see access_grant_form_submit()
 * @see access_grant_form_delete_submit()
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
  $is_new = empty($grant->gid);
  $admin = user_access('administer users');

  // Store the scheme property for entity_form_field_validate().
  $form['scheme'] = array('#type' => 'value', '#value' => $grant->scheme);

  // The ACK user reference field.
  $form['user'] = array(
    '#type' => 'container',
    '#weight' => -5,
    '#prefix' => '<div id="access-grant-form-user">',
    '#suffix' => '</div>',
  );
  if (!$is_new) {
    $account = user_load($grant->uid);
    $form['user']['user'] = array(
      '#type' => 'value',
      '#value' => $account->name,
    );
    $form['user']['user_display'] = array(
      '#type' => 'item',
      '#title' => t('User'),
      '#markup' => format_username($account),
    );
  }
  else {
    if (!empty($form_state['values']['user'])) {
      $account = user_load_by_name($form_state['values']['user']);
    }
    else {
      $account = FALSE;
    }
    $form['user']['user'] = array(
      '#type' => 'textfield',
      '#title' => t('User'),
      '#maxlength' => 60,
      '#required' => TRUE,
      '#autocomplete_path' => 'user/autocomplete',
      '#ajax' => array(
        'callback' => 'access_grant_form_ajax',
        'wrapper' => 'access-grant-form-role',
        'effect' => 'fade',
      ),
    );
    if ($account) {
      $form['user']['user']['#default_value'] = $account->name;
    }
  }

  // The ACK role reference field.
  $form['role'] = array(
    '#type' => 'container',
    '#weight' => -4,
    '#prefix' => '<div id="access-grant-form-role">',
    '#suffix' => '</div>',
  );
  if (!$is_new) {
    $role = user_role_load($grant->rid);
    $form['role']['role'] = array(
      '#type' => 'value',
      '#value' => $role->rid,
    );
    $form['role']['role_display'] = array(
      '#type' => 'item',
      '#title' => t('Role'),
      '#markup' => check_plain($role->name),
    );
  }
  else {
    $scheme = access_scheme_machine_name_load($grant->scheme);
    $role_options = $scheme->roles;
    if ($account) {
      foreach (array_keys($role_options) as $option) {
        // Duplicate grants (same user, role, and scheme) are not permitted.
        $duplicate = access_grant_load_by_condition(array(
          'uid' => $account->uid,
          'rid' => $option,
          'scheme' => $grant->scheme,
        ));
        // Non-admins cannot add users to new roles.
        $disallowed = !$admin && empty($account->roles[$option]);
        // Filter the role options.
        if ($duplicate || $disallowed) {
          unset($role_options[$option]);
        }
      }
    }
    $form['role']['role'] = array(
      '#type' => 'select',
      '#title' => t('Role'),
      '#options' => $role_options,
      '#required' => TRUE,
    );
    if ($admin) {
      $form['role']['role']['#description'] = t('The user will be added to this role, if not already a member.');
    }
    if (empty($role_options)) {
      if ($account) {
        $form['role']['role']['#description'] = t('No roles are available for new access grants for this user. Try editing the user\'s <a href="@url">existing grants</a> instead.', array('@url' => url('admin/access')));
      }
      else {
        $form['role']['role']['#description'] = t('No roles are available for new access grants.');
      }
    }
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  if (!$is_new) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#submit' => array('access_grant_form_delete_submit'),
    );
  }
  $cancel = $is_new ? 'admin/access' : 'admin/access/grant/' . $grant->gid;
  if (isset($_GET['destination'])) {
    $cancel = drupal_get_destination();
    $cancel = $cancel['destination'];
  }
  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), $cancel),
  );

  field_attach_form('access_grant', $grant, $form, $form_state);
  return $form;
}

/**
 * Form validation handler for access_grant_form().
 *
 * @see access_grant_form()
 * @see access_grant_form_submit()
 */
function access_grant_form_validate($form, &$form_state) {
  $grant = $form_state['grant'];

  // Validate the properties of new grants.
  if (empty($grant->gid)) {
    // Validate the user reference.
    $account = user_load_by_name($form_state['values']['user']);
    if (empty($account)) {
      form_set_error('user', t('The username %name does not exist.', array('%name' => $form_state['values']['user'])));
    }
    else {
      $form_state['values']['uid'] = $account->uid;
    }

    // Validate the role reference.
    $role = user_role_load($form_state['values']['role']);
    $scheme = access_scheme_machine_name_load($grant->scheme);
    if (empty($role)) {
      form_set_error('role', t('A role with ID %rid does not exist.', array('%rid' => $form_state['values']['role'])));
    }
    elseif (empty($scheme->roles[$role->rid])) {
      form_set_error('role', t('The %role role cannot be used in @scheme.', array('%role' => $role->name)));
    }
    elseif ($account && empty($account->roles[$role->rid]) && !user_access('administer users')) {
      form_set_error('role', t('%user is not a member of the %role role.  You must have administrative access to user accounts in order to add a user to a new role.', array('%user' => $account->name, '%role' => $role->name)));
    }
    else {
      $form_state['values']['rid'] = $role->rid;
    }

    // If the above tests passed, validate that the user-role-scheme combination
    // is unique.  Duplicate grants are not permitted.
    if (!form_get_errors()) {
      $existing = access_grant_load_by_condition(array(
        'uid' => $account->uid,
        'rid' => $role->rid,
        'scheme' => $grant->scheme,
      ));
      if (!empty($existing)) {
        $existing = reset($existing);
        form_set_error('', t('%user has already been granted access as %role in @scheme. Duplicate access grants are not permitted. Please <a href="@url">edit the existing grant</a> instead.', array(
          '%user' => $account->name,
          '%role' => $role->name,
          '@scheme' => $scheme->name,
          '@url' => url('admin/access/grant/' . $existing->gid . '/edit'),
        )));
      }
    }
  }

  entity_form_field_validate('access_grant', $form, $form_state);
}

/**
 * Form submission handler for access_grant_form().
 *
 * @see access_grant_form()
 * @see access_grant_form_validate()
 */
function access_grant_form_submit($form, &$form_state) {
  $grant = $form_state['grant'];
  entity_form_submit_build_entity('access_grant', $grant, $form, $form_state);

  $account = user_load($grant->uid);
  $role = user_role_load($grant->rid);
  $scheme = access_scheme_machine_name_load($grant->scheme);

  // Add the user to the role, if not already a member.
  if (empty($account->roles[$role->rid])) {
    $roles = $account->roles + array($role->rid => $role->name);
    $account->original = clone $account;
    user_save($account, array('roles' => $roles));
  }

  // Save the grant.
  $status = access_grant_save($grant);

  // Report the change.
  $t_args = array(
    '%user' => format_username($account),
    '%role' => $role->name,
    '%scheme' => $scheme->name,
  );
  switch ($status) {
    case SAVED_NEW:
      drupal_set_message(t("Added %scheme for %user's access as %role.", $t_args));
      break;

    case SAVED_UPDATED:
      drupal_set_message(t("Updated %scheme for %user's access as %role.", $t_args));
      break;
  }
  watchdog('access', "Changed %scheme for %user as %role.", $t_args, WATCHDOG_NOTICE, l(t('edit'), 'admin/access/grant/' . $grant->gid . '/edit'));
}

/**
 * Form submission handler for access_grant_form().
 *
 * Handles the "Delete" button on the grant form.
 *
 * @see access_grant_form()
 * @see access_grant_form_validate()
 */
function access_grant_form_delete_submit($form, &$form_state) {
  if (isset($_GET['destination'])) {
    drupal_get_destination();
    unset($_GET['destination']);
  }
  $grant = $form_state['grant'];
  $form_state['redirect'] = 'admin/access/grant/' . $grant->gid . '/delete';
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
  $scheme = access_scheme_machine_name_load($grant->scheme);

  $t_args = array(
    '%user' => format_username($account),
    '%role' => $role->name,
    '%scheme' => $scheme->name,
  );
  $message = t("Are you sure you want to revoke all %scheme for %user's access as %role?", $t_args);
  $caption = '<p>' . t('This action cannot be undone.') . '</p>';

  // If this is the user's last grant for this role, allow administrators to
  // also revoke the role itself at the same time.
  $existing = access_grant_load_by_condition(array('uid' => $grant->uid, 'rid' => $grant->rid));
  if (count($existing) == 1) {
    $form['revoke_role'] = array(
      '#type' => 'checkbox',
      '#title' => t("Also revoke %user's membership in the %role role?", $t_args),
      '#description' => t("This is %user's last remaining access grant for the %role role. By checking this box, %user will be removed from the %role role entirely.", $t_args),
      '#access' => user_access('administer users'),
    );
  }

  return confirm_form($form, $message, 'admin/access/grant/' . $grant->gid, $caption, t('Delete'));
}

/**
 * Form submission handler for access_grant_delete_confirm().
 */
function access_grant_delete_confirm_submit($form, &$form_state) {
  $grant = $form_state['grant'];
  $account = user_load($grant->uid);
  $role = user_role_load($grant->rid);
  $scheme = access_scheme_machine_name_load($grant->scheme);

  // Delete the grant.
  access_grant_delete($grant->gid);

  // Report the change.
  $t_args = array(
    '%user' => format_username($account),
    '%role' => $role->name,
    '%scheme' => $scheme->name,
  );
  drupal_set_message(t("Deleted %scheme for %user's access as %role.", $t_args));
  watchdog('access', "Deleted %scheme for %user as %role.", $t_args, WATCHDOG_NOTICE);

  // Revoke the role entirely, if that option was selected.
  if (!empty($form_state['values']['revoke_role'])) {
    $roles = array_diff($account->roles, array($role->rid => $role->name));
    $account->original = clone $account;
    user_save($account, array('roles' => $roles));
  }

  $form_state['redirect'] = 'admin/access';
}

/**
 * Form constructor for the access grant multiple delete confirmation form.
 *
 * @see access_grant_multiple_delete_confirm_submit()
 */
function access_grant_multiple_delete_confirm($form, &$form_state, $selected) {
  $gids = array_keys($selected);
  $grants = access_grant_load_multiple($gids);
  $roles = user_roles();
  $schemes = access_scheme_names();
  $usernames = db_query('SELECT g.uid, u.name FROM {access_grant} g INNER JOIN {users} u ON g.uid = u.uid WHERE g.gid IN (:gids)', array(':gids' => $gids))->fetchAllKeyed();

  $form['grants'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );
  foreach ($grants as $grant) {
    $form['grants'][$grant->gid] = array(
      '#type' => 'hidden',
      '#value' => $grant->gid,
      '#prefix' => '<li>',
      '#suffix' => t("@scheme for @user's access as @role", array(
        '@user' => $usernames[$grant->uid],
        '@role' => $roles[$grant->rid],
        '@scheme' => $schemes[$grant->scheme],
      )) . "</li>\n",
    );
  }

  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => 'delete',
  );
  $form['#submit'][] = 'access_grant_multiple_delete_confirm_submit';
  $confirm_question = format_plural(count($grants), 'Are you sure you want to delete this access grant?', 'Are you sure you want to delete these access grants?');
  return confirm_form($form, $confirm_question, 'admin/access', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}

/**
 * Form submission handler for access_grant_multiple_delete_confirm().
 *
 * @see access_grant_multiple_delete_confirm()
 */
function access_grant_multiple_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    foreach (array_keys($form_state['values']['grants']) as $gid) {
      access_grant_delete($gid);
    }
    $count = count($form_state['values']['grants']);
    watchdog('access', 'Deleted @count access grants.', array('@count' => $count));
    drupal_set_message(format_plural($count, 'Deleted 1 access grant.', 'Deleted @count access grants.'));
  }
  $form_state['redirect'] = 'admin/access';
}
