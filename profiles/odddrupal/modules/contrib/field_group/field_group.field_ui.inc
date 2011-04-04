<?php

/**
 * @file
 * Field_group.field_ui.inc is a file that contains most functions
 * needed on the Fields UI Manage forms (display and fields).
 */

/**
 * Helper function to get the form parameters to use while
 * building the fields and display overview form.
 */
function field_group_field_ui_form_params($form, $display_overview) {

  $params = new stdClass();
  $params->entity_type = $form['#entity_type'];
  $params->bundle = $form['#bundle'];
  $params->admin_path = _field_ui_bundle_admin_path($params->entity_type, $params->bundle);
  $params->display_overview = $display_overview;

  $formatters = field_group_formatter_info();

  if ($display_overview) {
    $params->region_callback = 'field_group_display_overview_row_region';
    $params->mode = $form['#view_mode'];
  }
  else {
    $params->region_callback = 'field_group_field_overview_row_region';
    $params->mode = 'form';
  }

  $params->groups = field_group_info_groups($params->entity_type, $params->bundle, $params->mode, TRUE);

  // Gather parenting data.
  $params->parents = array();
  foreach ($params->groups as $name => $group) {
    foreach ($group->children as $child) {
      $params->parents[$child] = $name;
    }
  }

  return $params;
}

/**
 * Function to alter the fields overview and display overview screen.
 */
