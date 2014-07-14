<?php

/**
 * @file
 * Definition of access_handler_relationship_ack_entity_field.
 */

/**
 * Relates an entity to access grants through a field.
 *
 * Definition items:
 * - All definition items defined in views_handler_relationship.
 * - scheme: The machine name of the access scheme to which the grants belong.
 * - entity type: The type of entity to which we are relating access grants.
 * - entity table: The name of the entity field table.
 * - entity field: The column in the entity field table that contains the value.
 * - realm table: The name of the realm field table.
 * - realm field: The column in the realm field table that contains the value.
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
    $def['table'] = 'access_grant';

    // We use a subselect to get the grants table in order to filter by scheme.
    // Thus, the entity base table is the left and the subselect is the right.
    $def['left_table'] = $this->table_alias;
    $def['left_field'] = $def['field'];
    $def['field'] = 'entity_id';
    $def['type'] = empty($this->options['required']) ? 'LEFT' : 'INNER';

    $query = db_select('access_grant', 'ag');
    // Join the access grants table to the realm field table.
    $query->addJoin($def['type'], $def['realm table'], 'rt', 'ag.gid = rt.entity_id');
    // Join the realm field value to corresponding values in the entity field.
    $realm_condition = 'rt.' . $def['realm field'] . ' = ' . 'et.' . $def['entity field'];
    $query->addJoin($def['type'], $def['entity table'], 'et', $realm_condition);
    // Only select grants from the designated access scheme.
    $query->condition('ag.scheme', $def['scheme']);
    // Make sure the realm field values apply to the selected grants.
    $query->condition('rt.entity_type', 'access_grant');
    $query->condition('rt.deleted', 0);
    // Make sure the entity field values belong to the correct entity type.
    $query->condition('et.entity_type', $def['entity type']);
    $query->condition('et.deleted', 0);
    // We need all values from the grant, plus the ID of the related entity
    // obtained from the entity field table.
    $query->fields('ag');
    $query->fields('et', array('entity_id'));
    $def['table formula'] = $query;

    $join = new views_join();
    $join->definition = $def;
    $join->construct();
    $join->adjusted = TRUE;

    // Use a short alias for the subselect; for example, "access_grant_node".
    $alias = $def['table'] . '_' . $this->table;
    $this->alias = $this->query->add_relationship($alias, $join, 'access_grant', $this->relationship);
  }

}
