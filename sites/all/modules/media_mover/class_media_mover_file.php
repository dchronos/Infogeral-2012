<?php

// $Id$

/**
 * @file
 * Base class for media mover files
 */


class media_mover_file {
  // Default file status
  var $status = MMA_FILE_STATUS_READY;


  /**
   * By default, load the file from cache if possible
   */
  function __construct($input = NULL, $cache = TRUE) {

    // If the input is a mmfid try to load the file
    if (! is_object($input) && ! empty($input)) {
      if (! $this->load($input, $cache)) {
        return FALSE;
      }
      return TRUE;
    }

    // Set some sane defaults for constructing the file.
    $defaults = array(
      'status' => MMA_FILE_STATUS_READY,
      'new' => TRUE,
    );

    // If the input is a media mover step we can construct the stub file from
    // a default set of data
    if (is_object($input) && get_class($input) == 'media_mover_step') {
      $defaults['cid'] = $input->cid;
      $defaults['step_order'] = $input->step_order;
      $this->data_set('steps', array('id' => $input->step_order, 'value' => $input));
    }

    // Add the defaults to the new file
    foreach ($defaults as $key => $value) {
      $this->{$key} = $value;
    }
  }


  /**
   * Get media mover file data for the requested id.
   *
   * @param $mmfid
   *   Media Mover file id
   * @return
   *   Boolean, did the file load or not?
   *
   */
  function load($mmfid, $cache = FALSE) {
    // By default we try to load a cached file
    if (! ($cache && $this->cache_fetch())) {
      // Load the file
      // @TODO how much integration should there be between managed drupal files?

      $query = db_select('media_mover_files', 'mmf')
        ->condition('mmf.mmfid', $mmfid, '=')
        ->fields('mmf');
      $result = $query->execute()->fetchAssoc();

      // No file data was found
      if (! $result) {
        $this->error[] = t('Failed to load !mmfid', array('!mmfid' => $mmfid));
        return FALSE;
      }

      // Load the data onto the file
      $this->load_data($result);

      // Cache this file by default
      if ($cache) {
        $this->cache();
      }
    }
    // Allow the file to be altered
    drupal_alter('media_mover_file_load', $this);
  }


  /**
   * Save data associated with a file.
   *
   * @NOTE this function is not thread safe. $file->status() and $file->lock/unlock
   *       can be used to protect the file from being hijacked from a new
   *       process.
   *
   * @param boolean $advance
   *   should the file's current step be advanced?
   */
  function save($advance = FALSE) {
    // Advance the step for this file if requested
    if ($advance) {
      $this->step_next();
    }

    // If this is a programatic usage do not access the database
    if (! empty($this->passthrough)) {
      drupal_alter('media_mover_file_update', $this);
      return;
    }

    // If a filesize was not passed in see if we can get one.
    // @TODO Can this move somewhere else?
    if (empty($this->filesize)) {
      if (file_exists($this->uri)) {
        $this->filesize = filesize($this->uri);
      }
    }

    // New files are different- complete file data is not present
    if (! empty($this->new)) {
      $this->date = time();
      $this->source_uri = $this->uri;
      drupal_write_record('media_mover_files', $this);
      drupal_alter('media_mover_file_insert', $this);
    }

    // This is an existing file
    else {
      drupal_write_record('media_mover_files', $this, array('mmfid'));
      drupal_alter('media_mover_file_update', $this);
    }

    // Reset the cache of this file.
    $this->uncache();
  }


  /**
   * Locks a media mover file to prevent it from being used
   * by multiple processes at the same time
   *
   * @return boolean
   */
  function lock($status_check = MMA_FILE_STATUS_READY, $new_status = MMA_FILE_STATUS_LOCKED) {
    // Was the status correctly set?
    return $this->status_set($status_check, $new_status);
  }


  /**
   * Unlock a file for further use, or set it to the specified $file->status
   */
  function unlock() {
    // If this file is still locked from processing, we are ready to unlock it. Files
    // with errors or custom status do not need to be updated
    if ($this->status == MMA_FILE_STATUS_LOCKED) {
      $this->status = MMA_FILE_STATUS_READY;
      // Reset the lock date
      $this->lock_date = 0;
      // Now we need to see if this is the last step in the configuration
      $configuration = media_mover_api_configuration_load($this->cid);
      // Is this the last step for this file?
      if ($this->step_order == $configuration->step_count() && $this->status == MMA_FILE_STATUS_READY) {
        $this->status = MMA_FILE_STATUS_FINISHED;
      }
    }

    // Set this file status to ready and reset lock date
    drupal_write_record('media_mover_files', $this, 'mmfid');
  }


