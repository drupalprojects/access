<?php
/**
 * @file
 * Access schemes administrative UI for the access control kit module.
 */

/**
 * Menu page callback; the access schemes overview page.
 */
function access_overview_schemes() {
  $schemes = access_scheme_get_schemes();
  $field_ui = module_exists('field_ui');
  $header = array(t('Name'), array(
    'data' => t('Operations'),
    'colspan' => $field_ui ? '4' : '2',
  ));
  $rows = array();

  foreach ($schemes as $key => $scheme) {
    $url = 'admin/structure/access/manage/' . str_replace('_', '-', $key);
    $row = array(theme('access_schemes_overview', array('scheme' => $scheme)));

    // The edit operation.
    $row[] = array('data' => l(t('edit'), $url));

    // Field UI operations.
    if ($field_ui) {
      // Manage fields.
      $row[] = array('data' => l(t('manage fields'), $url . '/fields'));
      // Manage display.
      $row[] = array('data' => l(t('manage display'), $url . '/display'));
    }

    // The delete operation.
    $row[] = array('data' => l(t('delete'), $url . '/delete'));

    $rows[] = $row;
  }

  $build['scheme_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No access schemes available. <a href="@link">Add an access scheme</a>.', array('@link' => url('admin/structure/access/add'))),
  );

  return $build;
}

/**
 * Returns HTML for the Name column on the access scheme admin overview page.
 *
 * @param $variables
 *   An associative array containing:
 *   - scheme: The access scheme object.
 */
function theme_access_schemes_overview($variables) {
  $scheme = $variables['scheme'];

  $output = check_plain($scheme->name);
  $output .= ' <small>' . t('(Machine name: @type)', array('@type' => $scheme->scheme)) . '</small>';
  $output .= '<div class="description">' . filter_xss_admin($scheme->description) . '</div>';
  return $output;
}

/**
 * Form constructor for the access scheme add/edit form.
 *
 * @param $scheme
 *   (optional) The access scheme to edit.
 *
 * @see access_scheme_form_validate()
 * @see access_scheme_form_submit()
 */
function access_scheme_form($form, &$form_state, $scheme = NULL) {
  if (!isset($scheme->scheme)) {
    $scheme = access_scheme_set_defaults();
  }
  $form['#scheme'] = $scheme;
  $form['old_scheme'] = array('#type' => 'value', '#value' => $scheme->scheme);

  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#default_value' => $scheme->name,
    '#description' => t('The human-readable name of this access scheme. It is recommended that this name begin with a capital letter and contain only letters, numbers, and spaces. This name must be unique.'),
    '#required' => TRUE,
    '#size' => 30,
  );

  $form['scheme'] = array(
    '#type' => 'machine_name',
    '#default_value' => $scheme->scheme,
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => 'access_scheme_load',
    ),
    '#description' => t('A unique machine-readable name for this access scheme. It must only contain lowercase letters, numbers, and underscores.'),
  );

  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $scheme->description,
    '#description' => t('A brief description of this access scheme.'),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save access scheme'),
    '#weight' => 40,
  );
  if (!empty($scheme->scheme)) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete access scheme'),
      '#weight' => 45,
    );
  }

  return $form;
}

/**
 * Form validation handler for access_scheme_form().
 *
 * @see access_scheme_form_submit()
 */
function access_scheme_form_validate($form, &$form_state) {
  $scheme = new stdClass();
  $scheme->scheme = trim($form_state['values']['scheme']);
  $scheme->name = trim($form_state['values']['name']);

  // Get the original machine name, before the form was submitted.
  $old_scheme = trim($form_state['values']['old_scheme']);

  // 'theme' conflicts with theme_access_scheme_form().
  // '0' is invalid because it wouldn't pass an empty() check.
  if (in_array($scheme->scheme, array('0', 'theme'))) {
    form_set_error('scheme', t('Invalid machine-readable name. Enter a name other than %invalid.', array('%invalid' => $scheme->scheme)));
  }

  // Check for duplicate human-readable names.
  $names = array_flip(access_scheme_get_names());
  if (isset($names[$scheme->name]) && $names[$scheme->name] != $old_scheme) {
    form_set_error('name', t('The human-readable name %name is already taken.', array('%name' => $scheme->name)));
  }
}

/**
 * Form submission handler for access_scheme_form().
 *
 * @see access_scheme_form_validate()
 */
function access_scheme_form_submit($form, &$form_state) {
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';

  $scheme = access_scheme_set_defaults();
  $scheme->scheme = trim($form_state['values']['scheme']);
  $scheme->old_scheme = isset($form_state['values']['old_scheme']) ? $form_state['values']['old_scheme'] : $scheme->scheme;
  $scheme->name = trim($form_state['values']['name']);
  $scheme->description = $form_state['values']['description'];

  if ($op == t('Delete access scheme')) {
    $form_state['redirect'] = 'admin/structure/access/manage/' . str_replace('_', '-', $scheme->old_scheme) . '/delete';
    return;
  }

  $status = access_scheme_save($scheme);
  menu_rebuild();

  $t_args = array('%name' => $scheme->name);
  if ($status == SAVED_UPDATED) {
    drupal_set_message(t('The access scheme %name has been updated.', $t_args));
  }
  elseif ($status == SAVED_NEW) {
    drupal_set_message(t('The access scheme %name has been added.', $t_args));
    watchdog('access', 'Added access scheme %name.', $t_args, WATCHDOG_NOTICE, l(t('view'), 'admin/structure/access'));
  }

  $form_state['redirect'] = 'admin/structure/access';
  return;
}

// @todo
function access_scheme_delete_confirm() {}
