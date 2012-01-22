<?php

// $Id$

/**
 * @file
 *
 */


/**
 * Implements hook_drush_help().
 * @param unknown_type $command
 */
function media_mover_api_drush_help($command) {
  if ($command == 'drush:media_mover') {
    return dt('Prints a list of media mover configurations.');
  }
  drush_print($command);
}


/**
 * Implements hook_drush_command().
 */
function media_mover_api_drush_command() {
  $items['media-mover-list'] = array(
    'description' => 'List Media Mover configurations',
    'examples' => array(
      'drush mm-ls' => 'Shows all Media Mover configurations'
    ),
    'aliases' => array('mm-ls')
  );
  $items['media-mover-queue-run'] = array(
    'description' => 'Run Media Mover cron and queue workers',
    'examples' => array(
      'drush mm-cron' => 'Runs Media Mover cron function which will harvest and put files in the queue and then runs drupal queue.'
    ),
    'aliases' => array('mm-queue')
  );
  $items['media-mover-file-status'] = array(
    'description' => 'Get file status data from specified configurations',
    'arguments' => array(
      'configuration' => 'Specify configuration',
    ),
    'examples' => array(
      'drush mm-file-status' => 'Displays information on files from a specified configuration'
    ),
    'aliases' => array('mm-file-status')
  );
  $items['media-mover-config-describe'] = array(
    'description' => 'Describe a Media Mover configuration',
    'arguments' => array(
      'configuration' => 'Specify configuration',
      'step_order' => 'Describe specific step'
    ),
    'examples' => array(
      'drush mm-config-describe convert_video' => 'Describe the configuration: convert_video. Note that this is the CID of the configuration. You can add a $step_order argument to describe a specific step.'
    ),
    'aliases' => array('mm-describe')
  );
  $items['media-mover-run'] = array(
    'description' => 'Run a Media Mover configuration',
    'arguments' => array(
      'configuration' => 'Specify configuration',
      'step' => 'Specify step to run,'
    ),
    'options' => array(
      'all' => 'Run all configurations',
    ),
    'examples' => array(
      'drush mm-run convert_video' => 'Run configuration id: convert_video. Note that this is the CID of the configuration.'
    ),
    'aliases' => array('mm-run')
  );
  // Runs a specified step
  $items['media-mover-run-step'] = array(
    'description' => 'Run a Media Mover configuration\'s step',
    'arguments' => array(
      'configuration' => 'Specify configuration',
      'step_order' => 'Specify which step to run (use $step->step_order)',
    ),
    'examples' => array(
      'drush mm-run convert_video' => 'Run configuration id: convert_video. Note that this is the CID of the configuration.'
    ),
    'aliases' => array('mm-run-step')
  );
  // Resets the status of a configuration
  $items['media-mover-config-reset'] = array(
    'description' => 'Reset a Media Mover configuration',
    'arguments' => array(
      'configuration' => 'Specified configuration',
    ),
    'examples' => array(
      'drush mm-reset convert_video' => 'Resets: convert_video status.'
    ),
    'aliases' => array('mm-reset')
  );
  // Purge files and reset a configuration
  $items['media-mover-config-purge'] = array(
    'description' => 'Purge files and reset a configuration',
    'arguments' => array(
      'configuration' => 'Specified configuration',
    ),
    'examples' => array(
      'drush mm-purge convert_video' => 'Purges: convert_video status.'
    ),
    'aliases' => array('mm-purge')
  );
  // Delete a configuration
  $items['media-mover-config-delete'] = array(
    'description' => 'Delete a configuration',
    'arguments' => array(
      'configuration' => 'Specified configuration',
    ),
    'examples' => array(
      'drush mm-rm convert_video' => 'Deletes: convert_video status.'
    ),
    'aliases' => array('mm-rm')
  );
  return $items;
}


/**
 * List all configurations
 */
