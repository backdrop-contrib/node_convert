<?php

/**
 * @file
 * Administration page callbacks for the node_convert module.
 */

/* ------------- Template adding, viewing, deleting. ----------------- */

/**
 * List of templates page callback.
 */
function node_convert_templates() {
  $rows = array();
  $headers = array(
    t("Name"),
    t("Source type"),
    t("Dest type"),
    t("Operations"),
  );
  $templates = node_convert_load_all_templates();
  foreach ($templates as $row) {
    $name = theme('label_machine_name__node_convert', array(
      'label' => $row['name'],
      'machine_name' => $row['machine_name'],
    ));
    $operations = array();
    if (!empty($row['machine_name'])) {
      $operations['edit'] = array(
        'title' => t("Edit"),
        'href' => 'admin/structure/node_convert_templates/edit/' . $row['machine_name'],
      );
      $operations['delete'] = array(
        'title' => t("Delete"),
        'href' => 'admin/structure/node_convert_templates/delete/' . $row['machine_name'],
      );
      $operations['view'] = array(
        'title' => t("View"),
        'href' => 'admin/structure/node_convert_templates/' . $row['machine_name'],
      );
    }
    $rows[] = array(
      $name,
      $row['source_type'],
      $row['destination_type'],
      array(
        'data' => array(
          '#type' => 'dropbutton',
          '#links' => $operations,
        ),
      ),
    );
  }
  $output = theme('table', array('header' => $headers, 'rows' => $rows));

  return $output;
}

/**
 * Template info page callback.
 */
function node_convert_template_info($machine_name) {
  $output = '';
  $rows = array();
  $headers = array(t("Property"), t("Value"));
  $row = node_convert_load_template($machine_name);
  $template_machine_name = isset($row['machine_name']) ? $row['machine_name'] : t('In Code');
  $rows[] = array(t("Name"), $row['name']);
  $rows[] = array(t("Machine name"), $row['machine_name']);
  $rows[] = array(t("Source type"), $row['source_type']);
  $rows[] = array(t("Destination type"), $row['destination_type']);
  $data = $row['data'];
  if ($data['no_fields'] == FALSE) {
    $source_fields_string = implode(', ', $data['fields']['source']);
    $dest_fields_string = implode(', ', $data['fields']['destination']);
    $rows[] = array(t("Source fields"), $source_fields_string);
    $rows[] = array(t("Destination fields"), $dest_fields_string);
  }
  if (!empty($data['hook_options'])) {
    $rows[] = array(t("Hook options"), print_r($data['hook_options'], TRUE));
  }
  $output .= theme('table', array('header' => $headers, 'rows' => $rows));

  return $output;
}

/**
 * Delete template confirmation form.
 */
function node_convert_template_delete_confirm($form, &$form_state, $template_machine_name) {
  $form['machine_name'] = array(
    '#type' => 'value',
    '#value' => $template_machine_name,
  );

  return confirm_form($form,
    t('Are you sure you want to delete this template?'),
    isset($_GET['destination']) ? $_GET['destination'] : 'admin/structure/node_convert_templates',
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Submit callback for delete template confirmation form.
 */
function node_convert_template_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $template = node_convert_load_template($form_state['values']['machine_name']);
    node_convert_delete_template($template);
  }
  $form_state['redirect'] = 'admin/structure/node_convert_templates';
}

/**
 * Add template page callback.
 */
