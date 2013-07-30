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
  $header = array(t('Name'), array(
    'data' => t('Operations'),
    'colspan' => $field_ui ? '4' : '2',
  ));
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
 * @param $variables
 *   An associative array containing:
 *   - scheme: The access scheme.
 */
function theme_access_overview_scheme_name($variables) {
  $scheme = $variables['scheme'];

  $output = check_plain($scheme->name);
  $output .= ' <small>' . t('(Machine name: @type)', array('@type' => $scheme->machine_name)) . '</small>';
  $output .= '<div class="description">' . filter_xss_admin($scheme->description) . '</div>';
  return $output;
}

/**
 * Menu page callback; the add access scheme list page.
 */
function access_scheme_add_list() {
  drupal_set_title(t('Add an access scheme'), PASS_THROUGH);

  // @todo Need some helpful instructions on the add scheme list page.
  $content = array();
  foreach (access_realm_info() as $realm_type => $realm_info) {
    $content[$realm_type] = array(
      'title' => $realm_info['label'],
      'description' => $realm_info['description'],
      'href' => 'admin/structure/access/add/' . str_replace('_', '-', $realm_type),
      'localized_options' => array(),
    );
  }
  return theme('admin_block_content', array('content' => $content));
}

/**
 * Menu page callback; add an access scheme of a given realm type.
 *
 * @param $realm_type_info
 *   The realm type definition.
 */
function access_scheme_add($realm_type_info) {
  drupal_set_title(t('Add access scheme: <em>@type</em>', array('@type' => $realm_type_info['label'])), PASS_THROUGH);

  $scheme = entity_get_controller('access_scheme')->create(array('realm_type' => $realm_type_info['realm_type']));
  $scheme->realm_info = $realm_type_info;
  return drupal_get_form('access_scheme_form', $scheme);
}

/**
 * Form constructor for the access scheme add/edit form.
 *
 * @param $scheme
 *   The access scheme to edit.
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

  // Add the realm settings elements, if such exist.
  $realm_settings = isset($scheme->realm_field) ? $scheme->realm_field['settings']['arguments'] : array();
  $realm_elements = module_invoke($scheme->realm_info['module'], 'access_realm_settings', $scheme->realm_type, $has_data, $realm_settings);
  if (!empty($realm_elements)) {
    $form['realm_settings'] = $realm_elements;
    $form['realm_settings']['#type'] = 'container';
    $form['realm_settings']['#tree'] = TRUE;
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
  if (!$is_new && $access_info = access_info()) {
    $form['handler_tabs'] = array(
      '#type' => 'vertical_tabs',
      '#prefix' => '<h2>' . t('Object access handlers') . '</h2>',
      '#attached' => array(
        'js' => array(drupal_get_path('module', 'access') . '/access_schemes.js'),
      ),
    );

    // Add a tab for each access-controllable object type.
    $handler_info = access_handler_info();
    $form['handlers'] = array('#tree' => TRUE);
    foreach ($access_info as $object_type => $object_info) {
      $fieldset = array(
        '#type' => 'fieldset',
        '#title' => check_plain($object_info['label']),
        '#group' => 'handler_tabs',
      );

      // Find handlers that support this realm type and object type.
      $handler_options = array();
      foreach ($object_info['handlers'] as $handler_class) {
        if (isset($handler_info[$handler_class]) && in_array($scheme->realm_type, $handler_info[$handler_class]['realm types'])) {
          $handler_options[$handler_class] = $handler_info[$handler_class]['label'];
        }
      }

      if (empty($handler_options)) {
        $fieldset['#description'] = t('No handlers available.');
        $fieldset['empty'] = array(
          '#markup' => t('No object access handlers are available to manage @object_type objects in a @realm_type scheme.', array('@object_type' => $object_info['label'], '@realm_type' => $scheme->realm_info['label'])),
        );
      }
      else {
        // Create an HTML ID for this object type's handler settings wrapper.
        // We use drupal_html_class() to sanitize, rather than drupal_html_id(),
        // so that the ID doesn't increment on each AJAX rebuild.
        $wrapper_id = drupal_html_class('access-handler-' . $object_type . '-settings');

        // Select box for choosing the handler.
        $fieldset['handler'] = array(
          '#type' => 'select',
          '#title' => t('@object_type access realms are determined by', array('@object_type' => $object_info['label'])),
          '#options' => $handler_options,
          '#default_value' => isset($scheme->handlers[$object_type]) ? get_class($scheme->handlers[$object_type]) : '',
          '#empty_option' => t('- No handler selected -'),
          '#ajax' => array(
            'callback' => 'access_handler_form_ajax',
            'wrapper' => $wrapper_id,
            'effect' => 'fade',
          ),
        );

        // Container for handler settings.
        $fieldset['settings'] = array(
          '#type' => 'container',
          '#prefix' => '<div id="' . $wrapper_id . '">',
          '#suffix' => '</div>',
        );

        // Try to determine which handler, if any, is currently selected above.
        $handler = NULL;
        if (isset($form_state['values']['handlers'][$object_type])) {
          $handler_values = $form_state['values']['handlers'][$object_type];

          // The handler selection may be empty if the #empty_option was chosen.
          if (!empty($handler_values['handler'])) {
            $class = $handler_values['handler'];

            // See if the handler is the same as the one saved on the scheme.
            if (isset($scheme->handlers[$object_type]) && get_class($scheme->handlers[$object_type]) == $class) {
              // If so, keep the same handler object.
              $handler = $scheme->handlers[$object_type];
            }
            // Otherwise, make sure the handler class exists and is registered.
            elseif (class_exists($class) && isset($handler_info[$class])) {
              // Instantiate a new handler object.
              $settings = isset($handler_values['settings']) ? $handler_values['settings'] : array();
              $handler = new $class($scheme, $settings);
            }
          }
        }
        // If there was nothing in the form state, fall back to the saved value.
        elseif (isset($scheme->handlers[$object_type])) {
          $handler = $scheme->handlers[$object_type];
        }

        // If we found the selected handler, add its description and settings.
        if (isset($handler)) {
          $fieldset['settings'] += $handler->settingsForm();
          if (!isset($fieldset['settings']['description'])) {
            $fieldset['settings']['description'] = array(
              '#markup' => $handler->description(),
            );
          }
        }
      }

      $form['handlers'][$object_type] = $fieldset;
    }
  }
  // If this is an existing scheme and no access-controllable object types were
  // found above, display an explanation for why there are no handlers showing.
  elseif (!$is_new) {
    $form['handlers_empty'] = array(
      '#markup' => t('No access-controllable objects available. In order to control access to content, menu links, or other Drupal objects, you must first <a href="@link">enable one or more modules</a> that provide Access Control Kit compatibility for those objects, such as the ACK node module.', array('@link' => url('admin/modules'))),
    );
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

  // @todo Validate attached handlers using similar logic as above for grabbing handlers from the form state.
}

/**
 * Form submission handler for access_scheme_form().
 *
 * @see access_scheme_form()
 * @see access_scheme_form_validate()
 */
