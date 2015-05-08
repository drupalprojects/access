<?php

/**
 * @file
 * Hooks provided by the access control kit module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Declares manageable object types for access control kit.
 *
 * Modules can implement this hook to integrate various types of Drupal objects
 * (such as nodes, menu links, etc.) with the access control kit module's access
 * scheme/grants system. It is up to the module to handle the actual mechanics
 * of that integration; implementing this hook simply notifies ACK that the
 * object types are available. For example, the ACK node module implements this
 * hook to notify ACK that it is making nodes available for access control. It
 * then implements other hooks (such as hook_access_handler_info(),
 * hook_permission() and hook_node_access()) to provide the integration.
 *
 * @return array
 *   An array whose keys are access-controllable object type names and whose
 *   values declare the properties of those types that are need by the access
 *   control kit module. If the object type is a Drupal entity, the object type
 *   name should be the same as the entity type name that was used as the key
 *   for its definition in hook_entity_info(). For example, if the module
 *   provides ACK integration for nodes, the key should be 'node'.
 *
 *   The properties of the object type are declared in an array as follows:
 *   - label: The human-readable name of the object type.
 *
 * @see access_info()
 * @see hook_access_info_alter()
 */
function hook_access_info() {
  // Declare nodes as access-controllable objects.
  $info['node'] = array('label' => t('Content'));
  return $info;
}

/**
 * Alters the access-controllable object type info.
 *
 * Modules may implement this hook to alter the information that defines the
 * types of objects that can be managed by access grants. All properties that
 * are available in hook_access_info() can be altered here.
 *
 * @param array $info
 *   The access-controllable object type info, keyed by object type name.
 *
 * @see hook_access_info()
 */
function hook_access_info_alter(&$info) {
  // Change the label used for nodes.
  $info['node']['label'] = t('Nodes');
}

/**
 * Registers object access handler classes with the access control kit module.
 *
 * @return array
 *   An array whose keys are object access handler class names and whose values
 *   declare the properties of the handler that are needed by access control kit
 *   to attach the handler to an access scheme. The registered classes must
 *   implement the AccessControlKitHandlerInterface.
 *
 *   The properties of the handler are declared in an array as follows:
 *   - label: The human-readable name of the handler.
 *   - scheme types: An array listing the access scheme types that the handler
 *     supports, as defined by hook_access_scheme_info().
 *   - object types: An array listing the access-controllable object types that
 *     the handler supports, as defined by hook_access_info(). A value of
 *     'fieldable entity' indicates that the handler supports all object types
 *     that are fieldable entities, as defined by hook_entity_info().
 *
 * @see access_handler_info()
 * @see hook_access_handler_info_alter()
 */
function hook_access_handler_info() {
  // Register the handler for the node "sticky" property.
  $info['ACKNodeSticky'] = array(
    'label' => t('Sticky'),
    'scheme types' => array('boolean'),
    'object types' => array('node'),
  );
  return $info;
}

/**
 * Alters the access handler info.
 *
 * Modules may implement this hook to alter the information that registers
 * object access handlers for use with access schemes. All properties that are
 * available in hook_access_handler_info() can be altered here.
 *
 * @param array $info
 *   The access handler info, keyed by object access handler class name.
 *
 * @see hook_access_handler_info()
 */
function hook_access_handler_info_alter(&$info) {
  // Change the label used for the node "sticky" property handler.
  $info['ACKNodeSticky']['label'] = t('Node is marked sticky');
}

