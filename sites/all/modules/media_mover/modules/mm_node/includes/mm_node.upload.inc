<?php

// $Id$

/**
 * @file
 *
 * Provides services for Drupal's upload module
 */

/**
 * Harvest from node uploads configuration
 *
 * @param $configuration
 *   array, configuration array
 * @return array
 */
function mm_node_upload_harvest_config($step) {
  $form["mm_node_upload_harvest_config"] = array(
    '#type' => 'fieldset',
    '#title' => t('Select from Drupal node files'),
    '#description' => t('This module harvests files attached to nodes that have been created on the site.'),
    '#collapsed' => FALSE,
  );
  $form['mm_node_upload_harvest_config']['mm_node_types'] = array(
    '#type' => 'select',
    '#title' => t('Choose files from these content types'),
    '#description' => t('Choose content types you wish to search for video files to convert. You can select multiple file types.'),
    '#options' => node_get_types('names') ,
    '#multiple' => TRUE,
    '#default_value' => $step->settings['mm_node_types'],
  );
  $form['mm_node_upload_harvest_config']['file_types'] = array(
    '#title' => t('File types'),
    '#type' => 'textfield',
    '#default_value' => isset($step->settings['file_types']) ? $step->settings['file_types'] : 'avi mov wmv mpeg mp4 mpeg2 dv 3gp 3g2 mpeg4',
    '#description' => t('Space seperated list of file types that can be harvested. These are file extensions and are values like "avi mov wmv mpeg mp4 mpeg2 dv 3gp 3g2 mpeg4". Leave blank to harvest any file type.'),
  );
  $form['mm_node_upload_harvest_config']['delete'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete when source is deleted'),
    '#description' => t('Delete the media mover file when the source file is deleted.'),
    '#default_value' => isset($step->settings['delete']) ? $step->settings['delete'] : FALSE,
  );
  return $form;
}



/**
 * Harvest files from Drupal attached files
 *
 * @param $step
 *   Object, Media Mover step object
 * @return $files
 *   Array, array of file array data
 */
function mm_node_upload_harvest($step) {
  // This builds a set of node types that is mysql friendly for n.type IN (  ...  )
  if ($node_types = $step->settings['mm_node_types'] ) {
    foreach ($node_types as $type) {
      if ($type) {
        $node_type_list[] = '"'. $type .'"';
      }
    }
    $node_type_list = 'n.type IN ('. implode(', ', $node_type_list) .')';

    // Select from specified file types
    if ($step->settings['file_types']) {
      $types = explode(' ', $step->settings['file_types']);
      foreach ($types as $type) {
        $conditions[] = "f.filepath LIKE '%%.$type'";
      }
      // build the SQL to check against the file types
      $file_type_conditions = ' AND ('. implode(' OR ', $conditions) .')';
    }

    // are we harvesting from a specific NID ?
    if (isset($step->parameters['nid'])) {
      $harvest_conditions = ' AND n.nid = '. $step->parameters['nid'];
    }

    // Select all files and join with nodes of $node_type_list
    $query = 'SELECT f.*, n.nid FROM {upload} u
      LEFT JOIN {files} f ON f.fid = u.fid
      LEFT JOIN {node} n ON n.nid = u.nid
      WHERE
      ' . $node_type_list . '
      ' . $file_type_conditions . '
      ' . $harvest_conditions . '
      AND u.fid NOT IN (
        SELECT mmf.fid FROM {media_mover_files} mmf
          WHERE
            mmf.cid = "%s" AND
            mmf.source_filepath = f.filepath
      )
      ORDER BY n.changed DESC';

    // Now run the query
    $results = db_query($query, $step->cid, $step->stop_time);
    $files = array();
    // take each result and add it to the output
    while ($result = db_fetch_array($results)) {
      // Now we harvest file
      $file['filepath'] = $result['filepath'];
      $file['nid'] = $result['nid'];
      $file['fid'] = $result['fid'];
      $file['data']['user']->uid = $result['uid'];
      $files[] = $result;
    }
    return $files;
  }
}


/**
 * Check to see if the specified file is on a node
 *
 * @param $node
 * @param $step
 * @param $file
 * @param $found
 */
function mm_node_upload_harvest_file_exists($node, $step, $mm_file, &$found) {
  if (isset($node->files)) {
    foreach ($node->files as $file) {
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
 * Attach a media mover file to a node
 *
 * @param $step
 *   object, media mover step object
 * @param $file
 *   object, media mover file object
 * @return string path to file
 */
function mm_node_upload_file_attach($step, $file) {
  // find out if there is any node data to work with
  if (! $node = $file->node_get()) {
    watchdog('mm_node', 'Failed to find a node to attach file !file', array('!file' => '<pre>' . print_r($file, TRUE) . '</pre>'), WATCHDOG_WARNING);
    return FALSE;
  }
  // Save the file
  $new_file = mm_node_save_drupal_file($step, $file->filepath, $node->uid);
  // Attach the new file to the node
  $node->files[$new_file->fid] = $new_file;
  // Save the node with upload save.
  upload_save($node);
  // Update the FID on the file
  $file->fid = $new_file->fid;
  // Return the new filepath
  return $new_file->filepath;
}