<?php

// $Id$

/**
 * @file
 *
 * Displays the landing of a single configuration
 */
?>
<table class="media-mover <?php print $configuration->status; ?>">
  <tbody>
    <tr>
      <td class="name">
        <?php print t('<strong>@name</strong> (@cid)<br /><em>@status</em>', array('@status' => $configuration->status_name, '@name' => $configuration->name, '@cid' => $configuration->cid)); ?>
      </td>
      <td class="ops">
        <?php print $header_ops; ?>
      </td>
    </tr>
    <tr>
      <td class="description" colspan="2">
        <em><?php print check_plain($configuration->description); ?></em>
      </td>
    </tr>
    <tr>
      <td class="steps">
        <?php print $content; ?>
      </td>
      <td class="extra-ops">
        <?php print $ops; ?>
      </td>
    </tr>
  </tbody>
</table>