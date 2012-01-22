<?php

// $Id$

/**
 * @file
 * This file provides some testing configuration for the s3 module
 * files is based on http://undesigned.org.za/2007/10/22/amazon-s3-php-class
 */

/* **************************************************** */
/* Forms */
/* **************************************************** */


/**
 * build a form that will allow testing of Media Mover configurations
 * ftp settings
 * @return unknown_type
 */
function mm_s3_test_form() {
  $form = array();

  // find all the media mover configurations that us FTP
  if (! $options = media_mover_api_configurations_get_by_module('mm_s3')) {
    drupal_set_message(t('You need to have Media Mover configurations which use S3 in order to test your connection'));
  }
  else {
    $form['display'] = array(
      '#type' => 'markup',
      '#value' => t('This test moves a file to Amazon S3 server, copies the file back, and then removes the file from S3'),
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
 * implement the testing functionality
 */
function mm_s3_test_form_submit($form, &$form_state) {

  // -------------------------------------------------
  // POSSIBLE CONFIGURATION
  // get the file we will push to s3
  $test_upload = drupal_get_path('module', 'mm_s3') . '/test.png';
  // file name is the file name
  $test_upload_name = basename($test_upload);
  // path to the download file. drupal files should be writeable
  $test_download = file_directory_path() . '/s3_download_'. $test_upload_name;

  // Get the configuration data
  $configuration = media_mover_api_configuration_load($form_state['values']['cid']);
  // Find the S3 settings
  foreach ($configuration->steps as $step) {
    if ($step->build['module'] == 'mm_s3') {
      $conf = $step->settings;
      break;
    }
  }

  $destination = $conf['mm_s3_bucket'] .'/'. baseName($test_upload);

  /* **************************************************** */

  // Check if our upload file exists
  if (!file_exists($test_upload) || !is_file($test_upload))
    exit("\nERROR: No such file: $test_upload\n\n");

  // Are the keys good?
  if ($s3 = mm_s3_initialize()) {
    // List your buckets:
    drupal_set_message("Listing all buckets for your S3 account:<br />". theme('item_list', mm_s3_scandir()));

    // Does the requested bucket exist?
    if (mm_s3_is_dir($conf['mm_s3_bucket'])) {
      drupal_set_message(t('We found your bucket: !bucket on your account on the Amazon server', array('!bucket' => $conf['mm_s3_bucket'])));
    }
    // Try to create the bucket
    else {
      $output[] = t('We did not find your bucket on your account on the Amazon server. Attempting to create it.');
      // Create the bucket
      if (! mm_s3_mkdir($conf['mm_s3_bucket'], $conf['mm_s3_perm'])) {
        drupal_set_message(t("Could not create your bucket: " . $conf['mm_s3_bucket'] . ",  probably already exists"), 'error');
      }
      else {

      }
    }

    // Put our file with the read access defined
    if (mm_s3_copy($test_upload, $destination, $conf['mm_s3_perm'])) {
      drupal_set_message(t('mm_s3_copy():  File copied to !path', array(
        '!path' => $destination
      )));
    }
    else {
      drupal_set_message(t('mm_s3_copy(): Failed to copy file\n'), 'error');
    }

    // Get object info
    $info = $s3->getObjectInfo($conf['mm_s3_bucket'], baseName($conf['test_upload']));
    drupal_set_message(t('S3::getObjecInfo(): Info for !file: !items', array(
      '!file' => $destination,
      '!items' => theme('item_list', $info)
    )));


    // Get the contents of our bucket
    if ($contents = mm_s3_scandir($conf['mm_s3_bucket'])) {
      // Get a usable list of files
      foreach ($contents as $id => $file) {
        $list[] = $id;
      }
      drupal_set_message(t('Showing the files in !bucket: !items', array(
        '!bucket' => $conf['mm_s3_bucket'],
        '!items' => theme('item_list', $list)
      )));
    }

    // Delete our file
    if (mm_s3_unlink($destination)) {
      drupal_set_message(t('mm_s3_unlink(): Deleted file: ' . $destination));
    }
    else {
      drupal_set_message(t('mm_s3_unlink(): Failed to delete file'), 'error');
    }

    // Delete the bucket we created (a bucket has to be empty to be deleted)
    if ($s3->deleteBucket($conf['mm_s3_bucket'])) {
      drupal_set_message(t('s3::deleteBucket(): Deleted bucket !bucket', array(
        '!bucket' => $conf['mm_s3_bucket']
      )));
    }
    else {
      drupal_set_message(t('S3::deleteBucket(): Failed to delete bucket- it may not be empty'), 'error');
    }

  }
  return;
}