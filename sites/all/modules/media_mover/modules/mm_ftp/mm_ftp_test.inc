<?php

// $Id$

/**
 * @file
 * Provides testing functions for mm_ftp
 * @TODO this all needs refactoring based on all of the recent changes
 */



/**
 * build a form that will allow testing of Media Mover configurations
 * ftp settings
 * @return unknown_type
 */
function mm_ftp_test_form() {
  $form = array();

  // find all the media mover configurations that us FTP
  if (! $options = media_mover_api_configurations_get_by_module('mm_ftp')) {
    drupal_set_message(t('You need to have Media Mover configurations which use FTP in order to test your connection'));
  }
  else {
    $form['display'] = array(
      '#type' => 'markup',
      '#value' => t('This test moves a file to your FTP server, copies the file back, and then removes the file from the FTP server'),
    );
    $form['cid'] = array(
      '#type' => 'select',
      '#title' => t('Select a configuration to test'),
      '#options' => $options,
      '#description' => t('Choose a configuration to test sending and receiving a file from.'),
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Test'),
    );
  }
  return $form;
}


/**
 * Run the FTP test
 * @param $form_id
 * @param $form_values
 * @return unknown_type
 */
function mm_ftp_test_form_submit($form, &$form_state) {
  // load the FTP library
  require_once('mm_ftp.inc');

  $configuration = media_mover_api_configuration_load($form_state['values']['cid']);
  // find the FTP settings
  foreach ($configuration->steps as $step) {
    if ($step->module == 'mm_ftp') {
      $conf = $step->settings;
      break;
    }
  }

  // now connect to FTP
  if (! $connection = mm_ftp_connect($conf, TRUE)) {
    drupal_set_message(t('Error connecting. Please adjust your settings'));
    return;
  }
  drupal_set_message(t('Connected to FTP server'));

  // Extract out the connection
  extract($connection);

  // -------------------------------------------
  // get our test file
  $path = drupal_get_path('module', 'mm_ftp') .'/test.png';
  // Create the remote filename
  $remote_file_name = $step->settings['mm_ftp_dir'] . '/' . basename($path);
  // Get the result, string if sucess, FALSE if failed
  // copy the file to the server
  if (mm_ftp_put_file($ftp_connection, $path, $remote_file_name)) {
    $modified_time = ftp_mdtm($ftp_connection, $remote_file_name);
    $delta = $modified_time - time();
    drupal_set_message(t('Copied file to: %server%path', array('%server' => $conf['mm_ftp_host'] .'/', '%path' => $remote_file_name)));
    drupal_set_message(t('FTP Server time differs with Web server time by approximately %delta seconds.', array('%delta' => $delta)));
  }

  // -------------------------------------------
  // now copy back from FTP
  // first we need to allow harvesting with out tokens, regardless of configuration settings
  $conf['mm_ftp_harvest_without_tokens'] = TRUE;
  // delete the file after we have retrieved it
  $conf['mm_ftp_delete_files'] = TRUE;
  // make sure that we can harvest PNG files
  $conf['mm_ftp_allowed_files'] .= ' png';

  // Build a filepath to store this file
  $local_filepath = file_create_filename(basename($path), media_mover_api_dir_path());
  // now get the file
  $mm_ftp_file_mod_time = variable_get('mm_ftp_file_mod_time', 2);
  variable_set('mm_ftp_file_mod_time', -100);
  if (! $return_file = mm_ftp_get($ftp_connection, $remote_file_name, $local_filepath)) {
    drupal_set_message(t('There was a problem downloading the file from the server. You may need to check your permissions.  Also check the time difference between the FTP and Web servers is less then 100 seconds.'), 'error');
  }
  else {
    drupal_set_message(t('Copied the file back to the local directory: %directory', array('%directory' => $return_file['harvest_file'])));
    file_delete($return_file['harvest_file']);
    drupal_set_message(t('Deleted the file from the local filesystem.'));
  }
  variable_set('mm_ftp_file_mod_time', $mm_ftp_file_mod_time);
  

  // close the connection manually
  ftp_close($connection['ftp_connection']);
}
