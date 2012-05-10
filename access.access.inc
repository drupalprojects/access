<?php
/**
 * @file
 * Implements the access control kit API to integrate with core modules.
 */

/**
 * Implements hook_access_info().
 */
function access_access_info() {
  $modules = array('list', 'taxonomy', 'user');
  $info = array();
  foreach ($modules as $module) {
    if (module_exists($module)) {
      $function = '_access_access_info_' . $module;
      $info += $function();
    }
  }
  return $info;
}

/**
 * Adds support for user.module to access_access_info().
 */
function _access_access_info_user() {
  $info['user'] = array(
    'label' => t('a user account'),
    'type' => 'integer',
    'group' => t('User'),
  );
  // @todo Support access to nodes based on authorship, menu items based on the
  // authorship of the node it links to, etc. (ex. "You can edit any nodes
  // written by Bob and any menu links that point to them.").
  return $info;
}

/**
 * Adds support for list.module to access_access_info().
 */
function _access_access_info_list() {
  $info['list_text'] = array(
    'label' => t("a text list field's allowed values"),
    'type' => 'text',
    'group' => t('List'),
  );
  // @todo Support the other list field types.
  return $info;
}

/**
 * Add support for taxonomy.module to access_access_info().
 */
function _access_access_info_taxonomy() {
  $info['taxonomy_term'] = array(
    'label' => t('a term'),
    'type' => 'integer',
    'group' => t('Taxonomy'),
  );
  // @todo Support cascading hierarchical privileges as an option.
  // @todo Support relationships based on fields and on taxonomy_index.
  return $info;
}
