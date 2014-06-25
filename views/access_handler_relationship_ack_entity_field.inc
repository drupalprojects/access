<?php

/**
 * @file
 * Definition of access_handler_relationship_ack_entity_field.
 */

/**
 * Relates an entity to access grants through a field.
 *
 * @ingroup views_relationship_handlers
 */
class access_handler_relationship_ack_entity_field extends views_handler_relationship {

  /**
   * Overrides views_handler_relationship::query().
   */
  public function query() {
    $this->ensure_my_table();
    $def = $this->definition;
    $type = empty($this->options['required']) ? 'LEFT' : 'INNER';

    // Join first from the entity to its access-managed field.
    $first = array(
      'left_table' => $this->table_alias,
      'left_field' => $def['entity field'],
      'table' => $def['field table'],
      'field' => 'entity_id',
      'type' => $type,
    );
    if (!empty($def['join_extra'])) {
      $first['extra'] = $def['join_extra'];
    }
    $first_join = new views_join();
    $first_join->definition = $first;
    $first_join->construct();
    $first_join->adjusted = TRUE;
    $first_alias = $this->query->add_table($def['field table'], $this->relationship, $first_join);

    // Next, join from the entity field to the realm field.
    $second = array(
      'left_table' => $first_alias,
      'left_field' => $def['field field'],
      'table' => $def['realm table'],
      'field' => $def['realm field'],
      'type' => $type,
      'extra' => array(
        array(
          'field' => 'deleted',
          'value' => 0,
          'numeric' => TRUE,
        ),
      ),
    );
    $second_join = new views_join();
    $second_join->definition = $second;
    $second_join->construct();
    $second_join->adjusted = TRUE;
    $second_alias = $this->query->add_table($def['realm table'], $this->relationship, $second_join);

    // Finally, join from the realm field to the access grant.
    $third = array(
      'left_table' => $second_alias,
      'left_field' => 'entity_id',
      'table' => 'access_grant',
      'field' => 'gid',
      'type' => $type,
    );
    $third_join = new views_join();
    $third_join->definition = $third;
    $third_join->construct();
    $third_join->adjusted = TRUE;
    // Use a short alias for the final join.
    $alias = 'ack_' . $def['scheme'] . '_' . $this->table;

    $this->alias = $this->query->add_relationship($alias, $third_join, $def['base'], $this->relationship);
  }

}
