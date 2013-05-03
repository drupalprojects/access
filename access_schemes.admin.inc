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
 * @see access_scheme_form_delete_submit()
 */
function access_scheme_form($form, &$form_state, $scheme = NULL) {
  if ($is_new = !isset($scheme)) {
    $scheme = access_scheme_set_defaults();
  }
  $form_state['scheme'] = $scheme;

  // Human-readable name.
  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#default_value' => $scheme->name,
    '#description' => t('The human-readable name of this access scheme. It is recommended that this name be plural, begin with a capital letter, and contain only letters, numbers, and spaces. This name must be unique.'),
    '#required' => TRUE,
    '#size' => 28,
  );

  // Machine name.  Not editable after creation.
  $form['scheme'] = array(
    '#type' => 'machine_name',
    '#default_value' => $scheme->scheme,
    '#maxlength' => 28,
    '#machine_name' => array(
      'exists' => 'access_scheme_load',
    ),
    '#description' => t('A unique machine-readable name for this access scheme. It must only contain lowercase letters, numbers, and underscores.'),
    '#disabled' => !$is_new,
  );

  // Realm type.  Not editable after creation.
  $form['realm_type'] = array(
    '#title' => t('Realm type'),
    '#type' => 'select',
    '#default_value' => $scheme->realm_type,
    '#options' => access_realm_options(),
    '#description' => t('The basis for dividing the site into access realms. For example, if the realm type is a list field, you will be able to grant users access to content based upon the values of a designated list field.'),
    '#required' => TRUE,
    '#disabled' => !$is_new,
  );

  // Description.
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $scheme->description,
    '#description' => t('A brief description of this access scheme.'),
  );

  // Realm settings, handler config, etc. become available after creation.
  if (!$is_new) {
    // Hide the basic properties in a collapsed fieldset.
    $form = array('basic' => $form);
    $form['basic']['#type'] = 'fieldset';
    $form['basic']['#title'] = t('Basic settings');
    $form['basic']['#collapsible'] = TRUE;
    $form['basic']['#collapsed'] = TRUE;

    // Get additional info about the scheme.
    $field_info = field_info_field(access_scheme_fieldname($scheme));
    $realm_info = access_realm_info($scheme->realm_type);

    // Determine whether grants already exist for this scheme.
    $query = new EntityFieldQuery();
    $query
      ->entityCondition('entity_type', 'access_grant')
      ->entityCondition('bundle', $scheme->scheme)
      ->range(0,1);
    $result = $query->execute();
    $has_data = !empty($result['access_grant']);

    // Add the realm settings elements, if such exist.
    $realm_settings = module_invoke($realm_info['module'], 'access_realm_settings', $scheme->realm_type, $has_data, $field_info['settings']['arguments']);
    if (!empty($realm_settings)) {
      $form['realm_settings'] = $realm_settings;
      $form['realm_settings']['#type'] = 'container';
      $form['realm_settings']['#tree'] = TRUE;
    }

    // Additional settings tabs.
    $form['additional_settings'] = array(
      '#type' => 'vertical_tabs',
      '#attached' => array(
        'js' => array(drupal_get_path('module', 'access') . '/access_schemes.js'),
      ),
    );

    // Roles used for access grants in this scheme.
    $form['roles'] = array(
      '#type' => 'fieldset',
      '#title' => t('Roles'),
      '#collapsible' => TRUE,
      '#group' => 'additional_settings',
    );
    $roles = user_roles(TRUE);
    unset($roles[DRUPAL_AUTHENTICATED_RID]);
    $form['roles']['role_options'] = array(
      '#type' => 'checkboxes',
      '#title' => t('User roles available for use within this access scheme'),
      '#options' => $roles,
      '#default_value' => array_keys(access_scheme_roles($scheme)),
      '#description' => t('Users with the <em>Administer access grants</em> permission will be able to grant access to users with these roles within one or more access realms.'),
    );

    // Access handlers.
    $form['handlers'] = array(
      '#type' => 'fieldset',
      '#title' => t('Object access handlers'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#group' => 'additional_settings',
    );
    $access_info = module_invoke_all('access_info');
    if (empty($access_info)) {
      $form['handlers']['empty'] = array(
        '#markup' => t('No access handlers available. In order to control access to content, menu items, or other Drupal objects, you must first <a href="@link">enable one or more modules</a> that provide Access Control Kit handlers, such as the ACK node module.', array('@link' => url('admin/modules'))),
      );
    }
    else {
      $form['handlers']['handlers'] = array(
        '#type' => 'container',
        '#tree' => TRUE,
        '#prefix' => '<div id="access-handlers">',
        '#suffix' => '</div>',
      );

      // Get the current realm settings values.
      if (!empty($form_state['values']['realm_settings'])) {
        $realm_values = $form_state['values']['realm_settings'];
      }
      elseif (isset($field_info['settings']['arguments'])) {
        $realm_values = $field_info['settings']['arguments'];
      }
      else {
        $realm_values = array();
      }

      // Get the current handler settings values.
      if (!empty($form_state['values']['handlers'])) {
        $handler_values = $form_state['values']['handlers'];
      }
      elseif (isset($scheme->handlers)) {
        $handler_values = $scheme->handlers;
      }
      else {
        $handler_values = array();
      }

      foreach ($access_info as $object_type => $object_info) {
        $fieldset = array(
          '#type' => 'fieldset',
          '#title' => check_plain($object_info['label']),
          '#collapsible' => TRUE,
        );

        // Select the handler for the object type.
        $fieldset['handler'] = array(
          '#type' => 'select',
          '#title' => t('@object_type access realms are determined by', array('@object_type' => $object_info['label'])),
          '#options' => array(),
          '#default_value' => isset($scheme->handlers[$object_type]) ? get_class($scheme->handlers[$object_type]) : '',
          '#empty_option' => t('- Not managed -'),
          '#ajax' => array(
            'callback' => 'access_handler_form_ajax',
            'wrapper' => 'access-handlers',
            'effect' => 'fade',
          ),
        );
        // Only show handlers that match the scheme's realm type.
        foreach ($object_info['handlers'] as $handler_name => $handler_info) {
          if (in_array($scheme->realm_type, $handler_info['realm types'])) {
            $fieldset['handler']['#options'][$handler_name] = $handler_info['label'];
          }
        }

        // If we know which handler is currently selected, get its object.
        if (isset($handler_values[$object_type])) {
          // If the actual handler object is available, use that.
          if (is_object($handler_values[$object_type])) {
            $handler = $handler_values[$object_type];
          }
          // If all we have is the name, but it's the same as the saved handler,
          // then use the saved handler object.
          elseif (isset($scheme->handlers) && isset($scheme->handlers[$object_type]) && $handler_values[$object_type]['handler'] == get_class($scheme->handlers[$object_type])) {
            $handler = $scheme->handlers[$object_type];
          }
          // Otherwise, we have to use an unconfigured instance of the handler.
          else {
            $class = $handler_values[$object_type]['handler'];
            $handler = new $class($scheme->scheme);
          }

          // Add the handler's settings form.
          $fieldset['settings'] = $handler->settingsForm($scheme->realm_type, $realm_values);
        }

        $form['handlers']['handlers'][$object_type] = $fieldset;
      }
    }
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => $is_new ? t('Save access scheme and continue') : t('Save access scheme'),
  );
  if (!$is_new) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete access scheme'),
      '#submit' => array('access_scheme_form_delete_submit'),
    );
  }

  return $form;
}

