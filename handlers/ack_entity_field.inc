<?php
/**
 * @file
 * Contains the base class for fieldable entity access handlers.
 */

/**
 * Controls access to a fieldable entity based on a Fields API field.
 */
abstract class ACKEntityField extends AccessControlKit {
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
   * Overrides AccessControlKit::__construct().
   */
  public function __construct($scheme = '', $object_type = '', array $settings = array()) {
    parent::__construct($scheme, $object_type, $settings);
    $this->field_name = empty($settings['field_name']) ? '' : $settings['field_name'];
    $this->field_value_key = 'value';
  }

  /**
   * Implements AccessControlKitInterface::getRealms().
   */
  public function getRealms($entity) {
    $values = array();
    if (!empty($this->field_name)) {
      if ($items = field_get_items($this->object_type, $entity, $this->field_name)) {
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
   * Implements AccessControlKitInterface::formAlterNew().
   */
  public function formAlterNew(&$form, &$form_state, $form_id, $entity, $realms = array()) {
    if (!empty($this->field_name) && !empty($form[$this->field_name])) {
      $language = $form[$this->field_name]['#language'];
      $this->filterElementOptions($form[$this->field_name][$language], $realms, '_none');
    }
  }

  /**
   * Implements AccessControlKitInterface::formAlterExisting().
   */
  public function formAlterExisting(&$form, &$form_state, $form_id, $entity, $realms = array()) {
    if (!empty($this->field_name)) {
      // Lock the field.
      if (!empty($form[$this->field_name])) {
        $form[$this->field_name]['#disabled'] = TRUE;
      }
      // Hide the Delete button if the user does not have delete access.
      if (isset($form['actions']['delete'])) {
        $access = FALSE;
        foreach ($this->getRealms($entity) as $value) {
          if (array_key_exists($value, $realms)) {
            $access = TRUE;
          }
        }
        $form['actions']['delete']['#access'] = $access;
      }
    }
  }
}
