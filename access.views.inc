<?php

/**
 * @file
 * Views integration for the access control kit module.
 */

/**
 * Implements hook_views_data().
 */
function access_views_data() {
  // Access grants.
  $data['access_grant'] = array(
    'table' => array(
      'group' => t('Access grant'),
      'base' => array(
        'field' => 'gid',
        'title' => t('Access grants'),
        'help' => t('Access grants are user role assignments for realms in an access scheme.'),
      ),
      'entity type' => 'access_grant',
    ),
    'gid' => array(
      'title' => t('Gid'),
      'help' => t('The grant ID of the grant.'),
      'field' => array(
        'handler' => 'views_handler_field',
        'click sortable' => TRUE,
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_numeric',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_numeric',
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
    ),
    'uid' => array(
      'title' => t('User uid'),
      'help' => t('The user to whom access is granted. If you need more fields than the uid, add the "access grant: user" relationship.'),
      'field' => array(
        'handler' => 'views_handler_field_user',
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_numeric',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_user_name',
      ),
      'relationship' => array(
        'title' => t('User'),
        'help' => t('Relate an access grant to its user.'),
        'handler' => 'views_handler_relationship',
        'base' => 'users',
        'base field' => 'uid',
        'label' => t('user'),
      ),
    ),
    'rid' => array(
      'title' => t('Role'),
      'help' => t('The role assigned by the grant.'),
      'field' => array(
        'handler' => 'access_handler_field_access_grant_role',
        'click sortable' => TRUE,
      ),
      'argument' => array(
        'handler' => 'access_handler_argument_access_grant_role',
      ),
      'filter' => array(
        'handler' => 'access_handler_filter_access_grant_role',
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
    ),
  );

  // Access schemes.
  $data['access_scheme'] = array(
    'table' => array(
      'group' => t('Access scheme'),
      'join' => array(
        'access_grant' => array(
          'left_field' => 'scheme',
          'field' => 'machine_name',
        ),
      ),
      'entity type' => 'access_scheme',
    ),
    'sid' => array(
      'title' => t('Sid'),
      'help' => t('The scheme ID of the scheme.'),
      'field' => array(
        'handler' => 'views_handler_field',
        'click sortable' => TRUE,
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_numeric',
        'name field' => 'name',
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
    ),
    'machine_name' => array(
      'title' => t('Machine name'),
      'help' => t('The machine-readable name of the scheme.'),
      'field' => array(
        'handler' => 'views_handler_field',
        'click sortable' => TRUE,
      ),
      'argument' => array(
        'handler' => 'views_handler_argument_string',
      ),
      'filter' => array(
        'handler' => 'views_handler_filter_in_operator',
        'options callback' => 'access_scheme_names',
      ),
    ),
    'name' => array(
      'title' => t('Name'),
      'help' => t('The human-readable name of the scheme.'),
      'field' => array(
        'handler' => 'views_handler_field',
        'click sortable' => TRUE,
      ),
      'sort' => array(
        'handler' => 'views_handler_sort',
      ),
    ),
    'description' => array(
      'title' => t('Description'),
      'help' => t('A description of the access scheme.'),
      'field' => array(
        'handler' => 'views_handler_field_xss',
      ),
    ),
  );

  return $data;
}

/**
 * Implements hook_views_data_alter().
 */
function access_views_data_alter(&$data) {
  // Relate users to their access grants.
  $data['users']['access_user_grants'] = array(
    'title' => t('Access grants'),
    'help' => t('Relate a user to assigned roles for realms in an access scheme. This relationship will create one record for each access grant.'),
    'real field' => 'uid',
    'relationship' => array(
      'handler' => 'views_handler_relationship',
      'base' => 'access_grant',
      'base field' => 'uid',
      'label' => t('access grants'),
    ),
  );

  // Relate realm fields to the objects that are members of the realm.
  foreach (access_scheme_load_multiple() as $scheme) {
    if ($scheme->realm_field['storage']['type'] != 'field_sql_storage') {
      continue;
    }
    $field_table = $scheme->realm_field['storage']['details']['sql'][FIELD_LOAD_CURRENT];
    $field_table_name = key($field_table);
    $field_table_value = $field_table[$field_table_name]['value'];
    foreach ($scheme->handlers as $object_type => $handler) {
      $handler->viewsDataAlter($data, $scheme->machine_name, $field_table_name, $field_table_value, $object_type);
    }
  }
}