/**
 * Form validation handler for access_scheme_form().
 *
 * @see access_scheme_form()
 * @see access_scheme_form_submit()
 */
function access_scheme_form_validate($form, &$form_state) {
  $original = $form_state['scheme'];
  $scheme = trim($form_state['values']['scheme']);
  $name = trim($form_state['values']['name']);

  $disallowed = array(
    // 'theme' would conflict with theme_access_scheme_form() if we implemented
    // per scheme form IDs (the way the node module does).
    'theme',
  );
  // Make sure that the machine name does not evaluate as empty (e.g., '0') and
  // is not one of our disallowed values.
  if (empty($scheme) || in_array($scheme, $disallowed)) {
    form_set_error('scheme', t('Invalid machine-readable name. Enter a name other than %invalid.', array('%invalid' => $scheme)));
  }

  // Check for duplicate human-readable names.
  $existing = access_scheme_get_names();
  if ($original->name != $name && in_array($name, $existing)) {
    form_set_error('name', t('The human-readable name %name is already taken.', array('%name' => $name)));
  }
}

/**
 * Form submission handler for access_scheme_form().
 *
 * @see access_scheme_form()
 * @see access_scheme_form_validate()
 */
function access_scheme_form_submit($form, &$form_state) {
  // Clean up the submission.
  form_state_values_clean($form_state);
  $scheme = (object) $form_state['values'];
  unset($scheme->additional_settings__active_tab);
  foreach (array('scheme', 'name') as $key) {
    $scheme->$key = trim($scheme->$key);
  }

  // Get the original values for use by access_scheme_save().
  $original = $form_state['scheme'];
  if (!empty($original->scheme)) {
    $scheme->original = $original;
  }

  // Format the selected roles as rid => role_name.
  if (isset($scheme->role_options)) {
    $roles = user_roles(TRUE);
    unset($roles[DRUPAL_AUTHENTICATED_RID]);
    $role_options = array_filter($scheme->role_options);
    $scheme->role_options = array_intersect_key($roles, $role_options);
  }
  else {
    $scheme->role_options = array();
  }

  // Save the scheme.
  $status = access_scheme_save($scheme);

  // Save the role options.
  variable_set('access_scheme_roles_' . $scheme->scheme, $scheme->role_options);

  // Save the handlers.
  if (isset($scheme->handlers)) {
    $modules = access_handler_modules();
    foreach ($scheme->handlers as $object_type => $handler_values) {
      $handler = $handler_values['handler'];
      if (!empty($handler)) {
        $module = $modules[$object_type][$handler];
        $settings = isset($handler_values['settings']) ? $handler_values['settings'] : array();
        access_handler_save($scheme->scheme, $object_type, $handler, $module, $settings);
      }
    }
  }

  // Rebuild the menu so that the "add access grant" page is up to date.
  menu_rebuild();

  $t_args = array('%name' => $scheme->name);
  if ($status == SAVED_UPDATED) {
    drupal_set_message(t('The access scheme %name has been updated.', $t_args));
    $form_state['redirect'] = 'admin/structure/access';
  }
  elseif ($status == SAVED_NEW) {
    drupal_set_message(t('The access scheme %name has been added.', $t_args));
    watchdog('access', 'Added access scheme %name.', $t_args, WATCHDOG_NOTICE, l(t('view'), 'admin/structure/access'));
    $form_state['redirect'] = 'admin/structure/access/manage/' . str_replace('_', '-', $scheme->scheme);
  }
}

