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
 * Informs the access control kit module about one or more object types that
 * can be managed through access grants.
 *
 * Modules can implement this hook to integrate various types of Drupal objects
 * (such as nodes, menu links, etc.) with the access control kit module's access
 * scheme/grants system.  It is up to the module to handle the actual mechanics
 * of that integration; implementing this hook simply notifies ACK that the
 * object types are available.  For example, the ACK node module implements this
 * hook to notify ACK that it is making nodes available for access control.  It
 * then implements other hooks (such as hook_access_handler_info(),
 * hook_permission(), and hook_node_access()) to provide the integration.
 *
 * @return
 *   An array whose keys are access-controllable object type names and whose
 *   values declare the properties of those types that are need by the access
 *   control kit module.  If the object type is a Drupal entity, the object type
 *   name should be same as the entity type name that was used as the key for
 *   its definition in hook_entity_info().  For example, if the module provides
 *   access control kit integration for nodes, then the key should be 'node'.
 *
 *   The properties of the object type are declared in an array as follows:
 *   - label: The human-readable name of the object type.
 *
 * @see access_info()
 * @see hook_access_info_alter()
 */
function hook_access_info() {
// @todo Should we also have a key to explicitly define an object type as an entity, rather than just relying on keys matching hook_entity_info()?
// @todo Add the 'module' property as an optional key, so that module implementing the alter hook can override it?
  // Declare nodes as access-controllable objects.
  $info['node'] = array('label' => t('Content'));
  return $info;
}

/**
 * Alters the access-controllable object type info.
 *
 * Modules may implement this hook to alter the information that defines the
 * types of objects that can be managed by access grants.  All properties that
 * are available in hook_access_info() can be altered here.
 *
 * @param $info
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
 * @return
 *   An array whose keys are object access handler class names and whose values
 *   declare the properties of the handler that are needed by access control kit
 *   to attach the handler to an access scheme.  The registered classes must
 *   implement the AccessControlKitHandlerInterface.
 *
 *   The properties of the handler are declared in an array as follows:
 *   - label: The human-readable name of the handler.
 *   - realm types: An array listing the access realm types that the handler
 *     supports, as defined by hook_access_realm_info().
 *   - object types: An array listing the access-controllable object types that
 *     the handler supports, as defined by hook_access_info().  A value of
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
    'realm types' => array('boolean'),
    'object types' => array('node'),
  );
  return $info;
}

/**
 * Alters the access handler info.
 *
 * Modules may implement this hook to alter the information that registers
 * object access handlers for use with access schemes.  All properties that are
 * available in hook_access_handler_info() can be altered here.
 *
 * @param $info
 *   The access handler info, keyed by object access handler class name.
 *
 * @see hook_access_handler_info()
 */
function hook_access_handler_info_alter(&$info) {
  // Change the label used for the node "sticky" property handler.
  $info['ACKNodeSticky']['label'] = t('Node is marked sticky');
}

/**
 * Informs the access control kit module about one or more realm types that
 * can be used to define access schemes.
 *
 * Modules can implement this hook to make various types of Drupal data
 * available to the access control kit module as the basis for defining realms
 * in an access scheme.  For example, ACK itself implements this hook to allow
 * access schemes to be based on taxonomy vocabularies or the allowed values of
 * list fields.
 *
 * Modules that implement this hook should also implement hook_access_realms(),
 * and may also implement hook_access_realm_settings().
 *
 * @return
 *   An array whose keys are realm type names and whose values declare the
 *   properties of those types that are needed by the access control kit module:
 *   - label: The human-readable name of the realm type.
 *   - field_type: The type of field that is used to store realm values.  Valid
 *     field types are access_boolean, access_integer, access_float, and
 *     access_text.
 *   - description: (optional) A translated string describing the realm type.
 *   - arguments: (optional) An array defining additional settings needed to
 *     configure the realm type.  Keys are the names of the settings variables
 *     and values are the default, unconfigured values for those variables.
 *     Module should implement hook_access_realm_settings() to provide a form
 *     for configuring these settings.
 *
 * @see access_realm_info()
 * @see hook_access_realm_info_alter()
 * @see hook_access_realm_settings()
 * @see hook_access_realms()
 */
function hook_access_realm_info() {
// @todo Is the arguments parameter even needed, or can we just take the submitted values of the settings form?
// @todo Replace hook_access_realm_settings() and hook_access_realms() with callback parameters?
  // Allow taxonomy vocabularies to be used as realm lists for access schemes.
  // Note that the field_type is an integer because the primary identifier for a
  // taxonomy term is its tid.
  $info['taxonomy_term'] = array(
    'label' => t('Taxonomy'),
    'field_type' => 'access_integer',
    'arguments' => array('vocabulary' => ''),
    'description' => t('A <em>taxonomy</em> scheme controls access based on the terms of a selected vocabulary.'),
  );
  return $info;
}

/**
 * Alters the access realm type info.
 *
 * Modules may implement this hook to alter the information that defines the
 * types of data that can form the basis for an access scheme.  All properties
 * that are available in hook_access_realm_info() can be altered here.
 *
 * @param $info
 *   The access realm type info, keyed by realm type name.
 *
 * @see hook_access_realm_info()
 */
function hook_access_realm_info_alter(&$info) {
  // Change the label used for taxonomy-based schemes.
  $info['taxonomy_term']['label'] = t('Tags');
}

/**
 * Provides the form elements needed to configure the realm list for a scheme.
 *
 * Modules that define an access realm type that requires additional settings
 * (as defined by the 'arguments' property in hook_access_realm_info()), should
 * implement this hook to provide the form elements needed to configure those
 * settings.
 *
 * @param $realm_type
 *   The name of the access realm type being configured.
 * @param $has_data
 *   Boolean indicating whether access grants already exist for the scheme that
 *   is using this realm type.
 * @param $values
 *   The current values of the realm type's arguments.
 *
 * @return
 *   An array containing the form elements that define the realm type's settings
 *   form.  The top-level keys should correspond to the realm type's 'arguments'
 *   array in hook_access_realm_info().
 *
 * @see hook_access_realm_info()
 */
