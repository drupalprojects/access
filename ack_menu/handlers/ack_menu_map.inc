<?php

/**
 * @file
 * Contains the handler class for mapping menu links to access realms.
 */

/**
 * Controls access to menu links based on realm mapping.
 */
class AckMenuMap extends AccessControlKitHandler implements AckMenuHandlerInterface {

  /**
   * The machine name of the scheme to which this handler is attached.
   *
   * @var string
   */
  protected $schemeMachineName;

  /**
   * The human-readable name of the scheme to which this handler is attached.
   *
   * @var string
   */
  protected $schemeName;

  /**
   * The list of realms in the handler's scheme.
   *
   * @var array
   */
  protected $schemeRealms;

  /**
   * The names of the menus where this handler applies.
   *
   * @var array
   */
  protected $menus;

  /**
   * Finds the mapped links for a realm.
   *
   * @param int $realm
   *   A realm value.
   *
   * @return array
   *   An array of menu link IDs.
   */
  protected function mappedLinks($realm) {
    return db_query('SELECT mlid FROM {ack_menu_map} WHERE scheme = :scheme AND realm = :realm', array(':scheme' => $this->schemeMachineName, ':realm' => $realm))->fetchCol();
  }

  /**
   * Finds the mapped realm for a menu link.
   *
   * @param int $mlid
   *   A menu link ID.
   *
   * @return int|null
   *   The link's assigned realm value, or NULL if no mapping found.
   */
  protected function mappedRealm($mlid) {
    $realm = db_query('SELECT realm FROM {ack_menu_map} WHERE scheme = :scheme AND mlid = :mlid', array(':scheme' => $this->schemeMachineName, ':mlid' => $mlid))->fetchField();
    return empty($realm) ? NULL : $realm;
  }

