<?php

/**
 * @file
 * Contains the access grant entity controller.
 */

/**
 * Provides the entity controller for access grants.
 */
class AccessGrantEntityController extends DrupalDefaultEntityController {

  /**
   * Constructs a new grant object without saving it.
   *
   * @param array $values
   *   (optional) An array of values to set, keyed by property name.
   *
   * @return object
   *   An access grant object.
   */
  public function create(array $values = array()) {
    $grant = new stdClass();
    $grant->gid = NULL;
    $grant->uid = empty($values['uid']) ? NULL : $values['uid'];
    $grant->rid = empty($values['rid']) ? NULL : $values['rid'];
    $grant->scheme = empty($values['scheme']) ? '' : $values['scheme'];
    return $grant;
  }

  /**
   * Overrides DrupalDefaultEntityController::attachLoad().
   */
  protected function attachLoad(&$grants, $revision_id = FALSE) {
    parent::attachLoad($grants, $revision_id);
    foreach ($grants as $grant) {
      // Make the realm field's values available as $grant->realms, an array
      // where the keys are the values of the grant's access realm field, and
      // the values are the realm labels.
      $grant->realms = array();
      $scheme = access_scheme_machine_name_load($grant->scheme);
      $field_name = $scheme->realm_field['field_name'];
      $assigned = isset($grant->$field_name) ? $grant->$field_name : array(LANGUAGE_NONE => array());
      if (!empty($assigned[LANGUAGE_NONE])) {
        foreach ($assigned[LANGUAGE_NONE] as $data) {
          $key = $data['value'];
          // Make sure that the assigned value exists in the realm list.
          if (isset($scheme->realms[$key])) {
            $grant->realms[$key] = field_filter_xss($scheme->realms[$key]);
          }
        }
      }
    }
  }

  /**
   * Saves an access grant to the database.
   *
   * @param object $grant
   *   An access grant object with the following properties:
   *   - gid: (optional) The unique ID for the grant being saved. If $grant->gid
   *     is empty or omitted, a new grant will be inserted.
   *   - uid: The ID of the user being granted access.
   *   - rid: The ID of the role being granted to the user.
   *   - scheme: The machine name of the scheme to which the grant belongs.
   *   - original: (optional) The original grant object before any changes were
   *     applied. When omitted, the unchanged grant object is loaded from the
   *     database and stored in this property.
   *   Since an access grant is an entity, any fields contained in the grant
   *   object are saved alongside the grant object.
   *
   * @return int
   *   Status constant indicating whether the grant was inserted (SAVED_NEW) or
   *   updated (SAVED_UPDATED). When inserting a new grant, $grant->gid will
   *   contain the ID of the newly created grant.
   */
  public function save($grant) {
    $transaction = db_transaction();
    try {
      // Load the stored entity, if any.
      if (!empty($grant->gid) && !isset($grant->original)) {
        $grant->original = entity_load_unchanged('access_grant', $grant->gid);
      }

      field_attach_presave('access_grant', $grant);
      module_invoke_all('access_grant_presave', $grant);
      module_invoke_all('entity_presave', $grant, 'access_grant');

      if (empty($grant->gid)) {
        $op = 'insert';
        $status = drupal_write_record('access_grant', $grant);
        field_attach_insert('access_grant', $grant);
        $this->resetCache();
      }
      else {
        $op = 'update';
        $status = drupal_write_record('access_grant', $grant, 'gid');
        field_attach_update('access_grant', $grant);
        $this->resetCache(array($grant->gid));
      }

      module_invoke_all('access_grant_' . $op, $grant);
      module_invoke_all('entity_' . $op, $grant, 'access_grant');
      unset($grant->original);

      return $status;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('access', $e);
      throw $e;
    }
  }

  /**
   * Deletes an access grant from the database.
   *
   * @param int $gid
   *   The grant ID.
   *
   * @return int
   *   Status constant indicating deletion.
   */
  public function delete($gid) {
    $transaction = db_transaction();
    try {
      $grant = entity_load_unchanged('access_grant', $gid);
      if ($grant) {
        module_invoke_all('access_grant_delete', $grant);
        module_invoke_all('entity_delete', $grant, 'access_grant');
        field_attach_delete('access_grant', $grant);

        db_delete('access_grant')
          ->condition('gid', $gid)
          ->execute();
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