function drush_media_mover_api_media_mover_list() {
  $configurations = media_mover_api_configurations_load();
  drush_print('List of all Media Mover configurations:');
  foreach ($configurations as $configuration) {
    drush_print('* ' . $configuration->name . ' (' . $configuration->cid . ')', 1);
  }
}


/**
 * Run Media Mover cron and queue workers
 */
function drush_media_mover_api_media_mover_queue_run() {
  drush_print('Running Media Mover cron functions');
  media_mover_api_cron();

  // This is all cribed from the drupal_queue module
  drupal_queue_include();
  // Try to increase the maximum execution time if it is too low.
  if (ini_get('max_execution_time') < 240 && !ini_get('safe_mode')) {
    set_time_limit(240);
  }

  // Get all the Media Mover queues
  $queues = media_mover_api_cron_queue_info();
  drupal_alter('cron_queue_info', $queues);

  // Work off queues.
  foreach ($queues as $queue_name => $info) {
    drush_print("Running queue worker: $queue_name");
    $function = $info['worker callback'];
    $end = time() + (isset($info['time']) ? $info['time'] : 15);
    $queue = DrupalQueue::get($queue_name);
    while (time() < $end && ($item = $queue->claimItem())) {
      $function($item->data);
      $queue->deleteItem($item);
    }
  }
}


/**
 * Get queue status for configurations
 */
function drush_media_mover_api_media_mover_file_status($configuration = FALSE) {
  if ($configuration) {
    if (! $configuration = _drush_media_mover_configuration_load($configuration)) {
      return;
    }
    $configurations = array($configuration);
  }
  else {
    $configurations = media_mover_api_configurations_load();
  }

  foreach ($configurations as $configuration) {
    $rows = array();
    drush_print(dt('Job counts for: ') . $configuration->name);
    // Get the stats for this configuration
    if ($stats = media_mover_api_configuration_stats($configuration)) {

      foreach ($stats as $step_stat => $stat) {
        $row = array(
          'Step: ' . $step_stat,
          'Status: ' . $stat['status'],
          'Count: ' . $stat['count']
        );
        $rows[] = $row;
      }
    }
    else {
      $rows[] = dt('No files found');
    }
    drush_print_table($rows, array('Step', 'Status', 'Count'));
  }
}


/**
 * Describe a specified configuration
 */
function drush_media_mover_api_media_mover_config_describe($configuration = FALSE, $step_order = FALSE) {
  if (! $configuration) {
    drush_die('No configuration specified.');
    return;
  }
  if (! $configuration = media_mover_api_configuration_load($configuration)) {
    drush_die('Specified configuration not found.');
    return;
  }
  drush_print('Configuration: ' . $configuration->name);
  drush_print('Description: ' . $configuration->description);
  // Describe the step
  if ($step_order !== FALSE) {
    drush_print('Step ' . $step_order . ': ' . $configuration->steps[$step_order]->build['description']);
    drush_print_r($configuration->steps[$step_order]);
  }
  // Describe the configuration
  else {
    foreach ($configuration->steps as $step_order => $step) {
      drush_print('Step ' . $step->step_order . ': ' . $step->build['description'] . ' (' . $step->sid . ')', 1);
    }
  }
}


/**
 * Reset configuration information
 */
function drush_media_mover_api_media_mover_config_reset($configuration) {
  if (! $configuration = _drush_media_mover_configuration_load($configuration)) {
    return;
  }
  drush_print('Reset configuration: ' . $configuration->name);
  $configuration->reset();
  _drush_media_mover_messages($configuration);
}


/**
 * Purge files and reset a configuration
 *
 * @TODO force confirm dialog here
 * @TODO files won't be deleted until after the next queue worker run
 * @param $configuration
 */
