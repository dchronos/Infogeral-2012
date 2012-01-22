<?php

// $Id$

/**
 * @file
 * The base class for media mover steps
 * @author arthur
 *
 */
class media_mover_step {

  /**
   * Construct a step
   * @return unknown_type
   */
  function __construct($sid = FALSE, $build = FALSE) {
    if ($sid) {
      $this->load($sid);
      return;
    }

    $this->settings = array();
    $this->status = MMA_STEP_STATUS_READY;
    $this->new = TRUE;

    if ($build) {
      $this->name = $build['description'];
      $this->build = $build;
      $this->module = $build['module'];
      $this->action_id = $build['action_id'];
    }
  }


  /**
   * Loads a complete step
   *
   * @param string $sid
   *   Step id
   */
  function load($sid) {
    $query = db_select('media_mover_steps', 'mms')
      ->fields('mms')
      ->condition('mms.sid', $sid)
      ->execute();
    if (! $results = $query->fetchObject()) {
      return FALSE;
    }
    foreach ($results as $key => $value) {
      // Unserialize the configuration data
      if ($key == 'settings') {
        $this->settings = unserialize($value);
      }
      else {
        $this->{$key} = $value;
      }
    }
    // Get this step's build data
    $this->build = media_mover_api_action_build_get($this->module, $this->action_id);
    // Step has been loaded
    $this->new = FALSE;
    drupal_alter('media_mover_step_load', $this);
  }


  /**
   * Saves a step to the database. $this->passthrough prevents save
   * to the database. If a configuration does not exist in the steps
   * database table, a new one is created.
   *
   */
  function save() {
    drupal_alter('media_mover_step_save', $this);
    // Should we save this step?
    if (! empty($this->passthrough)) {
      return;
    }
    // Save the settings for this step
    drupal_write_record('media_mover_steps', $this, empty($this->new) ? array('sid') : array());
    $this->new = FALSE;
  }


  /**
   * Delete this step from the DB

   *
   */
  function delete() {
    // @TODO
    // This needs to check to see if this step is running
    // This needs to have a flag to delete files

    // Open a transaction to keep this thread safe.
    $transaction = db_transaction();

    try {
      db_update('media_mover_steps')
        ->fields(array('status' => MMA_STEP_STATUS_DELETED))
        ->condition('sid', $this->sid, '=')
        ->execute();

      $this->delete_from_step_table();
      return TRUE;
    }
    catch (Exception $error) {
      $transaction->rollback();
      watchdog('media_mover_api', t('Failed to delete step: !sid from configuration !cid because the step was in state %state',
        array(
          '!sid' => $this->sid,
          '!cid' => $this->cid,
          '%state' => $status,
        )), WATCHDOG_ERROR);
      return FALSE;
    }
  }


  /**
   * Runs the step on an array of files. If the step order is 1, this
   * is a harvest function.
   *
   * @param $file
   *   Object, media mover file. If not passed in, assumes a harvest function
   */
  function run(&$file = NULL) {
    // Check to see if the system will allow this step to run right now
    $halts = array();
    drupal_alter('media_mover_process_control', $halts, $file, $this);
    if (count($halts)) {
      // @TODO add to configuration->log
      /* Does not work.  Need to log to the configuration which called this step to run.
      $configuration = media_mover_api_configuration_load($this->cid);
      $configuration->log('Run control: !halts',
        array('!halts' => implode('<br/>', $halts)),
        $configuration,
        WATCHDOG_INFO);
       */
      watchdog('media_mover_api', 'Run control: !halts', array('!halts' => implode('<br/>', $halts)), WATCHDOG_NOTICE);
      return FALSE;
    }

    // Ready the step
    if (! $this->start()) {
      dpm('Failed to start');
      return FALSE;
    }

    // Harvest if this step is a harvest operation.
    if (! empty($this->build['harvest'])) {
      $this->files_select();
    }

    // Non harvest operations
    else {
      if (isset($file)) {
        // If the file is not locked, lock it
        if ($file->lock() || $file->status == MMA_FILE_STATUS_LOCKED) {
          // Attach the step data to the file
          $file->data_set('steps', array('id' => $this->step_order, 'value' => $this));
          // Run the steps callback function
          $this->file_process($file);
          // Now unlock the file
          $file->unlock();
        }
      }
    }
    // Stop the step
    $this->stop();
    return TRUE;
  }


