<?php
/**
 * @file
 * Contains the Access Control Kit interface and base class.
 */

/**
 * Interface for an object access handler.
 */
interface AccessControlKitInterface {
  /**
   * Returns the list of access realms that apply to the given object.
   *
   * @param mixed $object
   *   A Drupal object, such as a node, term, or menu item.  The actual data
   *   type depends on the object for which the class is providing ACK support.
   *
   * @return
   *   An array of realms of which $object is a member according to the handler.
   */
  public function getRealms($object);

  /**
   * Returns form elements needed to configure the handler.
   *
   * @param $realm_type
   *   The realm type of the access scheme to which the handler is attached.
   * @param array $realm_settings
   *   (optional) An array of configuration arguments for the realm type.
   *
   * @return
   *   An array of form elements, or array() if the handler does not require
   *   configuration. The array should not normally include a submit button, as
   *   one will be provided automatically by the ACK UI. The submitted values of
   *   the form will be saved as-is in the handler's settings array when
   *   attaching the handler to an access scheme. Note that the returned
   *   elements will be wrapped in an element with '#tree' => TRUE when this
   *   method is called by the ACK UI, and thus the submitted values will retain
   *   their structure in $this->settings.
   */
   public function settingsForm($realm_type, array $realm_settings = array());

   /**
    * Alters the form for a new object to reflect realm permissions.
    *
    * It is up to the handler module to invoke this method as needed from
    * hook_form_alter() to enforce realm permissions on new objects.
    * Implementations of this method should modify the form in such a way as to
    * prevent the user from creating the object in a realm to which they do not
    * have access.
    *
    * For example, if creating a node where access is granted via a list field,
    * the list field options should be modified to only show the realms where
    * the user has access to create the node.
    *
    * @param &$form
    *   Nested array of form elements that comprise the form.
    * @param &$form_state
    *   A keyed array containing the current state of the form.
    * @param $form_id
    *   String representing the name of the form itself. Typically this is the
    *   name of the function that generated the form.
    * @param $object
    *   The object represented by this form.
    * @param $realms
    *   An array of realms in which the user has "create" access for the object,
    *   as realm_value => realm_label.
    */
    public function formAlterNew(&$form, &$form_state, $form_id, $object, $realms = array());

   /**
    * Alters the form for an existing object to reflect realm permissions.
    *
    * It is up to the handler module to invoke this method as needed from
    * hook_form_alter() to enforce realm permissions on existing objects.
    * Implementations of this method should modify the form in such a way as to
    * prevent the user from changing the object's assigned realms, and to
    * prevent deleting the object unless the user has delete permission for one
    * of the object's assigned realms.
    *
    * For example, if editing a node where access is granted via a list field,
    * the list field should be locked (i.e., #access => FALSE), and the delete
    * button should be removed unless the user has the "delete" permission for
    * one of the realm values currently assigned to the field.
    *
    * @param &$form
    *   Nested array of form elements that comprise the form.
    * @param &$form_state
    *   A keyed array containing the current state of the form.
    * @param $form_id
    *   String representing the name of the form itself. Typically this is the
    *   name of the function that generated the form.
    * @param $object
    *   The object represented by this form.
    * @param $realms
    *   An array of realms in which the user has "delete" access for the object,
    *   as realm_value => realm_label.
    */
    public function formAlterExisting(&$form, &$form_state, $form_id, $object, $realms = array());
}

/**
 * Base class for object access handlers.
 */
abstract class AccessControlKit implements AccessControlKitInterface {
  /**
   * The name of the access scheme to which the handler is attached.
   *
   * @var string
   */
  protected $scheme;

  /**
   * The type of object that this handler controls.
   *
   * @var string
   */
  protected $object_type;

  /**
   * The access handler configuration.
   *
   * @var array
   */
  protected $settings;

  /**
   * Constructs an AccessControlKit object.
   *
   * @param $scheme
   *   The name of the access scheme to which the handler is attached.
   * @param $object_type
   *   The type of object that this handler controls.
   * @param array $settings
   *   The handler configuration array.
   */
  public function __construct($scheme = '', $object_type = '', array $settings = array()) {
    $this->scheme = $scheme;
    $this->object_type = $object_type;
    $this->settings = $settings;
  }

  /**
   * Implements AccessControlKitInterface::getRealms().
   */
  public function getRealms($object) {
    return array();
  }

  /**
   * Implements AccessControlKitInterface::settingsForm().
   */
  public function settingsForm($realm_type, array $realm_settings = array()) {
    return array();
  }

  /**
   * Implements AccessControlKitInterface::formAlterNew().
   */
  public function formAlterNew(&$form, &$form_state, $form_id, $object, $realms = array()) {
    // No filtering needed in the abstract class.  Subclasses can make use of
    // the convenience function $this->filterElementOptions() to filter the
    // #options attribute of access-related form elements by $realms.
  }

  /**
   * Implements AccessControlKitInterface::formAlterExisting().
   */
  public function formAlterExisting(&$form, &$form_state, $form_id, $object, $realms = array()) {
    // No filtering needed in the abstract class.  Subclasses should lock any
    // access-related form fields.  Access to "Delete" buttons, etc., should
    // also be blocked if the current value of an access-related element is not
    // present in $realms.
  }

  /**
   * Filters a form element's #options to only include accessible realms.
   *
   * The element's '#options' array will be filtered according to the realms
   * parameter to only show values that the user can access.  If only one option
   * remains, the element will be set to that option and disabled.
   *
   * @param &$element
   *   A form element with an '#options' attribute (such as #type => 'select',
   *   'radio', or 'checkboxes'), passed by reference.
   * @param $realms
   *   An associative array indicating which options the user is allowed to set,
   *   as realm_value => realm_label.
   * @param $empty_value
   *   (optional) The #options key that denotes that no selection has been made.
   */
  protected function filterElementOptions(&$element, $realms, $empty_value = NULL) {
    $options = array();
    // Preserve the empty option, if one was specified.
    if (isset($empty_value) && isset($element['#options'][$empty_value])) {
      $options[$empty_value] = $element['#options'][$empty_value];
    }
    // Include only those options that correspond to allowed realms.
    $options += array_intersect_key($element['#options'], $realms);

    // If the element is required, and only one option remains besides the empty
    // value, then remove the empty value as an option.
    if (!empty($element['#required']) && count($options) == 2 && isset($empty_value) && isset($options[$empty_value])) {
      unset($options[$empty_value]);
    }

    $element['#options'] = $options;
    if (count($element['#options']) == 1) {
      $element['#disabled'] = TRUE;
      $element['#default_value'] = key($element['#options']);
    }
  }

  /**
   * Returns the settings for the scheme to which the handler is attached.
   *
   * @param $name
   *   (optional) The name of a setting.  If omitted, returns all settings.
   * @param $empty_value
   *   (optional) If $name does not exist in the scheme settings, this will be
   *   returned instead.  Defaults to NULL.  Ignored if $name is omitted.
   *
   * @return
   *   The value of the requested scheme setting, or all scheme settings in an
   *   array as name => value.
   */
  protected function getSchemeSetting($name = NULL, $empty_value = NULL) {
    $ack_fieldname = access_scheme_fieldname($this->scheme);
    $ack_field = field_info_field($ack_fieldname);
    $settings = empty($ack_field['settings']['arguments']) ? array() : $ack_field['settings']['arguments'];
    if (isset($name)) {
      return isset($settings[$name]) ? $settings[$name] : $empty_value;
    }
    return $settings;
  }
}