function drush_media_mover_api_media_mover_config_purge($configuration) {
  if (! $configuration = _drush_media_mover_configuration_load($configuration)) {
    return;
  }
  drush_print('Purging configuration: ' . $configuration->name);
  $configuration->delete_files();

  // We have to run the deletion queue worker
  drupal_queue_include();
  // Try to increase the maximum execution time if it is too low.
  if (ini_get('max_execution_time') < 240 && !ini_get('safe_mode')) {
    set_time_limit(240);
  }

  // Get all the Media Mover queues
  $queues = media_mover_api_cron_queue_info();
  drupal_alter('cron_queue_info', $queues);
  // We only want the media mover delete queue
  $queues = array('media_mover_api_queue_file_delete' => $queues['media_mover_api_queue_file_delete']);
  // Work the queues
  foreach ($queues as $queue_name => $info) {
    drush_print("Running queue worker: $queue_name");
    $function = $info['worker callback'];
    $end = time() + (isset($info['time']) ? $info['time'] : 15);
    $queue = DrupalQueue::get($queue_name);
    while (time() < $end && ($item = $queue->claimItem())) {
      $function($item->data);
      $queue->deleteItem($item);
    }
  }
  $configuration->steps_reset();
  _drush_media_mover_messages($configuration);
}

/**
 * Delete a configuration
 *
 * @TODO force confirm dialog here
 * @param $configuration
 */
function drush_media_mover_api_media_mover_config_delete($configuration) {
  drush_print('Deleting configuration: ' . $configuration);
  if ($configuration = _drush_media_mover_configuration_load($configuration)) {
    $configuration->delete();
    _drush_media_mover_messages($configuration);
  }
}


/**
 * Run a specified configurations
 *
 * @TODO implement options all
 */
function drush_media_mover_api_media_mover_run($configuration = FALSE, $step = FALSE) {
  if ($all = drush_get_option('all')) {
    $configurations = media_mover_api_configurations_load(MMA_CONFIG_STATUS_ENABLED);
  } else {
    if (! $configuration) {
      drush_die('No configuration specified.');
      return;
    }
    if (! $configuration = media_mover_api_configuration_load($configuration)) {
      drush_die('Specified configuration not found.');
      return;
    }
    $configurations[$configuration->cid] = $configuration;
  }

  foreach ($configurations as $configuration) {
    drush_print('Running configuration: ' . $configuration->name . '.... ');
    // Should we run a specified step?
    if ($step) {
      drush_print('Running step: ' . $configuration->steps[$step]->build['description']);
      $configuration->steps[$step]->run();
    }
    else {
      $configuration->run();
    }
    _drush_media_mover_messages($configuration);
  }
}


/**
 * Run a specified configuration's step
 */
function drush_media_mover_api_media_mover_run_step($configuration = FALSE, $step = FALSE) {
  if (! $configuration) {
    drush_die('No configuration specified.');
    return;
  }
  if ($step !== FALSE) {
    drush_die('No step was specified');
    return;
  }
  if (! $configuration = media_mover_api_configuration_load($configuration)) {
    drush_die('Specified configuration not found.');
    return;
  }
  // Make sure this step exists
  if (! $configuration->steps[$step]) {
    drush_die('Specified step does not exist on this configuration.');
    return;
  }
  //drush_print_r($configuration->steps[$step]);
  drush_print('Running configuration: ' . $configuration->name . ', step: ' . $configuration->steps[$step]->build['description'] . '.... ');
  $configuration->steps[$step]->run();

}


/**
 * Utility function to load configurations. Should probably be handled by a validation function
 * @param unknown_type $configuration
 */
function _drush_media_mover_configuration_load(&$configuration) {
  if (! $configuration) {
    drush_die('No configuration specified.');
    return;
  }
  if (! $configuration = media_mover_api_configuration_load($configuration)) {
    drush_die('Specified configuration not found.');
    return;
  }
  return $configuration;
}

function _drush_media_mover_messages($configuration) {
  if ($configuration->messages) {
    drush_print_table($configuration->messages, array('Severity', 'Type', 'Message'));
  }
}
