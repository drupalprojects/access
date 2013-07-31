<?php

/**
 * @file
 * Contains the handler class for Field API fields on entities.
 */

/**
 * Controls access to a fieldable entity based on a Fields API field.
 */
class ACKEntityField extends AccessControlKitHandler {

  /**
   * The name of the field that determines access.
   *
   * @var string
   */
  protected $field_name;

  /**
   * The key that contains the field's value in calls to field_get_items().
   *
   * @var string
   */
  protected $field_value_key;

  /**
   * Overrides AccessControlKitHandler::__construct().
   */
  public function __construct($scheme, array $settings = array()) {
    parent::__construct($scheme, $settings);
    $this->field_name = isset($scheme->settings['field_name']) ? $scheme->settings['field_name'] : NULL;
    $this->field_value_key = 'value';
  }

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    if (isset($this->field_name)) {
      return t('The value of %field_name will determine realm membership.', array('%field_name' => $this->field_name));
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
    if (!empty($this->field_name)) {
      $items = field_get_items($object_type, $object, $this->field_name);
      if (is_array($items)) {
        foreach ($items as $item) {
          if (isset($item[$this->field_value_key])) {
            $values[] = $item[$this->field_value_key];
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
    if (!empty($this->field_name) && !empty($form[$this->field_name])) {
      $language = $form[$this->field_name]['#language'];
      $element = &$form[$this->field_name][$language];

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
          $element['#default_value'] = key($element['#options']);
        }
      }
    }
  }
}