  /**
   * Overrides AccessControlKitHandler::__construct().
   */
  public function __construct($scheme, array $settings = array()) {
    parent::__construct($scheme, $settings);
    $this->schemeMachineName = $scheme->machine_name;
    $this->schemeName = $scheme->name;
    $this->schemeRealms = isset($scheme->realms) ? $scheme->realms : array();
    // Make sure that the configured menus actually exist.
    $this->menus = array();
    if (isset($settings['menus'])) {
      $menus = menu_get_menus();
      foreach ($settings['menus'] as $menu_name) {
        if (isset($menus[$menu_name])) {
          $this->menus[] = $menu_name;
        }
      }
    }
  }

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    return t('Users with the <em>administer menus for all schemes</em> permission will be able to edit links in the allowed menus and assign realm memberships to them. Users who have been granted the <em>manage menu links in assigned access realms</em> permission for those realms will then be able to manage these links and their descendents.');
  }

  /**
   * Overrides AccessControlKitHandler::settingsForm().
   */
  public function settingsForm() {
    return array(
      'menus' => array(
        '#type' => 'checkboxes',
        '#title' => t('Allowed menus'),
        '#description' => t('The scheme will only manage links in the selected menus.'),
        '#options' => menu_get_menus(),
        '#default_value' => $this->menus,
      ),
    );
  }

  /**
   * Overrides AccessControlKitHandler::objectRealms().
   */
  public function objectRealms($object_type, $menu_link) {
    // See if this link is mapped to a realm.
    $realm = $this->mappedRealm($menu_link['mlid']);
    // If no mapping was found, try the link's parent.
    if (!isset($realm) && !empty($menu_link['plid'])) {
      $parent = menu_link_load($menu_link['plid']);
      if (!empty($parent)) {
        return $this->objectRealms($object_type, $parent);
      }
    }
    return isset($realm) ? array($realm) : array();
  }

  /**
   * Overrides AccessControlKitHandler::objectFormAlter().
   */
  public function objectFormAlter($object_type, $menu_link, &$form, &$form_state, $form_id, $realms = NULL) {
    $admin = !empty($form_state['ack_menu']['admin']);
    $global = !empty($form_state['ack_menu']['global admin']);

    // If the user isn't a global admin, we need to filter the parent options.
    if (!$global) {
      // If a realm list wasn't provided, get the link's current realm.
      if (!isset($realms) && !empty($menu_link['mlid'])) {
        $realms = $this->objectRealms($object_type, $menu_link);
      }
      // Only filter if we were able to identify a realm. Links without a realm
      // are skipped, so as not to remove options needed by other schemes.
      if (!empty($realms)) {
        $menus = $this->managedMenus();

        // Realm menu admins can place the link anywhere in the managed menus.
        if ($admin) {
          $allowed = empty($menus) ? array() : menu_parent_options($menus, $menu_link);
        }
        else {
          // For non-admins, get the form option keys for all eligible parents.
          $allowed = array();
          foreach ($realms as $realm) {
            foreach (array_keys($menus) as $menu_name) {
              foreach ($this->realmLinks($realm, $menu_name) as $link) {
                // Exclude links that are mapped to other realms.
                $link_realms = $this->objectRealms('menu_link', $link);
                if (in_array($realm, $link_realms)) {
                  $allowed[$link['menu_name'] . ':' . $link['mlid']] = TRUE;
                }
              }
            }
          }
        }
        // For an existing link, we need to keep the default value as an option.
        if (!empty($menu_link['mlid'])) {
          $allowed[$form['parent']['#default_value']] = TRUE;
        }

        // Remove options that aren't in our list.
        foreach (array_keys($form['parent']['#options']) as $option) {
          if (empty($allowed[$option])) {
            unset($form['parent']['#options'][$option]);
          }
        }

        // For new links, set the first available mapped link as the default;
        // if none is found, use the first allowed parent option.
        if (empty($menu_link['mlid'])) {
          $default = NULL;
          reset($realms);
          while (empty($default) && $realm = next($realms)) {
            $mapped_links = $this->mappedLinks($realm);
            while (empty($default) && $mlid = array_pop($mapped_links)) {
              $default = menu_link_load($mlid);
            }
          }
          $form['parent']['#default_value'] = empty($default) ? key($allowed) : $default['menu_name'] . ':' . $default['mlid'];
        }
      }
    }

    // Realm menu admins can map the link to a realm from the menu item form.
    if ($admin && $form_id == 'menu_edit_item') {
      if (!isset($form['AckMenuMap'])) {
        $form['AckMenuMap'] = array(
          '#type' => 'fieldset',
          '#title' => t('Access control kit'),
          '#description' => t('This link may be associated with a realm in each of the following access schemes. Doing so will allow users with permission to manage menu links in that realm to edit this link and any of its child links.'),
          '#tree' => TRUE,
        );
      }
      // For a new link, if we know what realm it should be in, set the default
      // mapping to that realm.
      if (empty($menu_link['mlid']) && !empty($realms) && count($realms) == 1) {
        $mapped = reset($realms);
        $locked = TRUE;
      }
      else {
        // Otherwise, check to see if the link is already mapped.
        $mapped = empty($menu_link['mlid']) ? NULL : $this->mappedRealm($menu_link['mlid']);
        $locked = FALSE;
      }
      $form['AckMenuMap'][$this->schemeMachineName] = array(
        '#type' => 'select',
        '#title' => check_plain($this->schemeName),
        '#options' => $this->schemeRealms,
        '#empty_option' => t('- Not mapped -'),
        '#default_value' => $mapped,
        '#disabled' => $locked,
      );
    }
  }

  /**
   * Overrides AccessControlKitHandler::objectFormSubmit().
   */
  public function objectFormSubmit($object_type, $menu_link, $form, &$form_state) {
    if (isset($form_state['values']['AckMenuMap'])) {
      $map = $form_state['values']['AckMenuMap'];
      if (isset($map[$this->schemeMachineName]) && is_numeric($map[$this->schemeMachineName])) {
        db_merge('ack_menu_map')
          ->key(array(
            'mlid' => $menu_link['mlid'],
            'scheme' => $this->schemeMachineName,
          ))
          ->fields(array(
            'realm' => $map[$this->schemeMachineName],
          ))
          ->execute();
      }
      else {
        db_delete('ack_menu_map')
          ->condition('mlid', $menu_link['mlid'])
          ->condition('scheme', $this->schemeMachineName)
          ->execute();
      }
    }
  }

  /**
   * Implements AckMenuHandlerInterface::realmLinks().
   */
  public function realmLinks($realm, $menu_name) {
    $mapped_links = $this->mappedLinks($realm);
    if (!empty($mapped_links) && !empty($this->menus) && in_array($menu_name, $this->menus)) {
      // This is similar to menu_overview_form() and _menu_build_tree().
      $query = db_select('menu_links', 'ml', array('fetch' => PDO::FETCH_ASSOC));
      $query->addTag('translatable');
      $query->leftJoin('menu_router', 'm', 'm.path = ml.router_path');
      $query->fields('ml');
      $query->fields('m', array(
        'load_functions',
        'to_arg_functions',
        'access_callback',
        'access_arguments',
        'page_callback',
        'page_arguments',
        'delivery_callback',
        'tab_parent',
        'tab_root',
        'title',
        'title_callback',
        'title_arguments',
        'theme_callback',
        'theme_arguments',
        'type',
        'description',
      ));
      $query->condition('ml.menu_name', $menu_name);
      // Fetch the mapped links and their descendants, sorted by depth.
      $or = db_or();
      for ($i = 1; $i <= MENU_MAX_DEPTH; $i++) {
        $p = 'ml.p' . $i;
        $query->orderBy($p, 'ASC');
        $or->condition($p, $mapped_links, 'IN');
      }
      $query->condition($or);
      return $query->execute()->fetchAll();
    }
    return array();
  }

  /**
   * Implements AckMenuHandlerInterface::realmMenu().
   */
  public function realmMenu($realm) {
    // If the realm is mapped, use the menu from the first mapped link we find.
    // Otherwise, use the first menu we find in the handler's menu list.
    $mapped_links = $this->mappedLinks($realm);
    do {
      $mlid = array_pop($mapped_links);
      $link = empty($mlid) ? NULL : menu_link_load($mlid);
    } while (isset($mlid) && empty($link));
    return empty($link) ? reset($this->menus) : $link['menu_name'];
  }

  /**
   * Implements AckMenuHandlerInterface::managedMenus().
   */
  public function managedMenus() {
    $names = menu_get_menus();
    $managed = array();
    foreach ($this->menus as $menu) {
      $managed[$menu] = $names[$menu];
    }
    return $managed;
  }

}
