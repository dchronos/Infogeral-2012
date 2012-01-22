<?php

// $Id$

/**
 * @file
 * This contains the theming functions for node display
 */


/**
 * Display a media mover file on a node landing page
 *
 * @param type $files
 * @return string
 */
function theme_mm_node_files_view($files) {
  $header = array(t('Media Mover files'), t('Size'));
  $rows = array();
  foreach ($files as $cid => $configuration) {
    foreach ($configuration as $file) {
      // Create a link to the file
      $link = l(basename($file->filepath), mm_node_filepath_check($file->filepath));
      $rows[] = array($link, format_size($file->filesize));
    }
  }
  if (count($rows)) {
    return theme('table', $header, $rows, array('id' => 'attachments'));
  }
}


/**
 * Theme the Media Mover files list. Shamelessly stollen from upload.module
 *
 * @ingroup themeable
 */
function theme_mm_node_files_edit(&$form) {
  $header = array(t('List'), t('Delete'), t('Filepath'), t('Size'), t('Status'));
  foreach (element_children($form) as $key) {
    $row = array();
    $row[] = drupal_render($form[$key]['list']);
    $row[] = drupal_render($form[$key]['remove']);
    $row[] = drupal_render($form[$key]['reprocess']);
    $row[] = drupal_render($form[$key]['filepath']);
    $row[] = drupal_render($form[$key]['size']);
    $row[] = drupal_render($form[$key]['status']);
    $rows[] = array('data' => $row);
  }
  $output = theme('table', $header, $rows, array('id' => 'mm_node-files'));
  $output .= drupal_render($form);
  return $output;
}