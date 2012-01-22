<?php

// $Id$

/**
 * @file
 *
 * Provides Media Mover services for CCK module
 */

/**
 * Return the configuration form for harvesting from CCK fields
 *
 * @pram $step
 *   object, media mover step
 * @return array, drupal form array
 */
function mm_node_content_field_select_config($step) {
  $form = array();
  $form['mm_node_content_field_select_config'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select Configuration'),
    '#description' => t('This module selects files attached to a CCK field'),
  );
  $form['mm_node_content_field_select_config']['harvest_field'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Select files from these CCK fields'),
    '#description' => t('Select the CCK fields to harvest files from.'),
    '#options' => mm_node_cck_fields(array('text', 'filefield', 'imagefield')),
    '#default_value' => isset($step->settings['harvest_field']) ? $step->settings['harvest_field'] : '',
  );
  $form['mm_node_content_field_select_config']['file_types'] = array(
    '#title' => t('File types'),
    '#type' => 'textfield',
    '#default_value' => isset($step->settings['file_types']) ? $step->settings['file_types'] : 'avi mov wmv mpeg mp4 mpeg2 dv 3gp 3g2 mpeg4',
    '#description' => t('Space separated list of file types that
      can be harvested. These are file extensions and are values
      like "avi mov wmv mpeg mp4 mpeg2 dv". Leave black for any
      file type.'),
  );
  $form['mm_node_content_field_select_config']['delete'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete when source is deleted'),
    '#description' => t('Delete the media mover file when the source file is deleted.'),
    '#default_value' => isset($step->settings['delete']) ? $step->settings['delete'] : FALSE,
  );
  return $form;
}


/**
 * Harvest files from a CCK field
 *
 * @param array $config
 *   the configuration array
 * @param array $running_config
 *   the full configuration object that is running
 * @param int $nid
 *   node id if a single node is being processed
 * @return array
 */
function mm_node_content_field_select($step) {
  $files = array();
  $fields = content_fields();

  $harvest_conditions = '';

  // Are we harvesting from a specific node?
  if ($node = $step->parameter_get('node')) {
    $harvest_conditions = ' AND n.nid = '.  $node->nid;
  }

  // We need to harvest from each field that was selected for that node tyoe
  if ($step->settings['harvest_field']) {
    foreach ($step->settings['harvest_field'] as $identifier) {
      // Extract the node type and field name
      list($node_type, $field_name) = explode('--', $identifier);

      // Get the relevant database information for this field
      $db_info = content_database_info($fields[$field_name]);

      // Change the query based on field type
      switch ($fields[$field_name]['type']) {
        // @TODO need to handle this
        case 'text':
          // $node->{$field['field_name']} = array(array('value' => $file['process_file']));
        break;

        // handle image field case
        case 'filefield':
        case 'image':
          // are file types being checked?
          if ($step->settings['file_types']) {
            $types = explode(' ', $step->settings['file_types'] );
            foreach ($types as $type) {
              $conditions[] = "f.filepath LIKE '%.$type'";
            }
            // build the SQL to check against the file types
            $file_type_conditions = ' ('. implode(' OR ', $conditions) .')';
          }

          $results = db_query('SELECT DISTINCT f.filepath AS filepath, c.nid, f.fid FROM {files} f
            LEFT JOIN {'. $db_info['table'] .'} c ON c.'. $db_info['columns']['fid']['column'] .' = f.fid
            LEFT JOIN {node} n ON c.nid = n.nid
            LEFT JOIN {media_mover_files} m ON m.fid = f.fid AND m.cid = "%s"
            WHERE
              ' . $file_type_conditions . '
              AND f.filepath NOT IN (SELECT source_filepath FROM {media_mover_files} WHERE cid ="%s")
              AND n.type = "%s"
            ORDER BY f.timestamp',
            $step->cid, $step->cid, $node_type);

          // now build the out going file data
          while ($result = db_fetch_array($results)) {
            $files[] = $result;
          }
        break;
      }
    }
  }
  return $files;
}


/**
 * Check to see if the specified file is on a node
 *
 * @TODO non filefield fields need to be thought about here
 *
 * @param $node
 * @param $step
 * @param $file
 * @param $found
 */
function mm_node_content_field_select_file_exists($node, $step, $mm_file, &$found) {
  // Find the field name
  $field = $step->settings['harvest_field'];
  if (isset($node->{$field})) {
    foreach ($node->{$field} as $file) {
      // We only look at the original selected file
      if ($mm_file->source_filepath == $file->filepath) {
        $found['found'] = TRUE;
        return;
      }
    }
    // We did not find the file, check and see if we should delete
    if ($step->settings['delete'] == FALSE) {
      $found['delete'] = FALSE;
    }
  }
}


/**
 * Return the configuration form for saving to CCK fields
 *
 * @param $step
 *   object, media mover step
 * @return array
 */
function mm_node_content_field_store_config($step) {
  $form = array();
  $form['mm_node_content_field_store_config'] = array(
    '#type' => 'fieldset',
    '#title' => t('CCK Save Configuration'),
    '#description' => t('This module saves data to a CCK field'),
    '#collapsed' => FALSE,
  );
  $form['mm_node_content_field_store_config']['cck_field_save_node'] = array(
    '#type' => 'select',
    '#title' => t('Save the file to this CCK field'),
    '#description' => t('Select the CCK field to save files to. There must
      be incoming node data for this to function correctly.
      <em>PLEASE NOTE:</em> if you are attaching a file to a field that has
      data already you need to either 1) ensure that this is a multiple value
      field 2) Use the <em>Replace existing file</em> option below that will
      replace the file- this assumes that the file you are attaching is
      a derivative of the file that is being replaced. If neither of these
      is the case it is likely that your configuration will fail on this
      step.'),
    '#options' => mm_node_cck_fields(array('text', 'filefield', 'imagefield')),
    '#default_value' => $step->settings['cck_field_save_node'],
  );
  $form['mm_node_content_field_store_config']['replace_existing'] = array(
    '#type' => 'checkbox',
    '#title' => t('Replace existing file'),
    '#default_value' => isset($step->settings['replace_existing']) ? $step->settings['replace_existing'] : FALSE,
    '#description' => t('WARNING: this is a destructive replacement! This will
      DELETE your source material. THERE IS NO UNDO. If you are at all concerned
      about your material, do not use this. Otherwise, this will happily replace
      the source material with the file from the previous Media Mover step and
      delete the source file.')
  );
  return $form;
}


/**
 * Save file to a CCK field
 *
 * @param array $step
 *   Object, media mover step
 * @param array $file
 *   media mover file object
 * @return string
 *   new file location
 */
function mm_node_content_field_store($step, $file) {
  // Find a node to use
  if (! $node = $step->parameter_get('node')) {
    if (! $node = $file->node_get()) {
      watchdog('mm_node', 'No node data was passed for storing file data in a specified CCK field.',
        array(), WATCHDOG_ERROR, l(t($file->mmfid),  'admin/media_mover/file/edit/'. $file->mmfid));
      return;
    }
    // Ensure that we have CCK data
    content_load($node);
  }

  // Get the field that we are using
  $field_name = explode('--', $step->settings['cck_field_save_node']);
  $field = content_fields($field_name[1], $node->type);

  switch ($field['type']) {
    case 'text':
      $item = array('value' => $file->file_path);
    break;

    // Handle CCK image and CCK file field cases
    case 'image':
    case 'filefield':
      // Replacing existing file on node

      if ($step->settings['replace_existing']) {
        // @TODO should probably make sure this FID exists on this node
        $data = array(
          'filepath' => $file->filepath,
          'fid' => $file->fid,
          'filename' => basename($file->filepath),
          'filemime' => file_get_mimetype($file->filepath),
          'filesize' => file_exists($file->filepath) ? filesize($file->filepath) : 0,
        );
        // Overwrite the existing file
        drupal_write_record('files', $data, 'fid');
        // Delete the source material
        $source_filepath = db_query("SELECT filepath FROM {files} WHERE fid = %d", $file->fid);
        if ($file->filepath != $source_filepath) {
          file_delete($source_file);
        }

        // @NOTE Update the $file->source_path so that this file doesn't get harvested again?

      }

      // Attaching file to node
      else {
        if (! $item = mm_node_content_field_filefield($file, $field)) {
          return;
        }
        // We need to alter the file object slightly so it will display correctly
        $cck_file['data'] = array('description' => '');
        // Do we list the file?
        $cck_file['list'] = $step->settings['field_save_list'];
        // Use the new filepath
        $file->filepath = $item['filepath'];
      }
    break;

    // We handle emfield content here so that this function
    // does not have to be duplicated
    case 'emfield':
      $item = mm_emfield_parse_url($file->filepath, $field);
    break;

    // Append the new field data onto the node
    if (! is_array($node->{$field['field_name']})) {
      $node->{$field['field_name']} = $item;
    }
    elseif (! isset($node->{$field['field_name']}[0]))  {
      $node->{$field['field_name']} = $item;
    }
    // Files exist, append this file
    else {
      $node->{$field['field_name']}[] = $item;
    }
  }

  // If the node was passed in during hook_nodeapi we need to not save
  if ($step->parameter_get('nodeapi') != 'presave') {
    // Save our new data
    node_save($node);
    // Clear node cache
    cache_clear_all('content:'. $node->nid . ':' . $node->vid, 'cache_content', TRUE);
  }

  // return the file
  return $file->filepath;
}

function mm_node_content_field_store_file_exists($node, $step, $mm_file, &$found) {
  // $node, $step, $mm_file, &$found
}

/* ************************************************** */
/* Helper functions                                   */
/* ************************************************** */


/**
 * Helper function to create a new drupal file based on the CCK settings
 *
 * @param array $node
 *   Drupal node object
 * @param array $field
 *   CCK field array
 * @param string $file_path
 *   The file path
 * @param array $configuration
 * @return array
 */
function mm_node_content_field_filefield($file, $field) {
  // Is there a specified directory to use with this field?
  $destination_path = mm_node_content_field_widget_files_directory($file, $field);

  // get the validators for this field
  if ($field['type'] == 'image') {
    $validators = array_merge(filefield_widget_upload_validators($field), imagefield_widget_upload_validators($field));
  }
  else {
    $validators = filefield_widget_upload_validators($field);
    // Set filefield size validation to the per file limit set on the field. filefield_widget_upload_validators
    // sets the max filesize to the sites PHP upload limit if the field setting is greater than the PHP upload limit.
    // In the case of programmatic saving of files via media mover, we can use sizes greater than the PHP upload limit.
    if ($field['widget']['max_filesize_per_file']) {
      $validators['filefield_validate_size'][0] = parse_size($field['widget']['max_filesize_per_file']);
    }
  }

  // Create the new file
  if (! $cck_file = field_file_save_file($file->filepath, $validators, $destination_path)) {
    watchdog('MM Content', 'Could not save file', NULL, WATCHDOG_ERROR);
    return;
  }

  return $cck_file;
}


/**
 * Create a destination path
 *
 * @TODO Not sure that this is functioning as expected
 * @param $field
 * @return unknown_type
 */
function mm_node_content_field_widget_files_directory($file, $field) {
  $path = file_directory_path();
  if (isset($file->data['user']->uid) and module_exists('token')) {
    $user = user_load($file->data['user']->uid);
    $path_tokens = $field['widget']['file_path'];
    if ($directories = token_replace($path_tokens, 'user', $user)) {
      $path .= '/' . $directories;
    }
  }
  return $path;
}
