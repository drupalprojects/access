<?php

/**
 * @file
 * Contains the access scheme entity controller.
 */

/**
 * Provides the entity controller for access schemes.
 */
class AccessSchemeEntityController extends DrupalDefaultEntityController {

  /**
   * Constructs a new scheme object without saving it.
   *
   * @param array $values
   *   (optional) An array of values to set, keyed by property name.
   *
   * @return object
   *   An access scheme object.
   */
  public function create(array $values = array()) {
    $scheme = new stdClass();
    $scheme->sid = NULL;
    $scheme->machine_name = empty($values['machine_name']) ? '' : $values['machine_name'];
    $scheme->name = empty($values['name']) ? '' : $values['name'];
    $scheme->type = empty($values['type']) ? '' : $values['type'];
    $scheme->description = empty($values['description']) ? '' : $values['description'];
    $scheme->settings = array();
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
      // Make sure the settings property is an array.
      $scheme->settings = isset($scheme->settings) ? unserialize($scheme->settings) : array();

      // Attach scheme type definition.
      $scheme->info = access_scheme_info($scheme->type);

      // Attach realm information.
      $scheme->realm_field = field_info_field($scheme->realm_field_name);
      if (!empty($scheme->info['realms callback']) && function_exists($scheme->info['realms callback'])) {
        $scheme->realms = call_user_func_array($scheme->info['realms callback'], array($scheme));
      }
      else {
        $scheme->realms = array();
      }

      // Attach role information.
      $scheme_roles = variable_get('access_scheme_roles_' . $scheme->machine_name, array());
      $scheme->roles = array_intersect_assoc($roles, $scheme_roles);

      // Attach the scheme's object access handlers.
      $scheme->handlers = array();
      $result = db_query('SELECT * FROM {access_handler} WHERE scheme = :scheme', array(':scheme' => $scheme->machine_name));
      foreach ($result as $row) {
        // Initialize a handler using the class and settings from the database.
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
   * @param object $scheme
   *   A scheme object with the following properties:
   *   - sid: (optional) The unique ID for the scheme being saved. If
   *     $scheme->sid is empty or omitted, a new scheme will be inserted.
   *   - machine_name: The unique, machine-readable name of the scheme.
   *   - name: The human-readable name of the scheme.
   *   - type: The type of object or value that defines the scheme's realms.
   *   - description: A description of the access scheme.
   *   - realm_field_name: (optional) The name of the realm field.
   *   - settings: (optional) An array of additional scheme settings.
   *   - original: (optional) The original scheme object before any changes were
   *     applied. When omitted, the unchanged scheme object is loaded from the
   *     database and stored in the property.
   *
   * @return int
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

      // Make sure the settings property is valid.
      if (!isset($scheme->settings) || !is_array($scheme->settings)) {
        $scheme->settings = array();
      }

      // Load the stored scheme, if any.
      if (!empty($scheme->sid) && !isset($scheme->original)) {
        $scheme->original = entity_load_unchanged('access_scheme', $scheme->sid);
      }

      // The machine name and type are not editable after creation.
      if (!empty($scheme->original)) {
        $scheme->machine_name = $scheme->original->machine_name;
        $scheme->type = $scheme->original->type;
      }

      module_invoke_all('access_scheme_presave', $scheme);
      module_invoke_all('entity_presave', $scheme, 'access_scheme');

      if (empty($scheme->sid)) {
        $op = 'insert';
        $status = drupal_write_record('access_scheme', $scheme);
        access_scheme_static_reset();
        field_attach_create_bundle('access_grant', $scheme->machine_name);
      }
      else {
        $op = 'update';
        $status = drupal_write_record('access_scheme', $scheme, 'sid');
        access_scheme_static_reset(array($scheme->sid));
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
   * @param int $sid
   *   The scheme ID.
   *
   * @return int
   *   Status constant indicating deletion.
   */
  public function delete($sid) {
    $transaction = db_transaction();
    try {
      $scheme = entity_load_unchanged('access_scheme', $sid);
      if ($scheme) {
        // Delete all access grants in the scheme.
        $gids = db_query('SELECT gid FROM {access_grant} WHERE scheme = :scheme', array(':scheme' => $scheme->machine_name))->fetchCol();
        foreach ($gids as $gid) {
          access_grant_delete($gid);
        }

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
        cache_clear_all();
        access_scheme_static_reset();
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
