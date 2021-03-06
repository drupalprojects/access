<?php

/**
 * @file
 * Access schemes administrative UI for the access control kit module.
 */

/**
 * Menu page callback; the access schemes overview page.
 */
function access_overview_schemes() {
  $field_ui = module_exists('field_ui');
  $header = array(
    t('Name'),
    array(
      'data' => t('Operations'),
      'colspan' => $field_ui ? '4' : '2',
    ),
  );
  $rows = array();

  foreach (access_scheme_load_multiple() as $scheme) {
    $url = 'admin/structure/access/' . str_replace('_', '-', $scheme->machine_name);
    $row = array(theme('access_overview_scheme_name', array('scheme' => $scheme)));

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
 * @param array $variables
 *   An associative array containing:
 *   - scheme: The access scheme.
 *
 * @return string
 *   HTML representing the themed output.
 */
function theme_access_overview_scheme_name($variables) {
  $scheme = $variables['scheme'];

  $output = check_plain($scheme->name);
  $output .= ' <small>' . t('(Machine name: @name)', array('@name' => $scheme->machine_name)) . '</small>';
  $output .= '<div class="description">' . filter_xss_admin($scheme->description) . '</div>';
  return $output;
}

/**
 * Menu page callback; the add access scheme list page.
 */
function access_scheme_add_list() {
  drupal_set_title(t('Add an access scheme'), PASS_THROUGH);

  $content = array();
  foreach (access_scheme_info() as $type => $info) {
    $content[$type] = array(
      'title' => $info['label'],
      'description' => $info['description'],
      'href' => 'admin/structure/access/add/' . str_replace('_', '-', $type),
      'localized_options' => array(),
    );
  }
  return theme('admin_block_content', array('content' => $content));
}

/**
 * Menu page callback; add an access scheme of a given type.
 *
 * @param array $info
 *   The scheme type definition.
 *
 * @return array
 *   A form for adding a new access scheme.
 */
function access_scheme_add($info) {
  drupal_set_title(t('Add access scheme: <em>@type</em>', array('@type' => $info['label'])), PASS_THROUGH);

  $scheme = entity_get_controller('access_scheme')->create(array('type' => $info['type']));
  $scheme->info = $info;
  return drupal_get_form('access_scheme_form', $scheme);
}

/**
 * Form constructor for the access scheme add/edit form.
 *
 * @param array $form
 *   A Forms API array
 * @param array &$form_state
 *   An array representing the current state of the form.
 * @param object $scheme
 *   The access scheme to edit.
 *
 * @return array
 *   The access scheme form.
 *
 * @see access_scheme_form_validate()
 * @see access_scheme_form_submit()
 * @see access_scheme_form_delete_submit()
 */
function access_scheme_form($form, &$form_state, $scheme) {
  // During initial form build, add the scheme entity to the form state for use
  // during form building and processing. During a rebuild, use what is in the
  // form state.
  if (!isset($form_state['scheme'])) {
    $form_state['scheme'] = $scheme;
  }
  else {
    $scheme = $form_state['scheme'];
  }

  // Determine whether grants already exist for this scheme.
  $is_new = empty($scheme->sid);
  if ($is_new) {
    $has_data = FALSE;
  }
  else {
    $query = new EntityFieldQuery();
    $query
      ->entityCondition('entity_type', 'access_grant')
      ->entityCondition('bundle', $scheme->machine_name)
      ->range(0, 1);
    $result = $query->execute();
    $has_data = !empty($result['access_grant']);
  }

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
  $form['machine_name'] = array(
    '#type' => 'machine_name',
    '#default_value' => $scheme->machine_name,
    '#maxlength' => 28,
    '#machine_name' => array(
      'exists' => 'access_scheme_machine_name_load',
    ),
    '#description' => t('A unique machine-readable name for this access scheme. It must only contain lowercase letters, numbers, and underscores.'),
    '#disabled' => !$is_new,
  );

  // Load the callbacks' include file, if one exists.
  if (!empty($scheme->info['include file'])) {
    require_once DRUPAL_ROOT . '/' . $scheme->info['include file'];
  }

  // Add any additional settings defined for this scheme type.
  if (!empty($scheme->info['settings callback']) && function_exists($scheme->info['settings callback'])) {
    $form['settings'] = call_user_func_array($scheme->info['settings callback'], array($scheme, $has_data));
    $form['settings']['#type'] = 'container';
    $form['settings']['#tree'] = TRUE;
  }

  // Description.
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $scheme->description,
    '#description' => t('A brief description of this access scheme.'),
  );

  // Hide the basic properties in a collapsed fieldset on existing schemes.
  if (!$is_new) {
    $form = array('basic' => $form);
    $form['basic']['#type'] = 'fieldset';
    $form['basic']['#title'] = t('Basic settings');
    $form['basic']['#collapsible'] = TRUE;
    $form['basic']['#collapsed'] = TRUE;
  }

  // Realm-enabled roles.
  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Roles'),
    '#collapsible' => TRUE,
    '#collapsed' => !$is_new,
  );
  $roles = user_roles(TRUE);
  unset($roles[DRUPAL_AUTHENTICATED_RID]);
  $form['roles']['roles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('User roles available for use within this access scheme'),
    '#options' => $roles,
    '#default_value' => isset($scheme->roles) ? array_keys($scheme->roles) : array(),
    '#description' => t('Users with the <em>administer access grants</em> permission will be able to grant access to users with these roles within one or more access realms.'),
  );

  // Handler settings only become available after the scheme has been saved, and
  // only if we have access-controllable object types defined.
  if (!$is_new) {
    $access_info = access_info();
    if (empty($access_info)) {
      $form['handlers_empty'] = array(
        '#markup' => t('No access-controllable objects available. In order to control access to content, menu links, or other Drupal objects, you must first <a href="@link">enable one or more modules</a> that provide Access Control Kit compatibility for those objects, such as the ACK node module.', array('@link' => url('admin/modules'))),
      );
    }
    else {
      $handler_info = access_handler_info();
      $form['handler_tabs'] = array(
        '#type' => 'vertical_tabs',
        '#prefix' => '<h2>' . t('Object access handlers') . '</h2>',
        '#attached' => array(
          'js' => array(drupal_get_path('module', 'access') . '/access_schemes.js'),
        ),
      );
      // Add a tab for each access-controllable object type.
      $form['handlers'] = array('#tree' => TRUE);
      foreach ($access_info as $object_type => $object_info) {
        $form['handlers'][$object_type] = array(
          '#type' => 'fieldset',
          '#title' => check_plain($object_info['label']),
          '#group' => 'handler_tabs',
        );
        // Add elements for each usable handler.
        $handlers = array();
        foreach ($object_info['handlers'] as $handler_class) {
          // Make sure that the handler supports this scheme type.
          if (isset($handler_info[$handler_class]) && in_array($scheme->type, $handler_info[$handler_class]['scheme types'])) {
            $handlers[$handler_class] = access_scheme_form_handler_element($scheme, $object_type, $handler_class);
          }
        }
        if (empty($handlers)) {
          $form['handlers'][$object_type]['#description'] = t('No handlers available.');
          $form['handlers'][$object_type]['empty'] = array(
            '#markup' => t('No object access handlers are available to manage @object_type objects in a @scheme_type scheme.', array('@object_type' => $object_info['label'], '@scheme_type' => $scheme->info['label'])),
          );
        }
        else {
          $form['handlers'][$object_type]['empty'] = array(
            '#type' => 'container',
            'handler' => array(
              '#type' => 'radio',
              '#title' => t('Not managed'),
              '#return_value' => '',
              '#default_value' => isset($scheme->handlers[$object_type]) ? FALSE : '',
              '#description' => t('Objects of this type will not be managed by the access scheme.'),
              '#parents' => array('handlers', $object_type, 'handler'),
            ),
          );
          $form['handlers'][$object_type] += $handlers;
        }
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
 * Form element builder for enabling and configuring a handler on a scheme form.
 *
 * @param object $scheme
 *   The access scheme.
 * @param string $type
 *   The access-controlled object type.
 * @param string $class
 *   The handler class name.
 *
 * @return array
 *   The form elements for the handler.
 *
 * @see access_scheme_form()
 */
function access_scheme_form_handler_element($scheme, $type, $class) {
  // See if this handler is already attached to the scheme.
  if (isset($scheme->handlers[$type]) && get_class($scheme->handlers[$type]) == $class) {
    $handler = $scheme->handlers[$type];
    $chosen = TRUE;
  }
  // Otherwise, use an unconfigured handler object.
  else {
    $handler = new $class($scheme);
    $chosen = FALSE;
  }

  $info = access_handler_info();
  $elements = array('#type' => 'container');
  $elements['handler'] = array(
    '#type' => 'radio',
    '#title' => check_plain($info[$class]['label']),
    '#return_value' => $class,
    '#default_value' => $chosen,
    '#description' => $handler->description(),
    '#parents' => array('handlers', $type, 'handler'),
  );
  $settings = $handler->settingsForm();
  if (!empty($settings)) {
    $elements['settings'] = array(
      '#parents' => array('handlers', $type, $class),
      '#tree' => TRUE,
      '#title' => t('Additional settings'),
      '#type' => 'fieldset',
      '#states' => array(
        'visible' => array(
          ':input[name="handlers[' . $type . '][handler]"]' => array('value' => $class),
        ),
      ),
    );
    $elements['settings'] += $settings;
  }
  return $elements;
}

/**
 * Form validation handler for access_scheme_form().
 *
 * @see access_scheme_form()
 * @see access_scheme_form_submit()
 */
function access_scheme_form_validate($form, &$form_state) {
  $scheme = $form_state['scheme'];

  // Validate the machine name on new schemes.
  if (empty($scheme->sid)) {
    $machine_name = $form_state['values']['machine_name'];

    // Some machine names cannot be used.
    $disallowed = array(
      // 'theme' would conflict with theme_access_scheme_form() if we implement
      // per scheme form IDs (the way the node module does).
      'theme',
      // 'list' and 'add' would conflict with menu callbacks.
      'list',
      'add',
    );
    // We also don't want machine names that evaluate to FALSE.
    if (empty($machine_name) || in_array($machine_name, $disallowed)) {
      form_set_error('machine_name', t('Invalid machine-readable name. Enter a name other than %invalid.', array('%invalid' => $machine_name)));
    }
  }

  // Check for duplicate human-readable names.
  $name = trim($form_state['values']['name']);
  if ((empty($scheme->name) || $scheme->name != $name) && in_array($name, access_scheme_names())) {
    form_set_error('name', t('The name %name is already in use.', array('%name' => $name)));
  }
}

/**
 * Updates the form state's scheme entity by processing the submitted values.
 *
 * This is the default builder function for the access scheme form. It is called
 * during the "Save" submit handler to retrieve the entity to save. This
 * function can also be called by a "Next" button of a wizard to update the form
 * state with the current step's values before proceeding to the next step.
 *
 * @return object
 *   The updated access scheme.
 *
 * @see access_scheme_form()
 */
function access_scheme_form_submit_build_scheme($form, &$form_state) {
  $scheme = $form_state['scheme'];
  entity_form_submit_build_entity('access_scheme', $scheme, $form, $form_state);

  // Format the selected roles as rid => role_name.
  $roles = user_roles(TRUE);
  unset($roles[DRUPAL_AUTHENTICATED_RID]);
  $scheme->roles = array_filter($scheme->roles);
  $scheme->roles = array_intersect_key($roles, $scheme->roles);

  // Attach the handlers.
  if (isset($scheme->handlers)) {
    foreach ($scheme->handlers as $object_type => $handler_values) {
      // Check whether the handler element was set to "not managed."
      if (empty($handler_values['handler'])) {
        $scheme->handlers[$object_type] = NULL;
      }
      else {
        $handler = $handler_values['handler'];
        $settings = isset($handler_values[$handler]) ? $handler_values[$handler] : array();
        entity_get_controller('access_scheme')->attachHandler($scheme, $object_type, $handler, $settings);
      }
    }
  }

  return $scheme;
}

/**
 * Form submission handler for access_scheme_form().
 *
 * @see access_scheme_form()
 * @see access_scheme_form_validate()
 */
function access_scheme_form_submit($form, &$form_state) {
  // Save the scheme.
  $scheme = access_scheme_form_submit_build_scheme($form, $form_state);
  $status = access_scheme_save($scheme);

  // Report the change.
  $t_args = array(
    '%name' => $scheme->name,
  );
  switch ($status) {
    case SAVED_NEW:
      drupal_set_message(t('Added access scheme %name.', $t_args));
      // New schemes redirect to the edit page to configure handlers.
      $form_state['redirect'] = 'admin/structure/access/' . str_replace('_', '-', $scheme->machine_name);
      break;

    case SAVED_UPDATED:
      drupal_set_message(t('Updated access scheme %name.', $t_args));
      // Updated schemes redirect to the admin overview.
      $form_state['redirect'] = 'admin/structure/access';
      break;
  }
  watchdog('access', 'Saved access scheme %name.', $t_args, WATCHDOG_NOTICE, l(t('edit'), 'admin/structure/access/' . str_replace('_', '-', $scheme->machine_name)));

  // Rebuild the menu so that the "add access grant" page is up to date.
  menu_rebuild();
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
  if (isset($_GET['destination'])) {
    drupal_get_destination();
    unset($_GET['destination']);
  }
  $scheme = $form_state['scheme'];
  $form_state['redirect'] = 'admin/structure/access/' . str_replace('_', '-', $scheme->machine_name) . '/delete';
}

/**
 * Form constructor for the access scheme delete confirmation form.
 *
 * @see access_scheme_delete_confirm_submit()
 */
function access_scheme_delete_confirm($form, &$form_state, $scheme) {
  $form_state['scheme'] = $scheme;

  $t_args = array(
    '%scheme' => $scheme->name,
  );

  $message = t('Are you sure you want to delete the access scheme %scheme?', $t_args);
  $caption = '';

  $num_grants = db_query('SELECT COUNT(*) FROM {access_grant} WHERE scheme = :scheme', array(':scheme' => $scheme->machine_name))->fetchField();
  if ($num_grants) {
    $caption .= '<p>' . format_plural($num_grants, 'All access grants within the scheme will also be deleted. %scheme currently contains 1 access grant on your site. If you remove this scheme, the user may not be able to exercise the permissions assigned by that grant.', 'All access grants within the scheme will also be deleted. %scheme currently contains @count access grants on your site. If you remove this scheme, users may not be able to exercise the permissions assigned by those grants.', $t_args) . '</p>';
  }

  $caption .= '<p>' . t('This action cannot be undone.') . '</p>';

  return confirm_form($form, $message, 'admin/structure/access', $caption, t('Delete'));
}

/**
 * Form submission handler for access_scheme_delete_confirm().
 */
function access_scheme_delete_confirm_submit($form, &$form_state) {
  $scheme = $form_state['scheme'];
  access_scheme_delete($scheme->sid);

  // Report the change.
  $t_args = array(
    '%scheme' => $scheme->name,
  );
  drupal_set_message(t('Deleted access scheme %scheme.', $t_args));
  watchdog('access', 'Deleted access scheme %scheme.', $t_args, WATCHDOG_NOTICE);

  $form_state['redirect'] = 'admin/structure/access';

  // Rebuild the menu so that the "add access grant" page is up to date.
  menu_rebuild();
}