function node_convert_add_template($form, &$form_state, $template = NULL) {
  $form = array();

  // @TODO Figure out how to let a user edit templates that are stored in code via features.
  $is_editing_mode = FALSE;
  if (!empty($template)) {
    $is_editing_mode = TRUE;
    $form_state['storage']['is_editing_mode'] = $is_editing_mode;
  }

  /* Setting the steps */
  if (!isset($form_state['values']['step'])) {
    $op = 'choose_destination_type';
  }
  elseif ($form_state['values']['step'] == 'choose_destination_type') {
    $op = 'choose_destination_fields';
  }
  $form['step'] = array(
    '#type' => 'value',
    '#value' => $op,
  );

  if ($op == 'choose_destination_type') {
    // Get available content types
    $to_types = node_convert_return_access_node_types('to');
    $from_types = node_convert_return_access_node_types('from');
    if ($to_types != FALSE && $from_types != FALSE) {
      $form['template_name'] = array(
        '#type' => 'textfield',
        '#title' => t("Template name"),
        '#required' => TRUE,
      );
      $form['machine_name'] = array(
        '#type' => 'machine_name',
        '#title' => t('Machine name'),
        '#machine_name' => array(
          'exists' => 'node_convert_machine_name_check',
          'source' => array('template_name'),
        ),
        '#required' => TRUE,
      );
      $form['source_type'] = array(
        '#type' => 'select',
        '#title' => t("Source type"),
        '#options' => $from_types,
      );
      $form['dest_type'] = array(
        '#type' => 'select',
        '#title' => t("Destination type"),
        '#options' => $to_types,
      );

      if ($is_editing_mode) {
        $form['template_name']['#default_value'] = $template['name'];
        $form['machine_name']['#default_value'] = $template['machine_name'];
        $form['machine_name']['#disabled'] = TRUE;
        $form['source_type']['#default_value'] = $template['source_type'];
        $form['dest_type']['#default_value'] = $template['destination_type'];

      }

    }
    else {
      $form['no_types'] = array(
        '#type' => 'markup',
        '#value' => t("You don't have access to any node types."),
      );
    }
  }
  elseif ($op == 'choose_destination_fields') {
    // Get the fields of the source type
    $source_fields = field_info_instances('node', $form_state['storage']['source_type']);
    $fields_info = field_info_fields();
    // In case there are no fields, just convert the node type
    if (count($source_fields) == 0) {
      $no_fields = TRUE;
    }
    else {
      $no_fields = FALSE;
      // Get the destination type fields
      $dest_fields = field_info_instances('node', $form_state['storage']['dest_type']);
      $i = 0;
      foreach ($source_fields as $source_field_name => $source_field) {
        ++$i;
        $options = array();
        $options['discard'] = 'discard';
        $options[APPEND_TO_BODY] = t('Append to body');
        $options[REPLACE_BODY] = t('Replace body');

        // Insert destination type fields into $options that are of the same type as the source.
        foreach ($dest_fields as $dest_field_name => $dest_field) {
          if ($fields_info[$source_field_name]['type'] == $fields_info[$dest_field_name]['type']
             || ($fields_info[$source_field_name]['type'] == 'text_with_summary' && $fields_info[$dest_field_name]['type'] == 'text_long')
             || ($fields_info[$source_field_name]['type'] == 'text_long' && $fields_info[$dest_field_name]['type'] == 'text_with_summary')
            ) {
            $options[$dest_field['field_name']] = $dest_field['field_name'];
          }
        }
        // Remember the source fields to be converted.
        $form['source_field_' . $i] = array(
          '#type' => 'value',
          '#value' => $source_field['field_name'],
        );
        // The select populated with possible destination fields for each
        // source field, If the destination node type has the same field as the
        // source node type, the default value is set to it.
        $form['dest_field_' . $i] = array(
          '#type' => 'select',
          '#options' => $options,
          '#default_value' => $source_field['field_name'],
          '#title' => (isset($source_field['label'])
                  ? $source_field['label'] . ' (' . $source_field['field_name'] . ')'
                  : $source_field['field_name'])
                  . ' ' . t("should be inserted into"),
        );

        if ($is_editing_mode) {
          // Populate the previous fields, only if the selected node types haven't changed from the original ones.
          $source_type = $form_state['values']['source_type'];
          $destination_type = $form_state['values']['dest_type'];
          if ($source_type == $template['source_type'] && $destination_type == $template['destination_type']) {
            $form['dest_field_' . $i]['#default_value'] = $template['data']['fields']['destination'][$i - 1];
          }
        }

      }
      $form['number_of_fields'] = array(
        '#type' => 'value',
        '#value' => $i,
      );
    }
    $form['no_fields'] = array(
      '#type' => 'value',
      '#value' => $no_fields,
    );

    // All node specific form options needed for types like book, forum, etc. are done here
    $hook_options = node_convert_invoke_all('node_convert_change', array('dest_node_type' => $form_state['storage']['dest_type']), 'options');
    if (!empty($hook_options)) {
      $form['hook_options'] = $hook_options;
      array_unshift($form['hook_options'], array('#value' => '<strong>' . t("Also the following parameters are available:") . '</strong>'));
      $form['hook_options']['#tree'] = TRUE;
    }
  }

  if ($op == 'choose_destination_type' && $to_types != FALSE && $from_types != FALSE) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t("Next"),
    );
  }
  elseif ($op == "choose_destination_fields") {
    $submit_label = $is_editing_mode ? t('Update') : t('Create');
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => $submit_label,
      '#weight' => 100,
    );
  }

  return $form;
}