/**
 * Defines access scheme types.
 *
 * Modules can implement this hook to make various types of Drupal data
 * available to the access control kit module as the basis for defining realms
 * in an access scheme. For example, ACK itself implements this hook to allow
 * access schemes to be based on taxonomy vocabularies or the allowed values of
 * list fields.
 *
 * @section sec_callback_functions Callback functions
 * The definition for each scheme type must include a realms callback function,
 * which is invoked to find the list of realms in the scheme. The definition may
 * also include a settings callback function, if the scheme type requires
 * additional configuration. For example, in the taxonomy_term scheme type, the
 * settings callback provides a form element for selecting the vocabulary that
 * will define the scheme, and the realms callback returns a list of terms in
 * the selected vocabulary.
 *
 * @subsection sub_callback_realms Realms callbacks
 * Realms callbacks receive the scheme object as a parameter and return an array
 * listing the currently available access realms in the scheme. The returned
 * array will be used as the '#options' property of the realm selector on the
 * access grant form; thus, the keys of this array are the realm values to be
 * stored, and its values are the human-readable names of the access realms. For
 * example, ACK defines taxonomy terms as a scheme type like so:
 * @code
 *   function access_access_scheme_info() {
 *     $info['taxonomy_term'] = array(
 *       'realms callback' => 'access_scheme_taxonomy_term_realms',
 *       // ...
 *     );
 *     return $items;
 *   }
 *
 *   function access_scheme_taxonomy_term_realms($scheme) {
 *     // Re-use the allowed values function for term reference fields.
 *     $field = array();
 *     $field['settings']['allowed_values'][] = array(
 *       'vocabulary' => $scheme->settings['vocabulary'],
 *       'parent' => 0,
 *     );
 *     return taxonomy_allowed_values($field);
 *   }
 * @endcode
 *
 * @subsection sub_callback_settings Settings callbacks
 * Settings callbacks receive as parameters the scheme object and a boolean
 * indicating whether access grants already exist for the scheme. A callback
 * should return an array containing the form elements needed to configure the
 * scheme. These elements will be displayed on the scheme add/edit form, and the
 * submitted values will be stored in $scheme->settings. For example, ACK
 * configures the taxonomy term scheme type like so:
 * @code
 *   function access_access_scheme_info() {
 *     $info['taxonomy_term'] = array(
 *       'settings callback' => 'access_scheme_taxonomy_term_settings',
 *       // ...
 *     );
 *     return $items;
 *   }
 *
 *   function access_scheme_taxonomy_term_settings($scheme, $has_data) {
 *     $options = array();
 *     foreach (taxonomy_get_vocabularies() as $vocabulary) {
 *       $options[$vocabulary->machine_name] = $vocabulary->name;
 *     }
 *     $settings = $scheme->settings;
 *     $value = isset($settings['vocabulary']) ? $settings['vocabulary'] : NULL;
 *     $form['vocabulary'] = array(
 *       '#type' => 'select',
 *       '#title' => t('Vocabulary'),
 *       '#default_value' => $value,
 *       '#options' => $options,
 *       '#required' => TRUE,
 *       '#disabled' => $has_data,
 *     );
 *     return $form;
 *   }
 * @endcode
 *
 * @return array
 *   An array of scheme types. Each type has a key that defines the type's
 *   machine-readable name. The corresponding array value is an associative
 *   array that contains the following key-value pairs:
 *   - label: The human-readable name of the scheme type.
 *   - data_type: The data type of the realm values. Valid data types are
 *     'boolean', 'integer', 'float' and 'text'.
 *   - description: (optional) A translated string describing the scheme type.
 *   - realms callback: The name of the function that provides the realm list.
 *   - settings callback: (optional) The name of the function that provides the
 *     scheme type's settings form.
 *   - file: (optional) A file that will be included before the callbacks are
 *     executed; this allows the callback functions to be in a separate file.
 *     The file should be relative to the implementing module's directory unless
 *     otherwise specified by the "file path" option.
 *   - file path: (optional) The path to the directory containing the file
 *     specified in "file". This defaults to the path to the module implementing
 *     the hook.
 *
 * @see access_scheme_info()
 * @see hook_access_scheme_info_alter()
 */
function hook_access_scheme_info() {
  // Allow taxonomy vocabularies to be used as realm lists for access schemes.
  // Note that the data_type is an integer because the primary identifier for a
  // taxonomy term is its tid.
  $info['taxonomy_term'] = array(
    'label' => t('Taxonomy'),
    'data_type' => 'integer',
    'description' => t('A <em>taxonomy</em> scheme controls access based on the terms of a selected vocabulary.'),
    'realms callback' => 'access_scheme_taxonomy_term_realms',
    'settings callback' => 'access_scheme_taxonomy_term_settings',
    'file' => 'callbacks/access.taxonomy.inc',
  );
  return $info;
}

/**
 * Alters the access scheme type info.
 *
 * Modules may implement this hook to alter the information that defines the
 * types of data that can form the basis for an access scheme. All properties
 * that are available in hook_access_scheme_info() can be altered here.
 *
 * @param array $info
 *   The access scheme type info, keyed by scheme type name.
 *
 * @see hook_access_scheme_info()
 */
function hook_access_scheme_info_alter(&$info) {
  // Change the label used for taxonomy-based schemes.
  $info['taxonomy_term']['label'] = t('Tags');
}

/**
 * Acts on an access scheme that is about to be inserted or updated.
 *
 * This hook is invoked from AccessSchemeEntityController::save() before the
 * scheme is saved to the database.
 *
 * @param object $scheme
 *   The access scheme that is being inserted or updated.
 */
function hook_access_scheme_presave($scheme) {
  // Set a timestamp to log when the scheme was last modified.
  $scheme->modified = time();
}

