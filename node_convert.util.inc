<?php

/**
 * @file
 * API and Utility Functions.
 */

/**
 * Converts a node to another content type.
 *
 * @param $nid
 *   The nid of the node to be converted.
 * @param $dest_node_type
 *   A string containing the destination node type of the node.
 * @param $source_fields
 *   An array containing the source field names.
 * @param $dest_fields
 *   An array containing the destination field names.
 * @param $no_fields_flag
 *   A boolean containing if there are source fields that have to be converted.
 * @param $hook_options
 *   An array containing values used by the hook_node_convert_change functions.
 * @return bool Returns the $nid.
 */
function node_convert_node_convert($nid, $dest_node_type, $source_fields, $dest_fields, $no_fields_flag, $hook_options = NULL) {
  $node = node_load($nid);
  if ($node == FALSE) {
    return FALSE;
  }

  // Change the node type in the DB
  db_update('node')->fields(array('type' => $dest_node_type))->condition('nid', $nid)->execute();

  // If there are fields that can be converted
  if ($no_fields_flag == FALSE) {

    // Conversion process for each field
    $resave_body_field = FALSE;

    // Use node revisions to extract all field revision in node_convert_field_convert
    $node_revisions = node_revision_list($node);

    foreach ($source_fields as $key => $field) {
      $replaced_body = node_convert_field_convert($node, $field, $dest_fields[$key], $dest_node_type, $node_revisions);
      if ($replaced_body == REPLACE_BODY) {
        $resave_body_field = TRUE;
      }
    }
    // If something was appended to the body, or replaced the body, we update body field.
    if ($resave_body_field == TRUE) {
      $field_body = field_info_fields();
      $field_body = $field_body['body'];
      $field_ids = array($field_body['id'] => $field_body['id']);
      module_invoke($field_body['storage']['module'], 'field_storage_write', 'node', $node, FIELD_STORAGE_UPDATE, $field_ids);
    }
  }
  // We collate data to send to the hook implementations
  $data = array(
    'node' => $node,
    'dest_node_type' => $dest_node_type,
  );
  if (!empty($hook_options)) {
    $data['hook_options'] = $hook_options;
  }

  // We make sure that all custom node modules do their changes at the appropriate steps
  node_convert_load_includes();
  module_invoke_all('node_convert_change', $data, 'insert');
  module_invoke_all('node_convert_change', $data, 'delete');

  // TODO Check what to do about token cache clearing.
  /* if (module_exists('token')) {
    token_get_values('global', NULL, TRUE);
  }*/
  cache_clear_all('field:node:' . $node->nid, 'cache_field');
  $converted_node = node_load($nid, NULL, TRUE);
  $converted_node->type = $dest_node_type;
  try {
    node_save($converted_node);

    if (module_exists('rules')) {
      rules_invoke_event('node_convert_converted_node', $converted_node);
    }

  }
  catch (Exception $e) {
    drupal_set_message(t('Caught exception: @exception', array('@exception' => $e->getMessage())), 'error');
    $nid = FALSE;
  }

  return $nid;
}

/**
 * Transfers information from source_field to dest_field.
 *
 * @param $node
 *   The node object to be converted.
 * @param $source_field
 *   A string containing the name of the source field which contains to be transferred information.
 * @param $dest_field
 *   A string containing the name of the destination field where the information should be stored.
 * @param $dest_node_type
 *   A string containing the destination node type of the node.
 * @return
 *   Returns REPLACE_BODY if something replaced the body field, or was appended to it.
 */