function field_group_field_ui_overview_form_alter(&$form, &$form_state, $display_overview = FALSE) {

  // Add extra guidelines for webmaster.
  $form['#prefix'] = (isset($form['#prefix']) ? $form['#prefix'] : '') . t('<p class="fieldgroup-help">Fields can be dragged into groups with unlimited nesting. Each fieldgroup format comes with a configuration form, specific for that format type.<br />Note that some formats come in pair. These types have a html wrapper to nest its fieldgroup children. E.g. Place accordion items into the accordion, vertical tabs in vertical tab group and horizontal tabs in the horizontal tab group. There is one exception to this rule, you can use a vertical tab without a wrapper when the additional settings tabs are available. E.g. node forms.</p>');

  // Only start altering the form if we need to.
  if (empty($form['#fields']) && empty($form['#extra'])) {
    return;
  }

  $params = field_group_field_ui_form_params($form, $display_overview);

  // Add some things to be able to preserve synced usage of field_ui.
  if (!$display_overview) {
    // This key is used to store the current updated field.
    $form_state += array(
      'formatter_settings_edit' => NULL,
    );
    // Add AJAX wrapper.
    $form['fields']['#prefix'] = '<div id="field-display-overview-wrapper">';
    $form['fields']['#suffix'] = '</div>';
  }
  $form['#groups'] = array_keys($params->groups);

  $table = &$form['fields'];

  // Add a region for 'add_new' rows, but only when fields are
  // available and thus regions.
  if (isset($table['#regions'])) {
    $table['#regions'] += array(
      'add_new' => array('title' => '&nbsp;'),
    );
  }

  // Extend available parenting options.
  foreach ($params->groups as $name => $group) {
    $table['#parent_options'][$name] = $group->label;
  }
  $table['#parent_options']['_add_new_group'] = t('Add new group');

  // Update existing rows accordingly to the parents.
  foreach (element_children($table) as $name) {
    $table[$name]['parent_wrapper']['parent']['#options'] = $table['#parent_options'];
    $table[$name]['parent_wrapper']['parent']['#default_value'] = isset($params->parents[$name]) ? $params->parents[$name] : '';
  }

  $formatter_options = field_group_field_formatter_options($display_overview ? 'display' : 'form');

  $refresh_rows = isset($form_state['values']['refresh_rows']) ? $form_state['values']['refresh_rows'] : (isset($form_state['input']['refresh_rows']) ? $form_state['input']['refresh_rows'] : NULL);

  // Create the group rows and check actions.
  foreach (array_keys($params->groups) as $name) {

    // Play around with form_state so we only need to hold things
    // between requests, until the save button was hit.
    if (isset($form_state['field_group'][$name])) {
      $group = & $form_state['field_group'][$name];
    }
    else {
      $group = & $params->groups[$name];
    }

    // Check the currently selected formatter, and merge persisted values for
    // formatter settings for the group.
    // This needs to be done first, so all fields are updated before creating form elements.
    if (isset($refresh_rows) && $refresh_rows == $name) {
      $settings = isset($form_state['values']['fields'][$name]) ? $form_state['values']['fields'][$name] : (isset($form_state['input']['fields'][$name]) ? $form_state['input']['fields'][$name] : NULL);
      if (array_key_exists('settings_edit', $settings)) {
        //$group->format_type = $form_state['field_group'][$name]->format_type;
        $group = $form_state['field_group'][$name];
      }
      field_group_formatter_row_update($group, $settings);
    }

    // Save the group when the configuration is submitted.
    if (!empty($form_state['values'][$name . '_formatter_settings_update'])) {
      field_group_formatter_settings_update($group, $form_state['values']['fields'][$name]);
    }
    // After all updates are finished, let the form_state know.
    $form_state['field_group'][$name] = $group;

    $settings = field_group_format_settings_form($group);

    $id = strtr($name, '_', '-');
    $js_rows_data[$id] = array('type' => 'group', 'name' => $name);
    // A group cannot be selected as its own parent.
    $parent_options = $table['#parent_options'];
    unset($parent_options[$name]);
    $table[$name] = array(
      '#attributes' => array('class' => array('draggable', 'field-group'), 'id' => $id),
      '#row_type' => 'group',
      '#region_callback' => $params->region_callback,
      '#js_settings' => array('rowHandler' => 'group'),
      'human_name' => array(
        '#markup' => check_plain(t($group->label)),
        '#prefix' => '<span class="group-label">',
        '#suffix' => '</span>',
      ),
      'weight' => array(
        '#type' => 'textfield',
        '#default_value' => $group->weight,
        '#size' => 3,
        '#attributes' => array('class' => array('field-weight')),
      ),
      'parent_wrapper' => array(
        'parent' => array(
          '#type' => 'select',
          '#options' =>  $parent_options,
          '#empty_value' => '',
          '#default_value' => isset($params->parents[$name]) ? $params->parents[$name] : '',
          '#attributes' => array('class' => array('field-parent')),
          '#parents' => array('fields', $name, 'parent'),
        ),
        'hidden_name' => array(
          '#type' => 'hidden',
          '#default_value' => $name,
          '#attributes' => array('class' => array('field-name')),
        ),
      ),
    );

    $table[$name] += array(
      'group_name' => array(
        '#markup' => check_plain($name),
      ),
      'format' => array(
        'type' => array(
          '#type' => 'select',
          '#options' => $formatter_options,
          '#default_value' => $group->format_type,
          '#attributes' => array('class' => array('field-group-type')),
        ),
      ),
    );

    $base_button = array(
      '#submit' => array('field_ui_display_overview_multistep_submit'),
      '#ajax' => array(
        'callback' => 'field_ui_display_overview_multistep_js',
        'wrapper' => 'field-display-overview-wrapper',
        'effect' => 'fade',
      ),
      '#field_name' => $name,
    );

    if ($form_state['formatter_settings_edit'] == $name) {
      $table[$name]['format']['#cell_attributes'] = array('colspan' => $display_overview ? 3 : 3);
      $table[$name]['format']['format_settings'] = array(
        '#type' => 'container',
        '#attributes' => array('class' => array('field-formatter-settings-edit-form')),
        '#parents' => array('fields', $name, 'format_settings'),
        '#weight' => -5,
        'label' => array(
          '#markup' => t('Field group format:') . ' <span class="formatter-name">' . $group->format_type . '</span>',
        ),
        // Create a settings form where hooks can pick in.
        'settings' => $settings,
        'actions' => array(
          '#type' => 'actions',
          'save_settings' => $base_button + array(
            '#type' => 'submit',
            '#name' => $name . '_formatter_settings_update',
            '#value' => t('Update'),
            '#op' => 'update',
          ),
          'cancel_settings' => $base_button + array(
            '#type' => 'submit',
            '#name' => $name . '_formatter_settings_cancel',
            '#value' => t('Cancel'),
            '#op' => 'cancel',
            // Do not check errors for the 'Cancel' button.
            '#limit_validation_errors' => array(),
          ),
        ),
      );
      $table[$name]['#attributes']['class'][] = 'field-formatter-settings-editing';
      $table[$name]['format']['type']['#attributes']['class'] = array('element-invisible');
    }
    else {
      // After saving, the settings are updated here aswell. First we create
      // the element for the table cell.
      $table[$name]['settings_summary'] = array('#markup' => '');
      if (!empty($group->format_settings)) {
        $table[$name]['settings_summary'] = field_group_format_settings_summary($name, $group);
      }
      // Add the configure button.
      $table[$name]['settings_edit'] = $base_button + array(
        '#type' => 'image_button',
        '#name' => $name . '_group_settings_edit',
        '#src' => 'misc/configure.png',
        '#attributes' => array('class' => array('field-formatter-settings-edit'), 'alt' => t('Edit')),
        '#op' => 'edit',
        // Do not check errors for the 'Edit' button.
        '#limit_validation_errors' => array(),
        '#prefix' => '<div class="field-formatter-settings-edit-wrapper">',
        '#suffix' => '</div>',
      );
      if ($display_overview) {
        $table[$name]['settings_edit']['#suffix'] .= l(t('delete'), $params->admin_path . '/groups/' . $name . '/delete/' . $params->mode);
      }
    }

    if (!$display_overview) {
      $table[$name] += array(
        'delete' => array(
          '#markup' => l(t('delete'), $params->admin_path . '/groups/' . $name . '/delete/form'),
        ),
      );
    }
  }

  // Additional row: add new group.
  $parent_options = $table['#parent_options'];
  unset($parent_options['_add_new_group']);
  $table['_add_new_group'] = field_group_add_row('_add_new_group', $parent_options, $params);

  $table['_add_new_group'] += array(
    'format' => array(
      'type' => array(
        '#type' => 'select',
        '#options' => $formatter_options,
        '#default_value' => 'fieldset',
      ),
    ),
    'settings_summary' => array(),
    'settings_edit' => array(),
  );

  if (!$display_overview) {
    // See field_ui.admin.inc for more details on refresh rows.
    $form['refresh_rows'] = array('#type' => 'hidden');
    $form['refresh'] = array(
      '#type' => 'submit',
      '#value' => t('Refresh'),
      '#op' => 'refresh_table',
      '#submit' => array('field_ui_display_overview_multistep_submit'),
      '#ajax' => array(
        'callback' => 'field_ui_display_overview_multistep_js',
        'wrapper' => 'field-display-overview-wrapper',
        'effect' => 'fade',
        // The button stays hidden, so we hide the AJAX spinner too. Ad-hoc
        // spinners will be added manually by the client-side script.
        'progress' => 'none',
      ),
    );
  }

  $form['#attached']['css'][] = drupal_get_path('module', 'field_group') . '/field_group.field_ui.css';
  $form['#attached']['js'][] = drupal_get_path('module', 'field_group') . '/field_group.field_ui.js';

  $form['#validate'][] = 'field_group_field_overview_validate';
  $form['#submit'][] = 'field_group_field_overview_submit';
}

