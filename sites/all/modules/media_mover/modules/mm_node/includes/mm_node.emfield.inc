<?php

// $Id$

/**
 * @file
 *
 * Provides Media Mover services for emfield content
 */


/**
 * Harvest configuration form
 *
 * @param $step
 *   Object, Media Mover step
 */
function mm_node_emfield_harvest_config($step) {
  $form = array();
  $form['mm_emfield_select_conf'] = array(
    '#type' => 'fieldset',
    '#title' => t('Emfield'),
    '#description' => t('This module selects files attached to an Embedded Media Field'),
  );
  // Get content types
  if ($types = node_get_types()) {
    foreach ($types as $type) {
      $options[$type->type] = $type->name;
    }
  }
  $form['mm_emfield_select_conf']['mm_emfield_harvest_node_type'] = array(
    '#title' => t('Select content types to select from'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => $step->settings['mm_emfield_harvest_node_type'],
    '#description' => t('Choose which content types to select from.'),
  );
  $form['mm_emfield_select_conf']['mm_config_harvest_field'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Select files from these Embedded Media Fields'),
    '#description' => t('Select the Embedded Media Fields to select files from.'),
    '#options' => mm_node_cck_fields(array('emvideo', 'emimage', 'emaudio')),
    '#default_value' => $step->settings['mm_config_harvest_field'],
  );
  return $form;
}




/**
 * Harvest files from an Embedded Media Field
 * @param array $configuration
 *   The configuration array.
 * @param int $nid
 *   Node id if a single node is being processed.
 * @return array
 */
function mm_node_emfield_harvest($step) {
  $files = array();
  $fields = content_fields();

  $harvest_conditions = '';
  // Are we harvesting from a specific NID ?
  if ($nid = $step->parameters['nid']) {
    $harvest_conditions = ' AND n.nid = '. $nid;
  }

  // We need to harvest from each field that was selected.
  foreach ($step->settings['mm_config_harvest_field'] as $field_name) {
    // Get the relevant database information for this field.
    $db_info = content_database_info($fields[$field_name]);

    $results = db_query("SELECT c.". $field_name ."_embed AS harvest_file, n.nid
      FROM {". $db_info['table'] ."} c
      LEFT JOIN {node} n ON c.nid = n.nid
      LEFT JOIN {media_mover_files} m ON m.cid = %d
      WHERE ((n.nid IS NOT NULL AND m.nid IS NULL)
        OR (n.nid IS NOT NULL AND n.nid = m.nid AND c.". $field_name ."_embed <> m.harvest_file))
        AND n.type = \"%s\"
        AND c.". $field_name ."_embed NOT IN (SELECT harvest_file FROM {media_mover_files} WHERE cid = %d)
        AND n.created >= %d
      ORDER BY n.created
      ", $step->cid, $step->settings['mm_emfield_harvest_node_type'], $step->cid, $step->stop_time);

      // Now build the outgoing file data.
      while ($result = db_fetch_array($results)) {
        $files[] = $result;
      }
  }
  return $files;
}


/**
 * Delete files associated with this configuration.
 *
 * @TODO should we delete the full node here?
 *
 * @param $step
 * @param $file
 */
function mm_node_emfield_node_file_delete($step, $file) {
  // Check that this file is associated with a node; otherwise we cannot do
  // anything.
  if (! $nid = $file->nid) {
    return;
  }

  // Get all the node data.
  $node = node_load($nid);
  content_load($node);

  // Build the name of the field that we are looking for
  $field = $step->settings['mm_emfield_field_save'];

  // Make sure that this field exists on the node
  if (! $node->{$field}) {
    return;
  }

  // If we have a url, we can just delete that media and save the node.
  if ($url = $file->filepath) {
    // We need to look at the files stored in this field to find the url.
    foreach ($node->{$field} as $key => $node_field) {
      // Is this the file that we are looking for?
      if ($node_field['embed'] == $url) {
        // Remove the file from the node
        unset($node->{$field}[$key]);
        // Save the node without the file
        node_save($node);
        watchdog('MM Emfield', 'Deleted media attached to an Embedded Media Field for !node', array('!node' => l($node->title, 'node/'. $node->nid)), WATCHDOG_INFO);
      }
    }
  }
}