/**
 * Responds to the creation of a new access scheme.
 *
 * This hook is invoked from AccessSchemeEntityController::save() after the
 * database query that will insert the scheme into the access_scheme table is
 * scheduled for execution.
 *
 * Note that when this hook is invoked, the changes have not yet been written to
 * the database because a database transaction is still in progress. The
 * transaction is not finalized until the save operation is entirely completed
 * and the save() method goes out of scope. You should not rely on data in the
 * database at this time, as it has not been updated yet. You should also note
 * that any write/update database queries executed from this hook are also not
 * committed immediately. Check AccessSchemeEntityController::save() and
 * db_transaction() for more info.
 *
 * @param object $scheme
 *   The access scheme that is being created.
 */
function hook_access_scheme_insert($scheme) {
  // Create a variable to indicate that access grants should be audited whenever
  // the scheme configuration changes.
  variable_set('access_audit_' . $scheme->machine_name, FALSE);
}

/**
 * Responds to updates to an access scheme.
 *
 * This hook is invoked from AccessSchemeEntityController::save() after the
 * database query that will update the scheme in the access_scheme table is
 * scheduled for execution.
 *
 * Note that when this hook is invoked, the changes have not yet been written to
 * the database because a database transaction is still in progress. The
 * transaction is not finalized until the save operation is entirely completed
 * and the save() method goes out of scope. You should not rely on data in the
 * database at this time, as it has not been updated yet. You should also note
 * that any write/update database queries executed from this hook are also not
 * committed immediately. Check AccessSchemeEntityController::save() and
 * db_transaction() for more info.
 *
 * @param object $scheme
 *   The access scheme that is being updated.
 */
function hook_access_scheme_update($scheme) {
  // Indicate that access grants for this scheme should be audited.
  variable_set('access_audit_' . $scheme->machine_name, TRUE);
}

/**
 * Responds to access scheme deletion.
 *
 * This hook is invoked from AccessSchemeEntityController::delete() after the
 * database query that will delete the scheme from the access_scheme table is
 * scheduled for execution, but before the transaction actually completes and
 * the scheme is removed from the database.
 *
 * @param object $scheme
 *   The access scheme that is being deleted.
 */
function hook_access_scheme_delete($scheme) {
  // Remove the scheme's audit flag.
  variable_del('access_audit_' . $scheme->machine_name);
}

/**
 * Alters the realm fields' views table data.
 *
 * This hook is invoked from access_views_data_alter() for each access scheme.
 *
 * @param array &$data
 *   The array of Views table data for a scheme's realm field table. This is in
 *   the same format as a table in the return value of hook_views_data().
 * @param string $field_value_name
 *   The name of the table column that contains the realm field value.
 * @param string $field_name
 *   The name of the realm field.
 * @param string $scheme_type
 *   The access scheme type (e.g., 'list_integer').
 *
 * @see access_views_data_alter()
 */
function hook_access_scheme_views_data_alter(&$data, $field_value_name, $field_name, $scheme_type) {
  // Add a relationship to the referenced taxonomy term.
  if ($scheme_type == 'taxonomy_term') {
    $data[$field_value_name]['relationship'] = array(
      'handler' => 'views_handler_relationship',
      'base' => 'taxonomy_term_data',
      'base field' => 'tid',
      'label' => t('term from !field_name', array('!field_name' => $field_name)),
    );
  }
}

/**
 * Alters the realm field's views table data for a specific scheme type.
 *
 * Note that, due to the limitations of hook_hook_info() regarding dynamically
 * named hooks, this hook (if used) must be declared in the .module file, not
 * in MODULE.access.inc.
 *
 * @param array &$data
 *   The array of Views table data for a scheme's realm field table. This is in
 *   the same format as a table in the return value of hook_views_data().
 * @param string $field_value_name
 *   The name of the table column that contains the realm field value.
 * @param string $field_name
 *   The name of the realm field.
 * @param string $scheme_type
 *   The access scheme type (e.g., 'list_integer').
 *
 * @see hook_access_scheme_views_data_alter()
 * @see access_views_data_alter()
 */
function hook_access_scheme_SCHEME_TYPE_views_data_alter(&$data, $field_value_name, $field_name, $scheme_type) {
  // hook_access_scheme_taxonomy_term_views_data_alter():
  // Add a relationship to the referenced taxonomy term.
  $data[$field_value_name]['relationship'] = array(
    'handler' => 'views_handler_relationship',
    'base' => 'taxonomy_term_data',
    'base field' => 'tid',
    'label' => t('term from !field_name', array('!field_name' => $field_name)),
  );
}