/**
 * Return an array of field_group_formatter options.
 */
function field_group_field_formatter_options($type) {
  $options = &drupal_static(__FUNCTION__);

  if (!isset($options)) {
    $options = array();
    $field_group_types = field_group_formatter_info();
    foreach ($field_group_types[$type] as $name => $field_group_type) {
      $options[$name] = $field_group_type['label'];
    }
  }
  return $options;
}

/**
 * Helper function to add a row in the overview forms.
 */
function field_group_add_row($name, $parent_options, $params) {
  return array(
    '#attributes' => array('class' => array('draggable', 'field-group', 'add-new')),
    '#row_type' => 'add_new_group',
    '#js_settings' => array('rowHandler' => 'group'),
    '#region_callback' => $params->region_callback,
    'label' => array(
      '#type' => 'textfield',
      '#size' => 15,
      '#description' => t('Label'),
      '#prefix' => '<div class="label-input"><div class="add-new-placeholder">' . t('Add new group') . '</div>',
      '#suffix' => '</div>',
    ),
    'weight' => array(
      '#type' => 'textfield',
      '#default_value' => field_info_max_weight($params->entity_type, $params->bundle, $params->mode) + 3,
      '#size' => 3,
      '#title_display' => 'invisible',
      '#title' => t('Weight for new group'),
      '#attributes' => array('class' => array('field-weight')),
      '#prefix' => '<div class="add-new-placeholder">&nbsp;</div>',
    ),
    'parent_wrapper' => array(
      'parent' => array(
        '#type' => 'select',
        '#options' => $parent_options,
        '#empty_value' => '',
        '#attributes' => array('class' => array('field-parent')),
        '#prefix' => '<div class="add-new-placeholder">&nbsp;</div>',
        '#parents' => array('fields', $name, 'parent'),
      ),
      'hidden_name' => array(
        '#type' => 'hidden',
        '#default_value' => $name,
        '#attributes' => array('class' => array('field-name')),
      ),
    ),
    'group_name' => array(
      '#type' => 'textfield',
      // This field should stay LTR even for RTL languages.
      '#field_prefix' => '<span dir="ltr">group_',
      '#field_suffix' => '</span>&lrm;',
      '#attributes' => array('dir' => 'ltr'),
      '#size' => 15,
      '#description' => t('Group name (a-z, 0-9, _)'),
      '#prefix' => '<div class="add-new-placeholder">&nbsp;</div>',
      '#cell_attributes' => array('colspan' => $params->display_overview ? 1 : 2),
    ),
  );
}

