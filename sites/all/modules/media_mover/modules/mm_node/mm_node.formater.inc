<?php

// $Id$

/**
 * @file
 * Provides a formatter for CCK data- overrides filefield defaults
 *
 */

/**
 * Theme function for the 'default' filefield formatter.
 */
function theme_mm_node_filefield_formatter_default($element) {
  $file = $element['#item'];
  $field = content_fields($element['#field_name']);
  $output = theme('filefield_item', $file, $field);
  return $output;
}

/**
 * Theme function for the 'path_plain' formatter.
 */
function theme_mm_node_filefield_formatter_path_plain($element) {
  // Inside a View this function may be called with null data. In that case,
  // just return.
  if (empty($element['#item'])) {
    return '';
  }

  $field = content_fields($element['#field_name']);
  $item = $element['#item'];
  // If there is no image on the database, use default.
  if (empty($item['fid']) && $field['use_default_file']) {
    $item = $field['default_file'];
  }
  if (empty($item['filepath']) && !empty($item['fid'])) {
    $item = array_merge($item, field_file_load($item['fid']));
  }

  return empty($item['filepath']) ? '' : mm_node_filepath_check($item);;
}

/**
 * Theme function for the 'url_plain' formatter.
 */
function theme_mm_node_filefield_formatter_url_plain($element) {
  // Inside a View this function may be called with null data. In that case,
  // just return.
  if (empty($element['#item'])) {
    return '';
  }

  $field = content_fields($element['#field_name']);
  $item = $element['#item'];
  // If there is no image on the database, use default.
  if (empty($item['fid']) && $field['use_default_file']) {
    $item = $field['default_file'];
  }
  if (empty($item['filepath']) && !empty($item['fid'])) {
    $item = array_merge($item, field_file_load($item['fid']));
  }

  return empty($item['filepath']) ? '' : mm_node_filepath_check($item['filepath']);
}

/**
 * Theme function for any file that is managed by FileField.
 *
 * It doesn't really format stuff by itself but rather redirects to other
 * formatters that are telling us they want to handle the concerned file.
 *
 * This function checks if the file may be shown and returns an empty string
 * if viewing the file is not allowed for any reason. If you need to display it
 * in any case, please use theme('filefield') instead.
 */
function theme_mm_node_filefield_item($file, $field) {
  // Ensure that the filefield functions are loaded
  module_load_include('inc', 'filefield', 'filefield_formatter');
  if (filefield_view_access($field['field_name']) && filefield_file_listed($file, $field)) {
    return theme('filefield_file', $file);
  }
  return '';
}

/**
 * Overrides the standard filefield file display
 */
function theme_mm_node_filefield_file($file) {
  // Views may call this function with a NULL value, return an empty string.
  if (empty($file['fid'])) {
    return '';
  }

  $path = $file['filepath'];
  // If this filepath has a URI in it, do not format it
  $url = mm_node_filepath_check($file);

  $icon = theme('filefield_icon', $file);

  // Set options as per anchor format described at
  // http://microformats.org/wiki/file-format-examples
  // TODO: Possibly move to until I move to the more complex format described
  // at http://darrelopry.com/story/microformats-and-media-rfc-if-you-js-or-css
  $options = array(
    'attributes' => array(
      'type' => $file['filemime'] . '; length=' . $file['filesize'],
    ),
  );

  // Use the description as the link text if available.
  if (empty($file['data']['description'])) {
    $link_text = $file['filename'];
  }
  else {
    $link_text = $file['data']['description'];
    $options['attributes']['title'] = $file['filename'];
  }

  return '<div class="filefield-file clear-block">'. $icon . l($link_text, $url, $options) .'</div>';
}

/**
 * Overrides the standard drupal file attachment
 */
function theme_mm_node_upload_attachments($files) {
  $header = array(t('Attachment'), t('Size'));
  $rows = array();
  foreach ($files as $file) {
    $file = (object) $file;
    if ($file->list && empty($file->remove)) {
      $href = mm_node_filepath_check($file);
      $text = $file->description ? $file->description : $file->filename;
      $rows[] = array(l($text, $href), format_size($file->filesize));
    }
  }
  if (count($rows)) {
    return theme('table', $header, $rows, array('id' => 'attachments'));
  }
}