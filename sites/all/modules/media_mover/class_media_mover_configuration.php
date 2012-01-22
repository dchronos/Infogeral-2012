<?php

// $Id$

/**
 * @file
 * Base class for media mover configurations
 */

class media_mover_configuration {

  /**
   * Construct a new configuration
   */
  function __construct($cid = NULL) {
    // Make sure status is. Any other status will be overrriden
    $this->status = MMA_CONFIG_STATUS_ENABLED;
    $this->step = 0;
    $this->steps = array();
    // This flag allows a configuration to be saved with a new cid
    $this->new = TRUE;
    $this->name = '';
    $this->description = '';
    $this->cid = '';
    if ($cid) {
      $this->load($cid);
    }
  }


  /**
   * Load the requested configuration
   *
   * @param $cid
   *   int, configuration id
   */
  function load($cid) {
    // Get the configuration data
    $result = db_query("SELECT * FROM {media_mover_configurations} WHERE cid = :cid", array(
      ':cid' => $cid)
    );
    if (! $data = $result->fetchObject()) {
      return FALSE;
    }
    // Get the configuration settings
    $data->settings = unserialize($data->settings);
    $data->settings = empty($data->settings) ? array() : (array) $data->settings;
    foreach ($data as $key => $value) {
      $this->{$key} = $value;
    }
    // Get the specific step data for this configuration
    $this->steps_load();
    // Configuration has been loaded
    $this->new = FALSE;
    drupal_alter('media_mover_configuration_load', $this);
    return TRUE;
  }


  /**
   * This saves configuration data. It will create any data that is new
   * or overwrite existing data in the db.
   */
  function save() {
    drupal_alter('media_mover_api_configuration_save', $this);
    drupal_write_record('media_mover_configurations', $this, empty($this->new) ? array('cid') : array());
    // Save each of the steps
    foreach ($this->steps as $step_order => $step) {
      // Step has to know which configuration it belongs to if it does not already
      $step->cid = $this->cid;
      // Step has to have order
      $step->step_order = $step_order;
      // Save the completed step
      $step->save();

    }
    // This is no longer a new configuration
    $this->new = FALSE;
    // @TODO if this was a configuration in code we should note that it is overridden
  }


  /**
   * Get the files associated with this configuration
   *
   * @TODO this has to support limiting the number of files selected by the
   *       current configuration- large data sets could be a major problem
   *
   * @param string $status
   *   File status to select
   * @param int $step_order
   *   File step order to select
   * @param $load
   *   Boolean, should the full file be returned or only the mmfid?
   */
  function get_files($status = FALSE, $step_order = FALSE, $load = TRUE) {
    $query = db_select('media_mover_files', 'mmf');
    $query->fields('mmf', array('mmfid'));
    $query->condition('mmf.cid', $this->cid);
    if ($status) {
      $query->condition('mmf.status', $status);
    }
    if ($step_order) {
      $query->condition('mmf.step_order', $step_order);
    }

    $results = $query->execute()->fetchAll();

    // Put files into an array
    $files = array();

    foreach ($results as $result) {
      if ($load) {
        $file = media_mover_api_file_get($result->mmfid, FALSE);
        $files[] = $file;
      }
      else {
        $files[] = $result->mmfid;
      }
    }
    return $files;
  }


  /**
   * Return a file count for this configuration
   * @param $status
   *   string, media mover file status
   * @return int
   */
  function file_count($status = NULL) {
    $query = db_select('media_mover_files', 'mmf');
    $query->condition('mmf.cid', $this->cid);

    if ($status) {
      $query->condition('mmf.status', $status);
    }

    $query->addExpression('COUNT(mmfid)', 'file_count');
    $result = $query->execute()->fetchAssoc();
    return $result['file_count'];
  }


  /**
   * Run a complete configuration. Run on single or multiple files
   *
   * @param $file
   *   Object, is a media mover file
   *
   */
  function run($file = NULL) {
    // Run each step
    foreach ($this->steps as $step) {
      // Was a file passed in?
      if ($file) {
        $step->run($file);
      }
      else {
        $this->run_step($step);
      }
    }
  }


  /**
   * Run a specified step
   *
   * @param $step_order
   *   Object, Media Mover step object
   */
  function run_step($step) {
    // If we are harvesting, do not use any files
    if (! empty($step->build['harvest'])) {
      $this->log('Selecting files: %description', array('%description' => $step->build['description']), $step, WATCHDOG_INFO);
      $step->run();
    }
    // Look for new files to operate on
    else {
      $files = $this->get_files(MMA_FILE_STATUS_READY, $step->step_order);
      $this->log('Acting on %count files',
        array(
          '%description' => $step->build['description'],
          '%count' => count($files)
        ),
        $step, WATCHDOG_INFO);
      // Run the step on each of the files
      $count = 0;
      foreach ($files as $file) {
        $return = $step->run($file);
        if ($return !== FALSE) {
          $count++;
        }
      }
      $this->log('Processed %count files on this run for %description',
        array(
          '%description' => $step->build['description'],
          '%count' => $count
        ),
        $step, WATCHDOG_INFO);
    }
  }


  /**
   * Run a complete configuration on a existing file
   *
   * Takes a media mover file (from db or code) and runs
   * all the steps on it
   *
   * @param $file
   *   Object, Media Mover file object
   * @param $step_order
   *   Int, specify a step to start from
   */
  function run_file(&$file, $step_order = 0) {
    $steps = array_slice($this->steps, $step_order);
    // Run each step
    foreach ($steps as $step) {
      $step->run($file);
    }
  }