/**
 * Creates a form for field_group formatters.
 * @param Object $group The FieldGroup object.
 */
function field_group_format_settings_form(&$group) {
  $form = array();
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Field group label'),
    '#default_value' => $group->label,
    '#weight' => -5
  );
  $form += module_invoke_all('field_group_format_settings', $group);
  return $form;
}

/**
 * Update the row so that the group variables are updated.
 * The rendering of the elements needs the updated defaults.
 * @param Object $group
 * @param array $settings
 */
function field_group_formatter_row_update(& $group, $settings) {
  // if the row has changed formatter type, update the group object
  if (!empty($settings['format']['type']) && $settings['format']['type'] != $group->format_type) {
    $group->format_type = $settings['format']['type'];
    field_group_formatter_settings_update($group, $settings);
  }
}

/**
 * Update handler for field_group configuration settings.
 * @param Object $group The group object
 * @param Array $settings Configuration settings
 */
function field_group_formatter_settings_update(& $group, $settings) {

  // Fetch the field_group formatter defaults.
  $field_group_types = field_group_formatter_info();
  $mode = $group->mode == 'form' ? 'form' : 'display';
  $formatter = $field_group_types[$mode][$group->format_type];

  // for format changes we load the defaults.
  if (empty($settings['format_settings']['settings'])) {
    $group->format_settings = array(
      'formatter' => $formatter['default_formatter'],
      'instance_settings' => $formatter['instance_settings']
    );
  }
  else {
    $group->format_type = $settings['format']['type'];
    $group->label = $settings['format_settings']['settings']['label'];
    $group->format_settings = $settings['format_settings']['settings'];
    // This is avoided now, cause of the form_state['field_group']
    // ctools_include('export');
    // ctools_export_crud_save('field_group', $group);
    // cache_clear_all('field_groups', 'cache_field');
  }
}

/**
 * Creates a summary for the field format configuration summary.
 * @param String $group_name The name of the group
 * @param Object $group The group object
 * @return Array ready to be rendered.
 */
function field_group_format_settings_summary($group_name, $group) {
  $summary = implode('<br />', module_invoke_all('field_group_format_summary', $group));
  return array(
    '#markup' => '<div class="field-formatter-summary">' . $summary . '</div>',
    '#cell_attributes' => array('class' => array('field-formatter-summary-cell')),
  );
}

/**
 * Returns the region to which a row in the 'Manage fields' screen belongs.
 * @param Array $row A field or field_group row
 * @return String the current region.
 */
function field_group_field_overview_row_region($row) {
  switch ($row['#row_type']) {
    case 'group':
      return 'main';
    case 'add_new_group':
      // If no input in 'label', assume the row has not been dragged out of the
      // 'add new' section.
      if (empty($row['label']['#value'])) {
        return 'add_new';
      }
      return 'main';
  }
}

/**
 * Returns the region to which a row in the 'Manage display' screen belongs.
 * @param Array $row A field or field_group row
 * @return String the current region.
 */