/**
 * Acts on an access grant that is about to be inserted or updated.
 *
 * This hook is invoked from AccessGrantEntityController::save() before the
 * grant is saved to the database.
 *
 * @param object $grant
 *   The access grant that is being inserted or updated.
 */
function hook_access_grant_presave($grant) {
  // Set a timestamp to log when the grant was last modified.
  $grant->modified = time();
}

/**
 * Responds to the creation of a new access grant.
 *
 * This hook is invoked from AccessGrantEntityController::save() after the
 * database query that will insert the grant into the access_grant table is
 * scheduled for execution and field_attach_insert() is called.
 *
 * Note that when this hook is invoked, the changes have not yet been written to
 * the database because a database transaction is still in progress. The
 * transaction is not finalized until the save operation is entirely completed
 * and the save() method goes out of scope. You should not rely on data in the
 * database at this time, as it has not been updated yet. You should also note
 * that any write/update database queries executed from this hook are also not
 * committed immediately. Check AccessGrantEntityController::save() and
 * db_transaction() for more info.
 *
 * @param object $grant
 *   The access grant that is being created.
 */
function hook_access_grant_insert($grant) {
  // Notify the user whenever an access grant is added for user 1.
  if ($grant->uid == 1) {
    drupal_set_message(t('An access grant has been created for the site administrator.'));
  }
}

/**
 * Responds to updates to an access grant.
 *
 * This hook is invoked from AccessGrantEntityController::save() after the
 * database query that will update the grant in the access_grant table is
 * scheduled for execution and field_attach_update() is called.
 *
 * Note that when this hook is invoked, the changes have not yet been written to
 * the database because a database transaction is still in progress. The
 * transaction is not finalized until the save operation is entirely completed
 * and the save() method goes out of scope. You should not rely on data in the
 * database at this time, as it has not been updated yet. You should also note
 * that any write/update database queries executed from this hook are also not
 * committed immediately. Check AccessGrantEntityController::save() and
 * db_transaction() for more info.
 *
 * @param object $grant
 *   The access grant that is being updated.
 */
function hook_access_grant_update($grant) {
  // Notify the user whenever an access grant changes for user 1.
  if ($grant->uid == 1) {
    drupal_set_message(t('An access grant has changed for the site administrator.'));
  }
}

/**
 * Responds to access grant deletion.
 *
 * This hook is invoked from AccessGrantEntityController::delete() before
 * hook_entity_delete() and field_attach_delete() are called, and before the
 * grant is removed from the access_grant table in the database.
 *
 * @param object $grant
 *   The access grant that is being deleted.
 */
function hook_access_grant_delete($grant) {
  // Notify the user whenever an access grant is removed for user 1.
  if ($grant->uid == 1) {
    drupal_set_message(t('An access grant was deleted for the site administrator.'));
  }
}

/**
 * Acts on an access grant that is being assembled before rendering.
 *
 * The module may add elements to $grant->content prior to rendering. The
 * structure of $grant->content is a renderable array as expected by
 * drupal_render().
 *
 * @param object $grant
 *   The access grant that is being assembled for rendering.
 * @param string $view_mode
 *   The $view_mode parameter from access_grant_view().
 *
 * @see hook_entity_view()
 */
function hook_access_grant_view($grant, $view_mode) {
  // Display the user's email address on the grant page.
  if ($view_mode == 'full') {
    $account = user_load($grant->uid);
    $grant->content['email'] = array(
      '#markup' => t('Email address: @email', array('@email' => $account->mail)),
    );
  }
}

/**
 * Alters the results of access_grant_view().
 *
 * This hook is called after the grant has been assembled in a structured array
 * and may be used for doing processing which requires that the complete grant
 * content structure be built first.
 *
 * If the module wishes to act on the rendered HTML of the grant rather than the
 * structured array, it may use this hook to add a #post_render callback.
 * Alternatively, it could also implement hook_preprocess_access_grant(). See
 * drupal_render() and theme() documentation respectively for details.
 *
 * @param array $build
 *   A renderable array representing the access grant.
 *
 * @see access_grant_view()
 * @see hook_entity_view_alter()
 */
function hook_access_grant_view_alter(&$build) {
  // Move the email field to the top.
  if ($build['#view_mode'] == 'full' && isset($build['email'])) {
    $build['email']['#weight'] = -10;
  }

  // Add a #post_render callback to act on the rendered HTML of the grant.
  $build['#post_render'][] = 'my_module_access_grant_post_render';
}

/**
 * @} End of "addtogroup hooks".
 */
