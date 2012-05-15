<?php
/**
 * @file
 * Access grants administrative UI for the access control kit module.
 */

/**
 * Menu page callback; the access grants overview page.
 */
function access_overview_grants() {
  return '@todo';
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

  // The built-in user reference field.
  $form['user'] = array(
    '#type' => 'textfield',
    '#title' => t('User'),
    '#maxlength' => 60,
    '#weight' => -5,
  );
  // Only allow the user to be set on add; it can't be edited once saved.
  if (empty($grant->uid)) {
    $form['user']['#autocomplete_path'] = 'user/autocomplete';
    $form['user']['#default_value'] = '';
    $form['user']['#required'] = TRUE;
  }
  else {
    $account = user_load($grant->uid);
    $form['user']['#default_value'] = check_plain($account->name);
    $form['user']['#disabled'] = TRUE;
  }

  // The built-in role reference field.
  $form['role'] = array(
    '#type' => 'select',
    '#title' => t('Role'),
    '#weight' => -4,
  );
  // As with the user reference above, the role can't be changed after creation.
  if (empty($grant->rid)) {
    $roles = user_roles(TRUE);
    unset($roles[DRUPAL_AUTHENTICATED_RID]);
    $form['role']['#options'] = $roles;
    $form['role']['#required'] = TRUE;
  }
  else {
    $role = user_role_load($grant->rid);
    $form['role']['#options'] = array($role->rid => $role->name);
    $form['role']['#default_value'] = $role->rid;
    $form['role']['#disabled'] = TRUE;
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

  // Validate the user reference. Only matters if the UID isn't already set.
  if (empty($grant->uid) && !empty($grant->user) && !($account = user_load_by_name($grant->user))) {
    form_set_error('user', t('The username %name does not exist.', array('%name' => $grant->user)));
  }

  entity_form_field_validate('access_grant', $form, $form_state);
}

/**
 * Form submission handler for access_grant_form().
 *
 * @see access_grant_form_validate()
 */
function access_grant_form_submit($form, &$form_state) {
  // @todo Need to handle the "Delete" case.

  $grant = access_grant_form_submit_build_grant($form, $form_state);
  // @todo Need a save function to call here.
}

/**
 * Updates the form state's grant entity by processing the submission values.
 */
function access_grant_form_submit_build_grant($form, &$form_state) {
  $grant = $form_state['grant'];
  entity_form_submit_build_entity('access_grant', $grant, $form, $form_state);

  if (empty($grant->uid)) {
    $account = user_load_by_name($form_state['values']['user']);
    $grant->uid = $account->uid;
  }
  if (empty($grant->rid)) {
    $grant->rid = $form_state['values']['role'];
  }

  return $grant;
}

/**
 * Menu page callback; view a single access grant.
 */
function access_grant_view($grant) {
  return '@todo';
}
