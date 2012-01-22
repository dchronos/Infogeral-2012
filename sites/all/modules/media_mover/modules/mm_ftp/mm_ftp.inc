<?php

// $Id$

/**
 * @file
 * Provides FTP wrapper functionality for FTP operations
 */


/**
 * Helper function to get a list of files from a given directory.
 * By default this will
 * @param $ftp_connection
 * @param $directory
 * @return array
 */
function mm_ftp_get_file_list($ftp_connection, $directory, $recursive = TRUE) {
  $files = array();

  // Get a list of files for the specified directory
  $file_list = ftp_rawlist($ftp_connection, $directory, $recusive);
  // Make sure we have files being handed back
  if (! count($file_list)) {
    return;
  }

  $files = array();
  foreach ($file_list as $file_item) {
    $file = mm_ftp_parse_single_file($file_item, $directory);
    $files[$file['filepath']] = $file;
  }
  return $files;
}

/**
 * Takes an FTP file result and returns an array of data. This
 * is super ugly, but it is largely the fault of FTP for making
 * this such a pain to deal with
 *
 * @param $file_item
 *   string, the FTP file string from rawlist
 * @param $parrent_directory
 *   string, parent directory
 * @param $file_types
 *   array, filter for these filetypes
 * @return array
 */
function mm_ftp_parse_single_file($file_item, $parent_directory) {
  // This regex will pull data out of a file listing as specified by STDOUT
  // <directory/link/file><file mode> <number of links> <owner> <group> <bytes> <time> <filename>
  $regex = "/^(.)([rwx-]*)\s*([0-9]*)\s*([^\s]*)\s*(.*?)\s*([0-9]*?)\s([a-zA-Z]{3}\s[0-9]{2}\s..:..)\s(.*)$/";
  preg_match($regex, $file_item, $matches);
  switch ($matches[1]) {
    case '-':
      $type = 'file';
    break;
    case 'd':
      $type = 'directory';
    break;
    case 'l':
      $type = 'link';
    break;
  }

  $file = array(
    'type' => $type,
    'filepath' => $parent_directory . '/' . $matches[8],
    'owner' => $matches[4],
    'group' => $matches[5],
    'size' => $matches[6],
    'created' => strtotime($matches[7]),
  );
  return $file;
}


/**
 * Puts a file on the remote server
 *
 * @param $ftp_connection
 * @param $local_filepath
 * @param $remote_filepath
 * @param $recursive
 *   Boolean, should directories be created?
 * @return boolean
 */
function mm_ftp_put_file($ftp_connection, $local_filepath, $remote_filepath, $recursive = TRUE) {
  if (! file_exists($local_filepath)) {
    watchdog('mm_ftp', 'Putting file !file to remote server failed: file does not exist', array('!file' => $local_filepath), WATCHDOG_ERROR);
    return FALSE;
  }

  // Ensure that the directory structure exists
  if ($recursive) {
    $directories = dirname($remote_filepath);
    if (! mm_ftp_mkdir($ftp_connection, $directories)) {
      watchdog('mm_ftp', 'Failed to create directory !directory on remote server.', array('!file' => $directories), WATCHDOG_ERROR);
      return FALSE;
    }
  }

   // Open the file that is being moved
  $put_file = fopen($local_filepath, 'r');
  // put this file on the ftp server
  if (! ftp_fput($ftp_connection, $remote_filepath, $put_file, FTP_BINARY)) {
    watchdog(MM_FTP_WATCHDOG_TYPE, 'Failed to put file !file on remote server', array('!file' => $local_filepath), WATCHDOG_ERROR);
    return FALSE;
  }

  // Close the local file
  fclose($put_file);

  // what kind of file path are we returning?
  if ($configuration['mm_ftp_url'] == '') {
    return TRUE;
  }
  else {
    return $configuration['mm_ftp_url'] . $file_ftp_path;
  }
}


/**
 * Deletes a file from the FTP server
 * @param $ftp_connection
 * @param $remote_filepath
 *   String, path to file to be deleted
 * @return unknown_type
 */
function mm_ftp_delete_file($ftp_connection, $remote_filepath) {
  if (! ftp_delete($ftp_connection, $remote_filepath)) {
    watchdog('mm_ftp', t('Error deleting file: !file from FTP server'), array('!file' => $remote_filepath), WATCHDOG_ERROR);
    return FALSE;
  }
  return TRUE;
}


/**
 * Fetch a specified file and store it locally
 *
 * @param unknown_type $remote_filepath
 * @param unknown_type $local_filepath
 * @return $file
 */
function mm_ftp_get($ftp_connection, $remote_filepath, $local_filepath, $acl = 0600) {
  // Ensure that this file is not changing
  $modified_time = ftp_mdtm($ftp_connection, $remote_filepath);
  if ($modified_time > time() - variable_get('mm_ftp_file_mod_time', 2)) {
    watchdog('mm_ftp', 'FTP file appears to be modified: !file', array('!file' => $remote_filepath), WATCHDOG_ERROR);
    return FALSE;
  }

  // Download the file via FTP
  if (! ftp_get($ftp_connection, $local_filepath, $remote_filepath, FTP_BINARY)) {
    watchdog('mm_ftp', 'FTP file harvest failed to download file: !file', array('!file' => $local_filepath), WATCHDOG_ERROR);
    return FALSE;
  }

  // @NOTE SECURITY: it would be a good idea to check the actual mimetype
  //                 of the downladed file, but drupal's file_get_mimetype()
  //                 does not do this for us

  // No file should be allowed to executed by the webserver
  // @NOTE is this going to be a problem on some systems?
  chmod($local_filepath, $acl);
  return TRUE;
}


/**
 * Create a directory structure on the FTP server
 *
 * @param <type> $ftp_connection
 * @param <type> $path
 * @return boolean
 */
function mm_ftp_mkdir($ftp_connection, $path) {
  // Break the full path into an array of directories
  $dir = split("/", $path);
  $create_directory = $dir[0];
  for ($i=0; $i<count($dir); $i++) {
    // On the first pass we do not append the next directory
    if ($i) {
      $create_directory .= "/" . $dir[$i];
    }
    if (!ftp_chdir($ftp_connection, $path)) {
      ftp_chdir($ftp_connection,"/"); 
      // Create the directory
      if (! ftp_mkdir($ftp_connection, $create_directory)) {
        return FALSE;
      }
    }
  }
  return TRUE;
}
