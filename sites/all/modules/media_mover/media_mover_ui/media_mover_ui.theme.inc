<?php

// $Id$

/**
 * @file
 * Theme functions for the Media Mover UI
 */

/**
 * Display the add configuration action selector
 * @param $element
 * @return unknown_type
 */
function theme_media_mover_ui_configuration_action_select($element) {
  foreach (element_children($element) as $item) {
    if (! element_property($element[$item])) {
      $html[] = '<div style="float: left;">';
      $html[] = drupal_render($element[$item]);
      $html[] = '</div>';
    }
  }
  $html[] = '<div style="clear: left;"></div>';
  return implode("\n", $html);
}


/**
 * Display the header for a media mover configuration
 *
 * @param $configuration
 *   Object, media mover configuration
 * @return string
 */
function theme_media_mover_ui_configuration_view_header($vars) {
  $configuration = $vars['configuration'];
  // Build the operations that can be done on each step
  $ops = array();
  $ops[] = l(t('View'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/view');
  switch ($configuration->status) {
    case MMA_CONFIG_STATUS_ENABLED:
      $ops[] = l(t('Edit'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/edit');
      $ops[] = l(t('Disable'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/status/' . MMA_CONFIG_STATUS_DISABLED);
      $ops[] = l(t('Run'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/run');
    break;
    case MMA_CONFIG_STATUS_OVERRIDDEN:
      $ops[] = l(t('Edit'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/edit');
      $ops[] = l(t('Revert'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/revert');
      $ops[] = l(t('Disable'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/status/' . MMA_CONFIG_STATUS_DISABLED);
      $ops[] = l(t('Run'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/run');
    break;
    case MMA_CONFIG_STATUS_DEFAULT:
      $ops[] = l(t('Override'), 'admin/build/media_mover/configuration/' . $configuration->cid . '/edit');
      $ops[] = l(t('Disable'), 'admin/build/media_mover/configuration/' . $configuration->cid . '/status/' . MMA_CONFIG_STATUS_DISABLED);
      $ops[] = l(t('Run'), 'admin/build/media_mover/configuration/' . $configuration->cid . '/run');
    break;
    case MMA_CONFIG_STATUS_DISABLED:
      $ops[] = l(t('Enable'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/status/' . MMA_CONFIG_STATUS_ENABLED);
    break;
  }
  $ops[] = l(t('Files'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/files');
  $ops[] = l(t('Delete'), 'admin/config/media/media_mover/configuration/' . $configuration->cid . '/delete');
  return implode(' | ', $ops);
}