function field_group_display_overview_row_region($row) {
  switch ($row['#row_type']) {
    case 'group':
      return ($row['format']['type']['#value'] == 'hidden' ? 'hidden' : 'visible');
    case 'add_new_group':
      // If no input in 'label', assume the row has not been dragged out of the
      // 'add new' section.
      if (empty($row['label']['#value'])) {
        return 'add_new';
      }
      return ($row['format']['type']['#value'] == 'hidden' ? 'hidden' : 'visible');
  }
}

/**
 * Validate handler for the overview screens.
 * @param Array $form The complete form.
 * @param Array $form_state The state of the form.
 */
function field_group_field_overview_validate($form, &$form_state) {
  $form_values = $form_state['values']['fields'];
  $entity_type = $form['#entity_type'];
  $bundle = $form['#bundle'];
  $mode = (isset($form['#view_mode']) ? $form['#view_mode'] : 'form');

  $group = $form_values['_add_new_group'];

  // Validate if any information was provided in the 'add new group' row.
  if (array_filter(array($group['label'], $group['group_name']))) {
    // Missing label.
    if (!$group['label']) {
      form_set_error('fields][_add_new_group][label', t('Add new group: you need to provide a label.'));
    }

    // Missing group name.
    if (!$group['group_name']) {
      form_set_error('fields][_add_new_group][group_name', t('Add new group: you need to provide a group name.'));
    }
    // Group name validation.
    else {
      $group_name = $group['group_name'];

      // Add the 'group_' prefix.
      if (drupal_substr($group_name, 0, 6) != 'group_') {
        $group_name = 'group_' . $group_name;
        form_set_value($form['fields']['_add_new_group']['group_name'], $group_name, $form_state);
      }

      // Invalid group name.
      if (!preg_match('!^group_[a-z0-9_]+$!', $group_name)) {
        form_set_error('fields][_add_new_group][group_name', t('Add new group: the group name %group_name is invalid. The name must include only lowercase unaccentuated letters, numbers, and underscores.', array('%group_name' => $group_name)));
      }
      if (drupal_strlen($group_name) > 32) {
        form_set_error('fields][_add_new_group][group_name', t("Add new group: the group name %group_name is too long. The name is limited to 32 characters, including the 'group_' prefix.", array('%group_name' => $group_name)));
      }

      // Group name already exists.
      if (field_group_exists($group_name, $entity_type, $bundle, $mode)) {
        form_set_error('fields][_add_new_group][group_name', t('Add new group: the group name %group_name already exists.', array('%group_name' => $group_name)));
      }
    }
  }
}

/**
 * Submit handler for the overview screens.
 * @param Array $form The complete form.
 * @param Array $form_state The state of the form.
 */
