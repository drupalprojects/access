<?php
/**
 * @file
 * Access containers administrative UI for the access control kit module.
 */

/**
 * Menu page callback; the access containers overview page.
 */
function access_overview_containers() {
  return '@todo';
}

/**
 * Menu page callback; the access container add list page.
 */
function access_container_add_list() {
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
 * Menu page callback; add an access container for a given scheme.
 *
 * @param $scheme
 *   The access scheme in which to create a new container.
 */
function access_container_add($scheme) {
  $container = (object) array('scheme' => $scheme->scheme);
  return drupal_get_form('access_container_form', $container);
}

/**
 * Menu page callback; edit an access container.
 *
 * @param $container
 *   The access container to edit.
 */
function access_container_edit($container) {
  $scheme = access_scheme_get_scheme($container);
  drupal_set_title(t('<em>Edit @scheme</em> @container', array('@scheme' => $scheme->name, '@container' => $container->name)), PASS_THROUGH);
  return drupal_get_form('access_container_form', $container);
}

/**
 * Form constructor for the access container add/edit form.
 *
 * @param $container
 *   The access container to edit.
 *
 * @see access_container_form_validate()
 * @see access_container_form_submit()
 */
function access_container_form($form, &$form_state, $container) {
  // @todo
}

/**
 * Menu page callback; view a single access container.
 */
function access_container_view($container) {
  return 'todo';
}
