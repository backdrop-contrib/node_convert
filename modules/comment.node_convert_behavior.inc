<?php

/**
 * @file
 * Comment behavior.
 *
 * Updates the node type in the field_data_comment_body and field_revision_comment_body tables.
 */

/**
 * Implementation of node_convert_change().
 */
function comment_node_convert_change($data, $op) {
  if ($op == 'insert') {
    $node = $data['node'];
    $tables = array('field_data_comment_body', 'field_revision_comment_body');
    $prefix = '';

    global $databases;
    if (isset($databases['default']['default']['prefix'])) {
      $prefix = $databases['default']['default']['prefix'];
    }

    foreach ($tables as $table) {
      $query = '
        UPDATE ' . $prefix . $table . ' AS cb
        LEFT JOIN ' . $prefix . 'comment AS c ON cb.entity_id = c.cid AND cb.entity_type = :entity_type
        SET cb.bundle = :new_bundle
        WHERE c.nid = :nid
        ';
      $args = array(':entity_type' => 'comment', ':nid' => $node->nid, ':new_bundle' => 'comment_node_' . $data['dest_node_type']);
      db_query($query, $args);
    }
  }
}