function access_scheme_form_submit($form, &$form_state) {
  $scheme = $form_state['scheme'];
  form_state_values_clean($form_state);
  entity_form_submit_build_entity('access_scheme', $scheme, $form, $form_state);
  unset($scheme->handler_tabs__active_tab);

  // Format the selected roles as rid => role_name.
  $roles = user_roles(TRUE);
  unset($roles[DRUPAL_AUTHENTICATED_RID]);
  $scheme->roles = array_filter($scheme->roles);
  $scheme->roles = array_intersect_key($roles,  $scheme->roles);

  // Save the scheme.
  $status = access_scheme_save($scheme);

  // @todo Should role and attached handler saves be moved to the entity controller's save()? Or to access_scheme_save()?

  // Save the roles.
  variable_set('access_scheme_roles_' . $scheme->machine_name, $scheme->roles);

  // Save the attached handlers.
  if (isset($scheme->handlers)) {
    $handler_info = access_handler_info();
    foreach ($scheme->handlers as $object_type => $handler_values) {
      // Check whether the handler element was set to the #empty_option.
      if (empty($handler_values['handler'])) {
        // If there was a handler chosen before, we need to delete it.
        if (!empty($scheme->original) && !empty($scheme->original->handlers) && !empty($scheme->original->handlers[$object_type])) {
          access_handler_delete($scheme, $object_type);
        }
      }
      else {
        // Save the handler.
        $class = $handler_values['handler'];
        $module = $handler_info[$class]['module'];
        $settings = isset($handler_values['settings']) ? $handler_values['settings'] : array();
        access_handler_save($scheme, $object_type, $class, $module, $settings);
      }
    }
  }

  // Report the change.
  $t_args = array(
    '%name' => $scheme->name,
  );
  switch ($status) {
    case SAVED_NEW:
      $message = 'Added access scheme %name.';
      // New schemes redirect to the edit page to configure handlers.
      $form_state['redirect'] = 'admin/structure/access/' . str_replace('_', '-', $scheme->machine_name);
      break;

    case SAVED_UPDATED:
      $message = 'Updated access scheme %name.';
      // Updated schemes redirect to the admin overview.
      $form_state['redirect'] = 'admin/structure/access';
      break;
  }
  drupal_set_message(t($message, $t_args));
  watchdog('access', $message, $t_args, WATCHDOG_NOTICE, l(t('edit'), 'admin/structure/access/' . str_replace('_', '-', $scheme->machine_name)));

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
  $message = 'Deleted access scheme %scheme.';
  drupal_set_message(t($message, $t_args));
  watchdog('access', $message, $t_args, WATCHDOG_NOTICE);

  $form_state['redirect'] = 'admin/structure/access';

  // Rebuild the menu so that the "add access grant" page is up to date.
  menu_rebuild();
}
