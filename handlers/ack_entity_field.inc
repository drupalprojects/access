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
        foreach ($element['#options'] as $option_value => $option_name) {
          // Preserve options that correspond to allowed realms.
          // Also preserve the empty option, if one was provided.
          if (in_array($option_value, $realms) || $option_value == '_none') {
            continue;
          }
          // Remove unmatched options.
          unset($element['#options'][$option_value]);
        }

        // If the element is required and only one option remains besides the
        // empty value, then remove the empty value as an option.
        if (isset($element['#options']['_none']) && !empty($element['#required']) && count($element['#options']) == 2) {
          unset($element['#options']['_none']);
        }

        // If only one option remains, select it for the user.
        if (count($element['#options']) == 1) {
          $element['#disabled'] = TRUE;
          reset($element['#options']);
          $option_value = key($element['#options']);
          $element['#default_value'] = is_array($element['#default_value']) ? array($option_value) : $option_value;
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

          $pseudo_field_name = 'ack_' . $scheme_machine_name;
          $scheme_names = access_scheme_names();
          $t_args = array(
            '@entity' => $object_type,
            '@field' => $entity_field['field_name'],
            '@scheme' => $scheme_names[$scheme_machine_name],
            '@machine' => $scheme_machine_name,
          );

          // Relate the entity field to access grants through the realm field.
          $data[$entity_info['base table']][$pseudo_field_name] = array(
            'title' => t('Access grants on @entity for @scheme', $t_args),
            'real field' => $entity_info['entity keys']['id'],
            'relationship' => array(
              'label' => t('@machine access grants', $t_args),
              'help' => t('Relate the @entity to @scheme access grants through the @field field.', $t_args),
              'handler' => 'access_handler_relationship_ack_entity_field',
              'base' => 'access_grant',
              'field' => $entity_info['entity keys']['id'],
              'scheme' => $scheme_machine_name,
              'entity type' => $object_type,
              'entity table' => $entity_field_table_name,
              'entity field' => $entity_field_table_value,
              'realm table' => $realm_field_table_name,
              'realm field' => $realm_field_table_value,
            ),
          );
        }
      }
    }
  }
}
