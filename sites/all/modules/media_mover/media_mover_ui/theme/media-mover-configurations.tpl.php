<?php

// $Id$

/**
 * @file
 *
 * Displays the list of configurations on the administration screen.
 */

if (! isset($configurations)) { ?>
   <h3><?php print t('No configurations in the system. Please create one.');?></h3>
 <?php
  return;
}

foreach ($configurations as $configuration): ?>
  <table class="media-mover <?php print $configuration->status; ?>">
    <tbody>
      <tr>
        <td class="name">
          <?php print t('<strong>@name</strong> (@cid)<br /><em>@status</em>', array('@status' => $configuration->status_name, '@name' => $configuration->name, '@cid' => $configuration->cid)); ?>
          <?php if (!empty($view->tag)): ?>
            &nbsp;(<?php print $view->tag; ?>)
          <?php endif; ?>
        </td>
        <td class="ops"><?php print $configuration->ops ?></td>
      </tr>
      <tr>
        <td class="description">
          <em><?php print check_plain($configuration->description); ?></em>
        </td>
        <td class="stats">
          <?php print $configuration->stats; ?>
        </td>
      </tr>
    </tbody>
  </table>
<?php endforeach;