/**
 * Form submission handler for access_scheme_form().
 *
 * Handles the "Delete access scheme" button on the scheme form.
 *
 * @see access_scheme_form()
 * @see access_scheme_form_validate()
 */
function access_scheme_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $scheme = $form_state['scheme'];
  $form_state['redirect'] = 'admin/structure/access/manage/' . str_replace('_', '-', $scheme->scheme) . '/delete';
}

/**
 * Form constructor for the access scheme delete confirmation form.
 *
 * @see access_scheme_delete_confirm_submit()
 */
function access_scheme_delete_confirm($form, &$form_state, $scheme) {
  $form['scheme'] = array('#type' => 'value', '#value' => $scheme->scheme);
  $form['name'] = array('#type' => 'value', '#value' => $scheme->name);

  $message = t('Are you sure you want to delete the access scheme %scheme?', array('%scheme' => $scheme->name));
  $caption = '';

  $num_grants = db_query('SELECT COUNT(*) FROM {access_grant} WHERE scheme = :scheme', array(':scheme' => $scheme->scheme))->fetchField();
  if ($num_grants) {
    $caption .= '<p>' . format_plural($num_grants, '%scheme is used by 1 access grant on your site. If you remove this scheme, you will not be able to edit the %scheme grant and users may not be able to exercise permissions based on roles assigned by that grant.', '%scheme is used by @count access grants on your site. If you remove this scheme, you will not be able to edit the %scheme grants and users may not be able to exercise permissions based on roles assigned by those grants.', array('%scheme' => $scheme->name)) . '</p>';
  }

  $caption .= '<p>' . t('This action cannot be undone.') . '</p>';

  return confirm_form($form, $message, 'admin/structure/access', $caption, t('Delete'));
}

/**
 * Form submission handler for access_scheme_delete_confirm().
 */
function access_scheme_delete_confirm_submit($form, &$form_state) {
  access_scheme_delete($form_state['values']['scheme']);
  menu_rebuild();

  $t_args = array('%name' => $form_state['values']['name']);
  drupal_set_message(t('The access scheme %name has been deleted.', $t_args));
  watchdog('access', 'Deleted access scheme %name.', $t_args, WATCHDOG_NOTICE);

  $form_state['redirect'] = 'admin/structure/access';
  return;
}