function node_convert_field_convert(&$node, $source_field, $dest_field, $dest_node_type = '', $node_revisions = array()) {
  $field_info_source = field_info_fields(); // Get source field information
  $field_info_source = $field_info_source[$source_field];
  $db_info_source = $field_info_source['storage']; // Get DB specific source field information

  if ($dest_field == 'discard') {
    // Delete node info in the separate field table
    node_convert_invoke_field_storage_delete($field_info_source, $db_info_source, $node);
    return;
  }

  if (!in_array($dest_field, array('discard', APPEND_TO_BODY, REPLACE_BODY))) {
    $field_info_dest = field_info_fields($dest_field); // Get destination field information
    $field_info_dest = $field_info_dest[$dest_field]; // Get destination field information
    $db_info_dest = $field_info_dest['storage']; // Get DB specific destination field information
  }

  $source_values = array();
  // We save each field value from the DB for transfer. (this only applies to the current revision of the field)
  $source_values = field_get_items('node', $node, $source_field);

  if (count($node_revisions) > 1) {
    // Get all field revisions for current node
    $field_revision_values = array();
    $field_revision_source_table = current(array_keys($db_info_source['details']['sql']['FIELD_LOAD_REVISION']));
    $field_revision_destination_table = current(array_keys($db_info_dest['details']['sql']['FIELD_LOAD_REVISION']));

    $source_columns = array('entity_type', 'entity_id', 'revision_id', 'bundle', 'delta', 'language');
    foreach ($field_info_source['columns'] as $column => $attributes) {
      $source_columns[] = _field_sql_storage_columnname($source_field, $column);
    }

    $revision_query = db_select($field_revision_source_table, 'r', array('fetch' => PDO::FETCH_ASSOC))
      ->condition('entity_type', 'node')
      ->condition('bundle', $node->type)
      ->condition('entity_id', $node->nid)
      ->condition('revision_id', $node->vid, '<>')
      ->fields('r', $source_columns)->execute();

    // Change the bundle to the destination type of the node
    foreach ($revision_query as $row) {
      $row['bundle'] = $dest_node_type;
      $field_revision_values[] = $row;
    }

    // Remove all field revisions for current field in DB
    node_convert_invoke_field_storage_delete($field_info_source, $db_info_source, $node);

    // Reinsert the field revisions in the destination field revision table
    $query = db_insert($field_revision_destination_table);
    $columns = array('entity_type', 'entity_id', 'revision_id', 'bundle', 'delta', 'language');
    foreach ($field_info_dest['columns'] as $column => $attributes) {
      $columns[] = _field_sql_storage_columnname($dest_field, $column);
    }
    $query->fields($columns);
    foreach ($field_revision_values as $row) {
      $query->values(array_values($row));
    }
    $query->execute();

  }
  else {
    // After getting the source field values, we delete the values stored in the DB (this deletes values for all field revisions)
    node_convert_invoke_field_storage_delete($field_info_source, $db_info_source, $node);
  }

  // The source field value should be appended to the body or replaced.
  if ($dest_field == APPEND_TO_BODY || $dest_field == REPLACE_BODY) {
    static $node_body = '';
    //static $node_teaser = '';
    if (empty($node_body)) {
      $node_body_field = field_get_items('node', $node, 'body');
      $node_body = $node_body_field[0]['value'];
      //$node_teaser = $node_body_field[0]['summary'];
    }
    if (is_array($source_values)) {
      foreach ($source_values as $field_value) {
        if ($dest_field == APPEND_TO_BODY) {
          $node_body = $node_body . "\n" . $field_value['value'];
          //$node_teaser = $node_teaser . "\n" . $field_value['value'];
        }
        elseif ($dest_field == REPLACE_BODY) {
          $node_body = $field_value['value'];
          //$node_teaser = $field_value['value'];
        }
      }
      $langcode = field_language('node', $node, $source_field);
      $node->body[$langcode][0]['value'] = $node_body;
      //$node->body[$langcode][0]['summary'] = $node_teaser;
    }

    return REPLACE_BODY;
  }

  // We put each field value back into the DB
  // To do it we first get the id of the field, then we find its language code from the source value
  // We add $source_values into the node object, and invoke field_storage write
  $field_ids = array($field_info_dest['id'] => $field_info_dest['id']);
  $langcode = field_language('node', $node, $source_field);

  // Make sure that we actually have values in the source field
  if ($source_values !== FALSE) {
    $node->{$dest_field}[$langcode] = $source_values;
  }
  else {
    $node->{$dest_field} = array();
  }

  // Give possibility to fields to pre-process their data
  // (e.g. Link module transforms attribute array into a serialized array before insertion)
  field_attach_presave('node', $node);
  // For some reason link_field_presave doesn't exist anymore, so we have to call it the processing function used inside manually.
  if ($field_info_dest['type'] == 'link_field') {
    $instances = field_info_instances('node', $dest_node_type);
    link_field_update('node', $node, $field_info_dest, $instances[$dest_field], $langcode, $node->{$dest_field}[$langcode]);
  }
  try {
    module_invoke($db_info_dest['module'], 'field_storage_write', 'node', $node, FIELD_STORAGE_INSERT, $field_ids);
  } catch (Exception $e) {
    drupal_set_message(t('Caught exception: @exception', array('@exception' => $e->getMessage())));
  }
}