  /**
   * Set a temporary parameter that can be used
   *
   * @param $name
   * @param $value
   */
  function parameter_set($name, $value) {
    $this->parameters[$name] = $value;
  }


  /**
   * Get a parameter
   *
   * @param $name
   */
  function parameter_get($name) {
    if (isset($this->parameters[$name])) {
      return $this->parameters[$name];
    }
    return FALSE;
  }


  /**
   * Select files
   */
  private function files_select() {
    // Get the function to run
    $function = $this->build['callback'];
    // Load any additional functions needed for this step from include files
    if (! empty($this->build['file'])) {
      module_load_include('inc', $this->build['module'], $this->build['file']);
    }

    if (function_exists($function)) {
      if ($files = $function($this)) {
        foreach ($files as $id => $selected_file) {
          // Create the media mover file
          $mmfile = new media_mover_file($this);
          $mmfile->update_uri($selected_file['uri'], $this->step_order);
          $mmfile->data_set('files', array('id' => $this->step_order, 'value' => $selected_file['uri']));
          $mmfile->save(TRUE);
          $files[$id] = $mmfile;
        }
      }
    }
    return $files ? $files : array();
  }


  /**
   * Executes the callback function on a specific step.
   *
   * @param $file
   *   Object, media mover file object
   */
  private function file_process($file) {
    // Allow altering of the file pre process
    drupal_alter('media_mover_file_process_pre', $file, $this);
    // Get the function to run
    $function = $this->build['callback'];
    // Load any additional functions needed for this step from include files
    if (! empty($this->build['file'])) {
      module_load_include('inc', $this->build['module'], $this->build['file']);
    }
    // Process the file
    $uri = $function($this, $file);

    // Return value is one of: URI, TRUE, FALSE).  $file->status may also have
    // been updated. There are only two conditions where the $file can advance:
    // $uri == TRUE or $uri is a uri.
    if ($uri) {
      $advance = TRUE;
      $file->update_uri($uri, $this->step_order);
    }
    else {
      $advance = FALSE;
    }

    // Add this step data
    $file->data_set('steps', array('id' => $this->step_order, 'value' => $this));
    // Save the updated file
    $file->save($advance);
  }


 /**
   * Starts the run of a specified step. Will lock the status
   * and set the start/stop times
   *
   * @return boolean
   *   TRUE if the configuration started, FALSE if not
   */
  private function start() {
    // Open a transaction to keep this thread safe.
    $transaction = db_transaction();

    try {
      $result = db_select('media_mover_steps', 'mms')
        ->condition('mms.cid', $this->cid, '=')
        ->condition('mms.status', MMA_STEP_STATUS_READY, '=')
        ->fields('mms', array('status'))
        ->execute()
        ->fetchAssoc();

      if ($result) {
        // Update the status on our step
        $this->status = MMA_STEP_STATUS_RUNNING;
        // Set the stop time to the last time this started
        $this->stop_time = $this->start_time;
        $this->start_time = time();
        // Save the status
        drupal_write_record('media_mover_steps', $this, array('sid'));
        return TRUE;
      }
      return FALSE;
    }
    catch (Exception $error) {
      $transaction->rollback();
      return FALSE;
    }
  }


  /**
   * Stops a running step.
   *
   * @return boolean
   *   TRUE if the step was stopped, FALSE if not
   */
  private function stop() {
    // If this is a passthrough, ignore the lock
    if (! empty($this->passthrough)) {
      return TRUE;
    }

    // Open a transaction to keep this thread safe.
    $transaction = db_transaction();

    try {
      $query = db_select('media_mover_steps', 'mms')
        ->condition('mms.cid', $this->cid)
        ->condition('mms.status', MMA_STEP_STATUS_RUNNING, '=')
        ->fields('mms');
      $result = $query->execute()->fetchAssoc();
      if ($result) {
        // Update the status on our step
        $this->status = MMA_STEP_STATUS_READY;
        // Set the stop time to when this run started- this allows for
        // any activity that occured while this was running to get picked
        // up next time around
        $this->stop_time = $this->start_time;
        drupal_write_record('media_mover_steps', $this, array('sid'));
        return TRUE;
      }
      return FALSE;
    }
    catch (Exception $error) {
      $transaction->rollback();
      return FALSE;
    }
  }


}
