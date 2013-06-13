<?php

/**
 * @file
 * Contains the handler class for the taxonomy term node index.
 */

/**
 * Controls access to a node based on its relationship to a term in the index.
 */
class ACKNodeTaxonomyIndex extends AccessControlKitHandler {

  /**
   * The machine name of the vocabulary that defines the realms.
   *
   * @var string
   */
  protected $vocabulary;

  /**
   * Overrides AccessControlKitHandler::__construct().
   */
  public function __construct($scheme, array $settings = array()) {
    parent::__construct($scheme, $settings);
    // Get the vocabulary from the realm settings.
    $this->vocabulary = isset($scheme->realm_field['settings']['arguments']['vocabulary']) ? $scheme->realm_field['settings']['arguments']['vocabulary'] : NULL;
  }

  /**
   * Overrides AccessControlKitHandler::description().
   */
  public function description() {
    return t('Content will be considered a part of an access realm if it is tagged with that realm in any available taxonomy term reference field.');
  }

  /**
   * Overrides AccessControlKitHandler::objectRealms().
   */
  public function objectRealms($object_type, $node) {
    $query = db_select('taxonomy_index', 'i');
    $query->join('taxonomy_term_data', 't', 'i.tid = t.tid');
    $query->join('taxonomy_vocabulary', 'v', 't.vid = v.vid');
    $query
      ->fields('i', array('tid'))
      ->condition('i.nid', $node->nid)
      ->condition('v.machine_name', $this->vocabulary);
    $result = $query->execute();
    return $result->fetchCol();
  }

  /**
   * Overrides AccessControlKitHandler::objectFormAlter().
   */
  public function objectFormAlter($object_type, $node, &$form, &$form_state, $form_id, $realms = NULL) {
    if (!empty($this->vocabulary)) {
      // Find all term reference fields that work with the realm vocabulary.
      $fields = field_read_fields(array('type' => 'taxonomy_term_reference'));
      foreach ($fields as $field_name => $field) {
        if (!empty($field['settings']['allowed_values']) && $field['settings']['allowed_values'][0]['vocabulary'] == $this->vocabulary) {
          // See if the field is attached to this node.
          if (!empty($form[$field_name])) {
            $language = $form[$field_name]['#language'];
            $element = &$form[$field_name][$language];

            // Lock the field if no realms are allowed.
            if (!isset($realms)) {
              $element['#disabled'] = TRUE;
            }
            // If the field widget uses the #options property, filter the options.
            elseif (isset($element['#options'])) {
              $options = array();

              // Preserve the empty option, if one was specified.
              if (isset($element['#options']['_none'])) {
                $options['_none'] = $element['#options']['_none'];
              }

              // Include only those options that correspond to allowed realms.
              $options += array_intersect_key($element['#options'], $realms);

              // If the element is required and only one option remains besides the
              // empty value, then remove the empty value as an option.
              if (!empty($element['#required']) && isset($options['_none']) && count($options) == 2) {
                unset($options['_none']);
              }

              $element['#options'] = $options;
              // If only one option remains, select it for the user.
              if (count($element['#options']) == 1) {
                $element['#disabled'] = TRUE;
                $element['#default_value'] = key($element['#options']);
              }
            }
          }
        }
      }
    }
  }
}
