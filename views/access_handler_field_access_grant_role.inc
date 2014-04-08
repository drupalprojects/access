<?php

/**
 * @file
 * Definition of access_handler_field_access_grant_role.
 */

/**
 * Field handler to translate an access grant's role ID into its readable form.
 *
 * @ingroup views_field_handlers
 */
class access_handler_field_access_grant_role extends views_handler_field {

  /**
   * Overrides views_handler_field::option_definition().
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['output_format'] = array('default' => 'name');
    return $options;
  }

  /**
   * Overrides views_handler_field::options_form().
   */
  function options_form(&$form, &$form_state) {
    $form['output_format'] = array(
      '#type' => 'select',
      '#title' => t('Output format'),
      '#options' => array(
        'name' => t('Role name'),
        'rid' => t('Role ID'),
      ),
      '#default_value' => $this->options['output_format'],
    );
    parent::options_form($form, $form_state);
  }

  /**
   * Dereference a role ID to a role name.
   */
  function render_name($rid) {
    switch ($this->options['output_format']) {
      case 'name':
        $role = user_role_load($rid);
        return empty($role) ? '' : $this->sanitize_value($role->name);

      default:
        return $this->sanitize_value($rid);
    }
  }

  /**
   * Overrides views_handler_field::render().
   */
  function render($values) {
    $value = $this->get_value($values);
    return $this->render_name($value);
  }

}
