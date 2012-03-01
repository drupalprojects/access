<?php
/**
 * @file
 * Access schemes administrative UI for the access control kit module.
 */

/**
 * Menu page callback; the access schemes overview page.
 */
function access_overview_schemes() {
  $schemes = access_scheme_get_schemes();
  $field_ui = module_exists('field_ui');
  $header = array(t('Name'), array(
    'data' => t('Operations'),
    'colspan' => $field_ui ? '4' : '2',
  ));
  $rows = array();

  foreach ($schemes as $key => $scheme) {
    $url = 'admin/structure/access/manage/' . str_replace('_', '-', $key);
    $row = array(theme('access_schemes_overview', array('scheme' => $scheme)));

    // The edit operation.
    $row[] = array('data' => l(t('edit'), $url));

    // Field UI operations.
    if ($field_ui) {
      // Manage fields.
      $row[] = array('data' => l(t('manage fields'), $url . '/fields'));
      // Manage display.
      $row[] = array('data' => l(t('manage display'), $url . '/display'));
    }

    // The delete operation.
    $row[] = array('data' => l(t('delete'), $url . '/delete'));

    $rows[] = $row;
  }

  $build['scheme_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No access schemes available. <a href="@link">Add an access scheme</a>.', array('@link' => url('admin/structure/access/add'))),
  );

  return $build;
}

/**
 * Returns HTML for the Name column on the access scheme admin overview page.
 *
 * @param $variables
 *   An associative array containing:
 *   - scheme: The access scheme object.
 */
function theme_access_schemes_overview($variables) {
  $scheme = $variables['scheme'];

  $output = check_plain($scheme->name);
  $output .= ' <small>' . t('(Machine name: @type)', array('@type' => $scheme->scheme)) . '</small>';
  $output .= '<div class="description">' . filter_xss_admin($scheme->description) . '</div>';
  return $output;
}

// @todo
function access_scheme_form() {}

// @todo
function access_scheme_delete_confirm() {}
