<?php

/**
 * @file
 * Contains the access scheme entity controller.
 */

class AccessSchemeEntityController extends DrupalDefaultEntityController {

  /**
   * Constructs a new scheme object without saving it.
   *
   * @param array $values
   *   (optional) An array of values to set, keyed by property name.
   *
   * @return
   *   An access scheme object.
   */
  public function create(array $values = array()) {
    $scheme = new stdClass();
    $scheme->sid = NULL;
    $scheme->machine_name = empty($values['machine_name']) ? '' : $values['machine_name'];
    $scheme->name = empty($values['name']) ? '' : $values['name'];
    $scheme->realm_type = empty($values['realm_type']) ? '' : $values['realm_type'];
    $scheme->description = empty($values['description']) ? '' : $values['description'];
    return $scheme;
  }

  /**
   * Overrides DrupalDefaultEntityController::attachLoad().
   */
  protected function attachLoad(&$schemes, $revision_id = FALSE) {
    // Get all roles, excluding "anonymous users" and "authenticated users."
    $roles = user_roles(TRUE);
    unset($roles[DRUPAL_AUTHENTICATED_RID]);

    foreach ($schemes as $scheme) {
      // Attach realm information.
      $scheme->realm_field = field_info_field(_access_realm_field_name($scheme));
      $scheme->realm_info = access_realm_info($scheme->realm_type);
      $scheme->realms = module_invoke($scheme->realm_info['module'], 'access_realms', $scheme->realm_type, $scheme->realm_field['settings']['arguments']);

      // Attach role information.
      $scheme_roles = variable_get('access_scheme_roles_' . $scheme->machine_name, array());
      $scheme->roles = array_intersect_assoc($roles, $scheme_roles);

      // Attach the scheme's object access handlers.
      $scheme->handlers = array();
      $result = db_query('SELECT * FROM {access_handler} WHERE scheme = :scheme', array(':scheme' => $scheme->machine_name));
      foreach ($result as $row) {
        // Initialize the handler using the class and settings from the database.
        $class = $row->handler;
        $settings = unserialize($row->settings);
        $scheme->handlers[$row->object_type] = new $class($scheme, $settings);
      }
    }
    parent::attachLoad($schemes, $revision_id);
  }

  /**
   * Saves an access scheme to the database.
   *
   * @param $scheme
   *   A scheme object with the following properties:
   *   - sid: (optional) The unique ID for the scheme being saved. If
   *     $scheme->sid is empty or omitted, a new scheme will be inserted.
   *   - machine_name: The unique, machine-readable name of the scheme.
   *   - name: The human-readable name of the scheme.
   *   - realm_type: The type of object or value that defines the access realms.
   *   - description: A description of the access scheme.
   *   - original: (optional) The original scheme object before any changes were
   *     applied. When omitted, the unchanged scheme object is loaded from the
   *     database and stored in the property.
   *
   * @return
   *   Status constant indicating whether the scheme was inserted (SAVED_NEW) or
   *   updated (SAVED_UPDATED). When inserting a new scheme, $scheme->sid will
   *   contain the ID of the newly created scheme.
   */
  public function save($scheme) {
    $transaction = db_transaction();
    try {
      // Prevent leading and trailing spaces in scheme names.
      if (!empty($scheme->name)) {
        $scheme->name = trim($scheme->name);
      }

      // Load the stored scheme, if any.
      if (!empty($scheme->sid) && !isset($scheme->original)) {
        $scheme->original = entity_load_unchanged('access_scheme', $scheme->sid);
      }

      // The machine name and realm type are not editable after creation.
      // @todo Allow machine name to change by changing grants (and handlers?) to reference sid instead of machine name?  Doing so might also make the static cache in _machine_name_load() unnecessary.
      if (!empty($scheme->original)) {
        $scheme->machine_name = $scheme->original->machine_name;
        $scheme->realm_type = $scheme->original->realm_type;
      }

      module_invoke_all('access_scheme_presave', $scheme);
      module_invoke_all('entity_presave', $scheme, 'access_scheme');

      if (empty($scheme->sid)) {
        $op = 'insert';
        $status = drupal_write_record('access_scheme', $scheme);
        field_attach_create_bundle('access_grant', $scheme->machine_name);
      }
      else {
        $op = 'update';
        $status = drupal_write_record('access_scheme', $scheme, 'sid');
      }

      module_invoke_all('access_scheme_' . $op, $scheme);
      module_invoke_all('entity_' . $op, $scheme, 'access_scheme');
      unset($scheme->original);
      cache_clear_all();

      return $status;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('access', $e);
      throw $e;
    }
  }

  /**
   * Deletes an access scheme from the database.
   *
   * @param $sid
   *   The scheme ID.
   *
   * @return
   *   Status constant indicating deletion.
   */
  public function delete($sid) {
    $transaction = db_transaction();
    try {
      if ($scheme = entity_load_unchanged('access_scheme', $sid)) {
        // Detach any access handlers for this scheme.
        db_delete('access_handler')
          ->condition('scheme', $scheme->machine_name)
          ->execute();

        // Delete the access scheme definition.
        db_delete('access_scheme')
          ->condition('sid', $sid)
          ->execute();

        // Delete the scheme's role configuration.
        variable_del('access_scheme_roles_' . $scheme->machine_name);

        // Notify the Field API and other interested modules of the deletion.
        field_attach_delete_bundle('access_grant', $scheme->machine_name);
        module_invoke_all('access_scheme_delete', $scheme);
        module_invoke_all('entity_delete', $scheme, 'access_scheme');
      }
      return SAVED_DELETED;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('access', $e);
      throw $e;
    }
  }
}