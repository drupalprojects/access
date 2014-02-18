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

      // Attach the scheme's object access handlers.
      $scheme->handlers = array();
      $result = db_query('SELECT * FROM {access_handler} WHERE scheme = :scheme', array(':scheme' => $scheme->machine_name));
      foreach ($result as $row) {
        $this->attachHandler($scheme, $row->object_type, $row->handler, unserialize($row->settings));
      }
    }
    parent::attachLoad($schemes, $revision_id);
  }

  /**
   * Attaches a handler to a scheme object.
   *
   * @param object $scheme
   *   The access scheme to which the handler will be attached.
   * @param string $object_type
   *   The type of Drupal object that the handler will manage (e.g., node).
   * @param string $class
   *   The name of the access handler class.
   * @param array $settings
   *   (optional) An array of settings to control the handler's behavior.
   */
  public function attachHandler($scheme, $object_type, $class, $settings = array()) {
    $scheme->handlers[$object_type] = new $class($scheme, $settings);
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
   *   - handlers: (optional) An array of attached handler objects. If set, the
   *     scheme's handlers will be updated to match; setting array() will detach
   *     all handlers. If not set, the scheme's handlers are left unchanged.
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

      // Update attached handlers.
      if (isset($scheme->handlers)) {
        // Delete any handlers that were detached from the scheme.
        if (!empty($scheme->original)) {
          foreach (array_keys($scheme->original->handlers) as $object_type) {
            if (empty($scheme->handlers[$object_type])) {
              db_delete('access_handler')
                ->condition('scheme', $scheme->machine_name)
                ->condition('object_type', $object_type)
                ->execute();
            }
          }
        }
        // Save current handlers.
        $handler_info = access_handler_info();
        foreach ($scheme->handlers as $object_type => $handler) {
          if (!empty($handler)) {
            $key = array(
              'scheme' => $scheme->machine_name,
              'object_type' => $object_type,
            );
            $class = get_class($handler);
            $fields = array(
              'handler' => $class,
              'module' => $handler_info[$class]['module'],
              'settings' => serialize($handler->getSettings()),
            );
            db_merge('access_handler')
              ->key($key)
              ->fields($fields)
              ->execute();
          }
        }
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
