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
