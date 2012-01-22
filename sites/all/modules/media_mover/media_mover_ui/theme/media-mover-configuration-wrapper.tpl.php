<?php

// $Id$

/**
 * @file
 * Provides a wrapper for configuration display
 */

?>
<table class="media-mover <?php print $configuration->status; ?>">
  <tbody>
    <tr>
      <td class="name">
        <?php print t('<strong>@name</strong> (@cid)<br /><em>@status</em>', array('@status' => media_mover_api_configuration_status($configuration->status), '@name' => $configuration->name, '@cid' => $configuration->cid)); ?>
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
      <td class="content" colspan="2">
        <?php print $content; ?>
      </td>
    </tr>
  </tbody>
</table>