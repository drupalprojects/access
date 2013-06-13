<?php

/**
 * @file
 * Page callbacks for the access control kit module.
 */

/**
 * Menu page callback to view a single access grant.
 */
function access_grant_page($grant) {
  return access_grant_view_multiple(array($grant->gid => $grant), 'full');
}

/**
 * Process variables for access-grant.tpl.php.
 *
 * The $variables array contains the following arguments:
 * - $access_grant
 *
 * @see access-grant.tpl.php
 */
function template_preprocess_access_grant(&$variables) {
  $grant = $variables['elements']['#access_grant'];
  $variables['grant'] = $grant;

  // Place the renderable elements in the $content variable.
  foreach (element_children($variables['elements']) as $key) {
    $variables['content'][$key] = $variables['elements'][$key];
  }

  // Add variables for each attached field.
  field_attach_preprocess('access_grant', $grant, $variables['elements'], $variables);

  // Add helper variables for the access realm field, since the name is dynamic.
  $scheme = access_scheme_machine_name_load($grant->scheme);
  $field_name = $scheme->realm_field['field_name'];
  $variables['realm_field_name'] = $field_name;
  $variables['realms'] = empty($variables[$field_name]) ? array() : $variables[$field_name];

  // Add a class to identify the scheme.
  $variables['classes_array'][] = drupal_html_class('access-grant-' . $grant->scheme);

  // Allow per scheme templates.
  $variables['theme_hook_suggestions'][] = 'access_grant__' . $grant->scheme;
}