  /**
   * Updates a file's filepath
   *
   * As the file moves from step to step. Note that this is not thread safe.
   *
   * @param $filepath
   *   string, filepath
   * @param $step_oder
   *   int, what step
   */
  function update_uri($uri, $step_order) {
    // Some modules may return the filepath as TRUE or return FALSE
    // if there was an error. In either case we do nothing.
    if (empty($uri) || $uri === TRUE) {
      $filepath = $this->uri;
    }

    // Update the current uri
    $this->uri = $uri;
    // Store a copy of this file path in the steps
    $this->steps[$step_order]->uri = $uri;
  }


  /**
   * API to permantly store file data. Will overwrite existing data. Brital. Sorry.
   *
   * @param string $name
   *   Name of the data to store.
   * @param array, string $value
   *   If $value is an array, the array should be formated as
   *   array('id' => ID, 'value' => VALUE).
   */
  function data_set($key, $value) {
    if (is_string($value) || is_int($value)) {
      $this->data[$key] = $value;
    }
    // Non string data
    else {
      // A plain array is being stored at no specified ID
      if (! isset($value['id'])) {
        $this->data[$key][] = $value;
      }
      else {
        $this->data[$key][$value['id']] = $value['value'];
      }
    }

  }


  /**
   * API function to retrieve data from the file
   */
  function data_get($key) {
    if (! empty($this->data[$key])) {
      return $this->data[$key];
    }
  }


  /**
   * API function to remove data from the file
   *
   * @param string $key
   * @param string $index
   */
  function data_delete($key, $index = NULL) {
    if ($index) {
      unset($this->data[$key][$index]);
    }
    else {
      unset($this->data[$key]);
    }
  }

  /**
   * Return a node or nid from a file if it exists
   *
   * @return object
   */
  function node_get($load = TRUE) {
    if (! $nid = $this->nid) {
      if (! $nid = $this->data['node']->nid) {
        return FALSE;
      }
    }
    if ($load) {
      return node_load($nid);
    }
    return $nid;
  }


  /**
   * Utility function: return file steps filtered by module
   *
   * @param $module_name
   *   String, module name
   * @return array of steps that match $module_name
   */
  function steps_filtered_by_module($module_name) {
    $steps = array();
    if ($this->steps) {
      foreach ($this->steps as $step) {
        if ($step->build['module'] == $module_name) {
          $steps[$step->step_order] = $step;
        }
      }
      if ($steps) {
        return $steps;
      }
    }
    return FALSE;
  }


  /**
   * Moves the file one step forward and sets the file status
   * If the file is in the last step, mark completed.
   */
  private function step_next() {
    // Load the configuration
    $configuration = media_mover_api_configuration_load($this->cid);
    // If we are not on the final step, advance the file
    if ($this->step_order < $configuration->last_step() ) {
      $this->step_order = $this->step_order + 1;
    }
    // Are we on the final step?
    elseif ($this->step_order == $configuration->last_step()) {
      $this->status = MMA_FILE_STATUS_FINISHED;
    }
  }


  /**
   * Helper function to get a uri from a specified step
   *
   * @param $data
   *   string or array, is either a $sid or an array(MODULE_NAME, ACTION)
   * @return string, filepath
   */
  function retrive_uri($data) {
    if (is_array($data)) {
      foreach ($this->steps as $step) {
        if ($step->module == $data['module'] && $step->action_id == $data['action_id']) {
          $sid = $step->sid;
        }
      }
    }
    if (is_string($data)) {
      $sid = $data;
    }
    return $this->data['files'][$sid];
  }


  /**
   * Delete a single file
   */
  function delete() {
    if ($this->steps) {
      // NEVER delete harvest file unless explicitly told because it could be
      // in use by Drupal elsewhere
      $do_not_delete = $this->steps[0]->uri;
      foreach ($this->steps as $id => $step) {
        if ($function = $step->build['delete']) {
          if (function_exists($function)) {
            $function($file, $step);
          }
        }
        // If the file is present and it is not the source material, delete
        elseif ($step->uri != $do_not_delete) {
          file_delete($step->uri);
        }
      }
    }

    // Remove the file from the database
    db_query("DELETE FROM {media_mover_files} WHERE mmfid = %d", $this->mmfid);
  }


