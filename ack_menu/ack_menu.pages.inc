<?php

/**
 * @file
 * Page callbacks for managing menu links in assigned realms.
 */

/**
 * Menu page callback to list the realms wherein the user may manage menu links.
 *
 * @return array
 *   A renderable array.
 */
function ack_menu_overview_page() {
  $schemes = access_object_schemes('menu_link');
  $realms = array();
  // Global menu administrators can see all realms.
  if (user_access('administer menu')) {
    foreach ($schemes as $scheme) {
      $realms[$scheme->machine_name] = array_keys($scheme->realms);
    }
  }
  else {
    // Everyone else sees only the realms they are allowed to manage.
    $scheme_realms = access_user_permission_realms('ack manage menu links', NULL, $schemes);
    foreach ($schemes as $scheme) {
      foreach ($scheme_realms[$scheme->machine_name] as $realm) {
        if (ack_menu_access($scheme, $realm)) {
          $realms[$scheme->machine_name][] = $realm;
        }
      }
    }
  }

  $number_of_user_schemes = count($realms);
  $render = array();
  foreach ($schemes as $scheme) {
    if (!empty($realms[$scheme->machine_name])) {
      $label = check_plain($scheme->name);
      if ($number_of_user_schemes > 1) {
        $render[$scheme->machine_name]['header'] = array(
          '#type' => 'markup',
          '#markup' => '<h2>' . $label . '</h2>',
        );
      }
      $header = array(
        $label,
        array(
          'data' => t('Operations'),
          'colspan' => 2,
        ),
      );
      $rows = array();
      foreach ($realms[$scheme->machine_name] as $realm) {
        $row = array();
        $row[] = check_plain($scheme->realms[$realm]);
        $row[] = array('data' => l(t('list links'), 'ack_menu/manage/' . $scheme->machine_name . '/' . $realm));
        $row[] = array('data' => l(t('add link'), 'ack_menu/manage/' . $scheme->machine_name . '/' . $realm . '/add'));
        $rows[] = $row;
      }
      $render[$scheme->machine_name]['table'] = array(
        '#theme' => 'table',
        '#header' => $header,
        '#rows' => $rows,
      );
    }
  }

  if (empty($render)) {
    if (user_access('administer menu')) {
      $empty = t('No access schemes have been configured to manage menu links. To configure the menu, use the <a href="@url">menu administration page</a>.', array('@url' => url('admin/structure/menu')));
      if (user_access('administer access schemes')) {
        $empty .= ' ' . t('To configure an access scheme to manage the menu, use the <a href="@url">access scheme administration page</a> to add a menu access handler to a scheme.', array('@url' => url('admin/structure/access')));
      }
    }
    else {
      $empty = t('You have not been granted access to any menu trees.');
    }
    $render['empty'] = array(
      '#type' => 'markup',
      '#markup' => $empty,
    );
  }

  return $render;
}

/**
 * Form for editing the menu tree for a realm.
 *
 * Shows all of the menu links for the given realm and relevant operations.
 *
 * @param object $scheme
 *   An access scheme.
 * @param int $realm
 *   A realm value.
 *
 * @return array
 *   A form array.
 *
 * @see ack_menu_overview_form_submit()
 */