/**
 * Validation callback for adding a new template.
 */
function node_convert_add_template_validate($form, &$form_state) {
  if ($form_state['values']['step'] == 'choose_destination_type') {
    if ($form_state['values']['source_type'] == $form_state['values']['dest_type']) {
      form_set_error('source_type', t('Please select different node types.'));
      form_set_error('dest_type', t('Please select different node types.'));
    }
  }
  // All node specific form validations needed for types like book, forum, etc. are done here
  elseif ($form_state['values']['step'] == 'choose_destination_fields') {
    node_convert_invoke_all('node_convert_change', array('dest_node_type' => $form_state['storage']['dest_type'], 'form_state' => $form_state), 'options validate');
  }
}

/**
 * Submit callback for adding a new template.
 */
function node_convert_add_template_submit($form, &$form_state) {
  if ($form_state['values']['step'] == 'choose_destination_type') {
    $form_state['rebuild'] = TRUE;
    $form_state['storage']['template_name'] = $form_state['values']['template_name'];
    $form_state['storage']['machine_name'] = $form_state['values']['machine_name'];
    $form_state['storage']['source_type'] = $form_state['values']['source_type'];
    $form_state['storage']['dest_type'] = $form_state['values']['dest_type'];
  }
  elseif ($form_state['values']['step'] == 'choose_destination_fields') {
    $no_fields = $form_state['values']['no_fields'];
    // If there are fields that can to be converted
    if ($no_fields == FALSE) {
      for ($i = 1; $i <= $form_state['values']['number_of_fields']; $i++) {
        $source_fields[] = $form_state['values']['source_field_' . $i]; //  Source fields
        $dest_fields[] = $form_state['values']['dest_field_' . $i]; // Destination fields
      }
    }

    if (!empty($form['hook_options'])) {
      $hook_options = $form_state['values']['hook_options'];
    }
    else {
      $hook_options = NULL;
    }

    $fields = array(
      'source' => $source_fields,
      'destination' => $dest_fields,
    );
    $data = array(
      'fields' => $fields,
      'hook_options' => $hook_options,
      'no_fields' => $no_fields,
    );
    $data = serialize($data);

    $is_editing_mode = isset($form_state['storage']['is_editing_mode']);
    $template = node_convert_save_template(
      $form_state['storage']['template_name'],
      $form_state['storage']['machine_name'],
      $form_state['storage']['source_type'],
      $form_state['storage']['dest_type'],
      $data
    );

    if ($is_editing_mode) {
      backdrop_set_message(t("Template updated successfully."));
    }
    else {
      backdrop_set_message(t("Template created successfully."));
    }

    // We clear the storage so redirect works
    $form_state['storage'] = array();
    $form_state['redirect'] = 'admin/structure/node_convert_templates';
  }
}
