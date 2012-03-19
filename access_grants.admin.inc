<?php
/**
 * @file
 * Access grants administrative UI for the access control kit module.
 */

/**
 * Menu page callback; the access grants overview page.
 */
function access_overview_grants() {
  return '@todo';
}

/**
 * Menu page callback; the access grant add list page.
 */
function access_grant_add_list() {
  $schemes = access_scheme_get_schemes();
  $content = array();

  foreach ($schemes as $key => $scheme) {
    $content[$key] = array(
      'title' => $scheme->name,
      'description' => $scheme->description,
      'href' => 'admin/access/add/' . str_replace('_', '-', $scheme->scheme),
      'localized_options' => array(),
    );
  }

  if (empty($content)) {
    return t('You have not created any access schemes yet. Go to the <a href="@create-scheme">access scheme creation page</a> to add a new access scheme.', array('@create-scheme' => url('admin/structure/access/add')));
  }
  // Bypass this listing page if only one scheme is available.
  if (count($content) == 1) {
    $item = array_shift($content);
    drupal_goto($item['href']);
  }
  return theme('admin_block_content', array('content' => $content));
}

/**
 * Menu page callback; add an access grant for a given scheme.
 *
 * @param $scheme
 *   The access scheme in which to create a new grant.
 */
function access_grant_add($scheme) {
  $grant = (object) array('scheme' => $scheme->scheme);
  return drupal_get_form('access_grant_form', $grant);
}

/**
 * Menu page callback; edit an access grant.
 *
 * @param $grant
 *   The access grant to edit.
 */
function access_grant_edit($grant) {
  $scheme = access_scheme_get_scheme($grant);
  drupal_set_title(t('<em>Edit grant by @scheme</em> @grant', array('@scheme' => $scheme->name, '@grant' => access_grant_label($grant, $grant->scheme))), PASS_THROUGH);
  return drupal_get_form('access_grant_form', $grant);
}

/**
 * Form constructor for the access grant add/edit form.
 *
 * @param $grant
 *   The access grant to edit.
 *
 * @see access_grant_form_validate()
 * @see access_grant_form_submit()
 */
function access_grant_form($form, &$form_state, $grant) {
  // @todo
}

/**
 * Menu page callback; view a single access grant.
 */
function access_grant_view($grant) {
  return '@todo';
}
