<?php

/**
 * @file
 * Contains the handler class for Field API fields on entities.
 */

/**
 * Controls access to a fieldable entity based on a Field API field.
 */
class ACKEntityField extends AccessControlKitHandler {

  /**
   * The name of the field that determines access.
   *
   * @var string
   */
  protected $fieldName;

  /**
   * The key that contains the field's value in calls to field_get_items().
   *
   * @var string
   */
  protected $fieldValueKey;

  /**
   * Overrides AccessControlKitHandler::__construct().
   */
  public function __construct($scheme, array $settings = array()) {
    parent::__construct($scheme, $settings);
    $this->fieldName = isset($scheme->settings['field_name']) ? $scheme->settings['field_name'] : NULL;
    $this->fieldValueKey = 'value';
  }

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    if (isset($this->fieldName)) {
      return t('The value of %field_name will determine realm membership.', array('%field_name' => $this->fieldName));
    }
    else {
      return t('The value of the selected field will determine realm membership.');
    }
  }

  /**
   * Overrides AccessControlKitHandler::objectRealms().
   */
  public function objectRealms($object_type, $object) {
    $values = array();
    if (!empty($this->fieldName)) {
      $items = field_get_items($object_type, $object, $this->fieldName);
      if (is_array($items)) {
        foreach ($items as $item) {
          if (isset($item[$this->fieldValueKey])) {
            $values[] = $item[$this->fieldValueKey];
          }
        }
      }
    }
    return $values;
  }

  /**
   * Overrides AccessControlKitHandler::objectFormAlter().
   */
  public function objectFormAlter($object_type, $object, &$form, &$form_state, $form_id, $realms = NULL) {
    if (!empty($this->fieldName) && !empty($form[$this->fieldName])) {
      $language = $form[$this->fieldName]['#language'];
      $element = &$form[$this->fieldName][$language];

      // Lock the field if no realms are allowed.
      if (!isset($realms)) {
        $element['#disabled'] = TRUE;
      }
      // If the field widget uses the #options property, filter the options.
      elseif (isset($element['#options'])) {
        $options = array();

        // Preserve the empty option, if one was specified.
        if (isset($element['#options']['_none'])) {
          $options['_none'] = $element['#options']['_none'];
        }

        // Include only those options that correspond to allowed realms.
        foreach ($realms as $realm) {
          if (isset($element['#options'][$realm])) {
            $options[$realm] = $element['#options'][$realm];
          }
        }

        // If the element is required and only one option remains besides the
        // empty value, then remove the empty value as an option.
        if (!empty($element['#required']) && isset($options['_none']) && count($options) == 2) {
          unset($options['_none']);
        }

        $element['#options'] = $options;
        // If only one option remains, select it for the user.
        if (count($element['#options']) == 1) {
          $element['#disabled'] = TRUE;
          $key = key($element['#options']);
          $element['#default_value'] = is_array($element['#default_value']) ? array($key) : $key;
        }
      }
    }
  }

  /**
   * Overrides AccessControlKitHandler::viewsDataAlter().
   */
  public function viewsDataAlter(&$data, $scheme_machine_name, $realm_field_table_name, $realm_field_table_value, $object_type) {
    if (!empty($this->fieldName)) {
      $entity_info = entity_get_info($object_type);
      $entity_field = field_info_field($this->fieldName);

      if (isset($entity_info['base table']) && $entity_field['storage']['type'] == 'field_sql_storage') {
        $entity_field_table = $entity_field['storage']['details']['sql'][FIELD_LOAD_CURRENT];
        $entity_field_table_name = key($entity_field_table);

        if (isset($entity_field_table[$entity_field_table_name][$this->fieldValueKey])) {
          $entity_field_table_value = $entity_field_table[$entity_field_table_name][$this->fieldValueKey];

          $scheme_names = access_scheme_names();
          $pseudo_field_name = 'access_grants_' . $scheme_machine_name;
          $t_args = array(
            '@entity' => $object_type,
            '@scheme' => $scheme_names[$scheme_machine_name],
          );

          // Relate the entity to access grants for its realm.
          $data[$entity_info['base table']][$pseudo_field_name]['relationship'] = array(
            'title' => t('Access grants in @scheme', $t_args),
            'help' => t('Relate the @entity entity to access grants in the @scheme scheme.', $t_args),
            'handler' => 'access_handler_relationship_ack_entity_field',
            'scheme' => $scheme_machine_name,
            'base' => 'access_grant',
            'base field' => 'gid',
            'entity field' => $entity_info['entity keys']['id'],
            'field table' => $entity_field_table_name,
            'field field' => $entity_field_table_value,
            'realm table' => $realm_field_table_name,
            'realm field' => $realm_field_table_value,
            'join_extra' => array(
              array(
                'field' => 'entity_type',
                'value' => $object_type,
              ),
              array(
                'field' => 'deleted',
                'value' => 0,
                'numeric' => TRUE,
              ),
            ),
            'label' => t('@scheme access grants', $t_args),
          );
        }
      }
    }
  }
}
