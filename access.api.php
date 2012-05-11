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
 * Declares information about access controls.
 *
 * @todo Explain proper usage.
 */
function hook_access_info() {
  $info['taxonomy_term'] = array(
    'label' => t('a term'),
    'type' => 'integer',
    'group' => t('Taxonomy'),
  );
  return $info;
}

/**
 * Adds settings to an access field's settings form.
 *
 * Modules that implement hook_access_info() can implement this hook to gather
 * any additional arguments that they need to generate an allowed values list.
 * Settings added here will be available in the $arguments parameter of
 * hook_access_field_allowed_values().
 *
 * @param $type
 *   The access boundary type.
 * @param $values
 *   The current values of the access field's boundary arguments.
 *
 * @return
 *   Field elements to populate the access field's settings form.
 */
function hook_access_field_settings_form($type, $values = array()) {
  // @todo Provide example usage of the settings form hook.
}

/**
 * Returns the allowed values list for an access boundary field.
 *
 * Modules that implement hook_access_info() should also implement this hook to
 * provide the list of allowed values for the module's access field types.
 *
 * @param $type
 *   The access boundary type.
 * @param $arguments
 *   (optional) An array of additional arguments defined by the access boundary
 *   module in hook_access_field_settings_form().
 *
 * @return
 *   The array of allowed values for the field. Array keys are the values to be
 *   stored, and should match the data type declared for the field. Array values
 *   are the labels to display within the field's widget.  These labels should
 *   NOT be sanitized; options.module will handle that in the widget.
 */
function hook_access_field_allowed_values($type, $arguments = array()) {
  if ($type == 'example') {
    return array(
      0 => t('Zero'),
      1 => t('One'),
      2 => t('Two'),
    );
  }
}

/**
 * Act on access schemes when inserted.
 *
 * Modules implementing this hook can act on the scheme object after it has been
 * saved to the database.
 *
 * @param $scheme
 *   An access scheme object.
 */
function hook_access_scheme_insert($scheme) {
  if ($scheme->type == 'example') {
    variable_set('access_scheme_example', TRUE);
  }
}

/**
 * Act on access schemes when updated.
 *
 * Modules implementing this hook can act on the scheme object after it has been
 * updated in the database.
 *
 * @param $scheme
 *   An access scheme object.
 */
function hook_access_scheme_update($scheme) {
  $status = ($scheme->type == 'example') ? TRUE : FALSE;
  variable_set('access_scheme_example', $status);
}

/**
 * Respond to the deletion on access schemes.
 *
 * Modules implementing this hook can respond to the deletion of access schemes
 * from the database.
 *
 * @param $scheme
 *   An access scheme object.
 */
function hook_access_scheme_delete($scheme) {
  if ($scheme->type == 'example') {
    variable_del('access_scheme_example');
  }
}

/**
 * @} End of "addtogroup hooks".
 */
