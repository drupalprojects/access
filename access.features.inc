<?php

/**
 * @file
 * Features integration for the access control kit module.
 */

/**
 * Implements hook_features_export().
 */
function access_scheme_features_export($data, &$export, $module_name = '') {
  $pipe = array();

  $export['dependencies']['features'] = 'features';
  $export['dependencies']['access'] = 'access';

  // Add each selected scheme and its dependencies.
  $handler_info = access_handler_info();
  foreach ($data as $scheme_machine_name) {
    $scheme = access_scheme_machine_name_load($scheme_machine_name, TRUE);
    if (!empty($scheme)) {
      // Add the scheme.
      $export['features']['access_scheme'][$scheme->machine_name] = $scheme->machine_name;

      // Add the module that provides the scheme type.
      $module = $scheme->info['module'];
      $export['dependencies'][$module] = $module;

      // Add the modules that provide the attached handlers.
      foreach ($scheme->handlers as $handler) {
        if (!empty($handler)) {
          $class = get_class($handler);
          $module = $handler_info[$class]['module'];
          $export['dependencies'][$module] = $module;
        }
      }

      // Add all attached fields, including the realm field.
      $fields = field_info_instances('access_grant', $scheme->machine_name);
      foreach ($fields as $field) {
        $key = "access_grant-{$field['bundle']}-{$field['field_name']}";
        $pipe['field'][] = $key;
        $pipe['field_instance'][] = $key;
      }
    }
  }

  return $pipe;
}

/**
 * Implements hook_features_export_options().
 */
function access_scheme_features_export_options() {
  return access_scheme_names();
}

/**
 * Implements hook_features_export_render().
 */
function access_scheme_features_export_render($module_name, $data, $export = NULL) {
  $access_info = access_info();
  $handler_info = access_handler_info();

  $code = array();
  $code[] = '  $schemes = array();';

  foreach ($data as $scheme_machine_name) {
    $original = access_scheme_machine_name_load($scheme_machine_name, TRUE);
    if (!empty($original)) {
      // Clone the scheme before we manipulate it, so as not to alter the cache.
      $scheme = clone $original;

      // Remove properties that should not be exported.
      unset($scheme->sid);
      unset($scheme->info);
      unset($scheme->realm_field);
      unset($scheme->realms);
      // The scheme's machine name is used as the export key (to make it easier
      // to override a default scheme with an _alter() hook), so we don't need
      // it again in the exported properties.
      unset($scheme->machine_name);

      // Put the attached handlers into an exportable format.
      $handlers = array();
      foreach ($scheme->handlers as $object_type => $handler) {
        $handlers[$object_type] = array(
          'handler' => get_class($handler),
          'settings' => $handler->getSettings(),
        );
      }
      // For the sake of readability in the exported code, remove the handlers
      // for now so we can add them later in separate assignment statements.
      unset($scheme->handlers);

      // Export the scheme.
      $code[] = '';
      $code[] = '  // Access scheme: ' . $scheme->name;
      $code[] = '  $schemes[\'' . $scheme_machine_name . '\'] = ' . features_var_export($scheme, '  ') . ';';

      // Export the handlers.
      if (!empty($handlers)) {
        $code[] = '  $schemes[\'' . $scheme_machine_name . '\'][\'handlers\'] = array();';
        foreach ($handlers as $object_type => $handler) {
          $code[] = '  // ' . $access_info[$object_type]['label'] . ' handler: ' . $handler_info[$handler['handler']]['label'];
          $code[] = '  $schemes[\'' . $scheme_machine_name . '\'][\'handlers\'][\'' . $object_type . '\'] = ' . features_var_export($handler, '  ') . ';';
        }
      }
    }
  }
  $code[] = '';
  $code[] = '  return $schemes;';
  $code = implode("\n", $code);
  return array('access_default_schemes' => $code);
}

/**
 * Implements hook_features_revert().
 */
function access_scheme_features_revert($module_name) {
  access_scheme_features_rebuild($module_name);
}

/**
 * Implements hook_features_rebuild().
 */
function access_scheme_features_rebuild($module_name) {
  $schemes = features_get_default('access_scheme', $module_name);
  if (!empty($schemes)) {
    foreach ($schemes as $scheme_machine_name => $scheme) {
      $scheme = (object) $scheme;
      $scheme->machine_name = $scheme_machine_name;
      $existing = access_scheme_machine_name_load($scheme->machine_name, TRUE);
      if (!empty($existing)) {
        $scheme->sid = $existing->sid;
      }
      foreach ($scheme->handlers as $object_type => $handler) {
        entity_get_controller('access_scheme')->attachHandler($scheme, $object_type, $handler['handler'], $handler['settings']);
      }
      access_scheme_save($scheme);
    }
  }
}
