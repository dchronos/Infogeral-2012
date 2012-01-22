<?php

// $Id$

/**
 * @file
 *
 * This file contains all of the specific functions that
 * are needed to interact with media mover content connected
 * to nodes. These functions are called from hook_nodeapi()
 */


/* ************************************************** */
/* Configuration and action functions                 */
/* ************************************************** */


/**
 * Config config form for media mover
 * creates the options for each configuration instance
 * @param $action
 *   Object, complete action data
 * @return array, Drupal form array
 *
 */
function mm_node_publish_set_options_config($step) {
  $form['status']['complete_conditions'] = array(
    '#title' => t('Node publishing options'),
    '#type' => 'select',
    '#multiple' => TRUE,
    '#options' => mm_node_publish_options(),
    '#default_value' => $step->settings['complete_conditions'],
    '#description' => t('Set the publishing options for this node. <b>NOTE</b>:
      the settings you define here will override any previously set items for
      this node.'),
    );
  return $form;
}


/**
 * Configuration form for the storage option
 * @param $step
 *   Object, media mover step object
 * @return $form
 *   Array, Drupal form array
 */
function mm_node_save_config($step) {
  drupal_add_js(drupal_get_path('module', 'mm_node') . '/mm_node.js');
  $form['storage'] = array(
    '#type' => 'fieldset',
    '#title' => t('Node storage configuration'),
    '#collapsed' => FALSE,
    '#description' => t('This module will try to do a best guess of harvested data and save this to a node.'),
  );

  // Define the possible storage types
  $options = array();

  module_exists('upload') ? $options['upload--upload'] = t('Drupal file attachment') : NULL;
  // We can catch both emfield and cck content here
  if (module_exists('content')) {
    $options = array_merge($options, mm_node_cck_fields(array('text', 'filefield', 'imagefield', 'emvideo', 'emimage', 'emaudio')));
  }
  if ($options) {
    $form['storage']['storage_type'] = array(
      '#type' => 'select',
      '#title' => t('Save the file to'),
      '#description' => t('Select a place to store the file.'),
      '#options' => $options,
      '#default_value' => $step->settings['storage_type'],
    );
  }

  $form['storage']['new_node'] = array(
    '#type' => 'checkbox',
    '#title' => t('Create a new node'),
    '#description' => t('If this is selected, new nodes will always be created
      with the file. Otherwise if there is node data available the file will
      be saved to the existing node. Files with no node information will create
      new nodes with the below settings.'),
    '#default_value' => isset($step->settings['new_node']) ? $step->settings['new_node'] : FALSE,
  );

  $form['storage']['node'] = array(
    '#type' => 'fieldset',
    '#title' => t('Defaults for new nodes'),
    '#collapsed' => TRUE,
    '#collapsible' => true,
  );

  $form['storage']['node']['node_save_type'] = array(
    '#title' => t('Content type'),
    '#type' => 'select',
    '#options' => node_get_types('names'),
    '#default_value' => $step->settings['node_save_type'],
    '#description' => t('Save the file as new node of this type'),
  );

  $form['storage']['node']['node_save_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Default author'),
    '#maxlength' => 60,
    '#autocomplete_path' => 'user/autocomplete',
    '#default_value' => $step->settings['node_save_name'],
    '#description' => t('If a user is not passed in, this will be the default author. Leave blank for anonymous'),
    '#element_validate' => array('mm_node_save_config_validate'),
  );

  $form['storage']['node']['node_title_options'] = array(
    '#type' => 'radios',
    '#title' => t('Title options'),
    '#default_value' => $step->settings['node_title_options'] ? $step->settings['node_title_options'] : 'filename',
    '#options' => array(
      'filename' => t('Filename'),
      'default' => t('Use existing title if available'),
      'title' => t('Specify node title')
    ),
    '#description' => t('Chose how the title is set. "Filename" will use the name
      of the file as the title. "Use existing" will look for a title passed by
      a previous Media Mover action. If none is available, if you have specified a title, that
      will be used, otherwise the filename will be used. If you specify a node title, that
      will be used.'),
  );

  $form['storage']['node']['node_title_default'] = array(
    '#type' => 'textfield',
    '#title' => t('Default title'),
    '#maxlength' => 60,
    '#default_value' => $step->settings['node_title_default'],
    '#description' => t('Set the default title to this value.'),
    '#prefix' => '<div id="mm_node_title_default">',
    '#suffix' => '</div>',
  );

  $form['storage']['node']['node_body_default'] = array(
    '#type' => 'textarea',
    '#title' => t('Default body'),
    '#default_value' => $step->settings['node_body_default'] ? $step->settings['node_body_default'] : t('This was created without any additional information'),
    '#description' => t('If a body is not passed in, this will be the default node body.'),
  );

  // Language options
  if (function_exists('locale_language_list')) {
    $form['storage']['node']['node_save_language'] = array(
      '#type' => 'select',
      '#title' => t('Save node in language'),
      '#description' => t('Save the node in this language when creating it'),
      '#options' => locale_language_list(),
      '#default_value' => $step->settings['node_save_language'],
    );
  }

  $form['storage']['node']['node_publish_options'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Node publishing options'),
    '#options' => array(
      'comment' => t('Enable comments'),
      'status' => t('Published'),
      'promote' => t('Promoted to front page'),
      'sticky' => t('Sticky at the top of lists'),
    ),
    '#default_value' => isset($step->settings['node_publish_options']) ? $step->settings['node_publish_options'] : array(),
  );
  return $form;
}


/**
 * Validation duties for the node save configuration
 */
function mm_node_save_config_validate($element, &$form_state) {
  // Is the user valid?
  if (! $account = user_load(array('name' => $element['#value']))) {
    form_error($element,  t('Sorry, the name you chose for a default node author was not valid.'));
  }
  // Should validate fields against nodes

  // Should validate upload option agains nodes with attachments

}


/**
 * Get list of files that match assumed criteria
 *
 * @param array $configuration
 * @param array $file mm file array
 * @return array
 */
function mm_node_publish_set_options($step, $file) {
  // check and see if we have a NID
  if (! $node = $file->node_get()) {
    return FALSE;
  }

  // Load any CCK data to make sure that the node save does not break anything
  if (module_exists('content')) {
    content_load($node);
  }

  // Get the default publishing options
  foreach (mm_node_publish_options() as $name => $title) {
    if ($step->settings['complete_conditions'][$name]) {
      $node->{$name} = TRUE;
    }
    else {
      $node->{$name} = 0;
    }
  }

  // If revisions are enabled make a note in the logs
  if ($node->revision) {
    $node->log = t('New revision created by Media Mover');
  }
  node_save($node);
  return TRUE;
}


/**
 * Stores the actual data as a node
 *
 * @param array $file
 * @param array $configuration
 * @ TODO finish up this functionality to deploy
 *        requires creating the data store
 */
function mm_node_save($step, $file) {
  // Can we update an existing node?
  if (! $step->settings['new_node']) {
    $node = $file->node_get();
    if (module_exists('content')) {
      content_load($node);
    }
  }
  // Do we need to create a new node?
  if (! $node) {
    // start building the node
    $node = new stdClass();
    // set node type
    $node->type = $step->settings['node_save_type'];

    $node->is_new = TRUE;

    // Should we use the filename for the title?
    if ($step->settings['node_title_options'] == 'filename') {
      $node->title = basename($file->filepath);
    }
    // Should we use any node data if it exists on the file?
    elseif ($file->data['node']->title && ! $step->settings['node_title_options'] == 'default') {
      $node->title = $file->data['node']->title;
    }
    // Just use the default title
    else {
      $node->title = $step->settings['node_title_default'];
    }

    // Set the body
    if ($file->data['node']->body) {
      $node->body = $file->data['node']->body;
    }
    else {
      $node->body = $step->settings['node_body_default'];
    }

    // Set language options
    if ($step->settings['node_save_language']) {
      $node->language = $step->settings['node_save_language'];
    }

    // node options
    $node->comment = $step->settings['node_publish_options']['comment'] ? 2 : 0;
    $node->status = $step->settings['node_publish_options']['status'] ? 1 : 0;
    $node->sticky = $step->settings['node_publish_options']['sticky'] ? 1 : 0;
    $node->promote = $step->settings['node_publish_options']['promote'] ? 1 : 0;

    // Now build the node author
    $account = mm_node_map_user($file, $step);

    // attach the user data
    $node->uid = $account->uid;
    $node->name = $account->name;
    $node->mail = $account->mail;

    // Save the new node
    // Don't save the node here.  In case the file below can't be saved and we return false.
    //node_save($node);
  }

  // Now we have to handle the actual file storage
  if ($storage = $step->settings['storage_type']) {
    list($type, $kind) = explode('--', $storage);

    // Check for drupal uploads
    if ($kind == 'upload') {
      $new_file = mm_node_save_drupal_file($step, $file->filepath, $node->uid);
      $node->files[] = $new_file;
      // Now save the node with the updated data
      if ($node->is_new) {
        node_save($node);
      }
      upload_save($node);
      // Make sure we return the correct file path
      $filepath = $new_file->filepath;
      // Update the fid for this file
      $file->fid = $new_file->fid;
    }

    // Check for CCK fields
    else {
      // Get the field that we will save to.
      $field = content_fields($kind, $type);

      // Now what do we do with it?
      switch ($field['type']) {

        // Image and file field options
        case 'image':
        case 'filefield':
          module_load_include('inc', 'mm_node', 'includes/mm_node.content');
          if (! $cck_saved_file = mm_node_content_field_filefield($file, $field)) {
            return FALSE;
          }
          // Save the new filepath
          $file->filepath = $cck_saved_file['filepath'];

          // Attach saved file to node
          if (! $node->{$field['field_name']} || ! isset($node->{$field['field_name']})) {
            $node->{$field['field_name']} = array($cck_saved_file);
          }
          else {
            $node->{$field['field_name']}[] = $cck_saved_file;
          }
        break;

        // EMField options
        case 'emvideo':
        case 'emimage':
        case 'emaudio':
          // We have to have a provided file so emfield does not die.
          // @TODO PHP Stream Wrappers should address this in d7.
          if (! $item = mm_node_cck_emfield_parse_url($file, $field)) {
            watchdog('MM Emfield', 'Failed to parse %url for provider media to attach to the Embedded Media Field for !file.', array('%url' => $file->filepath, '!file' => l(t('MM file @file', array('@file' => $file->mmfid)),  'admin/media_mover/file/edit/'. $file->mmfid)), WATCHDOG_ERROR);
            return FALSE;
          }
          $cck_saved_file = array('embed' => $file->filepath);
          // Attach saved file to node
          if (! $node->{$field['field_name']} || ! isset($node->{$field['field_name']})) {
            $node->{$field['field_name']} = array($cck_saved_file);
          }
          else {
            $node->{$field['field_name']}[] = $cck_saved_file;
          }
        break;
      }

      // Now save the node with the updated data
      node_save($node);
    }
  }

  // Append the $nid to the $file for easy retrieval
  $file->nid = $node->nid;

  return $file->filepath;
}


/**
 * Removes references to files from files table where they were stored
 *
 * @ TODO this needs to be completed
 */
function mm_node_upload_file_delete($file) {
  if ($file->fid) {
    // Delete all files associated with the node
    db_query('DELETE FROM {files} WHERE fid = %d', $file->fid);
    file_delete($file->filepath);
  }

  // Delete the media mover file
  $file->delete();

}


/**
 * Generic form for node creation functions
 */
function mm_node_create_form() {
  // Node creation options
  $form['mm_emfield_storage']['node_create'] = array(
    '#type' => 'fieldset',
    '#title' => t('Node creation options'),
    '#collapsed' => isset($step->settings['mm_emfield_new_node']) ? FALSE : TRUE,
    '#collapsible' => TRUE,
    '#description' => t('When creating a new node, these options will be used'),
  );

  // Get content types.
  if ($types = node_get_types()) {
    foreach ($types as $type) {
      $options[$type->type] = $type->name;
    }
  }
  $form['mm_emfield_storage']['node_create']['mm_emfield_save_type'] = array(
    '#title' => t('Save node as type'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => $step->settings['mm_emfield_save_type'],
    '#description' => t('Save the harvested and processed data as this node type'),
  );
  $form['mm_emfield_storage']['node_create']['mm_emfield_save_author'] = array(
    '#type' => 'textfield',
    '#title' => t('Default author'),
    '#maxlength' => 60,
    '#autocomplete_path' => 'user/autocomplete',
    '#default_value' => isset($step->settings['mm_emfield_save_author']) ? $step->settings['mm_emfield_save_author']  : t('Anonymous'),
    '#description' => t('If a user is not passed in, this will be the default author.'),
  );
  $form['mm_emfield_storage']['node_create']['mm_emfield_title_default'] = array(
    '#type' => 'textfield',
    '#title' => t('Default title'),
    '#maxlength' => 60,
    '#default_value' => $step->settings['mm_emfield_title_default'],
    '#description' => t('If a title is not passed in, this will be the default node title. If left blank and there is no title information, the file name will be used.'),
  );
  $form['mm_emfield_storage']['node_create']['mm_emfield_body_default'] = array(
    '#type' => 'textarea',
    '#title' => t('Default body'),
    '#default_value' => $step->settings['mm_emfield_body_default'],
    '#description' => t('If a body is not passed in, this will be the default node body.'),
  );
  $form['mm_emfield_storage']['node_create']['mm_emfield_save_options'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Node options'),
    '#options' => array(
      'comment' => t('Enable comments'),
      'status' => t('Published'),
      'promote' => t('Promoted to front page'),
      'sticky' => t('Sticky at the top of lists'),
    ),
    '#default_value' => isset($step->settings['mm_emfield_save_options']) ? $step->settings['mm_emfield_save_options'] : array(),
  );

  // Language options.
  if (module_exists('locale')) {
    $options = locale_language_list();
    $form['mm_emfield_storage']['node_create']['mm_emfield_save_language'] = array(
      '#type' => 'select',
      '#title' => t('Save node in language'),
      '#description' => t('Save the node in this language when creating it'),
      '#options' => $options,
      '#default_value' => $step->settings['mm_emfield_save_language'],
    );
  }
  return $form;
}