function hook_access_realm_settings($realm_type, $has_data, $values = array()) {
  if ($realm_type == 'taxonomy_term') {
    $options = array();
    foreach (taxonomy_get_vocabularies() as $vocabulary) {
      $options[$vocabulary->machine_name] = $vocabulary->name;
    }
    $form['vocabulary'] = array(
      '#type' => 'select',
      '#title' => t('Vocabulary'),
      '#default_value' => isset($values['vocabulary']) ? $values['vocabulary'] : NULL,
      '#options' => $options,
      '#required' => TRUE,
      '#disabled' => $has_data,
    );
    return $form;
  }
}

/**
 * Returns the list of realms in an access scheme.
 *
 * Modules that implement hook_access_realm_info() must also implement this hook
 * to provide the list of realms for access schemes based on a given realm type.
 *
 * @param $realm_type
 *   The name of the access realm type being configured.
 * @param $arguments
 *   The configured realm settings, as defined by the realm type's 'arguments'
 *   property in hook_access_realm_info(), if such exists.
 *
 * @return
 *   An array listing the currently available access realms, where the keys are
 *   the realm values to be stored and the values are the human-readable names
 *   of the access realms.  This array will be used as the '#options' property
 *   for the form element that assigns realms to an access grant.
 *
 * @see hook_access_realm_info()
 */
function hook_access_realms($realm_type, $arguments = array()) {
  if ($realm_type == 'taxonomy_term') {
    // Re-use the allowed values function for term reference fields.
    $field = array();
    $field['settings']['allowed_values'][] = array('vocabulary' => $arguments['vocabulary'], 'parent' => 0);
    return taxonomy_allowed_values($field);
  }
}

/**
 * Acts on an access scheme that is about to be inserted or updated.
 *
 * This hook is invoked from AccessSchemeEntityController::save() before the
 * scheme is saved to the database.
 *
 * @param $scheme
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
 * the database because a database transaction is still in progress.  The
 * transaction is not finalized until the save operation is entirely completed
 * and the save() method goes out of scope.  You should not rely on data in the
 * database at this time, as it has not been updated yet.  You should also note
 * that any write/update database queries executed from this hook are also not
 * committed immediately.  Check AccessSchemeEntityController::save() and
 * db_transaction() for more info.
 *
 * @param $scheme
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
 * the database because a database transaction is still in progress.  The
 * transaction is not finalized until the save operation is entirely completed
 * and the save() method goes out of scope.  You should not rely on data in the
 * database at this time, as it has not been updated yet.  You should also note
 * that any write/update database queries executed from this hook are also not
 * committed immediately.  Check AccessSchemeEntityController::save() and
 * db_transaction() for more info.
 *
 * @param $scheme
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
 * @param $scheme
 *   The access scheme that is being deleted.
 */
function hook_access_scheme_delete($scheme) {
  // Remove the scheme's audit flag.
  variable_del('access_audit_' . $scheme->machine_name);
}

/**
 * Acts on an access grant that is about to be inserted or updated.
 *
 * This hook is invoked from AccessGrantEntityController::save() before the
 * grant is saved to the database.
 *
 * @param $grant
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
 * the database because a database transaction is still in progress.  The
 * transaction is not finalized until the save operation is entirely completed
 * and the save() method goes out of scope.  You should not rely on data in the
 * database at this time, as it has not been updated yet.  You should also note
 * that any write/update database queries executed from this hook are also not
 * committed immediately.  Check AccessGrantEntityController::save() and
 * db_transaction() for more info.
 *
 * @param $grant
 *   The access grant that is being created.
 */
function hook_access_grant_insert($grant) {
  // Notify the user whenever an access grant is added for user 1.
  if ($grant->uid == 1) {
    drupal_set_message('An access grant has been created for the site administrator.');
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
 * the database because a database transaction is still in progress.  The
 * transaction is not finalized until the save operation is entirely completed
 * and the save() method goes out of scope.  You should not rely on data in the
 * database at this time, as it has not been updated yet.  You should also note
 * that any write/update database queries executed from this hook are also not
 * committed immediately.  Check AccessGrantEntityController::save() and
 * db_transaction() for more info.
 *
 * @param $grant
 *   The access grant that is being updated.
 */
function hook_access_grant_update($grant) {
  // Notify the user whenever an access grant changes for user 1.
  if ($grant->uid == 1) {
    drupal_set_message('An access grant has changed for the site administrator.');
  }
}

/**
 * Responds to access grant deletion.
 *
 * This hook is invoked from AccessGrantEntityController::delete() before
 * hook_entity_delete() and field_attach_delete() are called, and before the
 * grant is removed from the access_grant table in the database.
 *
 * @param $grant
 *   The access grant that is being deleted.
 */
function hook_access_grant_delete($grant) {
  // Notify the user whenever an access grant is removed for user 1.
  if ($grant->uid == 1) {
    drupal_set_message('An access grant was deleted for the site administrator.');
  }
}

/**
 * Acts on an access grant that is being assembled before rendering.
 *
 * The module may add elements to $grant->content prior to rendering.  The
 * structure of $grant->content is a renderable array as expected by
 * drupal_render().
 *
 * @param $grant
 *   The access grant that is being assembled for rendering.
 * @param $view_mode
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
 * Alternatively, it could also implement hook_preprocess_access_grant().  See
 * drupal_render() and theme() documentation respectively for details.
 *
 * @param $build
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