  /**
   * Returns the filepath that should be reprocessed
   *
   * @param unknown_type $step
   */
  function reprocess_filepath($step = 0) {
    // the first step should return original file
    if ($step === 0) {
      return $this->source_uri;
    }
    return $this->steps[$step]->uri;
  }


  /**
   * Utility function to run the next step on this file
   */
  function process_next() {
    // We need the configuration that created this file
    $configuration = media_mover_api_configuration_load($this->cid);
    // Step to execute is one ahead of current file step
    //$step_order = $this->step_order++;
    $step_order = $this->step_order;
    // Run the step
    return $configuration->steps[$step_order]->run($this);
  }


  /**
   * Set file status
   *
   * This function is intended to be used only for changing a file's status.
   * $file->save will not change the file's status
   *
   * @param $status_check
   *   String, the status state to check if this file is in
   * @param $status_change
   *   String, the status to set the file to
   * @param $time
   *   Int, unix time stamp
   * @return boolean could the file status be set?
   */
  function status_set($status_check = MMA_FILE_STATUS_READY, $status_change = MMA_FILE_STATUS_LOCKED, $time = FALSE) {
    // Only lock if the file is in the db, note that this
    // prevents locking if we are passing through a file
    // rather than saving it to the db
    if (empty($this->mmfid) || ! empty($this->pass_through)) {
      return TRUE;
    }

    // Open a transaction to keep this thread safe.
    $transaction = db_transaction();

    try {
      $query = db_query("SELECT status FROM {media_mover_files} WHERE mmfid = :mmfid", array(
        ':mmfid' => $this->mmfid
      ));
      $result = $query->fetchObject();

      if ($result->status == $status_check || $status_check == '*' ) {
        // Update the status in the DB
        $this->status = $status_change;
        $this->lock_date = $time ? $time : time();
        // Create a dummy object so only lock status is saved- this is probably
        // never needed, but this function explicitly only sets the status of
        // the file so an implementor may unexpected results if we save the full
        // object
        $dummy = new stdClass();
        $dummy->mmfid = $this->mmfid;
        $dummy->status = $this->status;
        drupal_write_record('media_mover_files', $dummy, array('mmfid'));
        return TRUE;
      }
    }
    catch (Exception $error) {
      $transaction->rollback();
      return FALSE;
    }
  }


  /**
   * Cache this file
   */
  private function cache() {
    cache_set('media_mover_file_' . $this->mmfid, $this, 'cache_media_mover');
  }


  /**
   * Delete file cache
   */
  private function uncache() {
    cache_clear_all('media_mover_file_' . $this->mmfid, 'cache_media_mover');
  }


  /**
   * Attempts to load a file from the cache
   *
   * @param $mmfid
   *   Int, file id
   */
  private function cache_fetch() {
    $data = cache_get('media_mover_file_' . $this->mmfid, 'cache_media_mover');


    if ($data = $data->data) {
      // We have to map the cached values onto the current object
      foreach ($data as $key => $value) {
        $this->{$key} = $value;
      }
      $this->cached = TRUE;
      return TRUE;
    }
    return FALSE;
  }


  /**
   * Utility function to add data to the file
   *
   * @param $data
   *   Object, data to add to the file
   */
  private function load_data($data) {
    // Make sure we do not have serialized data
    if (! is_array($data['data'])) {
      $data['data'] = unserialize($data['data']);
    }
    // Make sure that we have a valid status @TODO is this right?
    if ($data['status'] == NULL) {
      $data['status'] = MMA_FILE_STATUS_READY;
    }
    // Add the data back onto the file
    foreach ($data as $key => $value) {
      $this->{$key} = $value;
    }
  }


  /**
   * Utility function to queue a file
   */
  function queue() {
    // Set the status to queued
    return $this->status_set(MMA_FILE_STATUS_READY, MMA_FILE_STATUS_QUEUED);
  }


  /**
   * Utility function to remove a file from the queued state
   */
  function dequeue($status = MMA_FILE_STATUS_LOCKED) {
    // Set the status to queued
    return $this->status_set(MMA_FILE_STATUS_QUEUED, $status);
  }


}