function node_convert_invoke_field_storage_delete($field_info_source, $db_info_source, $node) {
  $field_id = $field_info_source['id'];
  module_invoke($db_info_source['module'], 'field_storage_delete', 'node', $node, array($field_id => $field_id));
}

/**
 * Displays error messages if any occurred, otherwise the success message.
 *
 * @param $result
 *   The result value of the node conversion. Possible values
 *   - FALSE  Displays an error message.
 *   - Any other  Displays success message.
 * @param $params
 *   An array containing message parameters. Possible values
 *   - display_success  If TRUE, the success message will be displayed, otherwise no message is displayed.
 *   Default is TRUE.
 */
function node_convert_messages($result, $params = array()) {
  $params += array('display_success' => TRUE);

  $message_arguments = array('@nid' => $params['nid']);
  if ($result == FALSE) {
    $message = "Conversion failed for node nid @nid.";

    watchdog(NODE_CONVERT_WATCHDOG, $message, $message_arguments, WATCHDOG_ERROR);
    drupal_set_message(t($message, $message_arguments), 'error');
  }
  elseif ($params['display_success'] == TRUE) {
    $message = 'Node @nid has been converted successfully.';

    drupal_set_message(t($message, $message_arguments));
    watchdog(NODE_CONVERT_WATCHDOG, $message, $message_arguments, WATCHDOG_INFO);
  }

}

/**
 * Returns a string containing the value of the $field from the $node object.
 *
 * @param $node
 *   A $node object
 * @param $field
 *   The field who's value to get.
 * @return
 *   A string containing the value of the $field from the $node object.
 */
function node_convert_get_field_value($node, $field) {
  $value = '';

  /* @var $field_values Array*/
  $field_values = field_get_items('node', $node, $field['field_name']);
  if ($field['type'] == 'image') {
    $value = node_convert_format_field_value($field_values, 'File ID', 'File IDs', array('fid', 'title', 'uri'));
  }
  elseif ($field['type'] == 'link_field') {
    $value = node_convert_format_field_value($field_values, 'Link', 'Links', array('url', 'title'));
  }
  /* elseif ($field['type'] == "email") {
    $value = $field_values[0]['email'];
  }*/
  elseif ($field['type'] == 'file') {
    $value = node_convert_format_field_value($field_values, 'File ID', 'File IDs', array('fid', 'uri'));
  }
  elseif ($field['type'] == 'taxonomy_term_reference') {
    $value = node_convert_format_field_value($field_values, 'Term ID', 'Term IDs', array('tid'));
  }
  elseif ($field['type'] == 'node_reference') {
    $value = node_convert_format_field_value($field_values, 'Node ID', 'Node IDs', array('nid'));
  }
  elseif ($field['type'] == 'user_reference') {
    $value = node_convert_format_field_value($field_values, 'User ID', 'User IDs', array('uid'));
  }
  elseif ($field['type'] == 'entityreference') {
    $value = node_convert_format_field_value($field_values, 'Entity ID', 'Entity IDs', array('target_id'), 'Entity Type: ' . $field['settings']['target_type']);
  }
  else {
    if (!isset($field_values[0]['value'])) {
      $value = t("Unknown field format, can't display field value.");
    }
    else {
      $value = $field_values[0]['value'];
    }
  }
  if (empty($value)) {
    $value = 'NULL';
  }
  return $value;
}

/**
 * @param $field
 * @param $field_values
 * @return string
 */
function node_convert_format_field_value($field_values, $singular_label, $plural_label, $columns, $prefix_label = '') {
  $output = '';
  if (!empty($prefix_label)) {
    $output .= $prefix_label . ' ; ';
  }

  $count = count($field_values);
  $label = format_plural($count, $singular_label, $plural_label, array());

  $items = array();
  foreach ($field_values as $item) {
    $column_values = array();

    foreach ($columns as $column) {
      $column_values[] = $item[$column];
    }
    $items[] = implode(' ', $column_values);

  }
  $output .= $label . ': ' . implode(', ', $items);
  return $output;
}