function field_group_field_overview_submit($form, &$form_state) {

  $form_values = $form_state['values']['fields'];
  $entity_type = $form['#entity_type'];
  $bundle = $form['#bundle'];
  $mode = (isset($form['#view_mode']) ? $form['#view_mode'] : 'form');

  // Collect children.
  $children = array_fill_keys($form['#groups'], array());
  foreach ($form_values as $name => $value) {
    if (!empty($value['parent'])) {
      // Substitute newly added fields, in case they were dragged
      // directly in a group.
      if ($name == '_add_new_field' && isset($form_state['fields_added']['_add_new_field'])) {
        $name = $form_state['fields_added']['_add_new_field'];
      }
      elseif ($name == '_add_existing_field' && isset($form_state['fields_added']['_add_existing_field'])) {
        $name = $form_state['fields_added']['_add_existing_field'];
      }
      $children[$value['parent']][$name] = $name;
    }
  }

  // Prepare storage with ctools.
  ctools_include('export');

  // Create new group.
  if (!empty($form_values['_add_new_group']['group_name'])) {
    $values = $form_values['_add_new_group'];

    $field_group_types = field_group_formatter_info();
    $formatter = $field_group_types[($mode == 'form' ? 'form' : 'display')][$values['format']['type']];

    $new_group = (object) array(
      'group_name' => $values['group_name'],
      'entity_type' => $entity_type,
      'bundle' => $bundle,
      'mode' => $mode,
      'children' => isset($children['_add_new_group']) ? array_keys($children['_add_new_group']) : array(),
      'parent_name' => $values['parent'],
      'weight' => $values['weight'],
      'label' => $values['label'],
      'format_type' => $values['format']['type'],
    );
    $new_group->format_settings = array('formatter' => isset($formatter['default_formatter']) ? $formatter['default_formatter'] : '');
    if (isset($formatter['instance_settings'])) {
      $new_group->format_settings['instance_settings'] = $formatter['instance_settings'];
    }
    ctools_export_crud_save('field_group', $new_group);

    // Store new group information for any additional submit handlers.
    $form_state['groups_added']['_add_new_group'] = $new_group->group_name;
    drupal_set_message(t('New group %label successfully created.', array('%label' => $new_group->label)));

    // Replace the newly created group in the $children array, in case it was
    // dragged directly in an existing field.
    foreach (array_keys($children) as $parent) {
      if (isset($children[$parent]['_add_new_group'])) {
        unset($children[$parent]['_add_new_group']);
        $children[$parent][$new_group->group_name] = $new_group->group_name;
      }
    }
  }

  // Update existing groups.
  $groups = field_group_info_groups($entity_type, $bundle, $mode, TRUE);
  foreach ($form['#groups'] as $group_name) {
    $group = $groups[$group_name];
    $group->label = $form_state['field_group'][$group_name]->label;
    $group->children = array_keys($children[$group_name]);
    $group->parent_name = $form_values[$group_name]['parent'];
    $group->weight = $form_values[$group_name]['weight'];
    $group->format_type = isset($form_values[$group_name]['format']['type']) ? $form_values[$group_name]['format']['type'] : 'visible';
    if (isset($form_state['field_group'][$group_name]->format_settings)) {
      $group->format_settings = $form_state['field_group'][$group_name]->format_settings;
    }
    ctools_export_crud_save('field_group', $group);
  }

  cache_clear_all('field_groups', 'cache_field');
}

/**
 * Validate the entered css class from the submitted format settings.
 * @param Array $element The validated element
 * @param Array $form_state The state of the form.
 */
function field_group_validate_css_class($element, &$form_state) {
  if (!empty($form_state['values']['fields'][$form_state['formatter_settings_edit']]['format_settings']['settings']['instance_settings']['classes']) && !preg_match('!^[A-Za-z0-9-_ ]+$!', $form_state['values']['fields'][$form_state['formatter_settings_edit']]['format_settings']['settings']['instance_settings']['classes'])) {
    form_error($element, t('The css class must include only letters, numbers, underscores and dashes.'));
  }
}

/**
 * Implements hook_field_info_max_weight().
 */
function field_group_field_info_max_weight($entity_type, $bundle, $context) {
  $weights = array();
  foreach (field_group_info_groups($entity_type, $bundle, $context) as $group) {
    $weights[] = $group->weight;
  }
  return $weights ? max($weights) : NULL;
}

/**
 * Menu callback; present a form for removing a group.
 */
function field_group_delete_form($form, &$form_state, $group, $view_mode = 'form') {

  $form['#group'] = $group;
  $admin_path = _field_ui_bundle_admin_path($group->entity_type, $group->bundle);
  if ($view_mode == 'form') {
    $admin_path .= '/fields';
  }
  else {
    $admin_path .= '/display/' . $view_mode;
  }
  $form['#redirect'] = array($admin_path);
  $output = confirm_form($form,
    t('Are you sure you want to delete the group %group?', array('%group' => t($group->label))),
    $admin_path,
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel'),
    'confirm'
  );
  return $output;
}

/**
 * Remove group from bundle.
 *
 * @todo we'll have to reset all view mode settings - that will be fun :)
 */
function field_group_delete_form_submit($form, &$form_state) {

  $group = $form['#group'];
  $bundle = $group->bundle;
  $entity_type = $group->entity_type;
  $group->mode = $form_state['build_info']['args'][1];

  $bundles = field_info_bundles();
  $bundle_label = $bundles[$entity_type][$bundle]['label'];

  ctools_include('export');
  ctools_export_crud_delete('field_group', $group);

  drupal_set_message(t('The group %group has been deleted from the %type content type.', array('%group' => t($group->label), '%type' => $bundle_label)));

  // Redirect.
  $form_state['redirect'] = $form['#redirect'];
}

/**
 * eof().
 */