function ack_menu_overview_form($form, &$form_state, $scheme, $realm) {
  global $menu_admin;
  module_load_include('inc', 'menu', 'menu.admin');
  $form['#attached']['css'] = array(drupal_get_path('module', 'menu') . '/menu.css');
  $form_state['scheme'] = $scheme;
  $form_state['realm'] = $realm;

  if (isset($scheme->handlers['menu_link'])) {
    $handler = $scheme->handlers['menu_link'];
    $links = $handler->realmLinks($realm);
    if (!empty($links)) {
      $tree = menu_tree_data($links);
      $count = count($tree);

      // Loop through the links at the top level of the tree.
      foreach ($tree as $data) {
        $item = $data['link'];
        // Don't show callbacks; these have $item['hidden'] < 0.
        if ($item && $item['hidden'] >= 0) {
          _menu_link_translate($item);
          $element = array(
            '#type' => $count > 1 ? 'fieldset' : 'container',
            '#tree' => TRUE,
          );
          if ($count > 1) {
            $element['#title'] = l($item['title'], $item['href'], $item['localized_options']);
          }

          $destination = drupal_get_destination();
          $element['link'] = array(
            '#type' => 'link',
            '#title' => t('Edit the parent link'),
            '#href' => 'admin/structure/menu/item/' . $item['mlid'] . '/edit',
            '#prefix' => '<ul class="action-links"><li>',
            '#suffix' => '</li></ul>',
            '#options' => array(
              'query' => $destination,
            ),
          );
          $element['depth'] = array('#type' => 'value', '#value' => $item['depth']);
          if (!empty($data['below'])) {
            // Build the subtree form in the same way as menu_overview_form().
            $node_links = array();
            menu_tree_collect_node_links($data['below'], $node_links);
            $menu_admin = TRUE;
            menu_tree_check_access($data['below'], $node_links);
            $menu_admin = FALSE;
            drupal_static_reset('_menu_overview_tree_form');
            $element['subtree'] = _menu_overview_tree_form($data['below']);
            $element['subtree']['#theme'] = 'menu_overview_form';
            foreach (element_children($element['subtree']) as $key) {
              // Adjust depth so links cannot be moved above the subtree parent.
              if (isset($element['subtree'][$key]['#item'])) {
                $element['subtree'][$key]['#item']['depth'] -= $item['depth'];
              }
              // Set the redirect destination on the edit/delete/reset links.
              foreach (element_children($element['subtree'][$key]['operations']) as $op) {
                $element['subtree'][$key]['operations'][$op]['#options']['query'] = $destination;
              }
            }
          }

          $form[$item['mlid']] = $element;
        }
      }
    }
  }

  if (element_children($form)) {
    $form['actions'] = array('#type' => 'actions');
    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save configuration'),
    );
    $form['actions']['cancel'] = array(
      '#type' => 'link',
      '#title' => t('Cancel'),
      '#href' => 'ack_menu',
    );
  }
  else {
    $form['empty'] = array(
      '#type' => 'markup',
      '#markup' => t('There are no menu links yet for @realm. <a href="@link">Add a link</a>.', array('@realm' => $scheme->realms[$realm], '@link' => url('ack_menu/manage/' . $scheme->machine_name . '/' . $realm . '/add'))),
    );
  }
  return $form;
}

/**
 * Submit handler for editing the menu tree for a realm.
 *
 * @see ack_menu_overview_form()
 */
function ack_menu_overview_form_submit($form, &$form_state) {
  foreach (element_children($form) as $mlid) {
    // Save the descendant links.
    if (isset($form[$mlid]['subtree'])) {
      // Undo the depth adjustment from ack_menu_overview_form().
      $depth = $form_state['values'][$mlid]['depth'];
      $subtree = &$form[$mlid]['subtree'];
      foreach (element_children($subtree) as $key) {
        if (isset($subtree[$key]['#item'])) {
          $subtree[$key]['#item']['depth'] += $depth;
        }
      }
      // Prepare a faux $form_state array that represents the subtree in the
      // format expected by menu_overview_form_submit().
      $subtree_form_state = array(
        'input' => $form_state['input'][$mlid]['subtree'],
        'values' => $form_state['values'][$mlid]['subtree'],
      );
      menu_overview_form_submit($subtree, $subtree_form_state);
    }
  }
}

/**
 * Menu page callback to add a menu link to a realm.
 *
 * @param object $scheme
 *   An access scheme.
 * @param int $realm
 *   A realm value.
 *
 * @return array
 *   A renderable form.
 */
function ack_menu_link_add($scheme, $realm) {
  module_load_include('inc', 'menu', 'menu.admin');
  $handler = $scheme->handlers['menu_link'];
  $menu_name = $handler->realmMenu($realm);
  $menu = empty($menu_name) ? menu_load('navigation') : menu_load($menu_name);
  $form_state = array(
    'ack_menu' => array(
      'schemes' => array($scheme),
      'realms' => array($scheme->machine_name => array($realm)),
      'destination' => 'ack_menu/manage/' . $scheme->machine_name . '/' . $realm,
    ),
    'build_info' => array(
      'args' => array(
        'add',
        NULL,
        $menu,
      ),
    ),
  );
  return drupal_build_form('menu_edit_item', $form_state);
}