  /**
   * Run the harvest operation
   */
  function harvest($params = NULL) {
    $function = $this->steps[0]->build['callback'];
    if ($params) {
      return $function($this->steps[0], $params);
    }
    else {
      return $function($this->steps[0]);
    }
  }

  /**
   * Simple logging function
   * @param $message
   * @param $type
   *   Object or string
   * @param $severity
   */
  function log($message, $variables, $type = 'NA' , $severity = WATCHDOG_INFO) {
    // Get the human readable name for this severity
    $severities = watchdog_severity_levels();
    if (! $level = $severities[$severity]) {
      $level = $severity;
    }

    // Is this a media mover object?
    if (is_object($type)) {
      if (get_class($type) == 'media_mover_configuration') {
        $type = 'mm configuration: ' . $type->cid;
      }
      elseif (get_class($type) == 'media_mover_step') {
        $type = 'mm step: ' . $type->step_order . ' ' . $type->build['description'] . ' (' . $type->sid .')';
      }
      elseif (get_class($type) == 'media_mover_file') {
        $type = 'mm file: ' . $type->mmfid;
      }
    }

    $this->messages[] = array(
      'severity' => $level,
      'type' => $type,
      'message' => t($message, $variables)
    );
  }


  /**
   * Delete all files associated with this configuration
   *
   * @param $files
   *   Array, array of media mover files
   *
   * @param unknown_type $mmfid
   */
  function delete_files($files = FALSE) {
    // There is an edge case
    // where a file that is currently in use could be processed after
    // the process that owns it finishes. This could allow the file
    // to be opperated on again before the deletion queue finishes.
    // The concequences of this are hopefully small

    // Fetch the files if none are passed in. We assume that if
    // files are being passed in
    if (! $files) {
      $files = $this->get_files();
    }

    $delete_queue = DrupalQueue::get('media_mover_api_queue_file_delete');
    foreach ($files as $file) {
      // @TODO this is not very performance orriented. Figure out how how to set
      //       file status before this.
      $file->status_set('*', MMA_FILE_STATUS_DELETED);
      $delete_queue->createItem($file);
    }
  }

  /**
   * Checks to see if this configuration has already has this filepath
   *
   * @param $uri
   *   String, a file uri
   * @return boolean
   */
  function file_selected($uri) {
    $query = db_select('media_mover_files', 'mmf')
      ->fields('mmf')
      ->condition('mmf.source_uri', $uri, '=')
      ->condition('mmf.cid', $this->cid, '=')
      ->execute();
    $results = $query->fetchAll();
    if ($results) {
      return TRUE;
    }
    return FALSE;
  }


  /**
   * Retrieves all the steps for this configuration
   */
  private function steps_load() {
    $results = db_select('media_mover_steps', 'mms')
      ->condition('mms.cid', $this->cid)
      ->orderBy('step_order')
      ->fields('mms', array('sid', 'step_order'))
      ->execute();
    foreach ($results as $result) {
      // Load the step in question
      $this->steps[$result->step_order] = media_mover_api_step_get($result->sid);
    }
  }


  /**
   * Total number of steps in this configuration
   *
   * @return unknown
   */
  function step_count() {
    return count($this->steps);
  }


  /**
   * Returns the last numerical step in a configuration
   *
   * @return integer
   */
  function last_step() {
    return count($this->steps) - 1;
  }


  /**
   * Utility function to advance the current step.
   */
  function step_advance() {
    $this->step++;
  }

  /**
   * Set a temporary parameter on the configuration and steps
   *
   * @param unknown_type $name
   * @param unknown_type $value
   */
  function parameter_set($name, $value) {
    $this->parameters[$name] = $value;
    foreach ($this->steps as $step) {
      $this->steps[$step->step_order]->parameter_set($name, $value);
    }
  }


  /**
   * Utility function to reset a configuration to 0
   */
  function reset() {
    $this->delete_files();

    db_query('UPDATE {media_mover_steps} SET start_time = 0, stop_time = 0 WHERE cid = :cid', array(':cid' => $this->cid));
  }


  /**
   * Sets all the step statuses to ready
   */
  function steps_reset() {
    if ($this->steps) {
      foreach ($this->steps as $step) {
        $this->steps[$step->step_order]->status = MMA_STEP_STATUS_READY;
      }
      $this->save();
    }
  }


  /**
   * Completely deletes a configuration
   *
   * @param $files
   *   Boolean, should this configurations files be deleted?
   * @return boolean FALSE if delete conditions fail.
   */
  function delete($files = FALSE) {
    // Prepare to remove all steps
    foreach ($this->steps as $step) {
      if (! $step->delete()) {
        $this->log('Step %step was not able to be deleted.  More info may be found in the system log.  Aborting deletion of this configuration.',
          array(
            '%num' => $step->step_order,
            '%step' => $step->build['description']
          ),
        $step, WATCHDOG_ERROR);
        return FALSE;
      }
    }

    // Are there files to be deleted?
    if ($files) {
      $this->delete_files($files);
    }

    // Remove all of the configurations steps
    // @TODO can not delete these steps until all files are deleted.
    foreach ($this->steps as $step) {
      $step->delete();
    }

    db_query('DELETE FROM {media_mover_configurations} WHERE cid = "%s"', $this->cid);
  }

}
